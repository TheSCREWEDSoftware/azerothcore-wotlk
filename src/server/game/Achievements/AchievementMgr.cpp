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

#include "AchievementMgr.h"
#include "AchievementPackets.h"
#include "GameTime.h"
#include "CellImpl.h"
#include "ChatTextBuilder.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "GuildMgr.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "WorldSessionMgr.h"

struct VisibleAchievementCheck
{
    AchievementEntry const* operator()(std::pair<uint32, CompletedAchievementData> const& val)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(val.first);
        if (achievement && !(achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN))
            return achievement;
        return nullptr;
    }
};

AchievementMgr::AchievementMgr() : _achievementPoints(0) { }

AchievementMgr::~AchievementMgr() { }

/**
* called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet
*/
void AchievementMgr::CheckAllAchievementCriteria(Player* referencePlayer)
{
    // suppress sending packets
    for (uint32 i = 0; i < CRITERIA_TYPE_TOTAL; ++i)
        UpdateCriteria(CriteriaTypes(i), 0, 0, 0, NULL, referencePlayer);
}

bool AchievementMgr::HasAchieved(uint32 achievementId) const
{
    return _completedAchievements.find(achievementId) != _completedAchievements.end();
}

uint32 AchievementMgr::GetAchievementPoints() const
{
    return _achievementPoints;
}

bool AchievementMgr::CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const
{
    if (HasAchieved(tree->Achievement->ID))
    {
        LOG_TRACE("criteria.achievement", "AchievementMgr::CanUpdateCriteriaTree: (Id: {} Type {} Achievement {}) Achievement already earned",
            criteria->ID, CriteriaMgr::GetCriteriaTypeString(criteria->Entry->Type), tree->Achievement->ID);
        return false;
    }

    if (tree->Achievement->MapID != -1 && referencePlayer->GetMapId() != uint32(tree->Achievement->MapID))
    {
        LOG_TRACE("criteria.achievement", "AchievementMgr::CanUpdateCriteriaTree: (Id: {} Type {} Achievement {}) Wrong map",
            criteria->ID, CriteriaMgr::GetCriteriaTypeString(criteria->Entry->Type), tree->Achievement->ID);
        return false;
    }

    if ((tree->Achievement->Faction == ACHIEVEMENT_FACTION_HORDE    && referencePlayer->GetTeam() != HORDE) ||
        (tree->Achievement->Faction == ACHIEVEMENT_FACTION_ALLIANCE && referencePlayer->GetTeam() != ALLIANCE))
    {
        LOG_TRACE("criteria.achievement", "AchievementMgr::CanUpdateCriteriaTree: (Id: {} Type {} Achievement {}) Wrong faction",
            criteria->ID, CriteriaMgr::GetCriteriaTypeString(criteria->Entry->Type), tree->Achievement->ID);
        return false;
    }

    return CriteriaHandler::CanUpdateCriteriaTree(criteria, tree, referencePlayer);
}

bool AchievementMgr::CanCompleteCriteriaTree(CriteriaTree const* tree)
{
    AchievementEntry const* achievement = tree->Achievement;
    if (!achievement)
        return false;

    // counter can never complete
    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr->IsRealmCompleted(achievement))
            return false;

        if (sWorld->getBoolConfig(CONFIG_ACHIEVEMENT_REALM_FIRST_RACE_LIMIT_ONE_PER_CHARACTER))
        {
            // A character may only have 1 race-specific 'Realm First!' achievement
            // prevent clever use of the race/faction change service to obtain multiple 'Realm First!' achievements
            constexpr std::array<uint32, 9> raceSpecificRealmFirstAchievements { 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413 };
            bool isRaceSpecific = std::ranges::find(raceSpecificRealmFirstAchievements, achievement->ID) != std::ranges::end(raceSpecificRealmFirstAchievements);
            if (isRaceSpecific)
                for (uint32 raceAchievementId : raceSpecificRealmFirstAchievements)
                    if (raceAchievementId != achievement->ID && HasAchieved(raceAchievementId))
                        return false;
        }
    }

    return true;
}

