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

#include "ItemPackets.h"

void WorldPackets::Item::SplitItem::Read()
{
    _worldPacket >> SourceBag;
    _worldPacket >> SourceSlot;
    _worldPacket >> DestinationBag;
    _worldPacket >> DestinationSlot;
    _worldPacket >> Count;
}

void WorldPackets::Item::SwapInventoryItem::Read()
{
    _worldPacket >> DestinationSlot;
    _worldPacket >> SourceSlot;
}

void WorldPackets::Item::AutoEquipItemSlot::Read()
{
    // 5.4.8 18414, from Project SkyFire: an inventory position list follows the guid mask
    _worldPacket >> DestinationSlot;
    _worldPacket.ReadGuidMask(ItemGuid, { 6, 3, 0 });
    uint32 count = _worldPacket.ReadBits(2);
    _worldPacket.ReadGuidMask(ItemGuid, { 7 });
    std::vector<std::pair<bool, bool>> positions(count);
    for (std::pair<bool, bool>& position : positions)
    {
        position.second = _worldPacket.ReadBit();          // has slot
        position.first = _worldPacket.ReadBit();           // has bag
    }
    _worldPacket.ReadGuidMask(ItemGuid, { 5, 2, 1, 4 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 6, 3, 1, 7, 2, 0, 4, 5 });
    for (std::pair<bool, bool> const& position : positions)
    {
        if (position.second)
            _worldPacket.read_skip<uint8>();
        if (position.first)
            _worldPacket.read_skip<uint8>();
    }
}

void WorldPackets::Item::SwapItem::Read()
{
    // 5.4.8 18414, from Project SkyFire: the positions may be repeated in an optional list
    uint8 srcSlotAlt, srcBagAlt, dstBagAlt, dstSlotAlt;
    _worldPacket >> srcSlotAlt >> srcBagAlt >> dstBagAlt >> dstSlotAlt;
    uint32 count = _worldPacket.ReadBits(2);
    std::array<bool, 2> hasSlot = { };
    std::array<bool, 2> hasBag = { };
    for (uint32 i = 0; i < count; ++i)
    {
        bool slot = !_worldPacket.ReadBit();
        bool bag = !_worldPacket.ReadBit();
        if (i < 2)
        {
            hasSlot[i] = slot;
            hasBag[i] = bag;
        }
    }

    DestinationBag = hasBag[0] ? _worldPacket.read<uint8>() : dstBagAlt;
    DestinationSlot = hasSlot[0] ? _worldPacket.read<uint8>() : dstSlotAlt;
    SourceBag = hasBag[1] ? _worldPacket.read<uint8>() : srcBagAlt;
    SourceSlot = hasSlot[1] ? _worldPacket.read<uint8>() : srcSlotAlt;
}

void WorldPackets::Item::AutoEquipItem::Read()
{
    _worldPacket >> SourceBag;
    _worldPacket >> SourceSlot;
}

void WorldPackets::Item::DestroyItem::Read()
{
    _worldPacket >> Bag;
    _worldPacket >> Slot;
    _worldPacket >> Count;
}

void WorldPackets::Item::ReadItem::Read()
{
    _worldPacket >> Bag;
    _worldPacket >> Slot;
}

void WorldPackets::Item::TransmogrifyItems::Read()
{
    Items.resize(_worldPacket.ReadBits(22));

    for (TransmogrifyItem& item : Items)
    {
        item.SrcItemGUID[0] = _worldPacket.ReadBit();
        item.SrcItemGUID[5] = _worldPacket.ReadBit();
        item.SrcItemGUID[6] = _worldPacket.ReadBit();
        item.SrcItemGUID[2] = _worldPacket.ReadBit();
        item.SrcItemGUID[3] = _worldPacket.ReadBit();
        item.SrcItemGUID[7] = _worldPacket.ReadBit();
        item.SrcItemGUID[4] = _worldPacket.ReadBit();
        item.SrcItemGUID[1] = _worldPacket.ReadBit();
    }

    Npc[7] = _worldPacket.ReadBit();
    Npc[3] = _worldPacket.ReadBit();
    Npc[5] = _worldPacket.ReadBit();
    Npc[6] = _worldPacket.ReadBit();
    Npc[1] = _worldPacket.ReadBit();
    Npc[4] = _worldPacket.ReadBit();
    Npc[0] = _worldPacket.ReadBit();
    Npc[2] = _worldPacket.ReadBit();

    for (TransmogrifyItem& item : Items)
    {
        _worldPacket >> item.ItemID;

        _worldPacket.ReadByteSeq(item.SrcItemGUID[1]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[5]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[0]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[4]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[6]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[7]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[3]);
        _worldPacket.ReadByteSeq(item.SrcItemGUID[2]);

        _worldPacket >> item.Slot;
    }

    _worldPacket.ReadByteSeq(Npc[7]);
    _worldPacket.ReadByteSeq(Npc[2]);
    _worldPacket.ReadByteSeq(Npc[5]);
    _worldPacket.ReadByteSeq(Npc[4]);
    _worldPacket.ReadByteSeq(Npc[3]);
    _worldPacket.ReadByteSeq(Npc[1]);
    _worldPacket.ReadByteSeq(Npc[6]);
    _worldPacket.ReadByteSeq(Npc[0]);
}

