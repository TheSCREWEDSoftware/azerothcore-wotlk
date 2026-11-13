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

void WorldPackets::GuildFinder::AddRecruit::Read()
{
    uint32 classRoles = 0;
    uint32 interests = 0;
    uint32 availability = 0;
    _worldPacket >> classRoles >> interests >> availability;
    ClassRoles = classRoles;
    Interests = interests;
    Availability = availability;

    // The two string bit-lengths are interleaved with the guid's presence-mask bits in the real
    // capture, so this one packet reads its guid by hand instead of via ReadGuidBits/ReadGuidBytes.
    bool mask[8] = {};
    mask[3] = _worldPacket.ReadBit();
    mask[0] = _worldPacket.ReadBit();
    mask[6] = _worldPacket.ReadBit();
    mask[1] = _worldPacket.ReadBit();
    uint16 commentLength = _worldPacket.ReadBits(11);
    mask[5] = _worldPacket.ReadBit();
    mask[4] = _worldPacket.ReadBit();
    mask[7] = _worldPacket.ReadBit();
    uint8 nameLength = _worldPacket.ReadBits(7);
    mask[2] = _worldPacket.ReadBit();

    uint8 bytes[8] = {};
    if (mask[4]) _worldPacket >> bytes[4];
    if (mask[5]) _worldPacket >> bytes[5];
    Comment = _worldPacket.ReadString(commentLength);
    _worldPacket.ReadString(nameLength); // applicant's own name, unused server-side (see header)
    if (mask[7]) _worldPacket >> bytes[7];
    if (mask[2]) _worldPacket >> bytes[2];
    if (mask[0]) _worldPacket >> bytes[0];
    if (mask[6]) _worldPacket >> bytes[6];
    if (mask[1]) _worldPacket >> bytes[1];
    if (mask[3]) _worldPacket >> bytes[3];

    GuildGuid = AssembleGuid(bytes);
}

void WorldPackets::GuildFinder::Browse::Read()
{
    uint32 classRoles = 0;
    uint32 availability = 0;
    uint32 interests = 0;
    uint32 level = 0;
    _worldPacket >> classRoles >> availability >> interests >> level;
    ClassRoles = classRoles;
    Availability = availability;
    Interests = interests;
    PlayerLevel = level;
}

WorldPacket const* WorldPackets::GuildFinder::BrowseUpdated::Write()
{
    _worldPacket.WriteBits(Guilds.size(), 19);

    ByteBuffer bufferData;
    for (BrowseEntry const& entry : Guilds)
    {
        WriteGuidBits(_worldPacket, entry.GuildGuid, { 7, 5 });
        _worldPacket.WriteBits(entry.GuildName.size(), 8);
        WriteGuidBits(_worldPacket, entry.GuildGuid, { 0 });
        _worldPacket.WriteBits(entry.Comment.size(), 11);
        WriteGuidBits(_worldPacket, entry.GuildGuid, { 4, 1, 2, 6, 3 });

        bufferData << uint32(entry.EmblemColor);
        bufferData << uint32(entry.BorderStyle);
        bufferData << uint32(entry.EmblemStyle);

        bufferData.WriteString(entry.Comment);

        bufferData << uint8(0); // Unk

        WriteGuidBytes(bufferData, entry.GuildGuid, { 5 });

        bufferData << uint32(entry.Interests);

        WriteGuidBytes(bufferData, entry.GuildGuid, { 6, 4 });

        bufferData << uint32(entry.GuildLevel);

        bufferData.WriteString(entry.GuildName);

        bufferData << uint32(entry.AchievementPoints);

        WriteGuidBytes(bufferData, entry.GuildGuid, { 7 });

        bufferData << uint8(entry.RequestPending);

        WriteGuidBytes(bufferData, entry.GuildGuid, { 2, 0 });

        bufferData << uint32(entry.Availability);

        WriteGuidBytes(bufferData, entry.GuildGuid, { 1 });

        bufferData << uint32(entry.BackgroundColor);
        bufferData << uint32(0); // Unk Int (+128) - always 0 or 1 in TrinityCore's own capture; its
                                  // meaning was never identified upstream either, sent as 0.
        bufferData << uint32(entry.BorderColor);
        bufferData << uint32(entry.ClassRoles);

        WriteGuidBytes(bufferData, entry.GuildGuid, { 3 });

        bufferData << uint32(entry.MemberCount);
    }

    _worldPacket.FlushBits();
    _worldPacket.append(bufferData);

    return &_worldPacket;
}

