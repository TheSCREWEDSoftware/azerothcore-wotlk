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

#include "GuildFinderPackets.h"
#include "GameTime.h"

// The Guild Finder opcodes carry their ObjectGuid using real Cata's bit-packed wire format (a
// per-byte "is this byte present" bitmask written/read in one client-chosen order, followed by the
// actual non-zero bytes in a second order) rather than this fork's usual classic pack-guid scheme
// (ByteBuffer::appendPackGUID/readPackGUID) - same situation as CMSG_REFORGE_ITEM
// (see ReforgePackets.cpp). The exact mask/byte orders below are transcribed unchanged from
// TrinityCore's real client-capture-derived GuildFinderHandler.cpp for each opcode; they are not
// interchangeable between opcodes (each one is its own capture).
namespace
{
    void WriteGuidBits(ByteBuffer& buffer, ObjectGuid const& guid, std::initializer_list<uint8> maskOrder)
    {
        uint64 raw = guid.GetRawValue();
        for (uint8 byteIndex : maskOrder)
            buffer.WriteBit(uint32((raw >> (byteIndex * 8)) & 0xFF));
    }

    void WriteGuidBytes(ByteBuffer& buffer, ObjectGuid const& guid, std::initializer_list<uint8> byteOrder)
    {
        uint64 raw = guid.GetRawValue();
        for (uint8 byteIndex : byteOrder)
        {
            uint8 byteVal = uint8((raw >> (byteIndex * 8)) & 0xFF);
            if (byteVal)
                buffer << byteVal;
        }
    }

    void ReadGuidBits(ByteBuffer& buffer, bool (&mask)[8], std::initializer_list<uint8> maskOrder)
    {
        for (uint8 byteIndex : maskOrder)
            mask[byteIndex] = buffer.ReadBit();
    }

    void ReadGuidBytes(ByteBuffer& buffer, uint8 (&bytes)[8], bool const (&mask)[8], std::initializer_list<uint8> byteOrder)
    {
        for (uint8 byteIndex : byteOrder)
            if (mask[byteIndex])
                buffer >> bytes[byteIndex];
    }

    ObjectGuid AssembleGuid(uint8 const (&bytes)[8])
    {
        uint64 raw = 0;
        for (uint8 i = 0; i < 8; ++i)
            raw |= uint64(bytes[i]) << (i * 8);
        return ObjectGuid(raw);
    }
}

// 5.4.8 18414 layouts, from Project SkyFire
void WorldPackets::GuildFinder::AddRecruit::Read()
{
    uint32 classRoles = 0;
    uint32 availability = 0;
    uint32 interests = 0;
    _worldPacket >> classRoles >> availability >> interests;
    ClassRoles = classRoles;
    Availability = availability;
    Interests = interests;

    bool mask[8] = {};
    ReadGuidBits(_worldPacket, mask, { 7, 5, 2, 6, 1, 0 });
    uint16 commentLength = _worldPacket.ReadBits(10);
    ReadGuidBits(_worldPacket, mask, { 3, 4 });

    uint8 bytes[8] = {};
    ReadGuidBytes(_worldPacket, bytes, mask, { 4, 0, 2 });
    Comment = _worldPacket.ReadString(commentLength);
    ReadGuidBytes(_worldPacket, bytes, mask, { 6, 1, 5, 7, 3 });

    GuildGuid = AssembleGuid(bytes);
}

void WorldPackets::GuildFinder::Browse::Read()
{
    uint32 level = 0;
    uint32 availability = 0;
    uint32 classRoles = 0;
    uint32 interests = 0;
    _worldPacket >> level >> availability >> classRoles >> interests;
    PlayerLevel = level;
    Availability = availability;
    ClassRoles = classRoles;
    Interests = interests;
}