void AchievementMgr::CompletedCriteriaTree(CriteriaTree const* tree, Player* referencePlayer)
{
    // counter can never complete
    if (tree->Achievement->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // already completed and stored
    if (HasAchieved(tree->Achievement->ID))
        return;

    if (IsCompletedAchievement(tree->Achievement))
        CompletedAchievement(tree->Achievement, referencePlayer);
}

void AchievementMgr::AfterCriteriaTreeUpdate(CriteriaTree const* tree, Player* referencePlayer)
{
    // check again the completeness for SUMM and REQ COUNT achievements,
    // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
    if (tree->Achievement->Flags & ACHIEVEMENT_FLAG_SUMM)
        if (IsCompletedAchievement(tree->Achievement))
            CompletedAchievement(tree->Achievement, referencePlayer);

    if (std::vector<AchievementEntry const*> const* achRefList = sAchievementMgr->GetAchievementByReferencedId(tree->Achievement->ID))
        for (AchievementEntry const* refAchievement : *achRefList)
            if (IsCompletedAchievement(refAchievement))
                CompletedAchievement(refAchievement, referencePlayer);
}

bool AchievementMgr::IsCompletedAchievement(AchievementEntry const* entry)
{
    // counter can never complete
    if (entry->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(entry->CriteriaTree);
    if (!tree)
        return false;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT contained in the achievement, but in each individual criteria
    if (entry->Flags & ACHIEVEMENT_FLAG_SUMM)
    {
        uint64 progress = 0;
        CriteriaMgr::WalkCriteriaTree(tree, [this, &progress](CriteriaTree const* criteriaTree)
        {
            if (criteriaTree->Criteria)
                if (CriteriaProgress const* criteriaProgress = this->GetCriteriaProgress(criteriaTree->Criteria))
                    progress += criteriaProgress->Counter;
        });
        return progress >= tree->Entry->Amount;
    }

    return IsCompletedCriteriaTree(tree);
}

bool AchievementMgr::RequiredAchievementSatisfied(uint32 achievementId) const
{
    return HasAchieved(achievementId);
}

PlayerAchievementMgr::PlayerAchievementMgr(Player* owner) : _owner(owner)
{
}

void PlayerAchievementMgr::Reset()
{
    AchievementMgr::Reset();

    for (auto iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
    {
        WorldPackets::Achievement::AchievementDeleted achievementDeleted;
        achievementDeleted.AchievementID = iter->first;
        SendPacket(achievementDeleted.Write());
    }

    _completedAchievements.clear();
    _achievementPoints = 0;
    DeleteFromDB(_owner->GetGUID());

    // re-fill data
    CheckAllAchievementCriteria(_owner);
}

void PlayerAchievementMgr::DeleteFromDB(ObjectGuid const& guid)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT);
    stmt->SetData(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_PROGRESS);
    stmt->SetData(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, guid.GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void PlayerAchievementMgr::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult, PreparedQueryResult offlineUpdatesResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].Get<uint32>();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = _completedAchievements[achievementid];
            ca.Date = time_t(fields[1].Get<uint32>());
            ca.Changed = false;

            _achievementPoints += achievement->Points;

            // title achievement rewards are retroactive
            if (AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement))
                if (uint32 titleId = reward->TitleId[Player::TeamIdForRace(_owner->getRace()) == TEAM_ALLIANCE ? 0 : 1])
                    if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        _owner->SetTitle(titleEntry);

        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 id = fields[0].Get<uint32>();
            uint64 counter = fields[1].Get<uint64>();
            time_t date = time_t(fields[2].Get<uint32>());

            Criteria const* criteria = sCriteriaMgr->GetCriteria(id);
            if (!criteria)
            {
                // Removing non-existing criteria data for all characters
                LOG_ERROR("criteria.achievement", "Non-existing achievement criteria {} data has been removed from the table `character_achievement_progress`.", id);

                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->SetData(0, uint16(id));
                CharacterDatabase.Execute(stmt);

                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[id];
            progress.Counter = counter;
            progress.Date = date;
            progress.Changed = false;
        } while (criteriaResult->NextRow());
    }

    if (offlineUpdatesResult)
    {
        do
        {
            Field* fields = offlineUpdatesResult->Fetch();

            AchievementOfflinePlayerUpdate update;
            update.updateType = static_cast<AchievementOfflinePlayerUpdateType>(fields[0].Get<uint8>());
            update.arg1       = fields[1].Get<uint32>();
            update.arg2       = fields[2].Get<uint32>();
            update.arg3       = fields[3].Get<uint32>();

            _offlineUpdatesQueue.push_back(update);
        } while (offlineUpdatesResult->NextRow());

        if (!_offlineUpdatesQueue.empty())
            _offlineUpdatesDelayTimer = 5 * SECOND * IN_MILLISECONDS;
    }
}

void PlayerAchievementMgr::SaveToDB(CharacterDatabaseTransaction trans)
{
    if (!_completedAchievements.empty())
    {
        for (auto iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
        {
            if (!iter->second.Changed)
                continue;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_BY_ACHIEVEMENT);
            stmt->SetData(0, iter->first);
            stmt->SetData(1, _owner->GetGUID().GetCounter());
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT);
            stmt->SetData(0, _owner->GetGUID().GetCounter());
            stmt->SetData(1, iter->first);
            stmt->SetData(2, uint32(iter->second.Date));
            trans->Append(stmt);

            sScriptMgr->OnPlayerAchievementSave(trans, _owner, iter->first, iter->second);

            iter->second.Changed = false;
        }
    }

    if (!_criteriaProgress.empty())
    {
        for (auto iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
        {
            if (!iter->second.Changed)
                continue;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_PROGRESS_BY_CRITERIA);
            stmt->SetData(0, _owner->GetGUID().GetCounter());
            stmt->SetData(1, iter->first);
            trans->Append(stmt);

            if (iter->second.Counter)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_PROGRESS);
                stmt->SetData(0, _owner->GetGUID().GetCounter());
                stmt->SetData(1, iter->first);
                stmt->SetData(2, iter->second.Counter);
                stmt->SetData(3, uint32(iter->second.Date));
                trans->Append(stmt);
            }

            sScriptMgr->OnPlayerCriteriaSave(trans, _owner, iter->first, iter->second);

            iter->second.Changed = false;
        }
    }
}

void PlayerAchievementMgr::ResetCriteria(CriteriaTypes type, uint64 miscValue1, uint64 miscValue2, bool evenIfCriteriaComplete)
{
    LOG_DEBUG("criteria.achievement", "PlayerAchievementMgr::ResetCriteria({}, {}, {})", type, miscValue1, miscValue2);

    // disable for gamemasters with GM-mode enabled
    if (_owner->IsGameMaster())
        return;

    CriteriaList const& achievementCriteriaList = GetCriteriaByType(type);
    for (Criteria const* achievementCriteria : achievementCriteriaList)
    {
        if (achievementCriteria->Entry->FailEvent != miscValue1 || (achievementCriteria->Entry->FailAsset && achievementCriteria->Entry->FailAsset != miscValue2))
            continue;

        std::vector<CriteriaTree const*> const* trees = sCriteriaMgr->GetCriteriaTreesByCriteria(achievementCriteria->ID);
        if (!trees)
            continue;

        bool allComplete = true;
        for (CriteriaTree const* tree : *trees)
        {
            // don't update already completed criteria if not forced or achievement already complete
            if (!(IsCompletedCriteriaTree(tree) && !evenIfCriteriaComplete) || !HasAchieved(tree->Achievement->ID))
            {
                allComplete = false;
                break;
            }
        }

        if (allComplete)
            continue;

        RemoveCriteriaProgress(achievementCriteria);
    }
}