void WorldPackets::GuildFinder::DeclineRecruit::Read()
{
    bool mask[8] = {};
    ReadGuidBits(_worldPacket, mask, { 1, 4, 5, 2, 6, 7, 0, 3 });

    uint8 bytes[8] = {};
    ReadGuidBytes(_worldPacket, bytes, mask, { 5, 7, 2, 3, 4, 1, 0, 6 });

    PlayerGuid = AssembleGuid(bytes);
}

WorldPacket const* WorldPackets::GuildFinder::MembershipListUpdated::Write()
{
    _worldPacket.WriteBits(Applications.size(), 20);

    if (!Applications.empty())
    {
        ByteBuffer bufferData;
        for (MembershipApplication const& app : Applications)
        {
            WriteGuidBits(_worldPacket, app.GuildGuid, { 1, 0, 5 });
            _worldPacket.WriteBits(app.Comment.size(), 11);
            WriteGuidBits(_worldPacket, app.GuildGuid, { 3, 7, 4, 6, 2 });
            _worldPacket.WriteBits(app.GuildName.size(), 8);

            WriteGuidBytes(bufferData, app.GuildGuid, { 2 });
            bufferData.WriteString(app.Comment);
            WriteGuidBytes(bufferData, app.GuildGuid, { 5 });
            bufferData.WriteString(app.GuildName);

            bufferData << uint32(app.Availability);
            bufferData << uint32(app.SecondsUntilExpiry);

            WriteGuidBytes(bufferData, app.GuildGuid, { 0, 6, 3, 7 });

            bufferData << uint32(app.ClassRoles);

            WriteGuidBytes(bufferData, app.GuildGuid, { 4, 1 });

            bufferData << uint32(app.SecondsSinceSubmit);
            bufferData << uint32(app.Interests);
        }

        _worldPacket.FlushBits();
        _worldPacket.append(bufferData);
    }

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
    _worldPacket << uint32(0); // "Unk time" in TrinityCore's own capture - always sent as the
                                // current server time there; the client-side meaning was never
                                // identified upstream, and nothing here depends on a caller-visible
                                // value, so this is a fixed placeholder rather than plumbing
                                // GameTime through a packet field for a value nothing consumes.

    return &_worldPacket;
}

WorldPacket const* WorldPackets::GuildFinder::PostUpdated::Write()
{
    _worldPacket.WriteBit(IsGuildMaster);

    if (IsGuildMaster)
    {
        _worldPacket.WriteBits(Comment.size(), 11);
        _worldPacket.WriteBit(Listed);
        // TrinityCore's own capture goes straight from WriteBit() into raw '<<' writes here without
        // an intervening FlushBits(). That leaves a partial bit-byte (11+1=12 bits, not
        // byte-aligned) sitting in ByteBuffer's internal bit accumulator, never appended to the
        // wire, when this packet's other fields are plain '<<' writes into the very same buffer
        // (unlike BrowseUpdated/MembershipListUpdated/RecruitListUpdated below, which only ever
        // write bits into _worldPacket and put every raw-byte field into a *separate* ByteBuffer
        // appended afterward - a pattern that sidesteps this issue entirely). Flushing explicitly
        // here fixes what looks like an upstream oversight rather than reproducing it.
        _worldPacket.FlushBits();
        _worldPacket << uint32(Level);
        _worldPacket.WriteString(Comment);
        _worldPacket << uint32(0); // Unk Int32 - unidentified upstream, always 0 there too.
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
    ReadGuidBits(_worldPacket, mask, { 0, 4, 3, 5, 7, 6, 2, 1 });

    uint8 bytes[8] = {};
    ReadGuidBytes(_worldPacket, bytes, mask, { 4, 0, 3, 6, 5, 1, 2, 7 });

    GuildGuid = AssembleGuid(bytes);
}

void WorldPackets::GuildFinder::SetGuildPost::Read()
{
    uint32 level = 0;
    uint32 availability = 0;
    uint32 interests = 0;
    uint32 classRoles = 0;
    _worldPacket >> level >> availability >> interests >> classRoles;
    Level = level;
    Availability = availability;
    Interests = interests;
    ClassRoles = classRoles;

    uint16 length = _worldPacket.ReadBits(11);
    Listed = _worldPacket.ReadBit();
    Comment = _worldPacket.ReadString(length);
}
