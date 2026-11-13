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

#include "GuildFinderMgr.h"
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Guild.h"
#include "GuildFinderPackets.h"
#include "GuildMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include <algorithm>

MembershipRequest::MembershipRequest() : _guildId(0), _availability(0), _classRoles(0), _interests(0), _time(GameTime::GetGameTime().count())
{
}

MembershipRequest::MembershipRequest(ObjectGuid playerGUID, uint32 guildId, uint32 availability, uint32 classRoles, uint32 interests, std::string comment, time_t submitTime) :
    _comment(std::move(comment)), _guildId(guildId), _playerGUID(playerGUID), _availability(uint8(availability)), _classRoles(uint8(classRoles)), _interests(uint8(interests)), _time(submitTime)
{
}

uint8 MembershipRequest::GetClass() const
{
    CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(GetPlayerGUID());
    return cache ? cache->Class : 0;
}

uint8 MembershipRequest::GetLevel() const
{
    CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(GetPlayerGUID());
    return cache ? cache->Level : 0;
}

std::string const& MembershipRequest::GetName() const
{
    static std::string const empty;
    CharacterCacheEntry const* cache = sCharacterCache->GetCharacterCacheByGuid(GetPlayerGUID());
    return cache ? cache->Name : empty;
}

GuildFinderMgr::GuildFinderMgr()
{
}

GuildFinderMgr::~GuildFinderMgr()
{
}

GuildFinderMgr* GuildFinderMgr::instance()
{
    static GuildFinderMgr instance;
    return &instance;
}

void GuildFinderMgr::LoadFromDB()
{
    LoadGuildSettings();
    LoadMembershipRequests();
}

void GuildFinderMgr::LoadGuildSettings()
{
    uint32 oldMSTime = getMSTime();

    _guildSettings.clear();

    // Determine each guild's faction from an arbitrary member's race, same as TrinityCore's own
    // query (a guild can never actually have mixed-faction members, so any single member's race
    // is enough - this just avoids requiring a separate "faction" column on the guild itself).
    //                                                           0                1             2                  3             4           5             6         7
    QueryResult result = CharacterDatabase.Query("SELECT gfgs.guildId, gfgs.availability, gfgs.classRoles, gfgs.interests, gfgs.level, gfgs.listed, gfgs.comment, c.race "
                                                  "FROM guild_finder_guild_settings gfgs "
                                                  "LEFT JOIN guild_member gm ON gm.guildid = gfgs.guildId "
                                                  "LEFT JOIN characters c ON c.guid = gm.guid");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 guild finder guild-related settings. Table `guild_finder_guild_settings` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 guildId      = fields[0].Get<uint32>();
        uint8  availability = fields[1].Get<uint8>();
        uint8  classRoles   = fields[2].Get<uint8>();
        uint8  interests    = fields[3].Get<uint8>();
        uint8  level        = fields[4].Get<uint8>();
        bool   listed       = fields[5].Get<uint8>() != 0;
        std::string comment = fields[6].Get<std::string>();

        TeamId guildTeam = Player::TeamIdForRace(fields[7].Get<uint8>());
        if (guildTeam == TEAM_NEUTRAL)
            guildTeam = TEAM_ALLIANCE;

        _guildSettings[guildId] = LFGuildSettings(listed, guildTeam, guildId, classRoles, availability, interests, level, comment);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} guild finder guild-related settings in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void GuildFinderMgr::LoadMembershipRequests()
{
    uint32 oldMSTime = getMSTime();

    _membershipRequests.clear();

    //                                                      0         1           2            3           4         5         6
    QueryResult result = CharacterDatabase.Query("SELECT guildId, playerGuid, availability, classRole, interests, comment, submitTime "
                                                  "FROM guild_finder_applicant");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 guild finder membership requests. Table `guild_finder_applicant` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 guildId      = fields[0].Get<uint32>();
        uint32 playerId     = fields[1].Get<uint32>();
        uint8  availability = fields[2].Get<uint8>();
        uint8  classRoles   = fields[3].Get<uint8>();
        uint8  interests    = fields[4].Get<uint8>();
        std::string comment = fields[5].Get<std::string>();
        uint32 submitTime   = fields[6].Get<uint32>();

        MembershipRequest request(ObjectGuid::Create<HighGuid::Player>(playerId), guildId, availability, classRoles, interests, comment, time_t(submitTime));

        _membershipRequests[guildId].push_back(request);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} guild finder membership requests in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void GuildFinderMgr::AddMembershipRequest(uint32 guildId, MembershipRequest const& request)
{
    _membershipRequests[guildId].push_back(request);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_FINDER_APPLICANT);
    stmt->SetData(0, request.GetGuildId());
    stmt->SetData(1, request.GetPlayerGUID().GetCounter());
    stmt->SetData(2, request.GetAvailability());
    stmt->SetData(3, request.GetClassRoles());
    stmt->SetData(4, request.GetInterests());
    stmt->SetData(5, request.GetComment());
    stmt->SetData(6, uint32(request.GetSubmitTime()));
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);

    // Notify the applicant their submission has been added
    if (Player* player = ObjectAccessor::FindPlayer(request.GetPlayerGUID()))
        SendMembershipRequestListUpdate(*player);

    // Notify the guild master and officers the list changed
    if (Guild* guild = sGuildMgr->GetGuildById(guildId))
        SendApplicantListUpdate(*guild);
}