void PlayerAchievementMgr::ResetCriteriaByFailEvent(CriteriaCondition failEvent, uint64 failAsset, bool evenIfCriteriaComplete)
{
    for (uint32 i = 0; i < CRITERIA_TYPE_TOTAL; ++i)
        ResetCriteria(CriteriaTypes(i), failEvent, failAsset, evenIfCriteriaComplete);
}

void PlayerAchievementMgr::Update(uint32 timeDiff)
{
    if (_offlineUpdatesDelayTimer > 0)
    {
        if (timeDiff >= _offlineUpdatesDelayTimer)
        {
            _offlineUpdatesDelayTimer = 0;
            ProcessOfflineUpdatesQueue();
        }
        else
            _offlineUpdatesDelayTimer -= timeDiff;
    }

    UpdateTimedCriteria(timeDiff);
}

void PlayerAchievementMgr::ProcessOfflineUpdatesQueue()
{
    if (_offlineUpdatesQueue.empty())
        return;

    for (AchievementOfflinePlayerUpdate const& update : _offlineUpdatesQueue)
        ProcessOfflineUpdate(update);

    _offlineUpdatesQueue.clear();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, _owner->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

void PlayerAchievementMgr::ProcessOfflineUpdate(AchievementOfflinePlayerUpdate const& update)
{
    switch (update.updateType)
    {
        case ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_COMPLETE_ACHIEVEMENT:
            if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(update.arg1))
                CompletedAchievement(achievement, _owner);
            else
                LOG_ERROR("criteria.achievement", "Offline achievement update for {} completes unknown achievement {}", _owner->GetGUID().ToString(), update.arg1);
            break;
        case ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_UPDATE_CRITERIA:
            UpdateCriteria(CriteriaTypes(update.arg1), update.arg2, update.arg3, 0, nullptr, _owner);
            break;
        default:
            LOG_ERROR("criteria.achievement", "Unknown offline achievement update type {} for {}", update.updateType, _owner->GetGUID().ToString());
            break;
    }
}

bool PlayerAchievementMgr::CanSetCriteriaProgress(Criteria const* criteria, Player* /*referencePlayer*/) const
{
    return sScriptMgr->OnPlayerBeforeCriteriaProgress(_owner, criteria);
}

void PlayerAchievementMgr::AfterSetCriteriaProgress(Criteria const* criteria, Player* /*referencePlayer*/) const
{
    sScriptMgr->OnPlayerCriteriaProgress(_owner, criteria);
}

void PlayerAchievementMgr::SendAllData(Player const* /*receiver*/) const
{
    VisibleAchievementCheck filterInvisible;
    WorldPackets::Achievement::AllAchievementData achievementData;
    achievementData.Data.Earned.reserve(_completedAchievements.size());
    achievementData.Data.Progress.reserve(_criteriaProgress.size());

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        AchievementEntry const* achievement = filterInvisible(*itr);
        if (!achievement)
            continue;

        WorldPackets::Achievement::EarnedAchievement earned;
        earned.Id = itr->first;
        earned.Date = itr->second.Date;
        if (!(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
        {
            earned.Owner = _owner->GetGUID();
            earned.VirtualRealmAddress = earned.NativeRealmAddress = GetVirtualRealmAddress();
        }
        achievementData.Data.Earned.push_back(earned);
    }

    for (auto itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        WorldPackets::Achievement::CriteriaProgress progress;
        progress.Id = itr->first;
        progress.Quantity = itr->second.Counter;
        progress.Player = itr->second.PlayerGUID;
        progress.Flags = 0;
        progress.Date = itr->second.Date;
        progress.TimeFromStart = 0;
        progress.TimeFromCreate = 0;
        achievementData.Data.Progress.push_back(progress);
    }

    SendPacket(achievementData.Write());
}

void PlayerAchievementMgr::SendAchievementInfo(Player* receiver, uint32 /*achievementId = 0 */) const
{
    VisibleAchievementCheck filterInvisible;
    WorldPackets::Achievement::RespondInspectAchievements inspectedAchievements;
    inspectedAchievements.Player = _owner->GetGUID();
    inspectedAchievements.Data.Earned.reserve(_completedAchievements.size());
    inspectedAchievements.Data.Progress.reserve(_criteriaProgress.size());

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        AchievementEntry const* achievement = filterInvisible(*itr);
        if (!achievement)
            continue;

        WorldPackets::Achievement::EarnedAchievement earned;
        earned.Id = itr->first;
        earned.Date = itr->second.Date;
        if (!(achievement->Flags & ACHIEVEMENT_FLAG_ACCOUNT))
        {
            earned.Owner = _owner->GetGUID();
            earned.VirtualRealmAddress = earned.NativeRealmAddress = GetVirtualRealmAddress();
        }
        inspectedAchievements.Data.Earned.push_back(earned);
    }

    for (auto itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        WorldPackets::Achievement::CriteriaProgress progress;
        progress.Id = itr->first;
        progress.Quantity = itr->second.Counter;
        progress.Player = itr->second.PlayerGUID;
        progress.Flags = 0;
        progress.Date = itr->second.Date;
        progress.TimeFromStart = 0;
        progress.TimeFromCreate = 0;
        inspectedAchievements.Data.Progress.push_back(progress);
    }

    receiver->GetSession()->SendPacket(inspectedAchievements.Write());
}

