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

#ifndef _LFG_H
#define _LFG_H

#include "ObjectGuid.h"
#include "WorldPacket.h"
#include <algorithm>
#include <vector>
#include <map>
#include <sstream>

namespace lfg
{

    enum LFGEnum
    {
        LFG_TANKS_NEEDED                             = 1,
        LFG_HEALERS_NEEDED                           = 1,
        LFG_DPS_NEEDED                               = 3
    };

    enum LfgRoles
    {
        PLAYER_ROLE_NONE                             = 0x00,
        PLAYER_ROLE_LEADER                           = 0x01,
        PLAYER_ROLE_TANK                             = 0x02,
        PLAYER_ROLE_HEALER                           = 0x04,
        PLAYER_ROLE_DAMAGE                           = 0x08
    };

    enum LfgUpdateType
    {
        LFG_UPDATETYPE_DEFAULT                       = 0,      // Internal Use
        LFG_UPDATETYPE_LEADER_UNK1                   = 1,      // FIXME: At group leave
        LFG_UPDATETYPE_LEAVE_RAIDBROWSER             = 2,
        LFG_UPDATETYPE_JOIN_RAIDBROWSER              = 3,
        LFG_UPDATETYPE_ROLECHECK_ABORTED             = 4,
        LFG_UPDATETYPE_JOIN_QUEUE                    = 5,
        LFG_UPDATETYPE_ROLECHECK_FAILED              = 6,
        LFG_UPDATETYPE_REMOVED_FROM_QUEUE            = 7,
        LFG_UPDATETYPE_PROPOSAL_FAILED               = 8,
        LFG_UPDATETYPE_PROPOSAL_DECLINED             = 9,
        LFG_UPDATETYPE_GROUP_FOUND                   = 10,
        LFG_UPDATETYPE_ADDED_TO_QUEUE                = 12,
        LFG_UPDATETYPE_PROPOSAL_BEGIN                = 13,
        LFG_UPDATETYPE_UPDATE_STATUS                 = 14,
        LFG_UPDATETYPE_GROUP_MEMBER_OFFLINE          = 15,
        LFG_UPDATETYPE_GROUP_DISBAND_UNK16           = 16,     // FIXME: Sometimes at group disband
    };

    enum LfgState
    {
        LFG_STATE_NONE,                                        // Not using LFG / LFR
        LFG_STATE_ROLECHECK,                                   // Rolecheck active
        LFG_STATE_QUEUED,                                      // Queued
        LFG_STATE_PROPOSAL,                                    // Proposal active
        LFG_STATE_BOOT,                                        // Vote kick active
        LFG_STATE_DUNGEON,                                     // In LFG Group, in a Dungeon
        LFG_STATE_FINISHED_DUNGEON,                            // In LFG Group, in a finished Dungeon
        LFG_STATE_RAIDBROWSER                                  // Using Raid finder
    };

    /// Instance lock types
    enum LfgLockStatusType
    {
        LFG_LOCKSTATUS_INSUFFICIENT_EXPANSION        = 1,
        LFG_LOCKSTATUS_TOO_LOW_LEVEL                 = 2,
        LFG_LOCKSTATUS_TOO_HIGH_LEVEL                = 3,
        LFG_LOCKSTATUS_TOO_LOW_GEAR_SCORE            = 4,
        LFG_LOCKSTATUS_TOO_HIGH_GEAR_SCORE           = 5,
        LFG_LOCKSTATUS_RAID_LOCKED                   = 6,
        LFG_LOCKSTATUS_ATTUNEMENT_TOO_LOW_LEVEL      = 1001,
        LFG_LOCKSTATUS_ATTUNEMENT_TOO_HIGH_LEVEL     = 1002,
        LFG_LOCKSTATUS_QUEST_NOT_COMPLETED           = 1022,
        LFG_LOCKSTATUS_MISSING_ITEM                  = 1025,
        LFG_LOCKSTATUS_NOT_IN_SEASON                 = 1031,
        LFG_LOCKSTATUS_MISSING_ACHIEVEMENT           = 1034
    };

