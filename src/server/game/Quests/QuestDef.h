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

#ifndef AZEROTHCORE_QUEST_H
#define AZEROTHCORE_QUEST_H

#include "Common.h"
#include "DBCEnums.h"
#include "DatabaseEnv.h"
#include "Define.h"
#include "SharedDefines.h"
#include <string>
#include <vector>

class Player;

class ObjectMgr;

namespace WorldPackets
{
    namespace Quest
    {
        struct QuestRewards;
    }
}

#define MAX_QUEST_LOG_SIZE 25

#define QUEST_ITEM_DROP_COUNT 4
#define QUEST_REWARD_CHOICES_COUNT 6
#define QUEST_REWARD_ITEM_COUNT 4
#define QUEST_DEPLINK_COUNT 10
#define QUEST_REWARD_REPUTATIONS_COUNT 5
#define QUEST_EMOTE_COUNT 4
#define QUEST_REWARD_CURRENCY_COUNT 4

// 6.x values (QuestFailedReason, TrinityCore 6.x QUEST_ERR_*)
enum QuestFailedReason : uint32
{
    INVALIDREASON_DONT_HAVE_REQ                 = 0,
    INVALIDREASON_QUEST_FAILED_LOW_LEVEL        = 1,        // You are not high enough level for that quest.
    INVALIDREASON_QUEST_FAILED_WRONG_RACE       = 6,        // That quest is not available to your race.
    INVALIDREASON_QUEST_ALREADY_DONE            = 7,        // You have completed that quest.
    INVALIDREASON_QUEST_ONLY_ONE_TIMED          = 12,       // You can only be on one timed quest at a time.
    INVALIDREASON_QUEST_ALREADY_ON              = 13,       // You are already on that quest.
    INVALIDREASON_QUEST_FAILED_EXPANSION        = 16,       // This quest requires an expansion enabled account.
    INVALIDREASON_QUEST_ALREADY_ON2             = 18,       // You are already on that quest.
    INVALIDREASON_QUEST_FAILED_MISSING_ITEMS    = 21,       // You don't have the required items with you. Check storage.
    INVALIDREASON_QUEST_FAILED_NOT_ENOUGH_MONEY = 23,       // You don't have enough money for that quest.
    INVALIDREASON_QUEST_FAILED_CAIS             = 24,       // You cannot complete quests once you have reached tired time.
    INVALIDREASON_DAILY_QUEST_COMPLETED_TODAY   = 26,       // You have completed that daily quest today.
    INVALIDREASON_DAILY_QUESTS_REMAINING        = 26,       // no daily quest cap since 4.x; same message as above
    INVALIDREASON_QUEST_FAILED_SPELL            = 28,       // You haven't learned the required spell.
    INVALIDREASON_QUEST_HAS_IN_PROGRESS         = 30        // Progress Bar objective not completed
};

// 6.x QuestPushReason values
enum QuestShareMessages : uint8
{
    QUEST_PARTY_MSG_SHARING_QUEST           = 0,    // "Sharing quest with %s..."
    QUEST_PARTY_MSG_CANT_TAKE_QUEST         = 1,    // "%s is not eligible for that quest"
    QUEST_PARTY_MSG_ACCEPT_QUEST            = 2,    // "%s has accepted your quest"
    QUEST_PARTY_MSG_DECLINE_QUEST           = 3,    // "%s has declined your quest"
    QUEST_PARTY_MSG_BUSY                    = 4,    // "%s is busy"
    QUEST_PARTY_MSG_DEAD                    = 5,    // "%s is dead."
    QUEST_PARTY_MSG_LOG_FULL                = 6,    // "%s's quest log is full"
    QUEST_PARTY_MSG_HAVE_QUEST              = 7,    // "%s is already on that quest"
    QUEST_PARTY_MSG_FINISH_QUEST            = 8,    // "%s has completed that quest"
    QUEST_PARTY_MSG_CANT_BE_SHARED_TODAY    = 9,    // "That quest cannot be shared today"
    QUEST_PARTY_MSG_SHARING_TIMER_EXPIRED   = 10,   // "Quest sharing timer has expired"
    QUEST_PARTY_MSG_NOT_IN_PARTY            = 11,   // "You are not in a party"
    QUEST_PARTY_MSG_DIFFERENT_SERVER_DAILY  = 12,   // "%s is not eligible for that quest today"
    QUEST_PARTY_MSG_NOT_ALLOWED             = 13    // "That quest cannot be shared"
};

