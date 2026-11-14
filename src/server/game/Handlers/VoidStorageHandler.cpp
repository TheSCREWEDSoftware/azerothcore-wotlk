/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "VoidStoragePackets.h"

void WorldSession::SendVoidStorageTransferResult(VoidTransferError result)
{
    SendPacket(WorldPackets::VoidStorage::VoidTransferResult(result).Write());
}

void WorldSession::HandleVoidStorageUnlock(WorldPackets::VoidStorage::UnlockVoidStorage& unlockVoidStorage)
{
    Creature* unit = _player->GetNPCIfCanInteractWith(unlockVoidStorage.Npc, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleVoidStorageUnlock - {} not found or player can't interact with it.", unlockVoidStorage.Npc.ToString());
        return;
    }

    if (_player->IsVoidStorageUnlocked())
    {
        LOG_DEBUG("network", "WORLD: HandleVoidStorageUnlock - Player ({}, name: {}) tried to unlock void storage a 2nd time.", _player->GetGUID().ToString(), _player->GetName());
        return;
    }

    // The client only offers the unlock when the player can pay; check anyway
    if (!_player->HasEnoughMoney(uint32(VOID_STORAGE_UNLOCK)))
        return;

    _player->ModifyMoney(-int32(VOID_STORAGE_UNLOCK));
    _player->UnlockVoidStorage();
}

void WorldSession::HandleVoidStorageQuery(WorldPackets::VoidStorage::QueryVoidStorage& queryVoidStorage)
{
    Creature* unit = _player->GetNPCIfCanInteractWith(queryVoidStorage.Npc, UNIT_NPC_FLAG_TRANSMOGRIFIER | UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleVoidStorageQuery - {} not found or player can't interact with it.", queryVoidStorage.Npc.ToString());
        SendPacket(WorldPackets::VoidStorage::VoidStorageFailed().Write());
        return;
    }

    if (!_player->IsVoidStorageUnlocked())
    {
        LOG_DEBUG("network", "WORLD: HandleVoidStorageQuery - Player ({}, name: {}) queried void storage without unlocking it.", _player->GetGUID().ToString(), _player->GetName());
        SendPacket(WorldPackets::VoidStorage::VoidStorageFailed().Write());
        return;
    }

    uint8 count = 0;
    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
        if (_player->GetVoidStorageItem(i))
            ++count;

    WorldPackets::VoidStorage::VoidStorageContents voidStorageContents;
    voidStorageContents.Items.reserve(VOID_STORAGE_MAX_SLOT);

    for (uint8 i = 0; i < VOID_STORAGE_MAX_SLOT; ++i)
    {
        VoidStorageItem* item = _player->GetVoidStorageItem(i);
        if (!item)
            continue;

        WorldPackets::VoidStorage::VoidItem voidItem;
        voidItem.Guid = ObjectGuid::Create<HighGuid::Item>(item->ItemId);
        voidItem.Creator = item->CreatorGuid;
        voidItem.Slot = i;
        voidItem.Item.Initialize(item);

        voidStorageContents.Items.push_back(voidItem);
    }

    SendPacket(voidStorageContents.Write());
}

