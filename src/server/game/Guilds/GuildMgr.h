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

#ifndef _GUILDMGR_H
#define _GUILDMGR_H

#include "Guild.h"

// Cata Guild Challenges: one row per GuildChallengeType from the world table `guild_challenges`.
struct GuildChallenge
{
    uint32 ChallengeType = 0;
    uint32 ChallengeCount = 0;
    uint32 Experience = 0;
    uint32 Gold = 0;
    uint32 GoldMaxLevel = 0;
};

// Cata guild rewards: one row per reward item from the world table `guild_rewards`. Eligibility
// (has the required reputation standing, achievement, and gold) is computed client-side from
// this catalog plus the player's own state - see GuildMgr::LoadGuildRewards.
struct GuildReward
{
    uint32 Entry = 0;
    uint8 Standing = 0;
    int32 Racemask = 0;
    uint64 Price = 0;
    uint32 AchievementId = 0;
};

// Cata guild profession/recipe tracking (see .agents/docs/porting-from-trinitycore.md): one row
// per recipe (SkillLineAbility.dbc entry) that is flagged as "unique" (has a non-zero UniqueBit
// index) for its skill line. Built once from the DBCs, not from the database - see
// GuildMgr::LoadGuildProfessionData.
struct GuildProfession
{
    uint32 SpellId = 0;
    uint32 UniqueBits = 0;
};

class GuildMgr
{
private:
    GuildMgr();
    ~GuildMgr();

public:
    static GuildMgr* instance();

    Guild* GetGuildByLeader(ObjectGuid guid) const;
    Guild* GetGuildById(uint32 guildId) const;
    Guild* GetGuildByName(std::string_view guildName) const;
    std::string GetGuildNameById(uint32 guildId) const;

    void LoadGuilds();
    void AddGuild(Guild* guild);
    void RemoveGuild(uint32 guildId);

    uint32 GenerateGuildId();
    void SetNextGuildId(uint32 Id) { NextGuildId = Id; }

    void ResetTimes(bool weekly = false);

    // Cata guild leveling
    uint64 GetXPForGuildLevel(uint8 level) const;
    void LoadGuildXpForLevel();

    // Cata Guild Challenges
    void LoadGuildChallenges();
    std::vector<GuildChallenge> const& GetGuildChallengeData() const { return GuildChallenges; }

    // Cata guild rewards
    void LoadGuildRewards();
    std::vector<GuildReward> const& GetGuildRewards() const { return GuildRewards; }

    // Cata guild profession/recipe tracking
    void LoadGuildProfessionData();
    std::vector<GuildProfession> const& GetGuildProfessionData(uint32 skillId) const
    {
        auto itr = GuildProfessionStore.find(skillId);
        if (itr == GuildProfessionStore.end())
        {
            static std::vector<GuildProfession> const empty;
            return empty;
        }
        return itr->second;
    }
protected:
    typedef std::unordered_map<uint32, Guild*> GuildContainer;
    uint32 NextGuildId;
    GuildContainer GuildStore;

    std::vector<uint64> GuildXPperLevel;
    std::vector<GuildChallenge> GuildChallenges;
    std::vector<GuildReward> GuildRewards;
    std::unordered_map<uint32 /*skillId*/, std::vector<GuildProfession>> GuildProfessionStore;
};

#define sGuildMgr GuildMgr::instance()

#endif