enum QuestTradeSkill
{
    QUEST_TRSKILL_NONE           = 0,
    QUEST_TRSKILL_ALCHEMY        = 1,
    QUEST_TRSKILL_BLACKSMITHING  = 2,
    QUEST_TRSKILL_COOKING        = 3,
    QUEST_TRSKILL_ENCHANTING     = 4,
    QUEST_TRSKILL_ENGINEERING    = 5,
    QUEST_TRSKILL_FIRSTAID       = 6,
    QUEST_TRSKILL_HERBALISM      = 7,
    QUEST_TRSKILL_LEATHERWORKING = 8,
    QUEST_TRSKILL_POISONS        = 9,
    QUEST_TRSKILL_TAILORING      = 10,
    QUEST_TRSKILL_MINING         = 11,
    QUEST_TRSKILL_FISHING        = 12,
    QUEST_TRSKILL_SKINNING       = 13,
    QUEST_TRSKILL_JEWELCRAFTING  = 14,
};

enum QuestStatus : uint8
{
    QUEST_STATUS_NONE           = 0,
    QUEST_STATUS_COMPLETE       = 1,
    //QUEST_STATUS_UNAVAILABLE    = 2,
    QUEST_STATUS_INCOMPLETE     = 3,
    //QUEST_STATUS_AVAILABLE      = 4,
    QUEST_STATUS_FAILED         = 5,
    QUEST_STATUS_REWARDED       = 6,        // Not used in DB
    MAX_QUEST_STATUS
};

enum QuestGiverStatus : uint32
{
    DIALOG_STATUS_NONE                     = 0x000,
    DIALOG_STATUS_UNAVAILABLE              = 0x002,
    DIALOG_STATUS_LOW_LEVEL_AVAILABLE      = 0x004,
    DIALOG_STATUS_LOW_LEVEL_REWARD_REP     = 0x008,
    DIALOG_STATUS_LOW_LEVEL_AVAILABLE_REP  = 0x010,
    DIALOG_STATUS_INCOMPLETE               = 0x020,
    DIALOG_STATUS_REWARD_REP               = 0x040,
    DIALOG_STATUS_AVAILABLE_REP            = 0x080,
    DIALOG_STATUS_AVAILABLE                = 0x100,
    DIALOG_STATUS_REWARD2                  = 0x200,             // no yellow dot on minimap
    DIALOG_STATUS_REWARD                   = 0x400,            // yellow dot on minimap

    // Custom value meaning that script call did not return any valid quest status
    DIALOG_STATUS_SCRIPTED_NO_STATUS       = 0x1000,
};

enum QuestFlags
{
    // Flags used at server and sent to client
    QUEST_FLAGS_NONE                    = 0x00000000,
    QUEST_FLAGS_STAY_ALIVE              = 0x00000001,   // Quest is failed when the player dies
    QUEST_FLAGS_PARTY_ACCEPT            = 0x00000002,   // Not used currently. If player in party, all players that can accept this quest will receive confirmation box to accept quest CMSG_QUEST_CONFIRM_ACCEPT/SMSG_QUEST_CONFIRM_ACCEPT
    QUEST_FLAGS_EXPLORATION             = 0x00000004,   // Not used currently
    QUEST_FLAGS_SHARABLE                = 0x00000008,   // Can be shared: Player::CanShareQuest()
    QUEST_FLAGS_HAS_CONDITION           = 0x00000010,   // Not used currently
    QUEST_FLAGS_HIDE_REWARD_POI         = 0x00000020,   // Not used currently: Unsure of content
    QUEST_FLAGS_RAID                    = 0x00000040,   // Not used currently
    QUEST_FLAGS_TBC                     = 0x00000080,   // Not used currently: Available if TBC expansion enabled only
    QUEST_FLAGS_NO_MONEY_FROM_XP        = 0x00000100,   // Not used currently: Experience is not converted to gold at max level
    QUEST_FLAGS_HIDDEN_REWARDS          = 0x00000200,   // Items and money rewarded only sent in SMSG_QUESTGIVER_OFFER_REWARD (not in SMSG_QUESTGIVER_QUEST_DETAILS or in client quest log(SMSG_QUEST_QUERY_RESPONSE))
    QUEST_FLAGS_TRACKING                = 0x00000400,   // These quests are automatically rewarded on quest complete and they will never appear in quest log client side.
    QUEST_FLAGS_DEPRECATE_REPUTATION    = 0x00000800,   // Not used currently
    QUEST_FLAGS_DAILY                   = 0x00001000,   // Used to know quest is Daily one
    QUEST_FLAGS_FLAGS_PVP               = 0x00002000,   // Having this quest in log forces PvP flag
    QUEST_FLAGS_UNAVAILABLE             = 0x00004000,   // Used on quests that are not generically available
    QUEST_FLAGS_WEEKLY                  = 0x00008000,
    QUEST_FLAGS_AUTOCOMPLETE            = 0x00010000,   // auto complete
    QUEST_FLAGS_DISPLAY_ITEM_IN_TRACKER = 0x00020000,   // Displays usable item in quest tracker
    QUEST_FLAGS_OBJ_TEXT                = 0x00040000,   // use Objective text as Complete text
    QUEST_FLAGS_AUTO_ACCEPT             = 0x00080000,   // The client recognizes this flag as auto-accept. However, NONE of the current quests (3.3.5a) have this flag. Maybe blizz used to use it, or will use it in the future.

