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

// 6.x 19342: ported from TrinityCore 6.x (Handlers/InspectHandler.cpp)

#include "AchievementPackets.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "InspectPackets.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

namespace
{
    Player* FindInspectTarget(Player* inspector, ObjectGuid const& guid)
    {
        Player* player = ObjectAccessor::GetPlayer(*inspector, guid);
        if (!player)
        {
            LOG_DEBUG("network", "Inspect: {} target {} not found.", inspector->GetGUID().ToString(), guid.ToString());
            return nullptr;
        }

        if (!inspector->IsWithinDistInMap(player, INSPECT_DISTANCE, false))
            return nullptr;

        if (inspector->IsValidAttackTarget(player))
            return nullptr;

        return player;
    }
}

void WorldSession::HandleInspectOpcode(WorldPackets::Inspect::Inspect& inspect)
{
    Player* player = FindInspectTarget(_player, inspect.Target);
    if (!player)
        return;

    WorldPackets::Inspect::InspectResult inspectResult;
    inspectResult.InspecteeGUID = inspect.Target;

    for (uint8 i = 0; i < EQUIPMENT_SLOT_END; ++i)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            inspectResult.Items.emplace_back(item, i);

    inspectResult.ClassID = player->getClass();
    inspectResult.GenderID = player->getGender();

    if (_player->CanBeGameMaster() || sWorld->getBoolConfig(CONFIG_TALENTS_INSPECTING))
    {
        uint8 const activeSpec = player->GetActiveSpec();
        for (auto const& [spellId, talent] : player->GetTalentMap())
            if (talent->State != PLAYERSPELL_REMOVED && talent->IsInSpec(activeSpec))
                inspectResult.Talents.push_back(uint16(talent->talentID));

        for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
            inspectResult.Glyphs.push_back(uint16(player->GetGlyph(i)));

        inspectResult.SpecializationID = player->GetSpecializationId(activeSpec);
    }

    if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId()))
    {
        inspectResult.GuildData.emplace();
        inspectResult.GuildData->GuildGUID = guild->GetGUID();
        inspectResult.GuildData->NumGuildMembers = guild->GetMemberCount();
        inspectResult.GuildData->AchievementPoints = guild->GetAchievementMgr().GetAchievementPoints();
    }

    SendPacket(inspectResult.Write());
}

void WorldSession::HandleRequestHonorStatsOpcode(WorldPackets::Inspect::RequestHonorStats& request)
{
    Player* player = FindInspectTarget(_player, request.TargetGUID);
    if (!player)
        return;

    WorldPackets::Inspect::InspectHonorStats honorStats;
    honorStats.PlayerGUID = request.TargetGUID;
    honorStats.LifetimeHK = player->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS);
    honorStats.YesterdayHK = player->GetUInt16Value(PLAYER_FIELD_KILLS, 1);
    honorStats.TodayHK = player->GetUInt16Value(PLAYER_FIELD_KILLS, 0);
    honorStats.LifetimeMaxRank = 0;

    SendPacket(honorStats.Write());
}

void WorldSession::HandleInspectPVP(WorldPackets::Inspect::InspectPVPRequest& request)
{
    // cross-realm inspection (InspectRealmAddress) is not supported
    Player* player = FindInspectTarget(_player, request.InspectTarget);
    if (!player)
        return;

    WorldPackets::Inspect::InspectPVPResponse response;
    response.ClientGUID = request.InspectTarget;

    // brackets 0 (2v2) and 1 (3v3) come from the arena teams; 5v5 and rated battlegrounds have no client bracket in 6.x
    for (uint8 slot = 0; slot < 2; ++slot)
    {
        ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(player->GetArenaTeamId(slot));
        if (!team)
            continue;

        ArenaTeamMember const* member = team->GetMember(player->GetGUID());
        if (!member)
            continue;

        WorldPackets::Inspect::PVPBracketData bracket;
        bracket.Bracket = slot;
        bracket.Rating = member->PersonalRating;
        bracket.Rank = team->GetStats().Rank;
        bracket.WeeklyPlayed = member->WeekGames;
        bracket.WeeklyWon = member->WeekWins;
        bracket.SeasonPlayed = member->SeasonGames;
        bracket.SeasonWon = member->SeasonWins;
        bracket.WeeklyBestRating = member->PersonalRating;
        response.Bracket.push_back(bracket);
    }

    SendPacket(response.Write());
}

void WorldSession::HandleQueryInspectAchievements(WorldPackets::Inspect::QueryInspectAchievements& inspect)
{
    Player* player = FindInspectTarget(_player, inspect.Guid);
    if (!player)
        return;

    player->SendRespondInspectAchievements(_player);
}