void WorldPackets::Item::SellItem::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> Count;

    _worldPacket.ReadGuidMask(ItemGuid, { 4, 3, 7 });
    _worldPacket.ReadGuidMask(VendorGuid, { 6, 5, 1 });
    _worldPacket.ReadGuidMask(ItemGuid, { 5, 2, 1 });
    _worldPacket.ReadGuidMask(VendorGuid, { 2 });
    _worldPacket.ReadGuidMask(ItemGuid, { 6 });
    _worldPacket.ReadGuidMask(VendorGuid, { 4, 0, 7, 3 });
    _worldPacket.ReadGuidMask(ItemGuid, { 0 });

    _worldPacket.ReadGuidBytes(VendorGuid, { 6, 3, 1 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 1 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 2 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 7, 5 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 7 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 2 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 0, 5 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 3, 6 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 4 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 4, 0 });
}

void WorldPackets::Item::BuybackItem::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> Slot;
    _worldPacket.ReadGuidMask(VendorGuid, { 2, 3, 0, 4, 1, 7, 5, 6 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 0, 6, 1, 7, 5, 2, 3, 4 });
}

void WorldPackets::Item::BuyItemInSlot::Read()
{
    _worldPacket >> VendorGuid;
    _worldPacket >> Item;
    _worldPacket >> Slot;
    _worldPacket >> BagGuid;
    _worldPacket >> BagSlot;
    _worldPacket >> Count;
}

void WorldPackets::Item::BuyItem::Read()
{
    // 5.4.8 18414, from Project SkyFire
    BagSlot = uint8(_worldPacket.read<uint32>());
    _worldPacket >> Count;
    _worldPacket >> Item;
    _worldPacket >> Slot;

    _worldPacket.ReadGuidMask(VendorGuid, { 6 });
    _worldPacket.ReadGuidMask(BagGuid, { 6, 4 });
    _worldPacket.ReadGuidMask(VendorGuid, { 4 });
    ItemType = _worldPacket.ReadBits(2);
    _worldPacket.ReadGuidMask(VendorGuid, { 0, 3 });
    _worldPacket.ReadGuidMask(BagGuid, { 3 });
    _worldPacket.ReadGuidMask(VendorGuid, { 7, 5 });
    _worldPacket.ReadGuidMask(BagGuid, { 2 });
    _worldPacket.ReadGuidMask(VendorGuid, { 1 });
    _worldPacket.ReadGuidMask(BagGuid, { 7 });
    _worldPacket.ReadGuidMask(VendorGuid, { 2 });
    _worldPacket.ReadGuidMask(BagGuid, { 1, 0, 5 });

    _worldPacket.ReadGuidBytes(VendorGuid, { 5, 0 });
    _worldPacket.ReadGuidBytes(BagGuid, { 3, 1, 6 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 2, 7, 6 });
    _worldPacket.ReadGuidBytes(BagGuid, { 0, 5 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 4 });
    _worldPacket.ReadGuidBytes(BagGuid, { 2 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 3 });
    _worldPacket.ReadGuidBytes(BagGuid, { 7 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 1 });
    _worldPacket.ReadGuidBytes(BagGuid, { 4 });
}

void WorldPackets::Item::ListInventory::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(VendorGuid, { 6, 7, 3, 1, 2, 0, 4, 5 });
    _worldPacket.ReadGuidBytes(VendorGuid, { 0, 7, 1, 6, 4, 3, 5, 2 });
}

void WorldPackets::Item::AutoStoreBagItem::Read()
{
    _worldPacket >> SourceBag;
    _worldPacket >> SourceSlot;
    _worldPacket >> DestinationBag;
}