    // ... 4.x added flags up to 0x80000000 - all unknown for now
};

// last checked in 19802
enum QuestFlagsEx
{
    QUEST_FLAGS_EX_NONE                                     = 0X000000,
    QUEST_FLAGS_EX_KEEP_ADDITIONAL_ITEMS                    = 0X000001,
    QUEST_FLAGS_EX_SUPPRESS_GOSSIP_COMPLETE                 = 0X000002,
    QUEST_FLAGS_EX_SUPPRESS_GOSSIP_ACCEPT                   = 0X000004,
    QUEST_FLAGS_EX_DISALLOW_PLAYER_AS_QUESTGIVER            = 0X000008,
    QUEST_FLAGS_EX_DISPLAY_CLASS_CHOICE_REWARDS             = 0X000010,
    QUEST_FLAGS_EX_DISPLAY_SPEC_CHOICE_REWARDS              = 0X000020,
    QUEST_FLAGS_EX_REMOVE_FROM_LOG_ON_PERIDOIC_RESET        = 0X000040,
    QUEST_FLAGS_EX_ACCOUNT_LEVEL_QUEST                      = 0X000080,
    QUEST_FLAGS_EX_LEGENDARY_QUEST                          = 0X000100,
    QUEST_FLAGS_EX_NO_GUILD_XP                              = 0X000200,
    QUEST_FLAGS_EX_RESET_CACHE_ON_ACCEPT                    = 0X000400,
    QUEST_FLAGS_EX_NO_ABANDON_ONCE_ANY_OBJECTIVE_COMPLETE   = 0X000800,
    QUEST_FLAGS_EX_RECAST_ACCEPT_SPELL_ON_LOGIN             = 0X001000,
    QUEST_FLAGS_EX_UPDATE_ZONE_AURAS                        = 0X002000,
    QUEST_FLAGS_EX_NO_CREDIT_FOR_PROXY                      = 0X004000,
    QUEST_FLAGS_EX_DISPLAY_AS_DAILY_QUEST                   = 0X008000,
    QUEST_FLAGS_EX_PART_OF_QUEST_LINE                       = 0X010000,
    QUEST_FLAGS_EX_QUEST_FOR_INTERNAL_BUILDS_ONLY           = 0X020000,
    QUEST_FLAGS_EX_SUPPRESS_SPELL_LEARN_TEXT_LINE           = 0X040000,
    QUEST_FLAGS_EX_DISPLAY_HEADER_AS_OBJECTIVE_FOR_TASKS    = 0X080000,
    QUEST_FLAGS_EX_GARRISON_NON_OWNERS_ALLOWED              = 0X100000
};

