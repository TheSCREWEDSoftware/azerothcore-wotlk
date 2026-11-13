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
// Ported from TrinityCore's src/server/game/Guilds/GuildFinderMgr.h/.cpp, which is itself the
// real 4.3.4-era implementation of this subsystem (not later-retail work). A guild opts in and
// posts a recruitment message with criteria (LFGuildSettings, stored per guild in
// `guild_finder_guild_settings`); a guildless player browses/searches guilds matching their own
// criteria; a player files a "membership request" (MembershipRequest, stored per applicant in
// `guild_finder_applicant`) that a guild officer sees and can decline or ignore (there is no
// separate "accept" action - accepting an applicant happens through the ordinary
// CMSG_GUILD_INVITE/CMSG_GUILD_ACCEPT flow, exactly as in upstream TrinityCore; see
// Guild::AddMember's sGuildFinderMgr->RemoveAllMembershipRequestsFromPlayer call).

#ifndef AC_GUILDFINDER_H
#define AC_GUILDFINDER_H

#include "Common.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include <unordered_map>

class Guild;
class Player;

enum GuildFinderOptionsInterest
{
    INTEREST_QUESTING        = 0x01,
    INTEREST_DUNGEONS        = 0x02,
    INTEREST_RAIDS           = 0x04,
    INTEREST_PVP             = 0x08,
    INTEREST_ROLE_PLAYING    = 0x10,
    ALL_INTERESTS            = INTEREST_QUESTING | INTEREST_DUNGEONS | INTEREST_RAIDS | INTEREST_PVP | INTEREST_ROLE_PLAYING
};

enum GuildFinderOptionsAvailability
{
    AVAILABILITY_WEEKDAYS     = 0x1,
    AVAILABILITY_WEEKENDS     = 0x2,
    AVAILABILITY_ALWAYS       = AVAILABILITY_WEEKDAYS | AVAILABILITY_WEEKENDS
};

enum GuildFinderOptionsRoles
{
    GUILDFINDER_ROLE_TANK        = 0x1,
    GUILDFINDER_ROLE_HEALER      = 0x2,
    GUILDFINDER_ROLE_DPS         = 0x4,
    GUILDFINDER_ALL_ROLES        = GUILDFINDER_ROLE_TANK | GUILDFINDER_ROLE_HEALER | GUILDFINDER_ROLE_DPS
};

enum GuildFinderOptionsLevel
{
    ANY_FINDER_LEVEL       = 0x1,
    MAX_FINDER_LEVEL       = 0x2,
    ALL_GUILDFINDER_LEVELS = ANY_FINDER_LEVEL | MAX_FINDER_LEVEL
};

/// Holds all required information about a membership request (an applicant's submission to a guild).
class MembershipRequest
{
    public:
        MembershipRequest();
        MembershipRequest(ObjectGuid playerGUID, uint32 guildId, uint32 availability, uint32 classRoles, uint32 interests, std::string comment, time_t submitTime);

        uint32 GetGuildId() const      { return _guildId; }
        ObjectGuid GetPlayerGUID() const   { return _playerGUID; }
        uint8 GetAvailability() const  { return _availability; }
        uint8 GetClassRoles() const    { return _classRoles; }
        uint8 GetInterests() const     { return _interests; }
        uint8 GetClass() const;
        uint8 GetLevel() const;
        time_t GetSubmitTime() const   { return _time; }
        time_t GetExpiryTime() const   { return time_t(_time + 30 * 24 * 3600); } // Adding 30 days
        std::string const& GetComment() const { return _comment; }
        std::string const& GetName() const;

    private:
        std::string _comment;

        uint32 _guildId;
        ObjectGuid _playerGUID;

        uint8 _availability;
        uint8 _classRoles;
        uint8 _interests;

        time_t _time;
};

/// Holds all information about a player's finder settings. _NOT_ stored in the database on its own
/// (only the guild-side specialization, LFGuildSettings, is persisted).
class LFGuildPlayer
{
    public:
        LFGuildPlayer() : _guid(0), _roles(0), _availability(0), _interests(0), _level(0)
        {
        }

        LFGuildPlayer(uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level) :
            _guid(guid), _roles(role), _availability(availability), _interests(interests), _level(level)
        {
        }

