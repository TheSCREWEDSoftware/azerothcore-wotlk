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

#ifndef __TRINITY_ACHIEVEMENTMGR_H
#define __TRINITY_ACHIEVEMENTMGR_H

#include "CriteriaHandler.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Duration.h"

class Guild;

enum AchievementOfflinePlayerUpdateType
{
    ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_COMPLETE_ACHIEVEMENT = 1,
    ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_UPDATE_CRITERIA      = 2
};

// AzerothCore: changes queued while the character was offline (auction mail, commands), applied after login
struct AchievementOfflinePlayerUpdate
{
    AchievementOfflinePlayerUpdateType updateType;
    uint32 arg1;
    uint32 arg2;
    uint32 arg3;
};

struct AchievementReward
{
    uint32 TitleId[2];
    uint32 ItemId;
    uint32 SenderCreatureId;
    std::string Subject;
    std::string Body;
    uint32 MailTemplateId;
};

struct AchievementRewardLocale
{
    std::vector<std::string> Subject;
    std::vector<std::string> Body;
};

struct CompletedAchievementData
{
    std::time_t Date = std::time_t(0);
    GuidSet CompletingPlayers;
    bool Changed;
};

class AchievementMgr : public CriteriaHandler
{
public:
    AchievementMgr();
    ~AchievementMgr();

    void CheckAllAchievementCriteria(Player* referencePlayer);

    virtual void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) = 0;
    bool HasAchieved(uint32 achievementId) const;
    uint32 GetAchievementPoints() const;

protected:
    bool CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const override;
    bool CanCompleteCriteriaTree(CriteriaTree const* tree) override;
    void CompletedCriteriaTree(CriteriaTree const* tree, Player* referencePlayer) override;
    void AfterCriteriaTreeUpdate(CriteriaTree const* tree, Player* referencePlayer) override;

    bool IsCompletedAchievement(AchievementEntry const* entry);

    bool RequiredAchievementSatisfied(uint32 achievementId) const override;

protected:
    std::unordered_map<uint32, CompletedAchievementData> _completedAchievements;
    uint32 _achievementPoints;
};

class PlayerAchievementMgr : public AchievementMgr
{
public:
    explicit PlayerAchievementMgr(Player* owner);

    void Reset() override;

    static void DeleteFromDB(ObjectGuid const& guid);
    void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult offlineUpdatesResult = nullptr);
    void SaveToDB(CharacterDatabaseTransaction trans);

    void Update(uint32 timeDiff);

    void ResetCriteria(CriteriaTypes type, uint64 miscValue1 = 0, uint64 miscValue2 = 0, bool evenIfCriteriaComplete = false);
    // AzerothCore: reset the criteria of every type that fail on this event (death, map change, ...)
    void ResetCriteriaByFailEvent(CriteriaCondition failEvent, uint64 failAsset = 0, bool evenIfCriteriaComplete = false);

    void SendAllData(Player const* receiver) const override;
    void SendAchievementInfo(Player* receiver, uint32 achievementId = 0) const;

    void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) override;

protected:
    void SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const override;
    void SendCriteriaProgressRemoved(uint32 criteriaId) override;

    void SendAchievementEarned(AchievementEntry const* achievement) const;

    void SendPacket(WorldPacket const* data) const override;

    std::string GetOwnerInfo() const override;
    CriteriaList const& GetCriteriaByType(CriteriaTypes type) const override;

    bool CanSetCriteriaProgress(Criteria const* criteria, Player* referencePlayer) const override;
    void AfterSetCriteriaProgress(Criteria const* criteria, Player* referencePlayer) const override;

private:
    void ProcessOfflineUpdatesQueue();
    void ProcessOfflineUpdate(AchievementOfflinePlayerUpdate const& update);

    Player* _owner;
    std::vector<AchievementOfflinePlayerUpdate> _offlineUpdatesQueue;
    uint32 _offlineUpdatesDelayTimer = 0;
};

class GuildAchievementMgr : public AchievementMgr
{
public:
    explicit GuildAchievementMgr(Guild* owner);

    void Reset() override;

    static void DeleteFromDB(ObjectGuid const& guid);
    void LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult);
    void SaveToDB(CharacterDatabaseTransaction trans);

    void SendAllData(Player const* receiver) const override;
    void SendAchievementInfo(Player* receiver, uint32 achievementId = 0) const;
    void SendAllTrackedCriterias(Player* receiver, std::set<uint32> const& trackedCriterias) const;

    void CompletedAchievement(AchievementEntry const* entry, Player* referencePlayer) override;

protected:
    void SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const override;
    void SendCriteriaProgressRemoved(uint32 criteriaId) override;

    void SendAchievementEarned(AchievementEntry const* achievement) const;

    void SendPacket(WorldPacket const* data) const override;

    std::string GetOwnerInfo() const override;
    CriteriaList const& GetCriteriaByType(CriteriaTypes type) const override;

    // AzerothCore has no periodic guild save: guild progress is written when it changes
    void AfterSetCriteriaProgress(Criteria const* criteria, Player* referencePlayer) const override;
    void SaveChanges();

private:
    Guild* _owner;
};

class AchievementGlobalMgr
{
    AchievementGlobalMgr() { }
    ~AchievementGlobalMgr() { }

public:
    static AchievementGlobalMgr* Instance();

    std::vector<AchievementEntry const*> const* GetAchievementByReferencedId(uint32 id) const;
    AchievementEntry const* GetAchievement(uint32 achievementId) const;
    AchievementReward const* GetAchievementReward(AchievementEntry const* achievement) const;
    AchievementRewardLocale const* GetAchievementRewardLocale(AchievementEntry const* achievement) const;

    bool IsRealmCompleted(AchievementEntry const* achievement) const;
    void SetRealmCompleted(AchievementEntry const* achievement);

    void LoadAchievementReferenceList();
    void LoadCompletedAchievements();
    void LoadRewards();
    void LoadRewardLocales();

    // AzerothCore: queue changes for a character that is not online
    void CompletedAchievementForOfflinePlayer(ObjectGuid::LowType playerLowGuid, AchievementEntry const* entry);
    void UpdateAchievementCriteriaForOfflinePlayer(ObjectGuid::LowType playerLowGuid, CriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0);

private:
    // store achievements by referenced achievement id to speed up lookup
    std::unordered_map<uint32, std::vector<AchievementEntry const*>> _achievementListByReferencedId;

    // store realm first achievements
    // SystemTimePoint::min() is a placeholder value for realm firsts not yet completed
    // SystemTimePoint::max() is a value assigned to realm firsts complete before worldserver started
    std::unordered_map<uint32 /*achievementId*/, SystemTimePoint /*completionTime*/> _allCompletedAchievements;

    std::unordered_map<uint32, AchievementReward> _achievementRewards;
    std::unordered_map<uint32, AchievementRewardLocale> _achievementRewardLocales;
};

#define sAchievementMgr AchievementGlobalMgr::Instance()

#endif