enum QuestSpecialFlags
{
    QUEST_SPECIAL_FLAGS_NONE                    = 0x000,
    // Trinity flags for set SpecialFlags in DB if required but used only at server
    QUEST_SPECIAL_FLAGS_REPEATABLE              = 0x0001,    // Set by 1 in SpecialFlags from DB
    QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT    = 0x0002,    // Set by 2 in SpecialFlags from DB (if required area explore, spell SPELL_EFFECT_QUEST_COMPLETE casting, table `FECT_QUEST_COMPLETE casting, table `*_script` command SCRIPT_COMMAND_QUEST_EXPLORED use, set from script)
    QUEST_SPECIAL_FLAGS_AUTO_ACCEPT             = 0x0004,    // Set by 4 in SpecialFlags in DB if the quest is to be auto-accepted.
    QUEST_SPECIAL_FLAGS_DF_QUEST                = 0x0008,    // Set by 8 in SpecialFlags in DB if the quest is used by Dungeon Finder.
    QUEST_SPECIAL_FLAGS_MONTHLY                 = 0x0010,    // Set by 16 in SpecialFlags in DB if the quest is reset at the begining of the month
    QUEST_SPECIAL_FLAGS_CAST                    = 0x0020,    // Set by 32 in SpecialFlags in DB if the quest requires RequiredOrNpcGo killcredit but NOT kill (a spell cast)
    QUEST_SPECIAL_FLAGS_NO_REP_SPILLOVER        = 0x0040,    // Set by 64 in SpecialFlags in DB if the quest does not share rewarded reputation with other allied factions
    QUEST_SPECIAL_FLAGS_CAN_FAIL_IN_ANY_STATE   = 0x0080,    // Set by 128 in SpecialFlags in DB if the quest is allowed to fail in Player::FailQuest() independant of its current state
    QUEST_SPECIAL_FLAGS_NO_LOREMASTER_COUNT     = 0x0100,    // Set by 256 in SpecialFlags in DB if the quest should not count towards Loremaster Achievement
    // room for more custom flags

    QUEST_SPECIAL_FLAGS_DB_ALLOWED              = QUEST_SPECIAL_FLAGS_REPEATABLE | QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT | QUEST_SPECIAL_FLAGS_AUTO_ACCEPT |
                                                  QUEST_SPECIAL_FLAGS_DF_QUEST | QUEST_SPECIAL_FLAGS_MONTHLY | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_NO_REP_SPILLOVER |
                                                  QUEST_SPECIAL_FLAGS_CAN_FAIL_IN_ANY_STATE | QUEST_SPECIAL_FLAGS_NO_LOREMASTER_COUNT,

    QUEST_SPECIAL_FLAGS_DELIVER                 = 0x0200,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_SPEAKTO                 = 0x0400,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_KILL                    = 0x0800,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_TIMED                   = 0x1000,   // Internal flag computed only
    QUEST_SPECIAL_FLAGS_PLAYER_KILL             = 0x2000    // Internal flag computed only
};

enum QuestObjectiveType : uint8
{
    QUEST_OBJECTIVE_MONSTER                 = 0,
    QUEST_OBJECTIVE_ITEM                    = 1,
    QUEST_OBJECTIVE_GAMEOBJECT              = 2,
    QUEST_OBJECTIVE_TALKTO                  = 3,
    QUEST_OBJECTIVE_CURRENCY                = 4,
    QUEST_OBJECTIVE_LEARNSPELL              = 5,
    QUEST_OBJECTIVE_MIN_REPUTATION          = 6,
    QUEST_OBJECTIVE_MAX_REPUTATION          = 7,
    QUEST_OBJECTIVE_MONEY                   = 8,
    QUEST_OBJECTIVE_PLAYERKILLS             = 9,
    QUEST_OBJECTIVE_AREATRIGGER             = 10,
    QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC  = 11,
    QUEST_OBJECTIVE_DEFEATBATTLEPET         = 12,
    QUEST_OBJECTIVE_WINPVPPETBATTLES        = 13,
    QUEST_OBJECTIVE_CRITERIA_TREE           = 14,
    QUEST_OBJECTIVE_HAVE_CURRENCY           = 16,   // requires the player to have X currency when turning in but does not consume it
    QUEST_OBJECTIVE_OBTAIN_CURRENCY         = 17    // requires the player to gain X currency after starting the quest but not required to keep it until the end (does not consume)
};

struct QuestTemplateLocale
{
    std::vector<std::string> LogTitle;
    std::vector<std::string> LogDescription;
    std::vector<std::string> QuestDescription;
    std::vector<std::string> AreaDescription;
    std::vector<std::string> PortraitGiverText;
    std::vector<std::string> PortraitGiverName;
    std::vector<std::string> PortraitTurnInText;
    std::vector<std::string> PortraitTurnInName;
    std::vector<std::string> QuestCompletionLog;
};

