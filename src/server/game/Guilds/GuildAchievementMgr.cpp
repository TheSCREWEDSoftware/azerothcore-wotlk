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

#include "GuildAchievementMgr.h"
#include "AchievementMgr.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "Group.h"
#include "GameTime.h"
#include "Guild.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void GuildAchievementMgr::LoadAchievementFromDB(Field* fields)
{
    //        1            2     3
    // achievement, date, guids
    uint32 achievementId = fields[1].Get<uint16>();
    AchievementEntry const* achievement = sAchievementMgr->GetAchievement(achievementId);
    if (!achievement || !(achievement->flags & ACHIEVEMENT_FLAG_GUILD))
        return;

    CompletedAchievement& completed = _completed[achievementId];
    completed.Date = time_t(fields[2].Get<uint32>());
    for (std::string_view guid : Acore::Tokenize(fields[3].Get<std::string_view>(), ' ', false))
        if (Optional<ObjectGuid::LowType> lowGuid = Acore::StringTo<ObjectGuid::LowType>(guid))
            completed.Guids.insert(ObjectGuid::Create<HighGuid::Player>(*lowGuid));

    _achievementPoints += achievement->points;
}

void GuildAchievementMgr::LoadCriteriaFromDB(Field* fields)
{
    //     1        2     3              4
    // criteria, counter, date, completedGuid
    uint32 criteriaId = fields[1].Get<uint16>();
    if (!sAchievementCriteriaStore.LookupEntry(criteriaId))
        return;

    CriteriaProgress& progress = _criteriaProgress[criteriaId];
    progress.Counter = fields[2].Get<uint64>();
    progress.Date = time_t(fields[3].Get<uint32>());
    if (ObjectGuid::LowType lowGuid = fields[4].Get<uint32>())
        progress.CompletedGuid = ObjectGuid::Create<HighGuid::Player>(lowGuid);
}

void GuildAchievementMgr::DeleteFromDB(CharacterDatabaseTransaction trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEVEMENTS);
    stmt->SetData(0, _owner->GetId());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_ACHIEVEMENT_CRITERIA);
    stmt->SetData(0, _owner->GetId());
    trans->Append(stmt);
}

void GuildAchievementMgr::UpdateCriteria(AchievementCriteriaTypes type, uint64 miscValue1 /*= 0*/, uint64 miscValue2 /*= 0*/, Unit const* /*unit = nullptr*/, Player* source /*= nullptr*/)
{
    AchievementCriteriaEntryList const* criteriaList = sAchievementMgr->GetAchievementCriteriaByType(type);
    if (!criteriaList)
        return;

    for (AchievementCriteriaEntry const* criteria : *criteriaList)
    {
        AchievementEntry const* achievement = sAchievementMgr->GetAchievement(criteria->referredAchievement);
        if (!achievement || !(achievement->flags & ACHIEVEMENT_FLAG_GUILD) || HasAchieved(achievement->ID))
            continue;

        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_ACHIEVEMENT_CRITERIA, criteria->ID, nullptr))
            continue;

        if (IsCompletedCriteria(criteria))
            continue;

        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_BANK_SLOTS:
                if (!miscValue1)
                    continue;
                SetCriteriaProgress(criteria, miscValue1, Progress::Highest, source);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_SPENT_GOLD_GUILD_REPAIRS:
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                if (!miscValue1)
                    continue;
                SetCriteriaProgress(criteria, miscValue1, Progress::Accumulate, source);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                // guild boss kills only count in a guild group
                if (!miscValue1 || criteria->kill_creature.creatureID != miscValue1 || !source || !source->GetGroup()
                    || !source->GetGroup()->IsGuildGroupFor(source))
                    continue;
                SetCriteriaProgress(criteria, miscValue2, Progress::Accumulate, source);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
                if (!miscValue1 || !miscValue2)
                    continue;
                SetCriteriaProgress(criteria, miscValue2, Progress::Accumulate, source);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_GUILD_CHALLENGE_TYPE:
                if (!miscValue1 || miscValue1 != criteria->complete_guild_challenge_type.flag)
                    continue;
                SetCriteriaProgress(criteria, 1, Progress::Accumulate, source);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
            {
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(uint32(miscValue1));
                if (!proto || proto->Class == ITEM_CLASS_QUEST)
                    continue;
                SetCriteriaProgress(criteria, 1, Progress::Accumulate, source);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_TABARD:
                SetCriteriaProgress(criteria, 1, Progress::Set, source);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_GUILD_CHALLENGE:
                SetCriteriaProgress(criteria, 1, Progress::Accumulate, source);
                break;
            default:
                continue;
        }

        if (IsCompletedAchievement(achievement))
            CompleteAchievement(achievement, source);
    }
}

void GuildAchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* criteria, uint64 changeValue, Progress progressType, Player* source)
{
    CriteriaProgress& progress = _criteriaProgress[criteria->ID];
    uint64 const oldCounter = progress.Counter;

    switch (progressType)
    {
        case Progress::Set:
            progress.Counter = changeValue;
            break;
        case Progress::Accumulate:
            progress.Counter = std::min<uint64>(progress.Counter + changeValue, std::numeric_limits<uint64>::max() - 1);
            break;
        case Progress::Highest:
            progress.Counter = std::max(progress.Counter, changeValue);
            break;
    }

    if (progress.Counter == oldCounter && progress.Date)
        return;

    progress.Date = GameTime::GetGameTime().count();
    if (source && !progress.CompletedGuid && IsCompletedCriteria(criteria))
        progress.CompletedGuid = source->GetGUID();

    SaveCriteria(criteria->ID, progress);
}

bool GuildAchievementMgr::IsCompletedCriteria(AchievementCriteriaEntry const* criteria) const
{
    auto itr = _criteriaProgress.find(criteria->ID);
    if (itr == _criteriaProgress.end())
        return false;

    switch (AchievementCriteriaTypes(criteria->requiredType))
    {
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_TABARD:
            return itr->second.Counter >= 1;
        default:
            return itr->second.Counter >= std::max<uint32>(criteria->raw.count, 1);
    }
}

bool GuildAchievementMgr::IsCompletedAchievement(AchievementEntry const* achievement) const
{
    AchievementCriteriaEntryList const* criteriaList = sAchievementMgr->GetAchievementCriteriaByAchievement(achievement->ID);
    if (!criteriaList || criteriaList->empty())
        return false;

    uint32 completedCount = 0;
    bool completedAll = true;
    for (AchievementCriteriaEntry const* criteria : *criteriaList)
    {
        if (IsCompletedCriteria(criteria))
            ++completedCount;
        else
            completedAll = false;

        // some achievements only need a number of their criteria
        if (achievement->count && completedCount >= achievement->count)
            return true;
    }

    return completedAll;
}

void GuildAchievementMgr::CompleteAchievement(AchievementEntry const* achievement, Player* source)
{
    if (HasAchieved(achievement->ID))
        return;

    CompletedAchievement& completed = _completed[achievement->ID];
    completed.Date = GameTime::GetGameTime().count();

    // the members that took part are the online members in the source's group, or the source alone
    if (source)
    {
        completed.Guids.insert(source->GetGUID());
        if (Group const* group = source->GetGroup())
            for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player const* member = itr->GetSource())
                    if (member->GetGuildId() == _owner->GetId())
                        completed.Guids.insert(member->GetGUID());
    }

    std::string guids;
    for (ObjectGuid const& guid : completed.Guids)
        guids += Acore::ToString(guid.GetCounter()) + ' ';

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT);
    stmt->SetData(0, _owner->GetId());
    stmt->SetData(1, uint16(achievement->ID));
    stmt->SetData(2, uint32(completed.Date));
    stmt->SetData(3, guids);
    CharacterDatabase.Execute(stmt);

    _achievementPoints += achievement->points;

    // SMSG_GUILD_ACHIEVEMENT_EARNED to every member
    ObjectGuid guildGuid = _owner->GetGUID();
    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_EARNED, 1 + 8 + 4 + 4);
    data.WriteBit(guildGuid[3]);
    data.WriteBit(guildGuid[1]);
    data.WriteBit(guildGuid[0]);
    data.WriteBit(guildGuid[7]);
    data.WriteBit(guildGuid[4]);
    data.WriteBit(guildGuid[6]);
    data.WriteBit(guildGuid[2]);
    data.WriteBit(guildGuid[5]);
    data.FlushBits();
    data.WriteByteSeq(guildGuid[2]);
    data.AppendPackedTime(completed.Date);
    data.WriteByteSeq(guildGuid[0]);
    data.WriteByteSeq(guildGuid[4]);
    data.WriteByteSeq(guildGuid[1]);
    data.WriteByteSeq(guildGuid[3]);
    data << uint32(achievement->ID);
    data.WriteByteSeq(guildGuid[7]);
    data.WriteByteSeq(guildGuid[5]);
    data.WriteByteSeq(guildGuid[6]);
    _owner->BroadcastPacket(&data);

    if (achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_NEWS)
        _owner->AddGuildNews(GUILD_NEWS_GUILD_ACHIEVEMENT, ObjectGuid::Empty, achievement->flags & ACHIEVEMENT_FLAG_SHOW_IN_GUILD_HEADER, achievement->ID);

    UpdateCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS, achievement->points, 0, nullptr, source);

    // achievements that count this one
    if (AchievementEntryList const* refList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
        for (AchievementEntry const* refAchievement : *refList)
            if ((refAchievement->flags & ACHIEVEMENT_FLAG_GUILD) && IsCompletedAchievement(refAchievement))
                CompleteAchievement(refAchievement, source);
}

