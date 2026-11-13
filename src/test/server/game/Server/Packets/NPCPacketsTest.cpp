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

#include "Creature.h"
#include "ObjectGuid.h"
#include "ItemTemplate.h"
#include "NPCPackets.h"
#include "gtest/gtest.h"

TEST(NPCPacketsTest, VendorInventoryWrites548Layout)
{
    EXPECT_EQ(uint16(SMSG_VENDOR_INVENTORY), 0x1AAE);

    WorldPackets::NPC::VendorInventory packet;
    packet.Vendor = ObjectGuid::Create<HighGuid::Unit>(911, 79964);
    packet.Reason = 2;

    WorldPackets::NPC::VendorItem item;
    item.MuID = 1;
    item.Type = ITEM_VENDOR_TYPE_ITEM;
    item.ItemID = 0xABCD;
    item.ItemDisplayInfoID = 7;
    item.Quantity = -1;
    item.Price = 100;
    item.Durability = 50;
    item.StackCount = 1;
    item.ExtendedCostID = 2;
    packet.Items.push_back(item);
    packet.Items.push_back(WorldPackets::NPC::VendorItem());

    WorldPacket written = *packet.Write();
    ObjectGuid vendor;
    written.ReadGuidMask(vendor, { 5, 7, 1, 3, 6 });
    ASSERT_EQ(written.ReadBits(18), 2u);
    EXPECT_FALSE(written.ReadBit());
    EXPECT_FALSE(written.ReadBit());                    // has extended cost
    EXPECT_TRUE(written.ReadBit());
    EXPECT_FALSE(written.ReadBit());
    EXPECT_TRUE(written.ReadBit());                     // no extended cost
    EXPECT_TRUE(written.ReadBit());
    written.ReadGuidMask(vendor, { 4, 0, 2 });
    EXPECT_EQ(written.read<uint8>(), 2);                // reason

    EXPECT_EQ(written.read<int32>(), -1);               // quantity
    EXPECT_EQ(written.read<uint32>(), 100u);            // price
    EXPECT_EQ(written.read<uint32>(), uint32(ITEM_VENDOR_TYPE_ITEM));
    EXPECT_EQ(written.read<int32>(), 50);               // durability
    EXPECT_EQ(written.read<uint32>(), 7u);              // display id
    EXPECT_EQ(written.read<uint32>(), 1u);              // stack count
    EXPECT_EQ(written.read<uint32>(), 0xABCDu);         // item
    EXPECT_EQ(written.read<uint32>(), 2u);              // extended cost
    EXPECT_EQ(written.read<int32>(), 0);                // player condition
    EXPECT_EQ(written.read<int32>(), 1);                // slot, counted from 1

    written.read_skip(4 * 9);                           // the second item has no extended cost

    written.ReadGuidBytes(vendor, { 3, 7, 0, 6, 2, 1, 4, 5 });
    EXPECT_EQ(vendor, packet.Vendor);
    EXPECT_EQ(written.rpos(), written.size());
}

TEST(NPCPacketsTest, VendorInventoryWritesEmptyList)
{
    WorldPackets::NPC::VendorInventory packet;
    packet.Vendor = ObjectGuid::Create<HighGuid::Unit>(911, 79964);
    packet.Reason = 1;                                  // opens the empty window

    WorldPacket written = *packet.Write();
    ObjectGuid vendor;
    written.ReadGuidMask(vendor, { 5, 7, 1, 3, 6 });
    EXPECT_EQ(written.ReadBits(18), 0u);
    written.ReadGuidMask(vendor, { 4, 0, 2 });
    EXPECT_EQ(written.read<uint8>(), 1);
    written.ReadGuidBytes(vendor, { 3, 7, 0, 6, 2, 1, 4, 5 });
    EXPECT_EQ(vendor, packet.Vendor);
    EXPECT_EQ(written.rpos(), written.size());
}

