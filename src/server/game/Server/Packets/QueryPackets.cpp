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

#include "QueryPackets.h"

WorldPacket const* WorldPackets::Query::QuestNPCQueryResponse::Write()
{
    // 5.4.8 18414
    _worldPacket.WriteBits(Quests.size(), 21);
    for (auto const& [quest, entries] : Quests)
        _worldPacket.WriteBits(entries.size(), 22);
    _worldPacket.FlushBits();

    for (auto const& [quest, entries] : Quests)
    {
        _worldPacket << quest;
        for (uint32 entry : entries)
            _worldPacket << entry;
    }
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Query::HotfixNotifyBlob::Write()
{
    // 5.4.8 18414
    _worldPacket.WriteBits(Hotfixes.size(), 20);
    _worldPacket.FlushBits();

    for (Record const& hotfix : Hotfixes)
    {
        _worldPacket << hotfix.Timestamp;
        _worldPacket << hotfix.Entry;
        _worldPacket << hotfix.TableHash;
    }

    return &_worldPacket;
}

// 5.4.8 18414, from Project SkyFire (WorldSession::HandleNameQueryOpcode)
void WorldPackets::Query::NameQuery::Read()
{
    Guid[4] = _worldPacket.ReadBit();
    bool hasVirtualRealm = _worldPacket.ReadBit();
    for (uint8 index : { 6, 0, 7, 1 })
        Guid[index] = _worldPacket.ReadBit();
    bool hasNativeRealm = _worldPacket.ReadBit();
    for (uint8 index : { 5, 2, 3 })
        Guid[index] = _worldPacket.ReadBit();

    for (uint8 index : { 7, 5, 1, 2, 6, 3, 0, 4 })
        _worldPacket.ReadByteSeq(Guid[index]);

    if (hasVirtualRealm)
        VirtualRealmAddress = _worldPacket.read<uint32>();

    if (hasNativeRealm)
        NativeRealmAddress = _worldPacket.read<uint32>();
}

// 5.4.8 18414, from Project SkyFire (WorldSession::SendNameQueryOpcode)
WorldPacket const* WorldPackets::Query::NameQueryResponse::Write()
{
    ObjectGuid const accountGuid;                           // battle.net account guid, not used
    for (uint8 index : { 3, 6, 7, 2, 5, 4, 0, 1 })
        _worldPacket.WriteBit(Guid[index]);
    _worldPacket.FlushBits();

    for (uint8 index : { 5, 4, 7, 6, 1, 2 })
        _worldPacket.WriteByteSeq(Guid[index]);

    _worldPacket << uint8(NameUnknown);

    if (!NameUnknown)
    {
        _worldPacket << uint32(VirtualRealmAddress);
        _worldPacket << uint32(AccountID);
        _worldPacket << uint8(Class);
        _worldPacket << uint8(Race);
        _worldPacket << uint8(Level);
        _worldPacket << uint8(Sex);
    }

    _worldPacket.WriteByteSeq(Guid[0]);
    _worldPacket.WriteByteSeq(Guid[3]);

    if (NameUnknown)
        return &_worldPacket;

    _worldPacket.WriteBit(accountGuid[2]);
    _worldPacket.WriteBit(accountGuid[7]);
    _worldPacket.WriteBit(Guid[7]);
    _worldPacket.WriteBit(Guid[2]);
    _worldPacket.WriteBit(Guid[0]);
    _worldPacket.WriteBit(false);                           // deleted
    _worldPacket.WriteBit(accountGuid[4]);
    _worldPacket.WriteBit(Guid[5]);
    _worldPacket.WriteBit(accountGuid[1]);
    _worldPacket.WriteBit(accountGuid[3]);
    _worldPacket.WriteBit(accountGuid[0]);

    for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        _worldPacket.WriteBits(Declined ? DeclinedNames.name[i].size() : 0, 7);

    _worldPacket.WriteBit(Guid[6]);
    _worldPacket.WriteBit(Guid[3]);
    _worldPacket.WriteBit(accountGuid[5]);
    _worldPacket.WriteBit(Guid[1]);
    _worldPacket.WriteBit(Guid[4]);
    _worldPacket.WriteBits(Name.size(), 6);
    _worldPacket.WriteBit(accountGuid[6]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(Guid[6]);
    _worldPacket.WriteByteSeq(Guid[0]);
    _worldPacket.WriteString(Name);
    _worldPacket.WriteByteSeq(accountGuid[5]);
    _worldPacket.WriteByteSeq(accountGuid[2]);
    _worldPacket.WriteByteSeq(Guid[3]);
    _worldPacket.WriteByteSeq(accountGuid[4]);
    _worldPacket.WriteByteSeq(accountGuid[3]);
    _worldPacket.WriteByteSeq(Guid[4]);
    _worldPacket.WriteByteSeq(Guid[2]);
    _worldPacket.WriteByteSeq(accountGuid[7]);

    if (Declined)
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            _worldPacket.WriteString(DeclinedNames.name[i]);

    _worldPacket.WriteByteSeq(accountGuid[6]);
    _worldPacket.WriteByteSeq(Guid[7]);
    _worldPacket.WriteByteSeq(Guid[1]);
    _worldPacket.WriteByteSeq(accountGuid[1]);
    _worldPacket.WriteByteSeq(Guid[5]);
    _worldPacket.WriteByteSeq(accountGuid[0]);

    return &_worldPacket;
}

// 5.4.8 18414, from Project SkyFire (WorldSession::HandleCreatureQueryOpcode)
WorldPacket const* WorldPackets::Query::QueryCreatureResponse::Write()
{
    _worldPacket << uint32(CreatureID);
    _worldPacket.WriteBit(Allow);

    if (!Allow)
    {
        _worldPacket.FlushBits();
        return &_worldPacket;
    }

    _worldPacket.WriteBits(Stats.Title.empty() ? 0 : Stats.Title.size() + 1, 11);
    _worldPacket.WriteBits(Stats.QuestItems.size(), 22);
    _worldPacket.WriteBits(0, 11);                          // title alt
    // Name[0..3] and NameAlt[0..3], only the first name is filled
    _worldPacket.WriteBits(Stats.Name[0].size() + 1, 11);
    for (std::size_t i = 1; i < 8; ++i)
        _worldPacket.WriteBits(0, 11);
    _worldPacket.WriteBit(Stats.Leader);
    _worldPacket.WriteBits(Stats.CursorName.size() + 1, 6);
    _worldPacket.FlushBits();

    _worldPacket << uint32(Stats.ProxyCreatureID[0]);
    _worldPacket << uint32(Stats.CreatureDisplayID[3]);
    _worldPacket << uint32(Stats.CreatureDisplayID[1]);
    _worldPacket << uint32(Stats.RequiredExpansion);
    _worldPacket << uint32(Stats.CreatureType);
    _worldPacket << float(Stats.HpMulti);
    _worldPacket << uint32(Stats.Flags[0]);
    _worldPacket << uint32(Stats.Flags[1]);
    _worldPacket << uint32(Stats.Classification);
    _worldPacket << uint32(Stats.CreatureMovementInfoID);
    _worldPacket << Stats.Name[0];

    if (!Stats.Title.empty())
        _worldPacket << Stats.Title;

    _worldPacket << uint32(Stats.CreatureDisplayID[0]);
    _worldPacket << uint32(Stats.CreatureDisplayID[2]);
    _worldPacket << Stats.CursorName;

    for (uint32 item : Stats.QuestItems)
        _worldPacket << uint32(item);

    _worldPacket << uint32(Stats.ProxyCreatureID[1]);
    _worldPacket << float(Stats.EnergyMulti);
    _worldPacket << uint32(Stats.CreatureFamily);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Query::TimeQueryResponse::Write()
{
    _worldPacket << ServerTime;
    _worldPacket << TimeResponse;

    return &_worldPacket;
}

void WorldPackets::Query::CorpseMapPositionQuery::Read()
{
    // 5.4.8 18414
    for (uint8 index : { 7, 6, 3, 0, 4, 1, 5, 2 })
        CorpseGuid[index] = _worldPacket.ReadBit();
    for (uint8 index : { 1, 6, 0, 5, 3, 2, 4, 7 })
        _worldPacket.ReadByteSeq(CorpseGuid[index]);
}
