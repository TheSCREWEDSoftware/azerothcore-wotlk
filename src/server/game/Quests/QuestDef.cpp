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

#include "QuestDef.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestPackets.h"
#include "ScriptMgr.h"
#include "World.h"

Quest::Quest(Field* questRecord)
{
    ID = questRecord[0].Get<uint32>();
    Type = questRecord[1].Get<uint8>();
    Level = questRecord[2].Get<int32>();
    PackageID = questRecord[3].Get<uint32>();
    MinLevel = questRecord[4].Get<int32>();
    QuestSortID = questRecord[5].Get<int16>();
    QuestInfoID = questRecord[6].Get<uint16>();
    SuggestedPlayers = questRecord[7].Get<uint8>();
    NextQuestInChain = questRecord[8].Get<uint32>();
    RewardXPDifficulty = questRecord[9].Get<uint32>();
    RewardXPMultiplier = questRecord[10].Get<float>();
    RewardMoney = questRecord[11].Get<int32>();
    RewardMoneyDifficulty = questRecord[12].Get<uint32>();
    RewardMoneyMultiplier = questRecord[13].Get<float>();
    RewardBonusMoney = questRecord[14].Get<uint32>();
    RewardDisplaySpell = questRecord[15].Get<uint32>();
    RewardSpell = questRecord[16].Get<uint32>();
    RewardHonor = questRecord[17].Get<uint32>();
    RewardKillHonor = questRecord[18].Get<uint32>();
    SourceItemId = questRecord[19].Get<uint32>();
    Flags = questRecord[20].Get<uint32>();
    FlagsEx = questRecord[21].Get<uint32>();

    for (uint32 i = 0; i < QUEST_ITEM_DROP_COUNT; ++i)
    {
        RewardItemId[i] = questRecord[22 + i * 4].Get<uint32>();
        RewardItemCount[i] = questRecord[23 + i * 4].Get<uint32>();
        ItemDrop[i] = questRecord[24 + i * 4].Get<uint32>();
        ItemDropQuantity[i] = questRecord[25 + i * 4].Get<uint32>();

        if (RewardItemId[i])
            ++_rewItemsCount;
    }

    for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
    {
        RewardChoiceItemId[i] = questRecord[38 + i * 3].Get<uint32>();
        RewardChoiceItemCount[i] = questRecord[39 + i * 3].Get<uint32>();
        RewardChoiceItemDisplayId[i] = questRecord[40 + i * 3].Get<uint32>();

        if (RewardChoiceItemId[i])
            ++_rewChoiceItemsCount;
    }

    POIContinent = questRecord[56].Get<uint32>();
    POIx = questRecord[57].Get<float>();
    POIy = questRecord[58].Get<float>();
    POIPriority = questRecord[59].Get<uint32>();

    RewardTitleId = questRecord[60].Get<uint32>();
    RewardTalents = questRecord[61].Get<uint32>();
    RewardArenaPoints = questRecord[62].Get<int32>();
    RewardSkillId = questRecord[63].Get<uint32>();
    RewardSkillPoints = questRecord[64].Get<uint32>();

    QuestGiverPortrait = questRecord[65].Get<uint32>();
    QuestTurnInPortrait = questRecord[66].Get<uint32>();

    for (uint32 i = 0; i < QUEST_REWARD_REPUTATIONS_COUNT; ++i)
    {
        RewardFactionId[i] = questRecord[67 + i * 3].Get<uint32>();
        RewardFactionValue[i] = questRecord[68 + i * 3].Get<int32>();
        RewardFactionOverride[i] = questRecord[69 + i * 3].Get<int32>();
    }

    RewardReputationMask = questRecord[82].Get<uint32>();

    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        RewardCurrencyId[i] = questRecord[83 + i * 2].Get<uint32>();
        RewardCurrencyCount[i] = questRecord[84 + i * 2].Get<uint32>();

        if (RewardCurrencyId[i])
            ++_rewCurrencyCount;
    }

    SoundAccept = questRecord[91].Get<uint32>();
    SoundTurnIn = questRecord[92].Get<uint32>();
    AreaGroupID = questRecord[93].Get<uint32>();
    LimitTime = questRecord[94].Get<uint32>();
    AllowableRaces = questRecord[95].Get<int32>();

    LogTitle = questRecord[96].Get<std::string>();
    LogDescription = questRecord[97].Get<std::string>();
    QuestDescription = questRecord[98].Get<std::string>();
    AreaDescription = questRecord[99].Get<std::string>();
    PortraitGiverText = questRecord[100].Get<std::string>();
    PortraitGiverName = questRecord[101].Get<std::string>();
    PortraitTurnInText = questRecord[102].Get<std::string>();
    PortraitTurnInName = questRecord[103].Get<std::string>();
    QuestCompletionLog = questRecord[104].Get<std::string>();
}