TEST(NPCPacketsTest, TrainerListWrites548Layout)
{
    EXPECT_EQ(uint16(SMSG_TRAINER_LIST), 0x189F);

    WorldPackets::NPC::TrainerList packet;
    packet.TrainerGUID = ObjectGuid::Create<HighGuid::Unit>(911, 79964);
    packet.TrainerType = 0;     // class trainer
    packet.TrainerID = 2;       // the trainer table id, echoed back by the buy request
    packet.Greeting = "Hello";

    WorldPackets::NPC::TrainerListSpell spell;
    spell.SpellID = 6673;       // Battle Shout
    spell.Usable = 1;           // Known = 0, Available = 1, Unavailable = 2
    spell.MoneyCost = 1234;
    spell.ReqLevel = 5;
    spell.ReqSkillLine = 333;
    spell.ReqSkillRank = 100;
    spell.ReqAbility = { 78, 100, 0 };
    packet.Spells.push_back(spell);

    WorldPacket written = *packet.Write();
    ObjectGuid trainer;
    written.ReadGuidMask(trainer, { 4, 5 });
    ASSERT_EQ(written.ReadBits(19), 1u);
    ASSERT_EQ(written.ReadBits(11), 5u);
    written.ReadGuidMask(trainer, { 6, 2, 7, 1, 3, 0 });
    written.ReadGuidBytes(trainer, { 4 });

    EXPECT_EQ(written.read<uint8>(), 5);                // required level
    EXPECT_EQ(written.read<uint32>(), 1234u);           // cost
    EXPECT_EQ(written.read<uint32>(), 6673u);           // spell
    EXPECT_EQ(written.read<int32>(), 78);               // required abilities
    EXPECT_EQ(written.read<int32>(), 100);
    EXPECT_EQ(written.read<int32>(), 0);
    EXPECT_EQ(written.read<uint32>(), 333u);            // skill line
    EXPECT_EQ(written.read<uint32>(), 100u);            // skill rank
    EXPECT_EQ(written.read<uint8>(), 1);                // state

    EXPECT_EQ(written.ReadString(5), "Hello");
    written.ReadGuidBytes(trainer, { 6, 7, 1, 3 });
    EXPECT_EQ(written.read<uint32>(), 2u);              // trainer id
    written.ReadGuidBytes(trainer, { 5, 0, 2 });
    EXPECT_EQ(written.read<uint32>(), 0u);              // trainer type
    EXPECT_EQ(trainer, packet.TrainerGUID);
    EXPECT_EQ(written.rpos(), written.size());
}

TEST(NPCPacketsTest, TrainerBuySpellReads548Layout)
{
    EXPECT_EQ(uint16(CMSG_TRAINER_BUY_SPELL), 0x0352);

    ObjectGuid trainer = ObjectGuid::Create<HighGuid::Unit>(911, 79964);
    WorldPacket raw(CMSG_TRAINER_BUY_SPELL, 17);
    raw << int32(6673);                                 // spell
    raw << int32(2);                                    // trainer id from the list
    raw.WriteGuidMask(trainer, { 1, 4, 0, 6, 3, 2, 5, 7 });
    raw.FlushBits();
    raw.WriteGuidBytes(trainer, { 3, 1, 4, 7, 0, 5, 6, 2 });

    WorldPackets::NPC::TrainerBuySpell packet(std::move(raw));
    packet.Read();
    EXPECT_EQ(packet.TrainerGUID, trainer);
    EXPECT_EQ(packet.TrainerID, 2);
    EXPECT_EQ(packet.SpellID, 6673);
    EXPECT_EQ(packet.GetRawPacket()->rpos(), packet.GetSize());
}

TEST(NPCPacketsTest, TrainerBuySpellRejectsMissingTrainerId)
{
    WorldPacket raw(CMSG_TRAINER_BUY_SPELL, 4);
    raw << int32(6673);
    WorldPackets::NPC::TrainerBuySpell packet(std::move(raw));
    EXPECT_THROW(packet.Read(), ByteBufferException);
}

TEST(NPCPacketsTest, TrainerBuyFailedWrites548Layout)
{
    EXPECT_EQ(uint16(SMSG_TRAINER_BUY_FAILED), 0x042E);

    WorldPackets::NPC::TrainerBuyFailed packet;
    packet.TrainerGUID = ObjectGuid::Create<HighGuid::Unit>(911, 79964);
    packet.SpellID = 6673;
    packet.TrainerFailedReason = 1;

    WorldPacket written = *packet.Write();
    ObjectGuid trainer;
    written.ReadGuidMask(trainer, { 3, 0, 4, 7, 6, 1, 5, 2 });
    written.ReadGuidBytes(trainer, { 1, 2, 0, 3, 4 });
    EXPECT_EQ(written.read<int32>(), 1);
    written.ReadGuidBytes(trainer, { 5, 6, 7 });
    EXPECT_EQ(written.read<int32>(), 6673);
    EXPECT_EQ(trainer, packet.TrainerGUID);
    EXPECT_EQ(written.rpos(), written.size());
}
