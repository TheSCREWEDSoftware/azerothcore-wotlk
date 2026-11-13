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
#include "Opcodes.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"

enum DuelSpells
{
    SPELL_DUEL          = 7266,
    SPELL_MOUNTED_DUEL  = 62875
};

// 5.4.8 18414, from Project SkyFire: the client asks for a duel instead of casting the spell itself
void WorldSession::HandleDuelProposedOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    recvPacket.ReadGuidMask(guid, { 1, 5, 4, 6, 3, 2, 7, 0 });
    recvPacket.ReadGuidBytes(guid, { 4, 2, 5, 7, 1, 3, 6, 0 });

    if (Player* target = ObjectAccessor::GetPlayer(*GetPlayer(), guid))
        GetPlayer()->CastSpell(target, GetPlayer()->IsMounted() ? SPELL_MOUNTED_DUEL : SPELL_DUEL, false);
}

// 5.4.8 18414, from Project SkyFire: accepting and declining share one opcode
void WorldSession::HandleDuelResponseOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    recvPacket.ReadGuidMask(guid, { 7, 1, 3, 4, 0, 2, 6 });
    bool accepted = recvPacket.ReadBit();
    recvPacket.ReadGuidMask(guid, { 5 });
    recvPacket.ReadGuidBytes(guid, { 6, 4, 5, 0, 1, 2, 7, 3 });

    if (accepted)
        HandleDuelAccepted(guid);
    else
        HandleDuelCancelled();
}

void WorldSession::HandleDuelAccepted(ObjectGuid guid)
{
    Player* player = GetPlayer();
    if (!player->duel || player == player->duel->Initiator || player->duel->State != DUEL_STATE_CHALLENGED)
        return;

    Player* target = player->duel->Opponent;
    if (target->GetGuidValue(PLAYER_DUEL_ARBITER) != guid)
        return;

    LOG_DEBUG("network.opcode", "Player 1 is: {} ({})", player->GetGUID().ToString(), player->GetName());
    LOG_DEBUG("network.opcode", "Player 2 is: {} ({})", target->GetGUID().ToString(), target->GetName());

    time_t now = GameTime::GetGameTime().count();
    player->duel->StartTime = now + 3;
    target->duel->StartTime = now + 3;

    player->duel->State = DUEL_STATE_COUNTDOWN;
    target->duel->State = DUEL_STATE_COUNTDOWN;

    player->SendDuelCountdown(3000);
    target->SendDuelCountdown(3000);
}

void WorldSession::HandleDuelCancelled()
{
    Player* player = GetPlayer();

    // no duel requested
    if (!player->duel || player->duel->State == DUEL_STATE_COMPLETED)
        return;

    // player surrendered in a duel using /forfeit
    if (GetPlayer()->duel->State == DUEL_STATE_IN_PROGRESS)
    {
        GetPlayer()->CombatStopWithPets(true);
        GetPlayer()->duel->Opponent->CombatStopWithPets(true);

        GetPlayer()->CastSpell(GetPlayer(), 7267, true);    // beg
        GetPlayer()->DuelComplete(DUEL_WON);
        return;
    }

    GetPlayer()->DuelComplete(DUEL_INTERRUPTED);
}