struct QuestObjectivesLocale
{
    std::vector<std::string> Description;
};

struct QuestRequestItemsLocale
{
    std::vector<std::string> CompletionText;
};

struct QuestOfferRewardLocale
{
    std::vector<std::string> RewardText;
};

struct QuestObjective
{
    uint32 ID           = 0;
    uint32 QuestID      = 0;
    uint8  Type         = 0;
    int8   StorageIndex = 0;
    int32  ObjectID     = 0;
    int32  Amount       = 0;
    uint32 Flags        = 0;
    float  UnkFloat     = 0.0f;
    std::string Description;
    std::vector<int32> VisualEffects;

    // Objectives whose progress is counted in the quest log slot counters
    [[nodiscard]] bool IsStoringValue() const
    {
        switch (Type)
        {
            case QUEST_OBJECTIVE_MONSTER:
            case QUEST_OBJECTIVE_ITEM:
            case QUEST_OBJECTIVE_GAMEOBJECT:
            case QUEST_OBJECTIVE_TALKTO:
            case QUEST_OBJECTIVE_PLAYERKILLS:
            case QUEST_OBJECTIVE_AREATRIGGER:
            case QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC:
            case QUEST_OBJECTIVE_OBTAIN_CURRENCY:
                return true;
            default:
                break;
        }
        return false;
    }
};

typedef std::vector<QuestObjective> QuestObjectives;

// This Quest class provides a convenient way to access a few pretotaled (cached) quest details,
// all base quest information, and any utility functions such as generating the amount of
// xp to give
class Quest
{
    friend class ObjectMgr;
    friend class Player;
    friend class PlayerMenu;
public:
    // Loading data. All queries are in ObjectMgr::LoadQuests()
    Quest(Field* questRecord);
    void LoadQuestDetails(Field* fields);
    void LoadQuestRequestItems(Field* fields);
    void LoadQuestOfferReward(Field* fields);
    void LoadQuestTemplateAddon(Field* fields);
    void LoadQuestObjective(Field* fields);
    void LoadQuestObjectiveVisualEffect(Field* fields);

    [[nodiscard]] uint32 XPValue(uint32 playerLevel) const;
    [[nodiscard]] uint32 MoneyValue(uint8 playerLevel) const;

    [[nodiscard]] bool HasFlag(uint32 flag) const { return (Flags & flag) != 0; }
    void SetFlag(uint32 flag) { Flags |= flag; }

    [[nodiscard]] bool HasSpecialFlag(uint32 flag) const { return (SpecialFlags & flag) != 0; }
    void SetSpecialFlag(uint32 flag) { SpecialFlags |= flag; }

