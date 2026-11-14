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
#include "CharacterCache.h"
#include "GameTime.h"
#include "Guild.h"
#include "GuildFinderMgr.h"
#include "GuildFinderPackets.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Player.h"
#include "Realm.h"
#include "World.h"

// 6.x 19342: 6.x packets from TrinityCore 6.x (Handlers/GuildFinderHandler.cpp), AC's guild finder manager kept

void WorldSession::HandleGuildFinderAddRecruit(WorldPackets::GuildFinder::LFGuildAddRecruit& lfGuildAddRecruit)
{
    if (sGuildFinderMgr->GetAllMembershipRequestsForPlayer(GetPlayer()->GetGUID().GetCounter()).size() >= 10)
        return;

    if (!lfGuildAddRecruit.GuildGUID.IsGuild())
        return;
    if (!(lfGuildAddRecruit.ClassRoles & GUILDFINDER_ALL_ROLES) || lfGuildAddRecruit.ClassRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(lfGuildAddRecruit.Availability & AVAILABILITY_ALWAYS) || lfGuildAddRecruit.Availability > AVAILABILITY_ALWAYS)
        return;
    if (!(lfGuildAddRecruit.PlayStyle & ALL_INTERESTS) || lfGuildAddRecruit.PlayStyle > ALL_INTERESTS)
        return;

    uint32 guildId = lfGuildAddRecruit.GuildGUID.GetCounter();

    MembershipRequest request(GetPlayer()->GetGUID(), guildId, lfGuildAddRecruit.Availability, lfGuildAddRecruit.ClassRoles, lfGuildAddRecruit.PlayStyle,
        lfGuildAddRecruit.Comment, GameTime::GetGameTime().count());
    sGuildFinderMgr->AddMembershipRequest(guildId, request);
}

void WorldSession::HandleGuildFinderBrowse(WorldPackets::GuildFinder::LFGuildBrowse& lfGuildBrowse)
{
    if (!(lfGuildBrowse.ClassRoles & GUILDFINDER_ALL_ROLES) || lfGuildBrowse.ClassRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(lfGuildBrowse.Availability & AVAILABILITY_ALWAYS) || lfGuildBrowse.Availability > AVAILABILITY_ALWAYS)
        return;
    if (!(lfGuildBrowse.PlayStyle & ALL_INTERESTS) || lfGuildBrowse.PlayStyle > ALL_INTERESTS)
        return;
    if (lfGuildBrowse.CharacterLevel > int32(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)) || lfGuildBrowse.CharacterLevel < 1)
        return;

    Player* player = GetPlayer();

    LFGuildPlayer settings(player->GetGUID().GetCounter(), lfGuildBrowse.ClassRoles, lfGuildBrowse.Availability, lfGuildBrowse.PlayStyle, ANY_FINDER_LEVEL);
    LFGuildStore guildList = sGuildFinderMgr->GetGuildsMatchingSetting(settings, player->GetTeamId());

    WorldPackets::GuildFinder::LFGuildBrowseResult lfGuildBrowseResult;
    for (auto const& [guildId, guildSettings] : guildList)
    {
        Guild* guild = sGuildMgr->GetGuildById(guildId);
        if (!guild)
            continue;

        WorldPackets::GuildFinder::LFGuildBrowseData guildData;
        guildData.GuildName = guild->GetName();
        guildData.GuildGUID = guild->GetGUID();
        guildData.GuildVirtualRealm = realm.Id.Realm;
        guildData.GuildMembers = guild->GetMemberCount();
        guildData.GuildAchievementPoints = guild->GetAchievementMgr().GetAchievementPoints();
        guildData.PlayStyle = guildSettings.GetInterests();
        guildData.Availability = guildSettings.GetAvailability();
        guildData.ClassRoles = guildSettings.GetClassRoles();
        guildData.LevelRange = guildSettings.GetLevel();
        guildData.EmblemStyle = guild->GetEmblemInfo().GetStyle();
        guildData.EmblemColor = guild->GetEmblemInfo().GetColor();
        guildData.BorderStyle = guild->GetEmblemInfo().GetBorderStyle();
        guildData.BorderColor = guild->GetEmblemInfo().GetBorderColor();
        guildData.Background = guild->GetEmblemInfo().GetBackgroundColor();
        guildData.Comment = guildSettings.GetComment();
        guildData.Cached = 0;
        guildData.MembershipRequested = sGuildFinderMgr->HasRequest(player->GetGUID().GetCounter(), guildId);
        lfGuildBrowseResult.Post.push_back(guildData);
    }

    player->SendDirectMessage(lfGuildBrowseResult.Write());
}

