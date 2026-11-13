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

// Cata Guild Finder / Guild Recruitment port (see .agents/docs/porting-from-trinitycore.md).
// Ported from TrinityCore's src/server/game/Handlers/GuildFinderHandler.cpp. Kept as its own file
// (mirroring upstream's own layout) rather than folded into GuildHandler.cpp, since this is a
// large, functionally self-contained subsystem in its own right.

#include "WorldSession.h"
#include "GameTime.h"
#include "Guild.h"
#include "GuildFinderMgr.h"
#include "GuildFinderPackets.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Player.h"
#include "World.h"

void WorldSession::HandleGuildFinderAddRecruitOpcode(WorldPackets::GuildFinder::AddRecruit& packet)
{
    if (sGuildFinderMgr->GetAllMembershipRequestsForPlayer(GetPlayer()->GetGUID().GetCounter()).size() >= 10)
        return;

    if (!packet.GuildGuid.IsGuild())
        return;
    if (!(packet.ClassRoles & GUILDFINDER_ALL_ROLES) || packet.ClassRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(packet.Availability & AVAILABILITY_ALWAYS) || packet.Availability > AVAILABILITY_ALWAYS)
        return;
    if (!(packet.Interests & ALL_INTERESTS) || packet.Interests > ALL_INTERESTS)
        return;

    uint32 guildId = packet.GuildGuid.GetCounter();

    MembershipRequest request(GetPlayer()->GetGUID(), guildId, packet.Availability, packet.ClassRoles, packet.Interests, packet.Comment, GameTime::GetGameTime().count());
    sGuildFinderMgr->AddMembershipRequest(guildId, request);
}

void WorldSession::HandleGuildFinderBrowseOpcode(WorldPackets::GuildFinder::Browse& packet)
{
    if (!(packet.ClassRoles & GUILDFINDER_ALL_ROLES) || packet.ClassRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(packet.Availability & AVAILABILITY_ALWAYS) || packet.Availability > AVAILABILITY_ALWAYS)
        return;
    if (!(packet.Interests & ALL_INTERESTS) || packet.Interests > ALL_INTERESTS)
        return;
    if (packet.PlayerLevel > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) || packet.PlayerLevel < 1)
        return;

    Player* player = GetPlayer();

    LFGuildPlayer settings(player->GetGUID().GetCounter(), packet.ClassRoles, packet.Availability, packet.Interests, ANY_FINDER_LEVEL);
    LFGuildStore guildList = sGuildFinderMgr->GetGuildsMatchingSetting(settings, player->GetTeamId());

    WorldPackets::GuildFinder::BrowseUpdated response;
    for (auto const& [guildId, guildSettings] : guildList)
    {
        Guild* guild = sGuildMgr->GetGuildById(guildId);
        if (!guild)
            continue;

        WorldPackets::GuildFinder::BrowseEntry entry;
        entry.GuildGuid = ObjectGuid(HighGuid::Guild, guild->GetId());
        entry.GuildName = guild->GetName();
        entry.Comment = guildSettings.GetComment();
        entry.EmblemStyle = guild->GetEmblemInfo().GetStyle();
        entry.EmblemColor = guild->GetEmblemInfo().GetColor();
        entry.BorderStyle = guild->GetEmblemInfo().GetBorderStyle();
        entry.BorderColor = guild->GetEmblemInfo().GetBorderColor();
        entry.BackgroundColor = guild->GetEmblemInfo().GetBackgroundColor();
        entry.GuildLevel = guild->GetLevel();
        entry.MemberCount = guild->GetMemberCount();
        entry.Interests = guildSettings.GetInterests();
        entry.Availability = guildSettings.GetAvailability();
        entry.ClassRoles = guildSettings.GetClassRoles();
        entry.RequestPending = sGuildFinderMgr->HasRequest(player->GetGUID().GetCounter(), guildId);
        response.Guilds.push_back(entry);
    }

    player->SendDirectMessage(response.Write());
}

void WorldSession::HandleGuildFinderDeclineRecruitOpcode(WorldPackets::GuildFinder::DeclineRecruit& packet)
{
    if (!packet.PlayerGuid.IsPlayer())
        return;

    sGuildFinderMgr->RemoveMembershipRequest(packet.PlayerGuid.GetCounter(), GetPlayer()->GetGuildId());
}