WorldPacket const* WorldPackets::GuildFinder::BrowseUpdated::Write()
{
    _worldPacket.WriteBits(Guilds.size(), 18);

    ByteBuffer bufferData;
    for (BrowseEntry const& entry : Guilds)
    {
        WriteGuidBits(_worldPacket, entry.GuildGuid, { 6, 5, 4, 0, 1 });
        _worldPacket.WriteBits(entry.Comment.size(), 10);
        WriteGuidBits(_worldPacket, entry.GuildGuid, { 3 });
        _worldPacket.WriteBits(entry.GuildName.size(), 7);
        WriteGuidBits(_worldPacket, entry.GuildGuid, { 7, 2 });

        WriteGuidBytes(bufferData, entry.GuildGuid, { 3 });
        bufferData << uint32(entry.EmblemStyle);
        bufferData << uint8(entry.RequestPending);
        WriteGuidBytes(bufferData, entry.GuildGuid, { 0 });
        bufferData << uint32(entry.AchievementPoints);
        WriteGuidBytes(bufferData, entry.GuildGuid, { 2 });
        bufferData << uint32(entry.Interests);
        bufferData << uint32(entry.BackgroundColor);
        bufferData << uint32(entry.GuildLevel);
        bufferData << uint32(entry.Availability);
        bufferData << uint32(entry.ClassRoles);
        WriteGuidBytes(bufferData, entry.GuildGuid, { 5 });
        bufferData << uint8(0);                                 // unknown
        bufferData.WriteString(entry.GuildName);
        bufferData << uint32(0);                                // unknown
        bufferData << uint32(entry.BorderStyle);
        WriteGuidBytes(bufferData, entry.GuildGuid, { 7 });
        bufferData << uint32(entry.EmblemColor);
        WriteGuidBytes(bufferData, entry.GuildGuid, { 6 });
        bufferData << uint32(0);                                // unknown
        bufferData.WriteString(entry.Comment);
        bufferData << uint32(entry.BorderColor);
        bufferData << uint32(entry.MemberCount);
        WriteGuidBytes(bufferData, entry.GuildGuid, { 1, 4 });
    }

    _worldPacket.FlushBits();
    _worldPacket.append(bufferData);

    return &_worldPacket;
}

void WorldPackets::GuildFinder::DeclineRecruit::Read()
{
    bool mask[8] = {};
    ReadGuidBits(_worldPacket, mask, { 6, 7, 3, 1, 2, 0, 4, 5 });

    uint8 bytes[8] = {};
    ReadGuidBytes(_worldPacket, bytes, mask, { 0, 7, 1, 6, 4, 3, 5, 2 });

    PlayerGuid = AssembleGuid(bytes);
}

WorldPacket const* WorldPackets::GuildFinder::MembershipListUpdated::Write()
{
    _worldPacket.WriteBits(Applications.size(), 20);

    ByteBuffer bufferData;
    for (MembershipApplication const& app : Applications)
    {
        // SkyFire lists guid bit 2 twice and never bit 1; the second one is taken as bit 1
        WriteGuidBits(_worldPacket, app.GuildGuid, { 0, 4, 2, 7 });
        _worldPacket.WriteBits(app.GuildName.size(), 7);
        WriteGuidBits(_worldPacket, app.GuildGuid, { 1, 3 });
        _worldPacket.WriteBits(app.Comment.size(), 10);
        WriteGuidBits(_worldPacket, app.GuildGuid, { 6, 5 });

        bufferData << uint32(app.Interests);
        bufferData << uint32(0);                                // unknown
        bufferData.WriteString(app.GuildName);
        WriteGuidBytes(bufferData, app.GuildGuid, { 4 });
        bufferData << uint32(app.ClassRoles);
        WriteGuidBytes(bufferData, app.GuildGuid, { 6, 5 });
        bufferData << uint32(app.SecondsSinceSubmit);
        WriteGuidBytes(bufferData, app.GuildGuid, { 1, 3, 0, 7, 2 });
        bufferData << uint32(app.SecondsUntilExpiry);
        bufferData << uint32(app.Availability);
        bufferData.WriteString(app.Comment);
    }

    _worldPacket.FlushBits();
    _worldPacket.append(bufferData);
    _worldPacket << uint32(ApplicationsLeft);

    return &_worldPacket;
}