    // table data accessors:
    [[nodiscard]] uint32 GetQuestId() const { return ID; }
    [[nodiscard]] uint32 GetQuestType() const { return Type; }
    [[nodiscard]] uint32 GetQuestPackageID() const { return PackageID; }
    [[nodiscard]] int32  GetZoneOrSort() const { return QuestSortID; }
    [[nodiscard]] int32  GetMinLevel() const { return MinLevel; }
    [[nodiscard]] uint32 GetMaxLevel() const { return MaxLevel; }
    [[nodiscard]] int32  GetQuestLevel() const { return Level; }
    [[nodiscard]] uint32 GetQuestInfoID() const { return QuestInfoID; }
    [[nodiscard]] uint32 GetAllowableClasses() const { return AllowableClasses; }
    [[nodiscard]] int32  GetAllowableRaces() const { return AllowableRaces; }
    [[nodiscard]] uint32 GetRequiredSkill() const { return RequiredSkillId; }
    [[nodiscard]] uint32 GetRequiredSkillValue() const { return RequiredSkillPoints; }
    [[nodiscard]] uint32 GetRequiredMinRepFaction() const { return RequiredMinRepFaction; }
    [[nodiscard]] int32  GetRequiredMinRepValue() const { return RequiredMinRepValue; }
    [[nodiscard]] uint32 GetRequiredMaxRepFaction() const { return RequiredMaxRepFaction; }
    [[nodiscard]] int32  GetRequiredMaxRepValue() const { return RequiredMaxRepValue; }
    [[nodiscard]] uint32 GetSuggestedPlayers() const { return SuggestedPlayers; }
    [[nodiscard]] uint32 GetLimitTime() const { return LimitTime; }
    [[nodiscard]] int32  GetPrevQuestId() const { return PrevQuestID; }
    [[nodiscard]] int32  GetNextQuestId() const { return NextQuestID; }
    [[nodiscard]] int32  GetExclusiveGroup() const { return ExclusiveGroup; }
    [[nodiscard]] uint32 GetBreadcrumbForQuestId() const { return BreadcrumbForQuestId; }
    [[nodiscard]] uint32 GetNextQuestInChain() const { return NextQuestInChain; }
    [[nodiscard]] uint32 GetBonusTalents() const { return RewardTalents; }
    [[nodiscard]] int32  GetRewArenaPoints() const { return RewardArenaPoints; }
    [[nodiscard]] uint32 GetXPDifficulty() const { return RewardXPDifficulty; }
    [[nodiscard]] float  GetXPMultiplier() const { return RewardXPMultiplier; }
    [[nodiscard]] float  GetMoneyMultiplier() const { return RewardMoneyMultiplier; }
    [[nodiscard]] uint32 GetSrcItemId() const { return SourceItemId; }
    [[nodiscard]] uint32 GetSrcItemCount() const { return SourceItemIdCount; }
    [[nodiscard]] uint32 GetSrcSpell() const { return SourceSpellID; }
    [[nodiscard]] std::string const& GetLogTitle() const { return LogTitle; }
    [[nodiscard]] std::string const& GetLogDescription() const { return LogDescription; }
    [[nodiscard]] std::string const& GetQuestDescription() const { return QuestDescription; }
    [[nodiscard]] std::string const& GetAreaDescription() const { return AreaDescription; }
    [[nodiscard]] std::string const& GetOfferRewardText() const { return OfferRewardText; }
    [[nodiscard]] std::string const& GetRequestItemsText() const { return RequestItemsText; }
    [[nodiscard]] std::string const& GetQuestCompletionLog() const { return QuestCompletionLog; }
    [[nodiscard]] std::string const& GetPortraitGiverText() const { return PortraitGiverText; }
    [[nodiscard]] std::string const& GetPortraitGiverName() const { return PortraitGiverName; }
    [[nodiscard]] std::string const& GetPortraitTurnInText() const { return PortraitTurnInText; }
    [[nodiscard]] std::string const& GetPortraitTurnInName() const { return PortraitTurnInName; }
    [[nodiscard]] QuestObjectives const& GetObjectives() const { return Objectives; }
    [[nodiscard]] QuestObjective const* GetObjective(uint32 objectiveId) const;
    [[nodiscard]] uint32 GetRewMoneyDifficulty() const { return RewardMoneyDifficulty; }
    [[nodiscard]] uint32 GetRewHonor() const { return RewardHonor; }
    [[nodiscard]] uint32 GetRewKillHonor() const { return RewardKillHonor; }
    [[nodiscard]] uint32 GetRewMoneyMaxLevel() const; // use in XP calculation at client
    [[nodiscard]] uint32 GetRewSpell() const { return RewardSpell; }
    [[nodiscard]] uint32 GetRewDisplaySpell() const { return RewardDisplaySpell; }
    [[nodiscard]] uint32 GetRewMailTemplateId() const { return RewardMailTemplateId; }
    [[nodiscard]] uint32 GetRewMailDelaySecs() const { return RewardMailDelay; }
    [[nodiscard]] uint32 GetRewMailSenderEntry() const { return RewardMailSenderEntry; }
    [[nodiscard]] uint32 GetRewTitle() const { return RewardTitleId; }
    [[nodiscard]] uint32 GetPOIContinent() const { return POIContinent; }
    [[nodiscard]] float  GetPOIx() const { return POIx; }
    [[nodiscard]] float  GetPOIy() const { return POIy; }
    [[nodiscard]] uint32 GetPOIPriority() const { return POIPriority; }
    [[nodiscard]] uint32 GetSoundAccept() const { return SoundAccept; }
    [[nodiscard]] uint32 GetSoundTurnIn() const { return SoundTurnIn; }
    [[nodiscard]] uint32 GetIncompleteEmote() const { return EmoteOnIncomplete; }
    [[nodiscard]] uint32 GetCompleteEmote() const { return EmoteOnComplete; }
    [[nodiscard]] bool   IsRepeatable() const { return SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE; }
    [[nodiscard]] bool   IsAutoAccept() const;
    [[nodiscard]] bool   IsAutoComplete() const;
    [[nodiscard]] uint32 GetFlags() const { return Flags; }
    [[nodiscard]] uint32 GetFlagsEx() const { return FlagsEx; }
    [[nodiscard]] uint32 GetSpecialFlags() const { return SpecialFlags; }
    [[nodiscard]] uint32 GetAreaGroupID() const { return AreaGroupID; }
    [[nodiscard]] uint32 GetRewardSkillId() const { return RewardSkillId; }
    [[nodiscard]] uint32 GetRewardSkillPoints() const { return RewardSkillPoints; }
    [[nodiscard]] uint32 GetRewardReputationMask() const { return RewardReputationMask; }
    [[nodiscard]] uint32 GetQuestGiverPortrait() const { return QuestGiverPortrait; }
    [[nodiscard]] uint32 GetQuestTurnInPortrait() const { return QuestTurnInPortrait; }
    [[nodiscard]] bool   IsDaily() const { return (Flags & QUEST_FLAGS_DAILY) != 0; }
    [[nodiscard]] bool   IsWeekly() const { return (Flags & QUEST_FLAGS_WEEKLY) != 0; }
    [[nodiscard]] bool   IsMonthly() const { return (SpecialFlags & QUEST_SPECIAL_FLAGS_MONTHLY) != 0; }
    [[nodiscard]] bool   IsSeasonal() const { return (QuestSortID == -QUEST_SORT_SEASONAL || QuestSortID == -QUEST_SORT_SPECIAL || QuestSortID == -QUEST_SORT_LUNAR_FESTIVAL || QuestSortID == -QUEST_SORT_MIDSUMMER || QuestSortID == -QUEST_SORT_BREWFEST || QuestSortID == -QUEST_SORT_LOVE_IS_IN_THE_AIR || QuestSortID == -QUEST_SORT_NOBLEGARDEN) && !IsRepeatable(); }
    [[nodiscard]] bool   IsDailyOrWeekly() const { return (Flags & (QUEST_FLAGS_DAILY | QUEST_FLAGS_WEEKLY)) != 0; }
    [[nodiscard]] bool   IsRaidQuest(Difficulty difficulty) const;
    [[nodiscard]] bool   IsAllowedInRaid(Difficulty difficulty) const;
    [[nodiscard]] bool   IsDFQuest() const { return (SpecialFlags & QUEST_SPECIAL_FLAGS_DF_QUEST) != 0; }
    [[nodiscard]] bool   IsPVPQuest() const { return QuestInfoID == QUEST_INFO_PVP; }
    [[nodiscard]] uint32 CalculateHonorGain(uint8 level) const;