void GuildAchievementMgr::SaveCriteria(uint32 criteriaId, CriteriaProgress const& progress) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GUILD_ACHIEVEMENT_CRITERIA);
    stmt->SetData(0, _owner->GetId());
    stmt->SetData(1, uint16(criteriaId));
    stmt->SetData(2, progress.Counter);
    stmt->SetData(3, uint32(progress.Date));
    stmt->SetData(4, progress.CompletedGuid.GetCounter());
    CharacterDatabase.Execute(stmt);
}

void GuildAchievementMgr::SendAllData(Player* receiver) const
{
    uint32 count = 0;
    for (auto const& [achievementId, completed] : _completed)
        if (AchievementEntry const* achievement = sAchievementMgr->GetAchievement(achievementId))
            if (!(achievement->flags & ACHIEVEMENT_FLAG_HIDDEN))
                ++count;

    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DATA, 3 + count * (4 + 4));
    data.WriteBits(count, 23);
    data.FlushBits();
    for (auto const& [achievementId, completed] : _completed)
    {
        AchievementEntry const* achievement = sAchievementMgr->GetAchievement(achievementId);
        if (!achievement || (achievement->flags & ACHIEVEMENT_FLAG_HIDDEN))
            continue;

        data.AppendPackedTime(completed.Date);
        data << uint32(achievementId);
    }

    receiver->SendDirectMessage(&data);
}

void GuildAchievementMgr::SendCriteriaData(Player* receiver, uint32 achievementId) const
{
    std::vector<std::pair<uint32, CriteriaProgress const*>> criteria;
    if (AchievementCriteriaEntryList const* criteriaList = sAchievementMgr->GetAchievementCriteriaByAchievement(achievementId))
        for (AchievementCriteriaEntry const* entry : *criteriaList)
            if (auto itr = _criteriaProgress.find(entry->ID); itr != _criteriaProgress.end())
                criteria.emplace_back(entry->ID, &itr->second);

    WorldPacket data(SMSG_GUILD_CRITERIA_DATA, 3 + criteria.size() * (2 + 8 + 8 + 4 * 5));
    data.WriteBits(criteria.size(), 21);

    for (auto const& [criteriaId, progress] : criteria)
    {
        ObjectGuid counter(progress->Counter);              // to reach every byte of the counter
        ObjectGuid const& guid = progress->CompletedGuid;

        data.WriteBit(counter[4]);
        data.WriteBit(counter[1]);
        data.WriteBit(guid[2]);
        data.WriteBit(counter[3]);
        data.WriteBit(guid[1]);
        data.WriteBit(counter[5]);
        data.WriteBit(counter[0]);
        data.WriteBit(guid[3]);
        data.WriteBit(counter[2]);
        data.WriteBit(guid[7]);
        data.WriteBit(guid[5]);
        data.WriteBit(guid[0]);
        data.WriteBit(counter[6]);
        data.WriteBit(guid[6]);
        data.WriteBit(counter[7]);
        data.WriteBit(guid[4]);
    }

    data.FlushBits();

    for (auto const& [criteriaId, progress] : criteria)
    {
        ObjectGuid counter(progress->Counter);
        ObjectGuid const& guid = progress->CompletedGuid;

        data.WriteByteSeq(guid[5]);
        data << uint32(progress->Date);
        data.WriteByteSeq(counter[3]);
        data.WriteByteSeq(counter[7]);
        data << uint32(progress->Date);
        data.WriteByteSeq(counter[6]);
        data.WriteByteSeq(guid[4]);
        data.WriteByteSeq(guid[1]);
        data.WriteByteSeq(counter[4]);
        data.WriteByteSeq(guid[3]);
        data.WriteByteSeq(counter[0]);
        data.WriteByteSeq(guid[2]);
        data.WriteByteSeq(counter[1]);
        data.WriteByteSeq(guid[6]);
        data << uint32(progress->Date);                     // last update time, not packed
        data << uint32(criteriaId);
        data.WriteByteSeq(counter[5]);
        data << uint32(0);
        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(counter[2]);
        data.WriteByteSeq(guid[0]);
    }

    receiver->SendDirectMessage(&data);
}