WorldPacket const* WorldPackets::Item::EnchantmentLog::Write()
{
    // 5.4.8 18414, from Project SkyFire. SkyFire writes caster byte 3 twice and target byte 3 never;
    // the second one is the target's
    _worldPacket << uint32(ItemId);
    _worldPacket << uint32(Slot);
    _worldPacket << uint32(EnchantId);

    _worldPacket.WriteGuidMask(Caster, { 6, 7 });
    _worldPacket.WriteGuidMask(Target, { 6, 4 });
    _worldPacket.WriteGuidMask(Caster, { 5 });
    _worldPacket.WriteGuidMask(ItemGuid, { 7, 2, 3 });
    _worldPacket.WriteGuidMask(Caster, { 4, 3 });
    _worldPacket.WriteGuidMask(ItemGuid, { 6 });
    _worldPacket.WriteGuidMask(Target, { 1 });
    _worldPacket.WriteGuidMask(Caster, { 2 });
    _worldPacket.WriteGuidMask(Target, { 5 });
    _worldPacket.WriteGuidMask(ItemGuid, { 4 });
    _worldPacket.WriteGuidMask(Target, { 0 });
    _worldPacket.WriteGuidMask(ItemGuid, { 1 });
    _worldPacket.WriteGuidMask(Caster, { 0 });
    _worldPacket.WriteGuidMask(Target, { 3, 7 });
    _worldPacket.WriteGuidMask(ItemGuid, { 5, 0 });
    _worldPacket.WriteGuidMask(Target, { 2 });
    _worldPacket.WriteGuidMask(Caster, { 1 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(Caster, { 0 });
    _worldPacket.WriteGuidBytes(Target, { 2 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 7 });
    _worldPacket.WriteGuidBytes(Caster, { 1 });
    _worldPacket.WriteGuidBytes(Target, { 4 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 5 });
    _worldPacket.WriteGuidBytes(Caster, { 4 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 2 });
    _worldPacket.WriteGuidBytes(Target, { 6, 0 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 0, 4 });
    _worldPacket.WriteGuidBytes(Caster, { 3 });
    _worldPacket.WriteGuidBytes(Target, { 5 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 1 });
    _worldPacket.WriteGuidBytes(Target, { 3 });
    _worldPacket.WriteGuidBytes(Target, { 7 });
    _worldPacket.WriteGuidBytes(Caster, { 7 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 3 });
    _worldPacket.WriteGuidBytes(Caster, { 6, 2, 5 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 6 });
    _worldPacket.WriteGuidBytes(Target, { 1 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Item::ItemEnchantTimeUpdate::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(ItemGuid, { 4, 0 });
    _worldPacket.WriteGuidMask(PlayerGuid, { 3 });
    _worldPacket.WriteGuidMask(ItemGuid, { 3 });
    _worldPacket.WriteGuidMask(PlayerGuid, { 2, 6, 7 });
    _worldPacket.WriteGuidMask(ItemGuid, { 1 });
    _worldPacket.WriteGuidMask(PlayerGuid, { 4 });
    _worldPacket.WriteGuidMask(ItemGuid, { 6, 5 });
    _worldPacket.WriteGuidMask(PlayerGuid, { 0 });
    _worldPacket.WriteGuidMask(ItemGuid, { 2 });
    _worldPacket.WriteGuidMask(PlayerGuid, { 5, 1 });
    _worldPacket.WriteGuidMask(ItemGuid, { 7 });
    _worldPacket.FlushBits();

    _worldPacket << uint32(Slot);
    _worldPacket.WriteGuidBytes(PlayerGuid, { 4, 2 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 5, 4 });
    _worldPacket.WriteGuidBytes(PlayerGuid, { 6 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 1 });
    _worldPacket.WriteGuidBytes(PlayerGuid, { 0, 1 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 6, 2 });
    _worldPacket.WriteGuidBytes(PlayerGuid, { 7 });
    _worldPacket.WriteGuidBytes(ItemGuid, { 0, 3, 7 });
    _worldPacket.WriteGuidBytes(PlayerGuid, { 3, 5 });
    _worldPacket << uint32(Duration);

    return &_worldPacket;
}

void WorldPackets::Item::WrapItem::Read()
{
    _worldPacket >> GiftBag;
    _worldPacket >> GiftSlot;
    _worldPacket >> ItemBag;
    _worldPacket >> ItemSlot;
}

void WorldPackets::Item::SocketGems::Read()
{
    // 5.4.8 18414, from Project SkyFire
    for (uint8 index : { 4, 0, 6, 2, 1, 7, 3, 5 })
        for (ObjectGuid& gemGuid : GemGuids)
            gemGuid[index] = _worldPacket.ReadBit();
    _worldPacket.ReadGuidMask(ItemGuid, { 5, 0, 6, 2, 3, 4, 7, 1 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 7, 2, 6 });
    for (uint8 index : { 6, 4, 3, 2, 0, 1, 7, 5 })
        for (ObjectGuid& gemGuid : GemGuids)
            _worldPacket.ReadByteSeq(gemGuid[index]);
    _worldPacket.ReadGuidBytes(ItemGuid, { 4, 3, 1, 5, 0 });
}

void WorldPackets::Item::CancelTempEnchantment::Read()
{
    _worldPacket >> EquipmentSlot;
}

void WorldPackets::Item::ItemRefundInfo::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(ItemGuid, { 1, 0, 3, 2, 7, 4, 5, 6 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 3, 7, 5, 1, 0, 6, 4, 2 });
}

void WorldPackets::Item::ItemRefund::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(ItemGuid, { 2, 4, 1, 6, 3, 0, 5, 7 });
    _worldPacket.ReadGuidBytes(ItemGuid, { 3, 5, 6, 2, 7, 0, 1, 4 });
}