void WorldPackets::GuildFinder::GetRecruits::Read()
{
    uint32 unkTime = 0;
    _worldPacket >> unkTime;
    UnkTime = unkTime;
}

WorldPacket const* WorldPackets::GuildFinder::RecruitListUpdated::Write()
{
    _worldPacket.WriteBits(Recruits.size(), 20);

    ByteBuffer dataBuffer;
    for (RecruitEntry const& recruit : Recruits)
    {
        _worldPacket.WriteBits(recruit.Comment.size(), 11);
        WriteGuidBits(_worldPacket, recruit.PlayerGuid, { 2, 4, 3, 7, 0 });
        _worldPacket.WriteBits(recruit.Name.size(), 7);
        WriteGuidBits(_worldPacket, recruit.PlayerGuid, { 5, 1, 6 });

        WriteGuidBytes(dataBuffer, recruit.PlayerGuid, { 4 });
        dataBuffer << int32(recruit.Available);
        WriteGuidBytes(dataBuffer, recruit.PlayerGuid, { 3, 0, 1 });
        dataBuffer << int32(recruit.Level);
        WriteGuidBytes(dataBuffer, recruit.PlayerGuid, { 6, 7, 2 });
        dataBuffer << int32(recruit.SecondsSinceSubmit);
        dataBuffer << int32(recruit.Availability);
        dataBuffer << int32(recruit.ClassRoles);
        dataBuffer << int32(recruit.Interests);
        dataBuffer << int32(recruit.SecondsUntilExpiry);
        dataBuffer.WriteString(recruit.Name);
        dataBuffer.WriteString(recruit.Comment);
        dataBuffer << int32(recruit.Class);
        WriteGuidBytes(dataBuffer, recruit.PlayerGuid, { 5 });
    }

    _worldPacket.FlushBits();
    _worldPacket.append(dataBuffer);
    _worldPacket << uint32(GameTime::GetGameTime().count());

    return &_worldPacket;
}

WorldPacket const* WorldPackets::GuildFinder::PostUpdated::Write()
{
    _worldPacket.WriteBit(IsGuildMaster);

    if (IsGuildMaster)
    {
        // SkyFire writes the comment before these bits; it goes after them here, the first byte field
        _worldPacket.WriteBit(Listed);
        _worldPacket.WriteBits(Comment.size(), 11);
        _worldPacket.FlushBits();
        _worldPacket.WriteString(Comment);
        _worldPacket << uint32(Level);
        _worldPacket << uint32(0);                              // unknown
        _worldPacket << uint32(Availability);
        _worldPacket << uint32(ClassRoles);
        _worldPacket << uint32(Interests);
    }
    else
        _worldPacket.FlushBits();

    return &_worldPacket;
}

void WorldPackets::GuildFinder::RemoveRecruit::Read()
{
    bool mask[8] = {};
    ReadGuidBits(_worldPacket, mask, { 7, 5, 4, 1, 6, 3, 2, 0 });

    uint8 bytes[8] = {};
    ReadGuidBytes(_worldPacket, bytes, mask, { 6, 3, 7, 1, 2, 0, 5, 4 });

    GuildGuid = AssembleGuid(bytes);
}

void WorldPackets::GuildFinder::SetGuildPost::Read()
{
    uint32 level = 0;
    uint32 availability = 0;
    uint32 classRoles = 0;
    uint32 interests = 0;
    _worldPacket >> level >> availability >> classRoles >> interests;
    Level = level;
    Availability = availability;
    ClassRoles = classRoles;
    Interests = interests;

    uint16 length = _worldPacket.ReadBits(10);
    Listed = _worldPacket.ReadBit();
    Comment = _worldPacket.ReadString(length);
}