void Quest::LoadQuestDetails(Field* fields)
{
    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        DetailsEmote[i] = fields[1 + i].Get<uint16>();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        DetailsEmoteDelay[i] = fields[5 + i].Get<uint32>();
}

void Quest::LoadQuestRequestItems(Field* fields)
{
    EmoteOnComplete = fields[1].Get<uint16>();
    EmoteOnIncomplete = fields[2].Get<uint16>();
    EmoteOnCompleteDelay = fields[3].Get<uint32>();
    EmoteOnIncompleteDelay = fields[4].Get<uint32>();
    RequestItemsText = fields[5].Get<std::string>();
}

void Quest::LoadQuestOfferReward(Field* fields)
{
    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        OfferRewardEmote[i] = fields[1 + i].Get<uint16>();

    for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
        OfferRewardEmoteDelay[i] = fields[5 + i].Get<uint32>();

    OfferRewardText = fields[9].Get<std::string>();
}

void Quest::LoadQuestTemplateAddon(Field* fields)
{
    MaxLevel = fields[1].Get<uint8>();
    AllowableClasses = fields[2].Get<uint32>();
    SourceSpellID = fields[3].Get<uint32>();
    PrevQuestID = fields[4].Get<int32>();
    NextQuestID = fields[5].Get<int32>();
    ExclusiveGroup = fields[6].Get<int32>();
    BreadcrumbForQuestId = fields[7].Get<uint32>();
    RewardMailTemplateId = fields[8].Get<uint32>();
    RewardMailDelay = fields[9].Get<uint32>();
    RequiredSkillId = fields[10].Get<uint16>();
    RequiredSkillPoints = fields[11].Get<uint16>();
    RequiredMinRepFaction = fields[12].Get<uint16>();
    RequiredMaxRepFaction = fields[13].Get<uint16>();
    RequiredMinRepValue = fields[14].Get<int32>();
    RequiredMaxRepValue = fields[15].Get<int32>();
    SourceItemIdCount = fields[16].Get<uint8>();
    RewardMailSenderEntry = fields[17].Get<uint32>();
    SpecialFlags = fields[18].Get<uint32>();

    if (SpecialFlags & QUEST_SPECIAL_FLAGS_AUTO_ACCEPT)
        Flags |= QUEST_FLAGS_AUTO_ACCEPT;
}

void Quest::LoadQuestObjective(Field* fields)
{
    QuestObjective obj;
    obj.ID = fields[0].Get<uint32>();
    obj.QuestID = fields[1].Get<uint32>();
    obj.Type = fields[2].Get<uint8>();
    obj.StorageIndex = fields[3].Get<int8>();
    obj.ObjectID = fields[4].Get<int32>();
    obj.Amount = fields[5].Get<int32>();
    obj.Flags = fields[6].Get<uint32>();
    obj.UnkFloat = fields[7].Get<float>();
    obj.Description = fields[8].Get<std::string>();

    Objectives.push_back(obj);
}

void Quest::LoadQuestObjectiveVisualEffect(Field* fields)
{
    uint32 objID = fields[1].Get<uint32>();

    for (QuestObjective& obj : Objectives)
    {
        if (obj.ID == objID)
        {
            uint8 effectIndex = fields[3].Get<uint8>();
            if (effectIndex >= obj.VisualEffects.size())
                obj.VisualEffects.resize(effectIndex + 1, 0);

            obj.VisualEffects[effectIndex] = fields[4].Get<int32>();
            break;
        }
    }
}

QuestObjective const* Quest::GetObjective(uint32 objectiveId) const
{
    for (QuestObjective const& obj : Objectives)
        if (obj.ID == objectiveId)
            return &obj;

    return nullptr;
}

bool Quest::HasObjectiveType(QuestObjectiveType type) const
{
    for (QuestObjective const& obj : Objectives)
        if (obj.Type == type)
            return true;

    return false;
}

int32 Quest::GetRequiredMoney() const
{
    for (QuestObjective const& obj : Objectives)
        if (obj.Type == QUEST_OBJECTIVE_MONEY)
            return obj.Amount;

    return 0;
}

bool Quest::IsValidRewardChoice(uint32 reward) const
{
    if (reward >= QUEST_REWARD_CHOICES_COUNT)
        return false;

    if (RewardChoiceItemId[reward])
        return true;

    for (uint32 item : RewardChoiceItemId)
        if (item)
            return false;

    return reward == 0;
}

