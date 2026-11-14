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

#include "BankPackets.h"
#include "DBCStores.h"
#include "Item.h"
#include "Log.h"
#include "NPCPackets.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

bool WorldSession::CanUseBank(ObjectGuid bankerGUID) const
{
    // bankerGUID parameter is optional, set to 0 by default.
    if (!bankerGUID)
        bankerGUID = m_currentBankerGUID;

    bool isUsingBankCommand = (bankerGUID == GetPlayer()->GetGUID() && bankerGUID == m_currentBankerGUID);

    if (!isUsingBankCommand)
    {
        Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(bankerGUID, UNIT_NPC_FLAG_BANKER);
        if (!creature)
            return false;
    }

    return true;
}

void WorldSession::HandleBankerActivateOpcode(WorldPackets::NPC::Hello& packet)
{
    ObjectGuid guid = packet.Unit;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_BANKER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleBankerActivateOpcode - Unit ({}) not found or you can not interact with him.", guid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendShowBank(guid);
}

void WorldSession::HandleAutoBankItemOpcode(WorldPackets::Bank::AutoBankItem& packet)
{
    LOG_DEBUG("network", "STORAGE: receive bag = {}, slot = {}", packet.Bag, packet.Slot);

    if (!CanUseBank())
    {
        LOG_DEBUG("network", "WORLD: HandleAutoBankItemOpcode - Unit ({}) not found or you can't interact with him.", m_currentBankerGUID.ToString());
        return;
    }

    Item* item = _player->GetItemByPos(packet.Bag, packet.Slot);
    if (!item)
        return;

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanBankItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, item, nullptr);
        return;
    }

    if (dest.size() == 1 && dest[0].pos == item->GetPos())
    {
        _player->SendEquipError(EQUIP_ERR_NONE, item, nullptr);
        return;
    }

    _player->RemoveItem(packet.Bag, packet.Slot, true);
    _player->ItemRemovedQuestCheck(item->GetEntry(), item->GetCount());
    _player->BankItem(dest, item, true);
    _player->UpdateTitansGrip();
}

void WorldSession::HandleAutoStoreBankItemOpcode(WorldPackets::Bank::AutoStoreBankItem& packet)
{
    LOG_DEBUG("network", "STORAGE: receive bag = {}, slot = {}", packet.Bag, packet.Slot);

    if (!CanUseBank())
    {
        LOG_DEBUG("network", "WORLD: HandleAutoStoreBankItemOpcode - Unit ({}) not found or you can't interact with him.", m_currentBankerGUID.ToString());
        return;
    }

    Item* item = _player->GetItemByPos(packet.Bag, packet.Slot);
    if (!item)
        return;

    if (_player->IsBankPos(packet.Bag, packet.Slot))                    // moving from bank to inventory
    {
        ItemPosCountVec dest;
        InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
        if (msg != EQUIP_ERR_OK)
        {
            _player->SendEquipError(msg, item, nullptr);
            return;
        }

        _player->RemoveItem(packet.Bag, packet.Slot, true);
        if (Item const* storedItem = _player->StoreItem(dest, item, true))
            _player->ItemAddedQuestCheck(storedItem->GetEntry(), storedItem->GetCount());
    }
    else                                                    // moving from inventory to bank
    {
        ItemPosCountVec dest;
        InventoryResult msg = _player->CanBankItem(NULL_BAG, NULL_SLOT, dest, item, false);
        if (msg != EQUIP_ERR_OK)
        {
            _player->SendEquipError(msg, item, nullptr);
            return;
        }

        _player->RemoveItem(packet.Bag, packet.Slot, true);
        _player->ItemRemovedQuestCheck(item->GetEntry(), item->GetCount());
        _player->BankItem(dest, item, true);
        _player->UpdateTitansGrip();
    }
}

void WorldSession::HandleBuyBankSlotOpcode(WorldPackets::Bank::BuyBankSlot& buyBankSlot)
{
    // 6.x 19342: SMSG_BUY_BANK_SLOT_RESULT has no known 6.x opcode, the client only sees the new slot
    if (!CanUseBank(buyBankSlot.Guid))
    {
        LOG_DEBUG("network", "WORLD: HandleBuyBankSlotOpcode - {} not found or you can't interact with him.", buyBankSlot.Guid.ToString());
        return;
    }

    uint32 slot = _player->GetBankBagSlotCount();

    // next slot
    ++slot;

    BankBagSlotPricesEntry const* slotEntry = sBankBagSlotPricesStore.LookupEntry(slot);

    if (!slotEntry)
    {
        return;
    }

    uint32 price = slotEntry->Cost;

    if (!_player->HasEnoughMoney(price))
    {
        return;
    }

    _player->SetBankBagSlotCount(slot);
    _player->ModifyMoney(-int32(price));

    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT);
}

