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

#include "GameTime.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// 4.3.4: the client asks for Item.db2 / Item-sparse.db2 records it does not have (items added after 15595, or ones the
// server changed) with CMSG_REQUEST_HOTFIX (CMSG_DB_QUERY_BULK here) and waits for SMSG_DB_REPLY. The records are built
// from item_template, as TrinityCore 4.3.4 does (DB2Utilities::WriteItemDbReply / WriteItemSparseDbReply).
namespace
{
    enum Db2Hash : uint32
    {
        DB2_HASH_ITEM        = 0x50238EC2,   // Item.db2
        DB2_HASH_ITEM_SPARSE = 0x919BE54E    // Item-sparse.db2
    };

    void WriteItemRecord(ItemTemplate const* proto, ByteBuffer& buffer)
    {
        buffer << uint32(proto->ItemId);
        buffer << uint32(proto->Class);
        buffer << uint32(proto->SubClass);
        buffer << int32(proto->SoundOverrideSubclass);
        buffer << uint32(proto->Material);
        buffer << uint32(proto->DisplayInfoID);
        buffer << uint32(proto->InventoryType);
        buffer << uint32(proto->Sheath);
    }

    void WriteItemSparseRecord(ItemTemplate const* proto, LocaleConstant locale, ByteBuffer& buffer)
    {
        ItemLocale const* localeData = locale != LOCALE_enUS ? sObjectMgr->GetItemLocale(proto->ItemId) : nullptr;

        buffer << uint32(proto->ItemId);
        buffer << uint32(proto->Quality);
        buffer << uint32(proto->Flags);
        buffer << uint32(proto->Flags2);
        buffer << float(proto->PriceRandomValue);
        buffer << float(proto->PriceVariance);
        buffer << uint32(proto->BuyCount);
        buffer << int32(proto->BuyPrice);
        buffer << uint32(proto->SellPrice);
        buffer << uint32(proto->InventoryType);
        buffer << int32(proto->AllowableClass);
        buffer << int32(proto->AllowableRace);
        buffer << uint32(proto->ItemLevel);
        buffer << uint32(proto->RequiredLevel);
        buffer << uint32(proto->RequiredSkill);
        buffer << uint32(proto->RequiredSkillRank);
        buffer << uint32(proto->RequiredSpell);
        buffer << uint32(proto->RequiredHonorRank);
        buffer << uint32(proto->RequiredCityRank);
        buffer << uint32(proto->RequiredReputationFaction);
        buffer << uint32(proto->RequiredReputationRank);
        buffer << int32(proto->MaxCount);
        buffer << int32(proto->Stackable);
        buffer << uint32(proto->ContainerSlots);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            buffer << uint32(proto->ItemStat[i].ItemStatType);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            buffer << int32(proto->ItemStat[i].ItemStatValue);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            buffer << int32(0);                         // ItemStatUnk1 (socket/scaling allocation, not kept in item_template)

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            buffer << int32(0);                         // ItemStatUnk2

        buffer << uint32(proto->ScalingStatDistribution);
        buffer << uint32(proto->Damage[0].DamageType);
        buffer << uint32(proto->Delay);
        buffer << float(proto->RangedModRange);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            buffer << int32(proto->Spells[i].SpellId);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            buffer << uint32(proto->Spells[i].SpellTrigger);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            buffer << int32(proto->Spells[i].SpellCharges);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            buffer << int32(proto->Spells[i].SpellCooldown);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            buffer << uint32(proto->Spells[i].SpellCategory);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            buffer << int32(proto->Spells[i].SpellCategoryCooldown);

        buffer << uint32(proto->Bonding);

        std::string name = proto->Name1;
        if (localeData)
            ObjectMgr::GetLocaleString(localeData->Name, locale, name);

        buffer << uint16(name.length());
        if (!name.empty())
            buffer << name;

        for (uint32 i = 0; i < 3; ++i)                 // the other three names
            buffer << uint16(0);

        std::string description = proto->Description;
        if (localeData)
            ObjectMgr::GetLocaleString(localeData->Description, locale, description);

        buffer << uint16(description.length());
        if (!description.empty())
            buffer << description;

        buffer << uint32(proto->PageText);
        buffer << uint32(proto->LanguageID);
        buffer << uint32(proto->PageMaterial);
        buffer << uint32(proto->StartQuest);
        buffer << uint32(proto->LockID);
        buffer << int32(proto->Material);
        buffer << uint32(proto->Sheath);
        buffer << int32(proto->RandomProperty);
        buffer << int32(proto->RandomSuffix);
        buffer << uint32(proto->ItemSet);
        buffer << uint32(proto->Area);
        buffer << uint32(proto->Map);
        buffer << uint32(proto->BagFamily);
        buffer << uint32(proto->TotemCategory);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
            buffer << uint32(proto->Socket[i].Color);

        for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
            buffer << uint32(proto->Socket[i].Content);

        buffer << uint32(proto->socketBonus);
        buffer << uint32(proto->GemProperties);
        buffer << float(proto->ArmorDamageModifier);
        buffer << int32(proto->Duration);
        buffer << uint32(proto->ItemLimitCategory);
        buffer << uint32(proto->HolidayId);
        buffer << float(0.0f);                          // StatScalingFactor
        buffer << uint32(0);                            // CurrencySubstitutionId
        buffer << uint32(0);                            // CurrencySubstitutionCount
    }
}