void PlayerAchievementMgr::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    // disable for gamemasters with GM-mode enabled
    if (_owner->IsGameMaster())
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    if (!sScriptMgr->OnPlayerBeforeAchievementComplete(_owner, achievement))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = referencePlayer->GetGuild())
            guild->AddGuildNews(GUILD_NEWS_PLAYER_ACHIEVEMENT, referencePlayer->GetGUID(), achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    if (!_owner->GetSession()->PlayerLoading())
        SendAchievementEarned(achievement);

    LOG_DEBUG("criteria.achievement", "PlayerAchievementMgr::CompletedAchievement({}). {}", achievement->ID, GetOwnerInfo().c_str());

    CompletedAchievementData& ca = _completedAchievements[achievement->ID];
    ca.Date = time(NULL);
    ca.Changed = true;

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        _achievementPoints += achievement->Points;

    UpdateCriteria(CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, 0, NULL, referencePlayer);
    UpdateCriteria(CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->Points, 0, 0, NULL, referencePlayer);

    sScriptMgr->OnPlayerAchievementComplete(_owner, achievement);

    // reward items and titles if any
    AchievementReward const* reward = sAchievementMgr->GetAchievementReward(achievement);

    // no rewards
    if (!reward)
        return;

    // titles
    //! Currently there's only one achievement that deals with gender-specific titles.
    //! Since no common attributes were found, (not even in titleRewardFlags field)
    //! we explicitly check by ID. Maybe in the future we could move the achievement_reward
    //! condition fields to the condition system.
    if (uint32 titleId = reward->TitleId[achievement->ID == 1793 ? _owner->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER) : (_owner->GetTeam() == ALLIANCE ? 0 : 1)])
        if (CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            _owner->SetTitle(titleEntry);

    // mail
    if (reward->SenderCreatureId)
    {
        MailDraft draft(uint16(reward->MailTemplateId));

        if (!reward->MailTemplateId)
        {
            // subject and text
            std::string subject = reward->Subject;
            std::string text = reward->Body;

            LocaleConstant localeConstant = _owner->GetSession()->GetSessionDbLocaleIndex();
            if (localeConstant >= LOCALE_enUS)
            {
                if (AchievementRewardLocale const* loc = sAchievementMgr->GetAchievementRewardLocale(achievement))
                {
                    ObjectMgr::GetLocaleString(loc->Subject, localeConstant, subject);
                    ObjectMgr::GetLocaleString(loc->Body, localeConstant, text);
                }
            }

            draft = MailDraft(subject, text);
        }

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

        Item* item = reward->ItemId ? Item::CreateItem(reward->ItemId, 1, _owner) : NULL;
        if (item)
        {
            // save new item before send
            item->SaveToDB(trans);                               // save for prevent lost at next mail load, if send fail then item will deleted

            // item
            draft.AddItem(item);
        }

        draft.SendMailTo(trans, _owner, MailSender(MAIL_CREATURE, uint64(reward->SenderCreatureId)));
        CharacterDatabase.CommitTransaction(trans);
    }
}