void WorldSession::HandleGuildFinderDeclineRecruit(WorldPackets::GuildFinder::LFGuildDeclineRecruit& lfGuildDeclineRecruit)
{
    if (!GetPlayer()->GetGuildId())
        return;

    if (!lfGuildDeclineRecruit.RecruitGUID.IsPlayer())
        return;

    sGuildFinderMgr->RemoveMembershipRequest(lfGuildDeclineRecruit.RecruitGUID.GetCounter(), GetPlayer()->GetGuildId());
}

void WorldSession::HandleGuildFinderGetApplications(WorldPackets::GuildFinder::LFGuildGetApplications& /*lfGuildGetApplications*/)
{
    std::list<MembershipRequest> appliedGuilds = sGuildFinderMgr->GetAllMembershipRequestsForPlayer(GetPlayer()->GetGUID().GetCounter());
    time_t now = GameTime::GetGameTime().count();

    WorldPackets::GuildFinder::LFGuildApplications lfGuildApplications;
    lfGuildApplications.NumRemaining = 10 - sGuildFinderMgr->CountRequestsFromPlayer(GetPlayer()->GetGUID().GetCounter());

    for (MembershipRequest const& request : appliedGuilds)
    {
        Guild* guild = sGuildMgr->GetGuildById(request.GetGuildId());
        if (!guild)
            continue;

        LFGuildSettings guildSettings = sGuildFinderMgr->GetGuildSettings(request.GetGuildId());

        WorldPackets::GuildFinder::LFGuildApplicationData applicationData;
        applicationData.GuildGUID = guild->GetGUID();
        applicationData.GuildVirtualRealm = realm.Id.Realm;
        applicationData.GuildName = guild->GetName();
        applicationData.ClassRoles = guildSettings.GetClassRoles();
        applicationData.PlayStyle = guildSettings.GetInterests();
        applicationData.Availability = guildSettings.GetAvailability();
        applicationData.SecondsSinceCreated = uint32(now - request.GetSubmitTime());
        applicationData.SecondsUntilExpiration = uint32(request.GetExpiryTime() - now);
        applicationData.Comment = request.GetComment();
        lfGuildApplications.Application.push_back(applicationData);
    }

    GetPlayer()->SendDirectMessage(lfGuildApplications.Write());
}

void WorldSession::HandleGuildFinderGetGuildPost(WorldPackets::GuildFinder::LFGuildGetGuildPost& /*lfGuildGetGuildPost*/)
{
    Player* player = GetPlayer();

    Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId());
    if (!guild) // Player must be in guild
        return;

    WorldPackets::GuildFinder::LFGuildPost lfGuildPost;
    if (guild->GetLeaderGUID() == player->GetGUID())
    {
        LFGuildSettings settings = sGuildFinderMgr->GetGuildSettings(guild->GetId());
        lfGuildPost.Post.emplace();
        lfGuildPost.Post->Active = settings.IsListed();
        lfGuildPost.Post->PlayStyle = settings.GetInterests();
        lfGuildPost.Post->Availability = settings.GetAvailability();
        lfGuildPost.Post->ClassRoles = settings.GetClassRoles();
        lfGuildPost.Post->LevelRange = settings.GetLevel();
        lfGuildPost.Post->Comment = settings.GetComment();
    }

    player->SendDirectMessage(lfGuildPost.Write());
}