void WorldSession::HandleRequestHotfix(WorldPacket& recvPacket)
{
    uint32 type = recvPacket.read<uint32>();
    if (type != DB2_HASH_ITEM && type != DB2_HASH_ITEM_SPARSE)
    {
        LOG_DEBUG("network", "CMSG_REQUEST_HOTFIX: {} asked for records of unsupported DB2 0x{:08X}", GetPlayerInfo(), type);
        recvPacket.rfinish();
        return;
    }

    uint32 count = recvPacket.ReadBits(23);
    std::vector<ObjectGuid> guids(count);
    for (uint32 i = 0; i < count; ++i)
    {
        guids[i][0] = recvPacket.ReadBit();
        guids[i][4] = recvPacket.ReadBit();
        guids[i][7] = recvPacket.ReadBit();
        guids[i][2] = recvPacket.ReadBit();
        guids[i][5] = recvPacket.ReadBit();
        guids[i][3] = recvPacket.ReadBit();
        guids[i][6] = recvPacket.ReadBit();
        guids[i][1] = recvPacket.ReadBit();
    }

    for (uint32 i = 0; i < count; ++i)
    {
        uint32 entry = 0;
        recvPacket.ReadByteSeq(guids[i][5]);
        recvPacket.ReadByteSeq(guids[i][6]);
        recvPacket.ReadByteSeq(guids[i][7]);
        recvPacket.ReadByteSeq(guids[i][0]);
        recvPacket.ReadByteSeq(guids[i][1]);
        recvPacket.ReadByteSeq(guids[i][3]);
        recvPacket.ReadByteSeq(guids[i][4]);
        recvPacket >> entry;
        recvPacket.ReadByteSeq(guids[i][2]);

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry);
        if (!proto)
        {
            WorldPacket data(SMSG_DB_REPLY, 4 * 4);
            data << -int32(entry);                      // negative entry: record does not exist
            data << uint32(type);
            data << uint32(GameTime::GetGameTime().count());
            data << uint32(0);
            SendPacket(&data);
            continue;
        }

        WorldPacket data(SMSG_DB_REPLY, 4 * 4 + 400);
        data << int32(entry);
        data << uint32(type);
        data << uint32(0);                              // hotfix timestamp: no hotfix_data table, the record is always current

        std::size_t sizePos = data.wpos();
        data << uint32(0);                              // record size, filled below
        if (type == DB2_HASH_ITEM)
            WriteItemRecord(proto, data);
        else
            WriteItemSparseRecord(proto, GetSessionDbcLocale(), data);

        data.put<uint32>(sizePos, uint32(data.wpos() - sizePos - 4));
        SendPacket(&data);
    }
}