    // objective helpers
    [[nodiscard]] bool HasObjectiveType(QuestObjectiveType type) const;
    [[nodiscard]] int32 GetRequiredMoney() const;   // QUEST_OBJECTIVE_MONEY amount, 0 if none

    [[nodiscard]] uint32 GetRewChoiceItemsCount() const { return _rewChoiceItemsCount; }
    [[nodiscard]] uint32 GetRewItemsCount() const { return _rewItemsCount; }
    [[nodiscard]] uint32 GetRewCurrencyCount() const { return _rewCurrencyCount; }
    [[nodiscard]] bool IsValidRewardChoice(uint32 reward) const;

    void BuildQuestRewards(WorldPackets::Quest::QuestRewards& rewards, Player* player) const;

    typedef std::vector<int32> PrevQuests;
    PrevQuests prevQuests;
    typedef std::vector<uint32> PrevChainQuests;
    PrevChainQuests prevChainQuests;

    void SetEventIdForQuest(uint16 eventId) { _eventIdForQuest = eventId; }
    [[nodiscard]] uint16 GetEventIdForQuest() const { return _eventIdForQuest; }

private:
    uint32 _rewChoiceItemsCount = 0;
    uint32 _rewItemsCount = 0;
    uint32 _rewCurrencyCount = 0;
    uint16 _eventIdForQuest = 0; // pussywizard

public:
    // wdb data (quest query response)
    uint32 ID;
    uint32 Type;
    int32  Level;
    uint32 PackageID;
    int32  MinLevel;
    int32  QuestSortID;
    uint32 QuestInfoID;
    uint32 SuggestedPlayers;
    uint32 NextQuestInChain;
    uint32 RewardXPDifficulty;
    float  RewardXPMultiplier;
    int32  RewardMoney;
    uint32 RewardMoneyDifficulty;
    float  RewardMoneyMultiplier;
    uint32 RewardBonusMoney;
    uint32 RewardDisplaySpell;
    uint32 RewardSpell;
    uint32 RewardHonor;
    uint32 RewardKillHonor;
    uint32 SourceItemId;
    uint32 Flags;
    uint32 FlagsEx;
    uint32 RewardItemId[QUEST_REWARD_ITEM_COUNT];
    uint32 RewardItemCount[QUEST_REWARD_ITEM_COUNT];
    uint32 ItemDrop[QUEST_ITEM_DROP_COUNT];
    uint32 ItemDropQuantity[QUEST_ITEM_DROP_COUNT];
    uint32 RewardChoiceItemId[QUEST_REWARD_CHOICES_COUNT];
    uint32 RewardChoiceItemCount[QUEST_REWARD_CHOICES_COUNT];
    uint32 RewardChoiceItemDisplayId[QUEST_REWARD_CHOICES_COUNT];
    uint32 POIContinent;
    float  POIx;
    float  POIy;
    uint32 POIPriority;
    uint32 RewardTitleId;
    uint32 RewardTalents;
    int32  RewardArenaPoints;
    uint32 RewardSkillId;
    uint32 RewardSkillPoints;
    uint32 QuestGiverPortrait;
    uint32 QuestTurnInPortrait;
    uint32 RewardFactionId[QUEST_REWARD_REPUTATIONS_COUNT];
    int32  RewardFactionValue[QUEST_REWARD_REPUTATIONS_COUNT];
    int32  RewardFactionOverride[QUEST_REWARD_REPUTATIONS_COUNT];
    uint32 RewardReputationMask;
    uint32 RewardCurrencyId[QUEST_REWARD_CURRENCY_COUNT];
    uint32 RewardCurrencyCount[QUEST_REWARD_CURRENCY_COUNT];
    uint32 SoundAccept;
    uint32 SoundTurnIn;
    uint32 AreaGroupID;
    uint32 LimitTime;
    int32  AllowableRaces;
    QuestObjectives Objectives;
    std::string LogTitle;
    std::string LogDescription;
    std::string QuestDescription;
    std::string AreaDescription;
    std::string PortraitGiverText;
    std::string PortraitGiverName;
    std::string PortraitTurnInText;
    std::string PortraitTurnInName;
    std::string QuestCompletionLog;

protected:
    // quest_details table
    uint32 DetailsEmote[QUEST_EMOTE_COUNT] = {};
    uint32 DetailsEmoteDelay[QUEST_EMOTE_COUNT] = {};