    /// Answer state (Also used to check compatibilites)
    enum LfgAnswer
    {
        LFG_ANSWER_PENDING                           = -1,
        LFG_ANSWER_DENY                              = 0,
        LFG_ANSWER_AGREE                             = 1
    };

    enum LfgRandomDungeonIds : uint32
    {
        RANDOM_DUNGEON_NORMAL_TBC                    = 259,
        RANDOM_DUNGEON_HEROIC_TBC                    = 260,
        RANDOM_DUNGEON_NORMAL_WOTLK                  = 261,
        RANDOM_DUNGEON_HEROIC_WOTLK                  = 262
    };

    class Lfg5Guids;

    typedef std::list<Lfg5Guids> Lfg5GuidsList;
    typedef std::set<uint32> LfgDungeonSet;
    typedef std::map<uint32, uint32> LfgLockMap;
    typedef std::map<ObjectGuid, LfgLockMap> LfgLockPartyMap;
    typedef GuidSet LfgGuidSet;
    typedef GuidList LfgGuidList;
    typedef std::map<ObjectGuid, uint8> LfgRolesMap;
    typedef std::map<ObjectGuid, ObjectGuid> LfgGroupsMap;

    /// Queue entries (player or group guids) checked together for a match, sorted, plus their merged roles once matched.
    /// Holds up to a raid's worth of entries for Raid Finder despite the name.
    class Lfg5Guids
    {
    public:
        std::vector<ObjectGuid> guids;
        LfgRolesMap* roles;

        Lfg5Guids()
        {
            roles = nullptr;
        }

        Lfg5Guids(ObjectGuid g)
        {
            if (g)
                guids.push_back(g);
            roles = nullptr;
        }

        Lfg5Guids(Lfg5Guids const& x)
        {
            guids = x.guids;
            roles = x.roles ? (new LfgRolesMap(*(x.roles))) : nullptr;
        }

        Lfg5Guids(Lfg5Guids const& x, bool /*copyRoles*/)
        {
            guids = x.guids;
            roles = nullptr;
        }

        ~Lfg5Guids() { delete roles; }
        void addRoles(LfgRolesMap const& r) { roles = new LfgRolesMap(r); }
        void clear() { guids.clear(); }
        [[nodiscard]] bool empty() const { return guids.empty(); }
        [[nodiscard]] ObjectGuid front() const { return guids.empty() ? ObjectGuid::Empty : guids.front(); }

        [[nodiscard]] uint8 size() const { return static_cast<uint8>(guids.size()); }

        void insert(ObjectGuid const& g)
        {
            guids.insert(std::upper_bound(guids.begin(), guids.end(), g), g);
        }

        void force_insert_front(ObjectGuid const& g)
        {
            guids.insert(guids.begin(), g);
        }

        void remove(ObjectGuid const& g)
        {
            std::vector<ObjectGuid>::iterator it = std::find(guids.begin(), guids.end(), g);
            if (it != guids.end())
                guids.erase(it);
        }

        [[nodiscard]] bool hasGuid(ObjectGuid const& g) const
        {
            return g && std::find(guids.begin(), guids.end(), g) != guids.end();
        }

        bool operator<(Lfg5Guids const& x) const
        {
            return guids < x.guids;
        }

        bool operator==(Lfg5Guids const& x) const
        {
            return guids == x.guids;
        }

        void operator=(Lfg5Guids const& x)
        {
            if (this == &x)
                return;

            guids = x.guids;
            delete roles;
            roles = x.roles ? (new LfgRolesMap(*(x.roles))) : nullptr;
        }

        [[nodiscard]] std::string toString() const // for debugging
        {
            std::ostringstream o;
            for (size_t i = 0; i < guids.size(); ++i)
            {
                if (i)
                    o << ",";
                o << guids[i].ToString();
            }
            o << ":" << (roles ? 1 : 0);
            return o.str();
        }
    };

    std::string ConcatenateDungeons(LfgDungeonSet const& dungeons);
    std::string GetRolesString(uint8 roles);
    std::string GetStateString(LfgState state);

} // namespace lfg

#endif
