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

#include "AuthenticationPackets.h"
#include <algorithm>
#include <cctype>

WorldPacket const* WorldPackets::Auth::AuthChallenge::Write()
{
    // 5.4.8 18414 (SkyFire_548 WorldSocket::HandleSendAuthSession)
    _worldPacket << uint16(0);
    _worldPacket.append(DosChallenge.data(), DosChallenge.size());
    _worldPacket << DosZeroBits;
    _worldPacket.append(Challenge.data(), Challenge.size());

    return &_worldPacket;
}

void WorldPackets::Auth::AuthSession::Read()
{
    // 5.4.8 18414 (SkyFire_548 WorldSocket::HandleAuthSession)
    _worldPacket >> LoginServerID;
    _worldPacket >> BattlegroupID;
    _worldPacket >> Digest[18];
    _worldPacket >> Digest[14];
    _worldPacket >> Digest[3];
    _worldPacket >> Digest[4];
    _worldPacket >> Digest[0];
    _worldPacket >> RealmID;
    _worldPacket >> Digest[11];
    _worldPacket.read(LocalChallenge);
    _worldPacket >> Digest[19];
    _worldPacket.read_skip<uint8>();
    _worldPacket.read_skip<uint8>();
    _worldPacket >> Digest[2];
    _worldPacket >> Digest[9];
    _worldPacket >> Digest[12];
    _worldPacket >> DosResponse;
    _worldPacket >> RegionID;
    _worldPacket >> Digest[16];
    _worldPacket >> Digest[5];
    _worldPacket >> Digest[6];
    _worldPacket >> Digest[8];
    _worldPacket >> Build;
    _worldPacket >> Digest[17];
    _worldPacket >> Digest[7];
    _worldPacket >> Digest[13];
    _worldPacket >> Digest[15];
    _worldPacket >> Digest[1];
    _worldPacket >> Digest[10];

    uint32 addonInfoSize = 0;
    _worldPacket >> addonInfoSize;
    if (addonInfoSize > _worldPacket.size() - _worldPacket.rpos())
        throw ByteBufferPositionException(false, _worldPacket.rpos(), addonInfoSize, _worldPacket.size());

    if (addonInfoSize)
    {
        AddonInfo.resize(addonInfoSize);
        _worldPacket.read(AddonInfo.contents(), addonInfoSize);
    }

    UseIPv6 = _worldPacket.ReadBit();
    uint32 accountLength = _worldPacket.ReadBits(11);
    if (accountLength > _worldPacket.size() - _worldPacket.rpos())
        throw ByteBufferPositionException(false, _worldPacket.rpos(), accountLength, _worldPacket.size());

    Account.resize(accountLength);
    if (accountLength)
        _worldPacket.read(reinterpret_cast<uint8*>(Account.data()), accountLength);

    if (_worldPacket.rpos() != _worldPacket.size())
        throw ByteBufferInvalidValueException("auth session", "trailing bytes");
}

namespace
{
    // 5.4.8 playable races and classes with the expansion that unlocks them (SkyFire_548 AuthHandler)
    constexpr uint8 RaceExpansions[][2] =
    {
        { 6, 0 }, { 5, 0 }, { 2, 0 }, { 7, 0 }, { 9, 3 }, { 1, 0 }, { 8, 0 }, { 24, 4 },
        { 11, 1 }, { 22, 3 }, { 10, 1 }, { 4, 0 }, { 3, 0 }, { 25, 4 }, { 26, 4 }
    };

    constexpr uint8 ClassExpansions[][2] =
    {
        { 10, 4 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 7, 0 }, { 8, 0 }, { 9, 0 }, { 11, 0 }, { 6, 2 }
    };

    std::string NormalizeRealmName(std::string name)
    {
        name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char c) { return std::isspace(c); }), name.end());
        return name;
    }
}

WorldPacket const* WorldPackets::Auth::AuthResponse::Write()
{
    // 5.4.8 18414 (SkyFire_548 WorldSession::SendAuthResponse)
    _worldPacket.WriteBit(SuccessInfo.has_value());
    if (SuccessInfo)
    {
        std::string const normalizedName = NormalizeRealmName(SuccessInfo->RealmName);

        _worldPacket.WriteBits(1, 21);                      // realm names
        _worldPacket.WriteBits(SuccessInfo->RealmName.size(), 8);
        _worldPacket.WriteBits(normalizedName.size(), 8);
        _worldPacket.WriteBit(true);                        // home realm

        _worldPacket.WriteBits(std::size(ClassExpansions), 23);
        _worldPacket.WriteBits(0, 21);
        _worldPacket.WriteBit(false);
        _worldPacket.WriteBit(false);
        _worldPacket.WriteBit(false);
        _worldPacket.WriteBit(false);
        _worldPacket.WriteBits(std::size(RaceExpansions), 23);
        _worldPacket.WriteBit(false);
    }

    _worldPacket.WriteBit(WaitInfo.has_value());
    if (WaitInfo)
        _worldPacket.WriteBit(WaitInfo->HasFCM);

    _worldPacket.FlushBits();

    if (WaitInfo)
        _worldPacket << uint32(WaitInfo->WaitCount);

    if (SuccessInfo)
    {
        _worldPacket << uint32(SuccessInfo->VirtualRealmID);
        _worldPacket.WriteString(SuccessInfo->RealmName);
        _worldPacket.WriteString(NormalizeRealmName(SuccessInfo->RealmName));

        for (auto const& [race, expansion] : RaceExpansions)
            _worldPacket << uint8(expansion) << uint8(race);

        for (auto const& [classId, expansion] : ClassExpansions)
            _worldPacket << uint8(expansion) << uint8(classId);

        _worldPacket << uint32(SuccessInfo->TimeRemain);
        _worldPacket << uint8(SuccessInfo->ActiveExpansionLevel);
        _worldPacket << uint32(SuccessInfo->TimeSecondsUntilPCKick);
        _worldPacket << uint32(SuccessInfo->TimeRested);
        _worldPacket << uint8(SuccessInfo->AccountExpansionLevel);
        _worldPacket << uint32(0);
        _worldPacket << uint32(0);
        _worldPacket << uint32(0);
    }

    _worldPacket << uint8(Result);

    return &_worldPacket;
}