    // quest_request_items table
    uint32 EmoteOnComplete          = 0;
    uint32 EmoteOnIncomplete        = 0;
    uint32 EmoteOnCompleteDelay     = 0;
    uint32 EmoteOnIncompleteDelay   = 0;
    std::string RequestItemsText;

    // quest_offer_reward table
    uint32 OfferRewardEmote[QUEST_EMOTE_COUNT] = {};
    uint32 OfferRewardEmoteDelay[QUEST_EMOTE_COUNT] = {};
    std::string OfferRewardText;

    // quest_template_addon table (custom data)
    uint32 MaxLevel              = 0;
    uint32 AllowableClasses      = 0;
    uint32 SourceSpellID         = 0;
    int32  PrevQuestID           = 0;
    int32  NextQuestID           = 0;
    int32  ExclusiveGroup        = 0;
    uint32 BreadcrumbForQuestId  = 0;
    uint32 RewardMailTemplateId  = 0;
    uint32 RewardMailDelay       = 0;
    uint32 RequiredSkillId       = 0;
    uint32 RequiredSkillPoints   = 0;
    uint32 RequiredMinRepFaction = 0;
    int32  RequiredMinRepValue   = 0;
    uint32 RequiredMaxRepFaction = 0;
    int32  RequiredMaxRepValue   = 0;
    uint32 SourceItemIdCount     = 0;
    uint32 RewardMailSenderEntry = 0;
    uint32 SpecialFlags          = 0; // custom flags, not sniffed/WDB
};

struct QuestStatusData
{
    QuestStatus Status{QUEST_STATUS_NONE};
    uint32 Timer{0};
    std::vector<int32> ObjectiveData;
    bool Explored{false};   // AzerothCore: QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT done
};

#endif
