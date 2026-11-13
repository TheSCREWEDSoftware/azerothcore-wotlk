/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NPCPackets.h"

void WorldPackets::NPC::Hello::Read()
{
    // 5.4.8 18414 CMSG_TRAINER_LIST, from Project SkyFire
    _worldPacket.ReadGuidMask(Unit, { 0, 2, 7, 6, 1, 4, 5, 3 });
    _worldPacket.ReadGuidBytes(Unit, { 3, 6, 7, 5, 1, 0, 2, 4 });
}

WorldPacket const* WorldPackets::NPC::TrainerList::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(TrainerGUID, { 4, 5 });
    _worldPacket.WriteBits(Spells.size(), 19);
    _worldPacket.WriteBits(Greeting.size(), 11);
    _worldPacket.WriteGuidMask(TrainerGUID, { 6, 2, 7, 1, 3, 0 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(TrainerGUID, { 4 });

    for (TrainerListSpell const& spell : Spells)
    {
        _worldPacket << uint8(spell.ReqLevel);
        _worldPacket << uint32(spell.MoneyCost);
        _worldPacket << uint32(spell.SpellID);
        for (int32 reqAbility : spell.ReqAbility)
            _worldPacket << int32(reqAbility);
        _worldPacket << uint32(spell.ReqSkillLine);
        _worldPacket << uint32(spell.ReqSkillRank);
        _worldPacket << uint8(spell.Usable);
    }

    _worldPacket.WriteString(Greeting);
    _worldPacket.WriteGuidBytes(TrainerGUID, { 6, 7, 1, 3 });
    _worldPacket << uint32(TrainerID);
    _worldPacket.WriteGuidBytes(TrainerGUID, { 5, 0, 2 });
    _worldPacket << uint32(TrainerType);

    return &_worldPacket;
}

void WorldPackets::NPC::TrainerBuySpell::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> SpellID;
    _worldPacket >> TrainerID;
    _worldPacket.ReadGuidMask(TrainerGUID, { 1, 4, 0, 6, 3, 2, 5, 7 });
    _worldPacket.ReadGuidBytes(TrainerGUID, { 3, 1, 4, 7, 0, 5, 6, 2 });
}

WorldPacket const* WorldPackets::NPC::TrainerBuyFailed::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(TrainerGUID, { 3, 0, 4, 7, 6, 1, 5, 2 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(TrainerGUID, { 1, 2, 0, 3, 4 });
    _worldPacket << int32(TrainerFailedReason);
    _worldPacket.WriteGuidBytes(TrainerGUID, { 5, 6, 7 });
    _worldPacket << int32(SpellID);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::NPC::TrainerBuySucceeded::Write()
{
    _worldPacket << TrainerGUID;
    _worldPacket << int32(SpellID);

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::NPC::VendorItem const& item)
{
    // 5.4.8 18414, from Project SkyFire
    data << int32(item.Quantity);
    data << uint32(item.Price);
    data << uint32(item.Type);
    data << int32(item.Durability);
    data << uint32(item.ItemDisplayInfoID);
    data << uint32(item.StackCount);
    data << uint32(item.ItemID);

    if (item.ExtendedCostID.has_value())
        data << uint32(*item.ExtendedCostID);

    data << int32(item.PlayerConditionFailed.value_or(0));
    data << int32(item.MuID);

    return data;
}

WorldPacket const* WorldPackets::NPC::VendorInventory::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(Vendor, { 5, 7, 1, 3, 6 });
    _worldPacket.WriteBits(Items.size(), 18);

    for (VendorItem const& item : Items)
    {
        _worldPacket.WriteBit(false);
        _worldPacket.WriteBit(!item.ExtendedCostID.has_value());
        _worldPacket.WriteBit(true);
    }

    _worldPacket.WriteGuidMask(Vendor, { 4, 0, 2 });
    _worldPacket.FlushBits();

    // with no item shown the client only opens the window when this is not 0
    _worldPacket << uint8(Reason);

    for (VendorItem const& item : Items)
        _worldPacket << item;

    _worldPacket.WriteGuidBytes(Vendor, { 3, 7, 0, 6, 2, 1, 4, 5 });

    return &_worldPacket;
}