void WorldSession::SendShowBank(ObjectGuid guid)
{
    m_currentBankerGUID = guid;
    WorldPackets::NPC::ShowBank packet;
    packet.Guid = guid;
    SendPacket(packet.Write());
}

// 6.x reagent bank. TC 6.x left these opcodes unhandled; the reagent slots (87-184) are part of
// the inventory for crafting, as in TC 7.3.5, and only crafting reagents can be moved in.
static constexpr uint64 REAGENT_BANK_UNLOCK_COST = 100 * GOLD;

void WorldSession::HandleBuyReagentBankOpcode(WorldPackets::Bank::BuyReagentBank& packet)
{
    if (!CanUseBank(packet.Banker))
    {
        LOG_DEBUG("network", "WORLD: HandleBuyReagentBankOpcode - {} not found or you can't interact with him.", packet.Banker.ToString());
        return;
    }

    if (_player->HasPlayerFlagEx(PLAYER_FLAGS_EX_REAGENT_BANK_UNLOCKED))
        return;

    if (!_player->HasEnoughMoney(REAGENT_BANK_UNLOCK_COST))
    {
        _player->SendEquipError(EQUIP_ERR_NOT_ENOUGH_MONEY, nullptr, nullptr);
        return;
    }

    _player->ModifyMoney(-int64(REAGENT_BANK_UNLOCK_COST));
    _player->SetPlayerFlagEx(PLAYER_FLAGS_EX_REAGENT_BANK_UNLOCKED);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PLAYER_FLAGS_EX);
    stmt->SetData(0, _player->GetUInt32Value(PLAYER_FLAGS_EX));
    stmt->SetData(1, _player->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleDepositReagentBankOpcode(WorldPackets::Bank::DepositReagentBank& packet)
{
    if (!CanUseBank(packet.Banker))
    {
        LOG_DEBUG("network", "WORLD: HandleDepositReagentBankOpcode - {} not found or you can't interact with him.", packet.Banker.ToString());
        return;
    }

    if (!_player->HasPlayerFlagEx(PLAYER_FLAGS_EX_REAGENT_BANK_UNLOCKED))
    {
        _player->SendEquipError(EQUIP_ERR_REAGENT_BANK_LOCKED, nullptr, nullptr);
        return;
    }

    // Collect first: moving items while walking the bags would change what is being walked
    std::vector<Item*> reagents;
    auto collect = [&reagents](Item* item)
    {
        if (item && !item->IsInTrade() && item->GetTemplate()->IsCraftingReagent())
            reagents.push_back(item);
    };

    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        collect(_player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = _player->GetBagByPos(bagSlot))
            for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot)
                collect(bag->GetItemByPos(uint8(slot)));

    for (Item* item : reagents)
    {
        ItemPosCountVec dest;
        InventoryResult msg = _player->CanStoreReagentBankItem(dest, item);
        if (msg != EQUIP_ERR_OK)
        {
            _player->SendEquipError(msg, item, nullptr);
            break;
        }

        // Both sides count as inventory, so quest item counts do not change
        _player->RemoveItem(item->GetBagSlot(), item->GetSlot(), true);
        _player->StoreItem(dest, item, true);
    }
}

void WorldSession::HandleAutoBankReagentOpcode(WorldPackets::Bank::AutoBankReagent& packet)
{
    if (!CanUseBank())
    {
        LOG_DEBUG("network", "WORLD: HandleAutoBankReagentOpcode - Unit ({}) not found or you can't interact with him.", m_currentBankerGUID.ToString());
        return;
    }

    if (Player::IsReagentBankPos(packet.Bag, packet.Slot) || !_player->IsInventoryPos(packet.Bag, packet.Slot))
        return;

    Item* item = _player->GetItemByPos(packet.Bag, packet.Slot);
    if (!item)
        return;

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanStoreReagentBankItem(dest, item);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, item, nullptr);
        return;
    }

    _player->RemoveItem(packet.Bag, packet.Slot, true);
    _player->StoreItem(dest, item, true);
}

void WorldSession::HandleAutoStoreBankReagentOpcode(WorldPackets::Bank::AutoStoreBankReagent& packet)
{
    if (!CanUseBank())
    {
        LOG_DEBUG("network", "WORLD: HandleAutoStoreBankReagentOpcode - Unit ({}) not found or you can't interact with him.", m_currentBankerGUID.ToString());
        return;
    }

    if (!Player::IsReagentBankPos(packet.Bag, packet.Slot))
        return;

    Item* item = _player->GetItemByPos(packet.Bag, packet.Slot);
    if (!item)
        return;

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, item, nullptr);
        return;
    }

    _player->RemoveItem(packet.Bag, packet.Slot, true);
    _player->StoreItem(dest, item, true);
}