void WorldSession::HandleGuildFinderGetApplicationsOpcode(WorldPackets::GuildFinder::GetApplications& /*packet*/)
{
    std::list<MembershipRequest> appliedGuilds = sGuildFinderMgr->GetAllMembershipRequestsForPlayer(GetPlayer()->GetGUID().GetCounter());

    WorldPackets::GuildFinder::MembershipListUpdated response;
    for (MembershipRequest const& request : appliedGuilds)
    {
        Guild* guild = sGuildMgr->GetGuildById(request.GetGuildId());
        if (!guild)
            continue;

        LFGuildSettings guildSettings = sGuildFinderMgr->GetGuildSettings(request.GetGuildId());

        WorldPackets::GuildFinder::MembershipApplication app;
        app.GuildGuid = ObjectGuid(HighGuid::Guild, guild->GetId());
        app.GuildName = guild->GetName();
        app.Comment = request.GetComment();
        app.Availability = guildSettings.GetAvailability();
        app.ClassRoles = guildSettings.GetClassRoles();
        app.Interests = guildSettings.GetInterests();
        app.SecondsSinceSubmit = uint32(GameTime::GetGameTime().count() - request.GetSubmitTime());
        app.SecondsUntilExpiry = uint32(request.GetExpiryTime() - GameTime::GetGameTime().count());
        response.Applications.push_back(app);
    }

    response.ApplicationsLeft = 10 - sGuildFinderMgr->CountRequestsFromPlayer(GetPlayer()->GetGUID().GetCounter());

    GetPlayer()->SendDirectMessage(response.Write());
}

// Lists all recruits (pending applicants) for the requester's own guild.
void WorldSession::HandleGuildFinderGetRecruitsOpcode(WorldPackets::GuildFinder::GetRecruits& /*packet*/)
{
    Player* player = GetPlayer();
    if (!player->GetGuildId())
        return;

    std::vector<MembershipRequest> recruitsList = sGuildFinderMgr->GetAllMembershipRequestsForGuild(player->GetGuildId());

    WorldPackets::GuildFinder::RecruitListUpdated response;
    for (MembershipRequest const& request : recruitsList)
    {
        WorldPackets::GuildFinder::RecruitEntry entry;
        entry.PlayerGuid = request.GetPlayerGUID();
        entry.Name = request.GetName();
        entry.Comment = request.GetComment();
        entry.Available = GameTime::GetGameTime().count() <= request.GetExpiryTime();
        entry.Level = request.GetLevel();
        entry.Class = request.GetClass();
        entry.SecondsSinceSubmit = uint32(GameTime::GetGameTime().count() - request.GetSubmitTime());
        entry.SecondsUntilExpiry = uint32(request.GetExpiryTime() - GameTime::GetGameTime().count());
        entry.Availability = request.GetAvailability();
        entry.ClassRoles = request.GetClassRoles();
        entry.Interests = request.GetInterests();
        response.Recruits.push_back(entry);
    }

    player->SendDirectMessage(response.Write());
}

void WorldSession::HandleGuildFinderPostRequestOpcode(WorldPackets::GuildFinder::PostRequest& /*packet*/)
{
    Player* player = GetPlayer();

    if (!player->GetGuildId()) // Player must be in a guild
        return;

    bool isGuildMaster = true;
    if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId()))
        if (guild->GetLeaderGUID() != player->GetGUID())
            isGuildMaster = false;

    LFGuildSettings settings = sGuildFinderMgr->GetGuildSettings(player->GetGuildId());

    WorldPackets::GuildFinder::PostUpdated response;
    response.IsGuildMaster = isGuildMaster;
    if (isGuildMaster)
    {
        response.Listed = settings.IsListed();
        response.Level = settings.GetLevel();
        response.Comment = settings.GetComment();
        response.Availability = settings.GetAvailability();
        response.ClassRoles = settings.GetClassRoles();
        response.Interests = settings.GetInterests();
    }

    player->SendDirectMessage(response.Write());
}

void WorldSession::HandleGuildFinderRemoveRecruitOpcode(WorldPackets::GuildFinder::RemoveRecruit& packet)
{
    if (!packet.GuildGuid.IsGuild())
        return;

    sGuildFinderMgr->RemoveMembershipRequest(GetPlayer()->GetGUID().GetCounter(), packet.GuildGuid.GetCounter());
}

// Sent any time a guild master sets an option in the interface, and when (un)listing the guild.
void WorldSession::HandleGuildFinderSetGuildPostOpcode(WorldPackets::GuildFinder::SetGuildPost& packet)
{
    uint32 level = packet.Level;
    // Level sent is zero if untouched by the interface; force to "any", matching upstream TrinityCore.
    if (!level)
        level = ANY_FINDER_LEVEL;

    if (!(packet.ClassRoles & GUILDFINDER_ALL_ROLES) || packet.ClassRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(packet.Availability & AVAILABILITY_ALWAYS) || packet.Availability > AVAILABILITY_ALWAYS)
        return;
    if (!(packet.Interests & ALL_INTERESTS) || packet.Interests > ALL_INTERESTS)
        return;
    if (!(level & ALL_GUILDFINDER_LEVELS) || level > ALL_GUILDFINDER_LEVELS)
        return;

    Player* player = GetPlayer();

    if (!player->GetGuildId()) // Player must be in a guild
        return;

    if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId())) // Player must be the guild master
        if (guild->GetLeaderGUID() != player->GetGUID())
            return;

    LFGuildSettings settings(packet.Listed, player->GetTeamId(), player->GetGuildId(), packet.ClassRoles, packet.Availability, packet.Interests, level, packet.Comment);
    sGuildFinderMgr->SetGuildSettings(player->GetGuildId(), settings);
}