uint32 Quest::XPValue(uint32 playerLevel) const
{
    if (!playerLevel)
        return 0;

    int32 quest_level = (Level == -1 ? int32(playerLevel) : Level);
    QuestXPEntry const* xpentry = sQuestXPStore.LookupEntry(quest_level);
    if (!xpentry || RewardXPDifficulty >= 10)
        return 0;

    int32 diffFactor = 2 * (quest_level - int32(playerLevel)) + 20;
    if (diffFactor < 1)
        diffFactor = 1;
    else if (diffFactor > 10)
        diffFactor = 10;

    uint32 xp = uint32(diffFactor * xpentry->Exp[RewardXPDifficulty] * RewardXPMultiplier / 10);
    if (xp <= 100)
        xp = 5 * ((xp + 2) / 5);
    else if (xp <= 500)
        xp = 10 * ((xp + 5) / 10);
    else if (xp <= 1000)
        xp = 25 * ((xp + 12) / 25);
    else
        xp = 50 * ((xp + 25) / 50);

    return xp;
}

uint32 Quest::MoneyValue(uint8 playerLevel) const
{
    uint8 level = Level == -1 ? playerLevel : uint8(Level);

    if (QuestMoneyRewardEntry const* money = sQuestMoneyRewardStore.LookupEntry(level))
        if (GetRewMoneyDifficulty() < 10)
            return uint32(money->Money[GetRewMoneyDifficulty()] * GetMoneyMultiplier());

    return 0;
}

void Quest::BuildQuestRewards(WorldPackets::Quest::QuestRewards& rewards, Player* player) const
{
    rewards.ChoiceItemCount         = GetRewChoiceItemsCount();
    rewards.ItemCount               = GetRewItemsCount();
    bool const moneyInsteadOfXP = sScriptMgr->OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(player);
    uint32 xp = moneyInsteadOfXP ? 0 : player->GetQuestXPReward(this);
    sScriptMgr->OnPlayerQuestComputeXP(player, this, xp);

    rewards.Money                   = player->GetQuestMoneyReward(this);
    if (moneyInsteadOfXP || player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        rewards.Money += GetRewMoneyMaxLevel();
    rewards.XP                      = xp;
    rewards.Title                   = GetRewTitle();
    rewards.Talents                 = GetBonusTalents();
    rewards.FactionFlags            = GetRewardReputationMask();
    rewards.SpellCompletionDisplayID = GetRewDisplaySpell();
    rewards.SpellCompletionID       = GetRewSpell();
    rewards.SkillLineID             = GetRewardSkillId();
    rewards.NumSkillUps             = GetRewardSkillPoints();

    for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
    {
        rewards.ChoiceItems[i].ItemID = RewardChoiceItemId[i];
        rewards.ChoiceItems[i].Quantity = RewardChoiceItemCount[i];
    }

    for (uint32 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
    {
        rewards.ItemID[i] = RewardItemId[i];
        rewards.ItemQty[i] = RewardItemCount[i];
    }

    for (uint32 i = 0; i < QUEST_REWARD_REPUTATIONS_COUNT; ++i)
    {
        rewards.FactionID[i] = RewardFactionId[i];
        rewards.FactionValue[i] = RewardFactionValue[i];
        rewards.FactionOverride[i] = RewardFactionOverride[i];
    }

    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        rewards.CurrencyID[i] = RewardCurrencyId[i];
        rewards.CurrencyQty[i] = RewardCurrencyCount[i];
    }
}

uint32 Quest::GetRewMoneyMaxLevel() const
{
    // If Quest has flag to not give money on max level, it's 0
    if (HasFlag(QUEST_FLAGS_NO_MONEY_FROM_XP))
        return 0;

    // Else, return the rewarded copper sum modified by the rate
    return uint32(RewardBonusMoney * sWorld->getRate(RATE_REWARD_BONUS_MONEY));
}

bool Quest::IsAutoAccept() const
{
    return !sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_AUTO_ACCEPT) && HasFlag(QUEST_FLAGS_AUTO_ACCEPT);
}

bool Quest::IsAutoComplete() const
{
    return !sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_AUTO_COMPLETE) && Type == QUEST_TYPE_AUTOCOMPLETE;
}

bool Quest::IsRaidQuest(Difficulty difficulty) const
{
    switch (QuestInfoID)
    {
        case QUEST_INFO_RAID:
            return true;
        case QUEST_INFO_RAID_10:
            return difficulty == DIFFICULTY_10_N || difficulty == DIFFICULTY_10_HC;
        case QUEST_INFO_RAID_25:
            return difficulty == DIFFICULTY_25_N || difficulty == DIFFICULTY_25_HC;
        default:
            break;
    }

    return false;
}

bool Quest::IsAllowedInRaid(Difficulty difficulty) const
{
    if (IsRaidQuest(difficulty))
        return true;

    return sWorld->getBoolConfig(CONFIG_QUEST_IGNORE_RAID);
}

uint32 Quest::CalculateHonorGain(uint8 /*level*/) const
{
    // TeamContributionPoints.dbc no longer exists; quest honor is given through RewardHonor currency
    return 0;
}
