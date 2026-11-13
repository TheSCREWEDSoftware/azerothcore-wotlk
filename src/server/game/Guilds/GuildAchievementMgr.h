/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ACORE_GUILD_ACHIEVEMENT_MGR_H
#define ACORE_GUILD_ACHIEVEMENT_MGR_H

#include "DBCEnums.h"
#include "DatabaseEnvFwd.h"
#include "ObjectGuid.h"
#include <unordered_map>

class Guild;
class Player;
class Unit;
class Field;
struct AchievementEntry;
struct AchievementCriteriaEntry;

// 4.3.4 guild achievements: criteria progress and completed achievements kept per guild
class GuildAchievementMgr
{
public:
    explicit GuildAchievementMgr(Guild* owner) : _owner(owner) { }

    void LoadAchievementFromDB(Field* fields);
    void LoadCriteriaFromDB(Field* fields);
    void DeleteFromDB(CharacterDatabaseTransaction trans) const;

    // guild criteria types, called from the guild events (level up, repairs, tabards, kills, quests, ...)
    void UpdateCriteria(AchievementCriteriaTypes type, uint64 miscValue1 = 0, uint64 miscValue2 = 0, Unit const* unit = nullptr, Player* source = nullptr);

    [[nodiscard]] bool HasAchieved(uint32 achievementId) const { return _completed.contains(achievementId); }
    [[nodiscard]] uint32 GetAchievementPoints() const { return _achievementPoints; }

    void SendAllData(Player* receiver) const;
    void SendCriteriaData(Player* receiver, uint32 achievementId) const;

private:
    enum class Progress { Set, Accumulate, Highest };

    struct CriteriaProgress
    {
        uint64 Counter = 0;
        time_t Date = 0;
        ObjectGuid CompletedGuid;
    };

    struct CompletedAchievement
    {
        time_t Date = 0;
        GuidSet Guids;
    };

    void SetCriteriaProgress(AchievementCriteriaEntry const* criteria, uint64 changeValue, Progress progressType, Player* source);
    [[nodiscard]] bool IsCompletedCriteria(AchievementCriteriaEntry const* criteria) const;
    [[nodiscard]] bool IsCompletedAchievement(AchievementEntry const* achievement) const;
    void CompleteAchievement(AchievementEntry const* achievement, Player* source);
    void SaveCriteria(uint32 criteriaId, CriteriaProgress const& progress) const;

    Guild* _owner;
    std::unordered_map<uint32 /*criteriaId*/, CriteriaProgress> _criteriaProgress;
    std::unordered_map<uint32 /*achievementId*/, CompletedAchievement> _completed;
    uint32 _achievementPoints = 0;
};

#endif