void GuildFinderMgr::RemoveAllMembershipRequestsFromPlayer(uint32 playerId)
{
    for (auto& [guildId, requests] : _membershipRequests)
    {
        auto itr = std::find_if(requests.begin(), requests.end(), [playerId](MembershipRequest const& r)
        {
            return r.GetPlayerGUID().GetCounter() == playerId;
        });

        if (itr == requests.end())
            continue;

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_FINDER_APPLICANT);
        stmt->SetData(0, itr->GetGuildId());
        stmt->SetData(1, itr->GetPlayerGUID().GetCounter());
        trans->Append(stmt);
        CharacterDatabase.CommitTransaction(trans);

        requests.erase(itr);

        // Notify the guild master and officers the list changed
        if (Guild* guild = sGuildMgr->GetGuildById(guildId))
            SendApplicantListUpdate(*guild);
    }
}

void GuildFinderMgr::RemoveMembershipRequest(uint32 playerId, uint32 guildId)
{
    std::vector<MembershipRequest>& requests = _membershipRequests[guildId];
    auto itr = std::find_if(requests.begin(), requests.end(), [playerId](MembershipRequest const& r)
    {
        return r.GetPlayerGUID().GetCounter() == playerId;
    });

    if (itr == requests.end())
        return;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_FINDER_APPLICANT);
    stmt->SetData(0, itr->GetGuildId());
    stmt->SetData(1, itr->GetPlayerGUID().GetCounter());
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);

    // Notify the applicant their submission has been removed
    if (Player* player = ObjectAccessor::FindPlayer(itr->GetPlayerGUID()))
        SendMembershipRequestListUpdate(*player);

    requests.erase(itr);

    // Notify the guild master and officers the list changed
    if (Guild* guild = sGuildMgr->GetGuildById(guildId))
        SendApplicantListUpdate(*guild);
}

std::list<MembershipRequest> GuildFinderMgr::GetAllMembershipRequestsForPlayer(uint32 playerGuid)
{
    std::list<MembershipRequest> resultSet;
    for (auto const& [guildId, requests] : _membershipRequests)
        for (MembershipRequest const& request : requests)
            if (request.GetPlayerGUID().GetCounter() == playerGuid)
            {
                resultSet.push_back(request);
                break;
            }

    return resultSet;
}

uint8 GuildFinderMgr::CountRequestsFromPlayer(uint32 playerId)
{
    uint8 result = 0;
    for (auto const& [guildId, requests] : _membershipRequests)
    {
        for (MembershipRequest const& request : requests)
        {
            if (request.GetPlayerGUID().GetCounter() != playerId)
                continue;
            ++result;
            break;
        }
    }
    return result;
}

LFGuildStore GuildFinderMgr::GetGuildsMatchingSetting(LFGuildPlayer& settings, TeamId faction)
{
    LFGuildStore resultSet;
    for (auto const& [guildId, guildSettings] : _guildSettings)
    {
        if (!guildSettings.IsListed())
            continue;

        if (guildSettings.GetTeam() != faction)
            continue;

        if (!(guildSettings.GetAvailability() & settings.GetAvailability()))
            continue;

        if (!(guildSettings.GetClassRoles() & settings.GetClassRoles()))
            continue;

        if (!(guildSettings.GetInterests() & settings.GetInterests()))
            continue;

        if (!(guildSettings.GetLevel() & settings.GetLevel()))
            continue;

        resultSet.emplace(guildId, guildSettings);
    }

    return resultSet;
}

bool GuildFinderMgr::HasRequest(uint32 playerId, uint32 guildId)
{
    std::vector<MembershipRequest> const& requests = _membershipRequests[guildId];
    return std::any_of(requests.begin(), requests.end(), [playerId](MembershipRequest const& r)
    {
        return r.GetPlayerGUID().GetCounter() == playerId;
    });
}

void GuildFinderMgr::SetGuildSettings(uint32 guildId, LFGuildSettings const& settings)
{
    _guildSettings[guildId] = settings;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_FINDER_GUILD_SETTINGS);
    stmt->SetData(0, guildId);
    stmt->SetData(1, settings.GetAvailability());
    stmt->SetData(2, settings.GetClassRoles());
    stmt->SetData(3, settings.GetInterests());
    stmt->SetData(4, settings.GetLevel());
    stmt->SetData(5, settings.IsListed());
    stmt->SetData(6, settings.GetComment());
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);
}

void GuildFinderMgr::DeleteGuild(uint32 guildId)
{
    std::vector<MembershipRequest>& requests = _membershipRequests[guildId];
    for (MembershipRequest const& request : requests)
    {
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_FINDER_APPLICANT);
        stmt->SetData(0, request.GetGuildId());
        stmt->SetData(1, request.GetPlayerGUID().GetCounter());
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_FINDER_GUILD_SETTINGS);
        stmt->SetData(0, guildId);
        trans->Append(stmt);

        CharacterDatabase.CommitTransaction(trans);

        // Notify the applicant their submission has been removed
        if (Player* player = ObjectAccessor::FindPlayer(request.GetPlayerGUID()))
            SendMembershipRequestListUpdate(*player);
    }

    _membershipRequests.erase(guildId);
    _guildSettings.erase(guildId);
}

void GuildFinderMgr::SendApplicantListUpdate(Guild& guild)
{
    WorldPackets::GuildFinder::ApplicantListUpdated packet;
    WorldPacket const* raw = packet.Write();
    if (Player* player = ObjectAccessor::FindPlayer(guild.GetLeaderGUID()))
        player->SendDirectMessage(raw);
    guild.BroadcastPacketToRank(raw, GR_OFFICER);
}

void GuildFinderMgr::SendMembershipRequestListUpdate(Player& player)
{
    WorldPackets::GuildFinder::ApplicationsListChanged packet;
    player.SendDirectMessage(packet.Write());
}
