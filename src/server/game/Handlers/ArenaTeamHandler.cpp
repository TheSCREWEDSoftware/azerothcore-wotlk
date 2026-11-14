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

#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// 6.x: the client has no arena team management anymore (TC 6.x registers none of these opcodes). Arena
// teams stay the server side model for rated arena queues; only the messages below remain.

void WorldSession::SendArenaTeamCommandResult(uint32 teamAction, std::string const& team, std::string const& player, uint32 errorId)
{
    // AC opcode without a 6.x value (parked), the client does not receive it
    WorldPacket data(SMSG_ARENA_TEAM_COMMAND_RESULT, 2 + team.length() + player.length() + 4 + 4);
    data.WriteBits(player.length(), 7);
    data.WriteBits(team.length(), 8);
    data.FlushBits();
    data.WriteString(player);
    data << uint32(teamAction);
    data << uint32(errorId);
    data.WriteString(team);
    SendPacket(&data);
}

void WorldSession::SendNotInArenaTeamPacket(uint8 type)
{
    WorldPacket data(SMSG_ARENA_ERROR, 4 + 1);              // 886 - You are not in a %uv%u arena team
    uint32 unk = 0;
    data << uint32(unk);                                    // unk(0)
    if (!unk)
        data << uint8(type);                                // team type (2=2v2, 3=3v3, 5=5v5), can be used for custom types...
    SendPacket(&data);
}
