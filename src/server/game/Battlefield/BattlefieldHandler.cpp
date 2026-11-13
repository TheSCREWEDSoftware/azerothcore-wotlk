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

#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
    // The 4.3.4 client knows a battlefield by a guid and echoes it back. The battle id is kept in
    // the counter so the answers can be routed without looking at the player's zone.
    ObjectGuid MakeBattlefieldGuid(uint32 battleId)
    {
        return ObjectGuid::Create<HighGuid::BattleGround>(battleId);
    }

    Battlefield* GetBattlefieldFromGuid(ObjectGuid guid, Player* player)
    {
        if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(guid.GetCounter()))
            return bf;

        return sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId());
    }
}

// Sends to player the window to invite them to join the war
// Param1:(battleId) the BattleId of Bf
// Param2:(zoneId) the zone where the battle is (4197 for wg)
// Param3:(time) Time in seconds that the player has to accept
void WorldSession::SendBfInvitePlayerToWar(uint32 battleId, uint32 zoneId, uint32 time)
{
    ObjectGuid guid = MakeBattlefieldGuid(battleId);
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE, 16);

    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[0]);
    data.FlushBits();

    data.WriteByteSeq(guid[6]);
    data << uint32(zoneId);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[0]);
    data << uint32(GameTime::GetGameTime().count() + time); // Invite lasts until
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[5]);
    SendPacket(&data);
}

// Sends invitation to player to join the queue
// Param1:(battleId) the BattleId of Bf
void WorldSession::SendBfInvitePlayerToQueue(uint32 battleId)
{
    ObjectGuid guid = MakeBattlefieldGuid(battleId);
    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE, 5);

    data.WriteBit(1);               // unk
    data.WriteBit(0);               // Has Warmup
    data.WriteBit(1);               // unk
    data.WriteBit(guid[0]);
    data.WriteBit(1);               // unk
    data.WriteBit(guid[2]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[3]);
    data.WriteBit(1);               // unk
    data.WriteBit(0);               // unk
    data.WriteBit(guid[1]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[4]);
    data.WriteBit(1);               // unk
    data.WriteBit(guid[7]);
    data.FlushBits();

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data << uint8(1);               // Warmup
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);
    SendPacket(&data);
}

// Sends packet to inform player that they joined the queue
// Param1:(battleId) the BattleId of Bf
// Param2:(zoneId) the zone where the battle is (4197 for wg)
// Param3:(canQueue) if able to queue
// Param4:(full) on log in is full
void WorldSession::SendBfQueueInviteResponse(uint32 battleId, uint32 zoneId, bool canQueue, bool full)
{
    ObjectGuid guid = MakeBattlefieldGuid(battleId);
    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, 16);

    data.WriteBit(guid[1]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[7]);
    data.WriteBit(full);            // Logging In
    data.WriteBit(guid[0]);
    data.WriteBit(1);               // no second guid
    data.WriteBit(guid[4]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[2]);
    data.FlushBits();

    data << uint8(canQueue);        // Accepted
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[0]);
    data << uint8(1);               // Warmup
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[5]);
    data << uint32(zoneId);
    SendPacket(&data);
}

// Called when player accepts to join war
// Param1:(battleId) the BattleId of Bf
void WorldSession::SendBfEntered(uint32 battleId)
{
    ObjectGuid guid = MakeBattlefieldGuid(battleId);
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTERED, 11);

    data.WriteBit(0);               // unk
    data.WriteBit(_player->isAFK()); // Clear AFK
    data.WriteBit(guid[1]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[3]);
    data.WriteBit(0);               // unk
    data.WriteBit(guid[6]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[2]);
    data.FlushBits();

    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[6]);
    SendPacket(&data);
}

void WorldSession::SendBfLeaveMessage(uint32 battleId, BFLeaveReason reason)
{
    ObjectGuid guid = MakeBattlefieldGuid(battleId);
    WorldPacket data(SMSG_BATTLEFIELD_MGR_EJECTED, 11);

    data.WriteBit(guid[2]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[6]);
    data.WriteBit(0);               // Relocated
    data.WriteBit(guid[7]);
    data.WriteBit(guid[4]);
    data.FlushBits();

    data << uint8(2);               // BattleStatus
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[3]);
    data << uint8(reason);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[5]);
    SendPacket(&data);
}

// Sent by client when they click accept for queue
void WorldSession::HandleBfQueueInviteResponse(WorldPacket& recvData)
{
    ObjectGuid guid;

    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    bool accepted = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[5]);

    Battlefield* bf = GetBattlefieldFromGuid(guid, _player);
    if (!bf)
        return;

    if (accepted)
        bf->PlayerAcceptInviteToQueue(_player);
}

// Sent by client on clicking accept or refuse of invitation window to join game
void WorldSession::HandleBfEntryInviteResponse(WorldPacket& recvData)
{
    ObjectGuid guid;

    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    bool accepted = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[5]);

    Battlefield* bf = GetBattlefieldFromGuid(guid, _player);
    if (!bf)
        return;

    if (accepted)
        bf->PlayerAcceptInviteToWar(_player);
    else
    {
        if (_player->GetZoneId() == bf->GetZoneId())
            bf->KickPlayerFromBattlefield(_player->GetGUID());
    }
}

void WorldSession::HandleBfExitRequest(WorldPacket& recvData)
{
    ObjectGuid guid;

    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[6]);

    Battlefield* bf = GetBattlefieldFromGuid(guid, _player);
    if (!bf)
        return;

    bf->AskToLeaveQueue(_player);
}

// Sent by client when it asks to queue for the battlefield
void WorldSession::HandleBfQueueRequest(WorldPacket& recvData)
{
    ObjectGuid guid;

    guid[0] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[0]);

    if (Battlefield* bf = GetBattlefieldFromGuid(guid, _player))
        bf->PlayerAcceptInviteToQueue(_player);
}