void PlayerAchievementMgr::SendCriteriaUpdate(Criteria const* criteria, CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const
{
    WorldPackets::Achievement::CriteriaUpdate criteriaUpdate;

    criteriaUpdate.CriteriaID = criteria->ID;
    criteriaUpdate.Quantity = progress->Counter;
    criteriaUpdate.PlayerGUID = _owner->GetGUID();
    criteriaUpdate.Flags = 0;
    if (criteria->Entry->StartTimer)
        criteriaUpdate.Flags = timedCompleted ? 1 : 0; // 1 is for keeping the counter at 0 in client

    criteriaUpdate.CurrentTime = progress->Date;
    criteriaUpdate.ElapsedTime = timeElapsed;
    criteriaUpdate.CreationTime = 0;

    SendPacket(criteriaUpdate.Write());
}

void PlayerAchievementMgr::SendCriteriaProgressRemoved(uint32 criteriaId)
{
    WorldPackets::Achievement::CriteriaDeleted criteriaDeleted;
    criteriaDeleted.CriteriaID = criteriaId;
    SendPacket(criteriaDeleted.Write());
}

void PlayerAchievementMgr::SendAchievementEarned(AchievementEntry const* achievement) const
{
    // Don't send for achievements with ACHIEVEMENT_FLAG_HIDDEN
    if (achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    LOG_DEBUG("criteria.achievement", "PlayerAchievementMgr::SendAchievementEarned({})", achievement->ID);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
    {
        if (Guild* guild = sGuildMgr->GetGuildById(_owner->GetGuildId()))
        {
            Acore::BroadcastTextBuilder _builder(_owner, CHAT_MSG_GUILD_ACHIEVEMENT, BROADCAST_TEXT_ACHIEVEMENT_EARNED, _owner->getGender(), _owner, achievement->ID);
            Acore::LocalizedPacketDo<Acore::BroadcastTextBuilder> _localizer(_builder);
            guild->BroadcastWorker(_localizer, _owner);
        }

        if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
        {
            // broadcast realm first reached
            WorldPackets::Achievement::ServerFirstAchievement serverFirstAchievement;
            serverFirstAchievement.Name = _owner->GetName();
            serverFirstAchievement.PlayerGUID = _owner->GetGUID();
            serverFirstAchievement.AchievementID = achievement->ID;
            sWorldSessionMgr->SendGlobalMessage(serverFirstAchievement.Write());
        }
        // if player is in world he can tell his friends about new achievement
        else if (_owner->IsInWorld())
        {
            Acore::BroadcastTextBuilder _builder(_owner, CHAT_MSG_ACHIEVEMENT, BROADCAST_TEXT_ACHIEVEMENT_EARNED, _owner->getGender(), _owner, achievement->ID);
            Acore::LocalizedPacketDo<Acore::BroadcastTextBuilder> _localizer(_builder);
            Acore::PlayerDistWorker<Acore::LocalizedPacketDo<Acore::BroadcastTextBuilder> > _worker(_owner, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), _localizer);
            Cell::VisitObjects(_owner, _worker, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
        }
    }

    WorldPackets::Achievement::AchievementEarned achievementEarned;
    achievementEarned.Sender = _owner->GetGUID();
    achievementEarned.Earner = _owner->GetGUID();
    achievementEarned.EarnerNativeRealm = achievementEarned.EarnerVirtualRealm = GetVirtualRealmAddress();
    achievementEarned.AchievementID = achievement->ID;
    achievementEarned.Time = time(NULL);
    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        _owner->SendMessageToSetInRange(achievementEarned.Write(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), true);
    else
        _owner->SendDirectMessage(achievementEarned.Write());
}

void PlayerAchievementMgr::SendPacket(WorldPacket const* data) const
{
    _owner->SendDirectMessage(data);
}

CriteriaList const& PlayerAchievementMgr::GetCriteriaByType(CriteriaTypes type) const
{
    return sCriteriaMgr->GetPlayerCriteriaByType(type);
}

GuildAchievementMgr::GuildAchievementMgr(Guild* owner) : _owner(owner)
{
}

void GuildAchievementMgr::Reset()
{
    AchievementMgr::Reset();

    ObjectGuid guid = _owner->GetGUID();
    for (auto iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
    {
        WorldPackets::Achievement::GuildAchievementDeleted guildAchievementDeleted;
        guildAchievementDeleted.AchievementID = iter->first;
        guildAchievementDeleted.GuildGUID = guid;
        guildAchievementDeleted.TimeDeleted = time(NULL);
        SendPacket(guildAchievementDeleted.Write());
    }

    _achievementPoints = 0;
    _completedAchievements.clear();
    DeleteFromDB(guid);
}

void GuildAchievementMgr::DeleteFromDB(ObjectGuid const& guid)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEVEMENTS);
    stmt->SetData(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEVEMENT_CRITERIA);
    stmt->SetData(0, guid.GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void GuildAchievementMgr::LoadFromDB(PreparedQueryResult achievementResult, PreparedQueryResult criteriaResult)
{
    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();
            uint32 achievementid = fields[0].Get<uint32>();

            // must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementid);
            if (!achievement)
                continue;

            CompletedAchievementData& ca = _completedAchievements[achievementid];
            ca.Date = time_t(fields[1].Get<uint32>());
            std::string guids = fields[2].Get<std::string>();
            std::replace(guids.begin(), guids.end(), ',', ' ');
            for (std::string_view guid : Acore::Tokenize(guids, ' ', false))
                if (Optional<ObjectGuid::LowType> lowGuid = Acore::StringTo<ObjectGuid::LowType>(guid))
                    ca.CompletingPlayers.insert(ObjectGuid::Create<HighGuid::Player>(*lowGuid));

            ca.Changed = false;

            _achievementPoints += achievement->Points;
        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = criteriaResult->Fetch();
            uint32 id = fields[0].Get<uint32>();
            uint64 counter = fields[1].Get<uint64>();
            time_t date = time_t(fields[2].Get<uint32>());
            ObjectGuid::LowType guid = fields[3].Get<uint32>();

            Criteria const* criteria = sCriteriaMgr->GetCriteria(id);
            if (!criteria)
            {
                // we will remove not existed criteria for all guilds
                LOG_ERROR("criteria.achievement", "Non-existing achievement criteria {} data removed from table `guild_achievement_progress`.", id);

                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA_GUILD);
                stmt->SetData(0, uint16(id));
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& progress = _criteriaProgress[id];
            progress.Counter = counter;
            progress.Date = date;
            progress.PlayerGUID = ObjectGuid::Create<HighGuid::Player>(guid);
            progress.Changed = false;
        } while (criteriaResult->NextRow());
    }
}

void GuildAchievementMgr::SaveToDB(CharacterDatabaseTransaction trans)
{
    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        if (!itr->second.Changed)
            continue;

        std::ostringstream guidstr;
        for (ObjectGuid const& guid : itr->second.CompletingPlayers)
            guidstr << guid.GetCounter() << ' ';

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT);
        stmt->SetData(0, _owner->GetId());
        stmt->SetData(1, itr->first);
        stmt->SetData(2, uint32(itr->second.Date));
        stmt->SetData(3, guidstr.str());
        trans->Append(stmt);

        itr->second.Changed = false;
    }

    for (auto itr = _criteriaProgress.begin(); itr != _criteriaProgress.end(); ++itr)
    {
        if (!itr->second.Changed)
            continue;

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT_CRITERIA);
        stmt->SetData(0, _owner->GetId());
        stmt->SetData(1, itr->first);
        stmt->SetData(2, itr->second.Counter);
        stmt->SetData(3, uint32(itr->second.Date));
        stmt->SetData(4, itr->second.PlayerGUID.GetCounter());
        trans->Append(stmt);

        itr->second.Changed = false;
    }
}

void GuildAchievementMgr::SaveChanges()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    SaveToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

void GuildAchievementMgr::AfterSetCriteriaProgress(Criteria const* /*criteria*/, Player* /*referencePlayer*/) const
{
    const_cast<GuildAchievementMgr*>(this)->SaveChanges();
}

void GuildAchievementMgr::SendAllData(Player const* receiver) const
{
    VisibleAchievementCheck filterInvisible;
    WorldPackets::Achievement::AllGuildAchievements allGuildAchievements;
    allGuildAchievements.Earned.reserve(_completedAchievements.size());

    for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
    {
        AchievementEntry const* achievement = filterInvisible(*itr);
        if (!achievement)
            continue;

        WorldPackets::Achievement::EarnedAchievement earned;
        earned.Id = itr->first;
        earned.Date = itr->second.Date;
        allGuildAchievements.Earned.push_back(earned);
    }

    receiver->GetSession()->SendPacket(allGuildAchievements.Write());
}