// Lists all recruits (pending applicants) for the requester's own guild.
void WorldSession::HandleGuildFinderGetRecruits(WorldPackets::GuildFinder::LFGuildGetRecruits& /*lfGuildGetRecruits*/)
{
    Player* player = GetPlayer();
    if (!player->GetGuildId())
        return;

    time_t now = GameTime::GetGameTime().count();

    WorldPackets::GuildFinder::LFGuildRecruits lfGuildRecruits;
    lfGuildRecruits.UpdateTime = now;
    for (MembershipRequest const& request : sGuildFinderMgr->GetAllMembershipRequestsForGuild(player->GetGuildId()))
    {
        WorldPackets::GuildFinder::LFGuildRecruitData recruitData;
        recruitData.RecruitGUID = request.GetPlayerGUID();
        recruitData.Name = request.GetName();
        recruitData.RecruitVirtualRealm = realm.Id.Realm;
        recruitData.Comment = request.GetComment();
        recruitData.CharacterClass = request.GetClass();
        recruitData.CharacterLevel = request.GetLevel();
        recruitData.ClassRoles = request.GetClassRoles();
        recruitData.PlayStyle = request.GetInterests();
        recruitData.Availability = request.GetAvailability();
        recruitData.SecondsSinceCreated = uint32(now - request.GetSubmitTime());
        recruitData.SecondsUntilExpiration = uint32(request.GetExpiryTime() - now);
        if (CharacterCacheEntry const* charInfo = sCharacterCache->GetCharacterCacheByGuid(request.GetPlayerGUID()))
            recruitData.CharacterGender = charInfo->Sex;
        lfGuildRecruits.Recruits.push_back(recruitData);
    }

    player->SendDirectMessage(lfGuildRecruits.Write());
}

void WorldSession::HandleGuildFinderRemoveRecruit(WorldPackets::GuildFinder::LFGuildRemoveRecruit& lfGuildRemoveRecruit)
{
    if (!lfGuildRemoveRecruit.GuildGUID.IsGuild())
        return;

    sGuildFinderMgr->RemoveMembershipRequest(GetPlayer()->GetGUID().GetCounter(), lfGuildRemoveRecruit.GuildGUID.GetCounter());
}

// Sent any time a guild master sets an option in the interface, and when (un)listing the guild.
void WorldSession::HandleGuildFinderSetGuildPost(WorldPackets::GuildFinder::LFGuildSetGuildPost& lfGuildSetGuildPost)
{
    uint32 level = lfGuildSetGuildPost.LevelRange;
    // Level sent is zero if untouched by the interface; force to "any", matching upstream TrinityCore.
    if (!level)
        level = ANY_FINDER_LEVEL;

    if (!(lfGuildSetGuildPost.ClassRoles & GUILDFINDER_ALL_ROLES) || lfGuildSetGuildPost.ClassRoles > GUILDFINDER_ALL_ROLES)
        return;
    if (!(lfGuildSetGuildPost.Availability & AVAILABILITY_ALWAYS) || lfGuildSetGuildPost.Availability > AVAILABILITY_ALWAYS)
        return;
    if (!(lfGuildSetGuildPost.PlayStyle & ALL_INTERESTS) || lfGuildSetGuildPost.PlayStyle > ALL_INTERESTS)
        return;
    if (!(level & ALL_GUILDFINDER_LEVELS) || level > ALL_GUILDFINDER_LEVELS)
        return;

    Player* player = GetPlayer();

    if (!player->GetGuildId()) // Player must be in a guild
        return;

    if (Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId())) // Player must be the guild master
        if (guild->GetLeaderGUID() != player->GetGUID())
            return;

    LFGuildSettings settings(lfGuildSetGuildPost.Active, player->GetTeamId(), player->GetGuildId(), lfGuildSetGuildPost.ClassRoles,
        lfGuildSetGuildPost.Availability, lfGuildSetGuildPost.PlayStyle, level, lfGuildSetGuildPost.Comment);
    sGuildFinderMgr->SetGuildSettings(player->GetGuildId(), settings);
}