void WorldSession::HandleVoidStorageTransfer(WorldPackets::VoidStorage::VoidStorageTransfer& voidStorageTransfer)
{
    Creature* unit = _player->GetNPCIfCanInteractWith(voidStorageTransfer.Npc, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleVoidStorageTransfer - {} not found or player can't interact with it.", voidStorageTransfer.Npc.ToString());
        return;
    }

    if (!_player->IsVoidStorageUnlocked())
    {
        LOG_DEBUG("network", "WORLD: HandleVoidStorageTransfer - Player ({}, name: {}) queried void storage without unlocking it.", _player->GetGUID().ToString(), _player->GetName());
        return;
    }

    if (voidStorageTransfer.Deposits.size() > _player->GetNumOfVoidStorageFreeSlots())
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_FULL);
        return;
    }

    uint32 freeBagSlots = 0;
    if (!voidStorageTransfer.Withdrawals.empty())
    {
        // make this a Player function
        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
            if (Bag* bag = _player->GetBagByPos(i))
                freeBagSlots += bag->GetFreeSlots();

        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
            if (!_player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                ++freeBagSlots;
    }

    if (voidStorageTransfer.Withdrawals.size() > freeBagSlots)
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INVENTORY_FULL);
        return;
    }

    if (!_player->HasEnoughMoney(uint32(voidStorageTransfer.Deposits.size() * VOID_STORAGE_STORE_ITEM)))
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_NOT_ENOUGH_MONEY);
        return;
    }

    WorldPackets::VoidStorage::VoidStorageTransferChanges voidStorageTransferChanges;
    voidStorageTransferChanges.AddedItems.reserve(VOID_STORAGE_MAX_DEPOSIT);
    voidStorageTransferChanges.RemovedItems.reserve(VOID_STORAGE_MAX_DEPOSIT);

    uint8 depositCount = 0;
    for (uint32 i = 0; i < voidStorageTransfer.Deposits.size(); ++i)
    {
        Item* item = _player->GetItemByGuid(voidStorageTransfer.Deposits[i]);
        if (!item)
        {
            LOG_DEBUG("network", "WORLD: HandleVoidStorageTransfer - {} {} wants to deposit an invalid item ({}).", _player->GetGUID().ToString(), _player->GetName(), voidStorageTransfer.Deposits[i].ToString());
            continue;
        }

        // Destroying a bag destroys its contents too; the client never offers a non-empty bag
        if (Bag const* bag = item->ToBag())
        {
            if (!bag->IsEmpty())
            {
                LOG_DEBUG("network", "WORLD: HandleVoidStorageTransfer - {} wants to deposit a non-empty bag ({}).", _player->GetGUID().ToString(), voidStorageTransfer.Deposits[i].ToString());
                continue;
            }
        }

        std::vector<uint32> const& bonuses = item->GetDynamicValues(ITEM_DYNAMIC_FIELD_BONUSLIST_IDS);
        VoidStorageItem itemVS(sObjectMgr->GenerateVoidStorageItemId(), item->GetEntry(), item->GetGuidValue(ITEM_FIELD_CREATOR),
            item->GetItemRandomPropertyId(), item->GetItemSuffixFactor(), item->GetModifier(ITEM_MODIFIER_UPGRADE_ID), std::vector<int32>(bonuses.begin(), bonuses.end()));

        WorldPackets::VoidStorage::VoidItem voidItem;
        voidItem.Guid = ObjectGuid::Create<HighGuid::Item>(itemVS.ItemId);
        voidItem.Creator = item->GetGuidValue(ITEM_FIELD_CREATOR);
        voidItem.Item.Initialize(&itemVS);
        voidItem.Slot = _player->AddVoidStorageItem(itemVS);
        if (voidItem.Slot >= VOID_STORAGE_MAX_SLOT)
            break;

        voidStorageTransferChanges.AddedItems.push_back(voidItem);

        _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        ++depositCount;
    }

    if (depositCount)
        _player->ModifyMoney(-int32(depositCount * VOID_STORAGE_STORE_ITEM));

    for (uint32 i = 0; i < voidStorageTransfer.Withdrawals.size(); ++i)
    {
        uint8 slot = 0;
        VoidStorageItem* itemVS = _player->GetVoidStorageItem(voidStorageTransfer.Withdrawals[i].GetCounter(), slot);
        if (!itemVS)
        {
            LOG_DEBUG("network", "WORLD: HandleVoidStorageTransfer - {} {} tried to withdraw an invalid item (id: {})", _player->GetGUID().ToString(), _player->GetName(), voidStorageTransfer.Withdrawals[i].ToString());
            continue;
        }

        ItemPosCountVec dest;
        InventoryResult msg = _player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemVS->ItemEntry, 1);
        if (msg != EQUIP_ERR_OK)
        {
            SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INVENTORY_FULL);
            LOG_DEBUG("network", "WORLD: HandleVoidStorageTransfer - {} {} couldn't withdraw item id {} because inventory was full.", _player->GetGUID().ToString(), _player->GetName(), voidStorageTransfer.Withdrawals[i].ToString());
            return;
        }

        AllowedLooterSet noLooters;
        Item* item = _player->StoreNewItem(dest, itemVS->ItemEntry, true, itemVS->ItemRandomPropertyId, noLooters, false, itemVS->BonusListIDs);
        item->SetUInt32Value(ITEM_FIELD_PROPERTY_SEED, itemVS->ItemSuffixFactor);
        item->SetGuidValue(ITEM_FIELD_CREATOR, itemVS->CreatorGuid);
        item->SetModifier(ITEM_MODIFIER_UPGRADE_ID, itemVS->ItemUpgradeId);
        item->SetBinding(true);
        _player->SendNewItem(item, 1, false, false, false);

        voidStorageTransferChanges.RemovedItems.push_back(ObjectGuid::Create<HighGuid::Item>(itemVS->ItemId));

        _player->DeleteVoidStorageItem(slot);
    }

    SendPacket(voidStorageTransferChanges.Write());

    SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_NO_ERROR);
}

void WorldSession::HandleVoidSwapItem(WorldPackets::VoidStorage::SwapVoidItem& swapVoidItem)
{
    Creature* unit = _player->GetNPCIfCanInteractWith(swapVoidItem.Npc, UNIT_NPC_FLAG_VAULTKEEPER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleVoidSwapItem - {} not found or player can't interact with it.", swapVoidItem.Npc.ToString());
        return;
    }

    if (!_player->IsVoidStorageUnlocked())
    {
        LOG_DEBUG("network", "WORLD: HandleVoidSwapItem - Player ({}, name: {}) queried void storage without unlocking it.", _player->GetGUID().ToString(), _player->GetName());
        return;
    }

    uint8 oldSlot;
    if (!_player->GetVoidStorageItem(swapVoidItem.VoidItemGuid.GetCounter(), oldSlot))
    {
        LOG_DEBUG("network", "WORLD: HandleVoidSwapItem - {} {} requested swapping an invalid item (slot: {}, itemid: {}).", _player->GetGUID().ToString(), _player->GetName(), swapVoidItem.DstSlot, swapVoidItem.VoidItemGuid.ToString());
        return;
    }

    bool usedDestSlot = _player->GetVoidStorageItem(swapVoidItem.DstSlot) != NULL;
    ObjectGuid itemIdDest;
    if (usedDestSlot)
        itemIdDest = ObjectGuid::Create<HighGuid::Item>(_player->GetVoidStorageItem(swapVoidItem.DstSlot)->ItemId);

    if (!_player->SwapVoidStorageItem(oldSlot, swapVoidItem.DstSlot))
    {
        SendVoidStorageTransferResult(VOID_TRANSFER_ERROR_INTERNAL_ERROR_1);
        return;
    }

    WorldPackets::VoidStorage::VoidItemSwapResponse voidItemSwapResponse;
    voidItemSwapResponse.VoidItemA = swapVoidItem.VoidItemGuid;
    voidItemSwapResponse.VoidItemSlotA = swapVoidItem.DstSlot;
    if (usedDestSlot)
    {
        voidItemSwapResponse.VoidItemB = itemIdDest;
        voidItemSwapResponse.VoidItemSlotB = oldSlot;
    }

    SendPacket(voidItemSwapResponse.Write());
}