void GuildAchievementMgr::SendAchievementInfo(Player* receiver, uint32 achievementId /*= 0*/) const
{
    WorldPackets::Achievement::GuildCriteriaUpdate guildCriteriaUpdate;
    if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId))
    {
        if (CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(achievement->CriteriaTree))
        {
            CriteriaMgr::WalkCriteriaTree(tree, [this, &guildCriteriaUpdate](CriteriaTree const* node)
            {
                if (node->Criteria)
                {
                    auto progress = this->_criteriaProgress.find(node->Criteria->ID);
                    if (progress != this->_criteriaProgress.end())
                    {
                        WorldPackets::Achievement::GuildCriteriaProgress guildCriteriaProgress;
                        guildCriteriaProgress.CriteriaID = node->Criteria->ID;
                        guildCriteriaProgress.DateCreated = 0;
                        guildCriteriaProgress.DateStarted = 0;
                        guildCriteriaProgress.DateUpdated = progress->second.Date;
                        guildCriteriaProgress.Quantity = progress->second.Counter;
                        guildCriteriaProgress.PlayerGUID = progress->second.PlayerGUID;
                        guildCriteriaProgress.Flags = 0;

                        guildCriteriaUpdate.Progress.push_back(guildCriteriaProgress);
                    }
                }
            });
        }
    }

    receiver->GetSession()->SendPacket(guildCriteriaUpdate.Write());
}

void GuildAchievementMgr::SendAllTrackedCriterias(Player* receiver, std::set<uint32> const& trackedCriterias) const
{
    WorldPackets::Achievement::GuildCriteriaUpdate guildCriteriaUpdate;
    guildCriteriaUpdate.Progress.reserve(trackedCriterias.size());

    for (uint32 criteriaId : trackedCriterias)
    {
        auto progress = _criteriaProgress.find(criteriaId);
        if (progress == _criteriaProgress.end())
            continue;

        WorldPackets::Achievement::GuildCriteriaProgress guildCriteriaProgress;
        guildCriteriaProgress.CriteriaID = criteriaId;
        guildCriteriaProgress.DateCreated = 0;
        guildCriteriaProgress.DateStarted = 0;
        guildCriteriaProgress.DateUpdated = progress->second.Date;
        guildCriteriaProgress.Quantity = progress->second.Counter;
        guildCriteriaProgress.PlayerGUID = progress->second.PlayerGUID;
        guildCriteriaProgress.Flags = 0;

        guildCriteriaUpdate.Progress.push_back(guildCriteriaProgress);
    }

    receiver->GetSession()->SendPacket(guildCriteriaUpdate.Write());
}

void GuildAchievementMgr::CompletedAchievement(AchievementEntry const* achievement, Player* referencePlayer)
{
    LOG_DEBUG("criteria.achievement", "GuildAchievementMgr::CompletedAchievement({})", achievement->ID);

    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement->ID))
        return;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        if (Guild* guild = referencePlayer->GetGuild())
            guild->AddGuildNews(GUILD_NEWS_GUILD_ACHIEVEMENT, ObjectGuid::Empty, achievement->Flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    SendAchievementEarned(achievement);
    CompletedAchievementData& ca = _completedAchievements[achievement->ID];
    ca.Date = time(NULL);
    ca.Changed = true;

    if (achievement->Flags & ACHIEVEMENT_FLAG_SHOW_GUILD_MEMBERS)
    {
        if (referencePlayer->GetGuildId() == _owner->GetId())
            ca.CompletingPlayers.insert(referencePlayer->GetGUID());

        if (Group const* group = referencePlayer->GetGroup())
            for (GroupReference const* ref = group->GetFirstMember(); ref != NULL; ref = ref->next())
                if (Player const* groupMember = ref->GetSource())
                    if (groupMember->GetGuildId() == _owner->GetId())
                        ca.CompletingPlayers.insert(groupMember->GetGUID());
    }

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    if (!(achievement->Flags & ACHIEVEMENT_FLAG_TRACKING_FLAG))
        _achievementPoints += achievement->Points;

    UpdateCriteria(CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, 0, 0, 0, NULL, referencePlayer);
    UpdateCriteria(CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, achievement->Points, 0, 0, NULL, referencePlayer);

    SaveChanges();
}

void GuildAchievementMgr::SendCriteriaUpdate(Criteria const* entry, CriteriaProgress const* progress, uint32 /*timeElapsed*/, bool /*timedCompleted*/) const
{
    WorldPackets::Achievement::GuildCriteriaUpdate guildCriteriaUpdate;
    guildCriteriaUpdate.Progress.resize(1);

    WorldPackets::Achievement::GuildCriteriaProgress& guildCriteriaProgress = guildCriteriaUpdate.Progress[0];
    guildCriteriaProgress.CriteriaID = entry->ID;
    guildCriteriaProgress.DateCreated = 0;
    guildCriteriaProgress.DateStarted = 0;
    guildCriteriaProgress.DateUpdated = progress->Date;
    guildCriteriaProgress.Quantity = progress->Counter;
    guildCriteriaProgress.PlayerGUID = progress->PlayerGUID;
    guildCriteriaProgress.Flags = 0;

    _owner->BroadcastPacketIfTrackingAchievement(guildCriteriaUpdate.Write(), entry->ID);
}

void GuildAchievementMgr::SendCriteriaProgressRemoved(uint32 criteriaId)
{
    WorldPackets::Achievement::GuildCriteriaDeleted guildCriteriaDeleted;
    guildCriteriaDeleted.GuildGUID = _owner->GetGUID();
    guildCriteriaDeleted.CriteriaID = criteriaId;
    SendPacket(guildCriteriaDeleted.Write());
}

