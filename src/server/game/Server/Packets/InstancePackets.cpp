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

#include "InstancePackets.h"

WorldPacket const* WorldPackets::Instance::InstanceReset::Write()
{
    _worldPacket << MapId;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Instance::InstanceResetFailed::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteBits(Reason, 2);
    _worldPacket.FlushBits();
    _worldPacket << uint32(MapId);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Instance::SetDungeonDifficulty::Write()
{
    // 5.4.8 18414, from Project SkyFire: the Difficulty.dbc id only
    _worldPacket << uint32(GetClientDungeonDifficulty(::Difficulty(Difficulty)));

    return &_worldPacket;
}

void WorldPackets::Instance::SetDungeonDifficultyClient::Read()
{
    // 5.4.8 sends the Difficulty.dbc id
    Mode = GetDifficultyFromClient(_worldPacket.read<uint32>());
}

WorldPacket const* WorldPackets::Instance::ResetFailedNotify::Write()
{
    _worldPacket << MapId;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Instance::SetRaidDifficulty::Write()
{
    // 5.4.8 18414, from Project SkyFire: the Difficulty.dbc id only
    _worldPacket << uint32(GetClientRaidDifficulty(::Difficulty(Difficulty)));

    return &_worldPacket;
}

void WorldPackets::Instance::SetRaidDifficultyClient::Read()
{
    // 5.4.8 sends the Difficulty.dbc id
    Mode = GetDifficultyFromClient(_worldPacket.read<uint32>());
}

void WorldPackets::Instance::InstanceLockResponse::Read()
{
    Accept = _worldPacket.ReadBit();                        // 5.4.8, from Project SkyFire
}