        LFGuildPlayer(uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level, std::string const& comment) :
            _comment(comment), _guid(guid), _roles(role), _availability(availability), _interests(interests), _level(level)
        {
        }

        uint32 GetGUID() const         { return _guid; }
        uint8 GetClassRoles() const    { return _roles; }
        uint8 GetAvailability() const  { return _availability; }
        uint8 GetInterests() const     { return _interests; }
        uint8 GetLevel() const         { return _level; }
        std::string const& GetComment() const { return _comment; }

    private:
        std::string _comment;
        uint32 _guid;
        uint8 _roles;
        uint8 _availability;
        uint8 _interests;
        uint8 _level;
};

/// Holds a guild's Guild Finder settings/recruitment post. Saved to database.
class LFGuildSettings : public LFGuildPlayer
{
    public:
        LFGuildSettings() : LFGuildPlayer(), _listed(false), _team(TEAM_ALLIANCE) { }

        LFGuildSettings(bool listed, TeamId team) : LFGuildPlayer(), _listed(listed), _team(team) { }

        LFGuildSettings(bool listed, TeamId team, uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level) :
            LFGuildPlayer(guid, role, availability, interests, level), _listed(listed), _team(team) { }

        LFGuildSettings(bool listed, TeamId team, uint32 guid, uint8 role, uint8 availability, uint8 interests, uint8 level, std::string const& comment) :
            LFGuildPlayer(guid, role, availability, interests, level, comment), _listed(listed), _team(team) { }

        bool IsListed() const      { return _listed; }
        void SetListed(bool state) { _listed = state; }

        TeamId GetTeam() const     { return _team; }

    private:
        bool _listed;
        TeamId _team;
};

typedef std::unordered_map<uint32 /* guildId */, LFGuildSettings> LFGuildStore;
typedef std::unordered_map<uint32 /* guildId */, std::vector<MembershipRequest>> MembershipRequestStore;

class GuildFinderMgr
{
    private:
        GuildFinderMgr();
        ~GuildFinderMgr();
        GuildFinderMgr(GuildFinderMgr const&) = delete;
        GuildFinderMgr& operator=(GuildFinderMgr const&) = delete;

        LFGuildStore _guildSettings;

        MembershipRequestStore _membershipRequests;

        void LoadGuildSettings();
        void LoadMembershipRequests();

    public:
        static GuildFinderMgr* instance();

        void LoadFromDB();

        /// Stores a guild's Guild Finder settings and persists them (guildId is the guild's database id).
        void SetGuildSettings(uint32 guildId, LFGuildSettings const& settings);

        /// Returns Guild Finder settings for a guild (a default-constructed LFGuildSettings if it never posted).
        LFGuildSettings GetGuildSettings(uint32 guildId) { return _guildSettings[guildId]; }

        /// Files a membership request to a guild and persists it.
        void AddMembershipRequest(uint32 guildId, MembershipRequest const& request);

        /// Removes all membership requests filed by a player (e.g. once they join a guild).
        void RemoveAllMembershipRequestsFromPlayer(uint32 playerId);

        /// Removes a single membership request (declined by the guild, or withdrawn by the applicant).
        void RemoveMembershipRequest(uint32 playerId, uint32 guildId);

        /// Wipes everything related to a guild. Used when that guild is disbanded.
        void DeleteGuild(uint32 guildId);

        /// Returns the set of membership requests filed against a guild.
        std::vector<MembershipRequest> GetAllMembershipRequestsForGuild(uint32 guildId) { return _membershipRequests[guildId]; }

        /// Returns the list of membership requests a player has filed, across all guilds.
        std::list<MembershipRequest> GetAllMembershipRequestsForPlayer(uint32 playerGuid);

        /// Returns the guilds whose posted recruitment settings match the given player criteria/faction.
        LFGuildStore GetGuildsMatchingSetting(LFGuildPlayer& settings, TeamId faction);

        /// True if playerId already has a pending request filed with guildId.
        bool HasRequest(uint32 playerId, uint32 guildId);

        /// Counts how many guilds a player currently has a pending request filed with (capped at 10, see AddRecruit handler).
        uint8 CountRequestsFromPlayer(uint32 playerId);

        void SendApplicantListUpdate(Guild& guild);
        void SendMembershipRequestListUpdate(Player& player);
};

#define sGuildFinderMgr GuildFinderMgr::instance()

#endif // AC_GUILDFINDER_H