void GuildAchievementMgr::SendAchievementEarned(AchievementEntry const* achievement) const
{
    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // broadcast realm first reached
        WorldPackets::Achievement::ServerFirstAchievement serverFirstAchievement;
        serverFirstAchievement.Name = _owner->GetName();
        serverFirstAchievement.PlayerGUID = _owner->GetGUID();
        serverFirstAchievement.AchievementID = achievement->ID;
        serverFirstAchievement.GuildAchievement = true;
        sWorldSessionMgr->SendGlobalMessage(serverFirstAchievement.Write());
    }

    WorldPackets::Achievement::GuildAchievementEarned guildAchievementEarned;
    guildAchievementEarned.AchievementID = achievement->ID;
    guildAchievementEarned.GuildGUID = _owner->GetGUID();
    guildAchievementEarned.TimeEarned = time(NULL);
    SendPacket(guildAchievementEarned.Write());
}

void GuildAchievementMgr::SendPacket(WorldPacket const* data) const
{
    _owner->BroadcastPacket(data);
}

CriteriaList const& GuildAchievementMgr::GetCriteriaByType(CriteriaTypes type) const
{
    return sCriteriaMgr->GetGuildCriteriaByType(type);
}

std::string PlayerAchievementMgr::GetOwnerInfo() const
{
    return Acore::StringFormat("{} {}", _owner->GetGUID().ToString().c_str(), _owner->GetName().c_str());
}

std::string GuildAchievementMgr::GetOwnerInfo() const
{
    return Acore::StringFormat("Guild ID {} {}", _owner->GetId(), _owner->GetName().c_str());
}

AchievementGlobalMgr* AchievementGlobalMgr::Instance()
{
    static AchievementGlobalMgr instance;
    return &instance;
}

std::vector<AchievementEntry const*> const* AchievementGlobalMgr::GetAchievementByReferencedId(uint32 id) const
{
    auto itr = _achievementListByReferencedId.find(id);
    return itr != _achievementListByReferencedId.end() ? &itr->second : NULL;
}

AchievementEntry const* AchievementGlobalMgr::GetAchievement(uint32 achievementId) const
{
    return sAchievementStore.LookupEntry(achievementId);
}

AchievementReward const* AchievementGlobalMgr::GetAchievementReward(AchievementEntry const* achievement) const
{
    auto iter = _achievementRewards.find(achievement->ID);
    return iter != _achievementRewards.end() ? &iter->second : NULL;
}

AchievementRewardLocale const* AchievementGlobalMgr::GetAchievementRewardLocale(AchievementEntry const* achievement) const
{
    auto iter = _achievementRewardLocales.find(achievement->ID);
    return iter != _achievementRewardLocales.end() ? &iter->second : NULL;
}

bool AchievementGlobalMgr::IsRealmCompleted(AchievementEntry const* achievement) const
{
    auto itr = _allCompletedAchievements.find(achievement->ID);
    if (itr == _allCompletedAchievements.end())
        return false;

    if (itr->second == SystemTimePoint::min())
        return false;

    if (!sScriptMgr->IsRealmCompleted(this, achievement, itr->second))
        return false;

    if (itr->second == SystemTimePoint::max())
        return true;

    // Allow completing the realm first kill for configurable time window after first person did it
    // it may allow more than one group to achieve it (highly unlikely)
    // but apparently this is how blizz handles it as well
    if (achievement->Flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL)
    {
        Seconds windowSeconds = Seconds(sWorld->getIntConfig(CONFIG_ACHIEVEMENT_REALM_FIRST_KILL_WINDOW));

        if (windowSeconds == 0s)
            return true;

        return (GameTime::GetSystemTime() - itr->second) > windowSeconds;
    }

    sScriptMgr->SetRealmCompleted(achievement);

    return true;
}

void AchievementGlobalMgr::SetRealmCompleted(AchievementEntry const* achievement)
{
    if (IsRealmCompleted(achievement))
        return;

    _allCompletedAchievements[achievement->ID] = std::chrono::system_clock::now();
}

//==========================================================
void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    uint32 oldMSTime = getMSTime();

    if (sAchievementStore.GetNumRows() == 0)
    {
        LOG_INFO("server.loading", ">> Loaded 0 achievement references.");
        return;
    }

    uint32 count = 0;

    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->SharesCriteria)
            continue;

        _achievementListByReferencedId[achievement->SharesCriteria].push_back(achievement);
        ++count;
    }

    // Once Bitten, Twice Shy (10 player) - Icecrown Citadel
    if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(4539))
        const_cast<AchievementEntry*>(achievement)->MapID = 631;    // Correct map requirement (currently has Ulduar); 6.0.3 note - it STILL has ulduar requirement

    LOG_INFO("server.loading", ">> Loaded {} achievement references in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT achievement FROM character_achievement GROUP BY achievement");

    // Populate _allCompletedAchievements with all realm first achievement ids to make multithreaded access safer
    // while it will not prevent races, it will prevent crashes that happen because std::unordered_map key was added
    // instead the only potential race will happen on value associated with the key
    for (uint32 i = 0; i < sAchievementStore.GetNumRows(); ++i)
        if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(i))
            if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
                _allCompletedAchievements[achievement->ID] = SystemTimePoint::min();

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 completed achievements. DB table `character_achievement` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint16 achievementId = fields[0].Get<uint16>();
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementId);
        if (!achievement)
        {
            // Remove non existent achievements from all characters
            LOG_ERROR("achievement", "Non-existing achievement {} data removed from table `character_achievement`.", achievementId);

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEVMENT);

            stmt->SetData(0, uint16(achievementId));
            CharacterDatabase.Execute(stmt);

            continue;
        }
        else if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
            _allCompletedAchievements[achievementId] =  SystemTimePoint::max();
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} completed achievements in {} ms", (unsigned long)_allCompletedAchievements.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::LoadRewards()
{
    uint32 oldMSTime = getMSTime();

    _achievementRewards.clear();                           // need for reload case

    //                                               0      1        2        3     4       5        6     7
    QueryResult result = WorldDatabase.Query("SELECT ID, TitleA, TitleH, ItemID, Sender, Subject, Body, MailTemplateID FROM achievement_reward");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 achievement rewards. DB table `achievement_reward` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].Get<uint32>();
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entry);
        if (!achievement)
        {
            LOG_ERROR("sql.sql", "Table `achievement_reward` has wrong achievement (Entry: {}). Ignoring.", entry);
            continue;
        }

        AchievementReward reward;
        reward.TitleId[0]   = fields[1].Get<uint32>(); // Alliance title
        reward.TitleId[1]   = fields[2].Get<uint32>(); // Horde title
        reward.ItemId       = fields[3].Get<uint32>();
        reward.SenderCreatureId       = fields[4].Get<uint32>(); // The sender of the mail (a creature from creature_template)
        reward.Subject      = fields[5].Get<std::string>();
        reward.Body         = fields[6].Get<std::string>(); // Body in DB
        reward.MailTemplateId = fields[7].Get<uint32>();

        // Must reward a title or send a mail else, skip it.
        if (!reward.TitleId[0] && !reward.TitleId[1] && !reward.SenderCreatureId)
        {
            LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) does not have any title or item reward data. Ignoring.", entry);
            continue;
        }

        if (achievement->Faction == ACHIEVEMENT_FACTION_ANY && (!reward.TitleId[0] ^ !reward.TitleId[1]))
            LOG_DEBUG("achievement", "Table `achievement_reward` (Entry: {}) has title (A: {} H: {}) set for only one team.", entry, reward.TitleId[0], reward.TitleId[1]);

        if (reward.TitleId[0])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.TitleId[0]);
            if (!titleEntry)
            {
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has invalid title id ({}) in `title_A`. Setting it to 0.", entry, reward.TitleId[0]);
                reward.TitleId[0] = 0;
            }
        }

        if (reward.TitleId[1])
        {
            CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(reward.TitleId[1]);
            if (!titleEntry)
            {
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has invalid title id ({}) in `title_H`. Setting it to 0.", entry, reward.TitleId[1]);
                reward.TitleId[1] = 0;
            }
        }

        // Check mail data before item for report including wrong item case
        if (reward.SenderCreatureId)
        {
            if (!sObjectMgr->GetCreatureTemplate(reward.SenderCreatureId))
            {
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has invalid creature_template entry {} as Sender. Will not send the mail reward.", entry, reward.SenderCreatureId);
                reward.SenderCreatureId = 0;
            }
        }
        else
        {
            if (reward.ItemId)
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has itemId reward set but does not have Sender data set. Item will not be sent.", entry);

            if (!reward.Subject.empty())
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has mail Subject but does not have Sender data set.", entry); // Maybe add "Mail will not be sent." ?

            if (!reward.Body.empty())
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has mail text (Body) set but does not have Sender data set.", entry); // Maybe add "Mail will not be sent." ?

            if (reward.MailTemplateId)
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has mailTemplate set does not have Sender data set.", entry); // Maybe add "Mail will not be sent." ?
        }

        if (reward.MailTemplateId)
        {
            if (!sMailTemplateStore.LookupEntry(reward.MailTemplateId))
            {
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has invalid mailTemplate ({}) (check the DBC).", entry, reward.MailTemplateId);
                reward.MailTemplateId = 0;
            }
            else if (!reward.Subject.empty() || !reward.Body.empty())
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has mailTemplate ({}) and mail Subject/Body. To use the column mailTemplate, Subject and Body must be empty.", entry, reward.MailTemplateId);
        }

        if (reward.ItemId)
        {
            if (!sObjectMgr->GetItemTemplate(reward.ItemId))
            {
                // Not sure it's an error, it's probably an outDebug instead, because we can simply send a mail with no reward, right?
                LOG_ERROR("sql.sql", "Table `achievement_reward` (Entry: {}) has invalid item_template id {}. Reward mail will not contain any item.", entry, reward.ItemId);
                reward.ItemId = 0;
            }
        }

        _achievementRewards[entry] = reward;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} achievement rewards in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::LoadRewardLocales()
{
    uint32 oldMSTime = getMSTime();

    _achievementRewardLocales.clear();                       // need for reload case

    //                                               0   1       2        3
    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, Subject, Text FROM achievement_reward_locale");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 achievement reward locale strings.  DB table `achievement_reward_locale` is empty");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();
        if (_achievementRewards.find(ID) == _achievementRewards.end())
        {
            LOG_ERROR("sql.sql", "Table `achievement_reward_locale` (Entry: {}) has locale strings for non-existing achievement reward.", ID);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        AchievementRewardLocale& data = _achievementRewardLocales[ID];
        ObjectMgr::AddLocaleString(fields[2].Get<std::string>(), locale, data.Subject);
        ObjectMgr::AddLocaleString(fields[3].Get<std::string>(), locale, data.Body);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Achievement Reward Locale Strings in {} ms", (unsigned long)_achievementRewardLocales.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::CompletedAchievementForOfflinePlayer(ObjectGuid::LowType playerLowGuid, AchievementEntry const* entry)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, playerLowGuid);
    stmt->SetData(1, ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_COMPLETE_ACHIEVEMENT);
    stmt->SetData(2, entry->ID);
    stmt->SetData(3, 0);
    stmt->SetData(4, 0);
    CharacterDatabase.Execute(stmt);
}

void AchievementGlobalMgr::UpdateAchievementCriteriaForOfflinePlayer(ObjectGuid::LowType playerLowGuid, CriteriaTypes type, uint32 miscValue1, uint32 miscValue2)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, playerLowGuid);
    stmt->SetData(1, ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_UPDATE_CRITERIA);
    stmt->SetData(2, uint32(type));
    stmt->SetData(3, miscValue1);
    stmt->SetData(4, miscValue2);
    CharacterDatabase.Execute(stmt);
}
