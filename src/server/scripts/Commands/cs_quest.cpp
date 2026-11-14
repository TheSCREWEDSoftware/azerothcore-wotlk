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

#include "Chat.h"
#include "CommandScript.h"
#include "ConditionMgr.h"
#include "DisableMgr.h"
#include "GameTime.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ReputationMgr.h"

using namespace Acore::ChatCommands;

class quest_commandscript : public CommandScript
{
public:
    quest_commandscript() : CommandScript("quest_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable questCommandTable =
        {
            { "add",      HandleQuestAdd,      rbac::RBAC_PERM_COMMAND_QUEST_ADD,      Console::Yes },
            { "complete", HandleQuestComplete, rbac::RBAC_PERM_COMMAND_QUEST_COMPLETE, Console::Yes },
            { "remove",   HandleQuestRemove,   rbac::RBAC_PERM_COMMAND_QUEST_REMOVE,   Console::Yes },
            { "reward",   HandleQuestReward,   rbac::RBAC_PERM_COMMAND_QUEST_REWARD,   Console::Yes },
            { "status",   HandleQuestStatus,   rbac::RBAC_PERM_COMMAND_QUEST,          Console::Yes },
        };
        static ChatCommandTable commandTable =
        {
            { "quest", questCommandTable },
        };
        return commandTable;
    }

    static bool HandleQuestAdd(ChatHandler* handler, Quest const* quest, Optional<PlayerIdentifier> playerTarget)
    {
        if (!playerTarget)
        {
            playerTarget = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        uint32 entry = quest->GetQuestId();

        // check item starting quest (it can work incorrectly if added without item in inventory)
        ItemTemplateContainer const* itc = sObjectMgr->GetItemTemplateStore();
        ItemTemplateContainer::const_iterator result = find_if(itc->begin(), itc->end(), Finder<uint32, ItemTemplate>(entry, &ItemTemplate::StartQuest));

        if (result != itc->end())
        {
            handler->SendErrorMessage(LANG_COMMAND_QUEST_STARTFROMITEM, entry, result->second.ItemId);
            return false;
        }

        if (Player* player = playerTarget->GetConnectedPlayer())
        {
            if (player->IsActiveQuest(entry))
            {
                handler->SendErrorMessage(LANG_COMMAND_QUEST_ACTIVE, quest->GetLogTitle().c_str(), entry);
                return false;
            }

            // ok, normal (creature/GO starting) quest
            if (player->CanAddQuest(quest, true))
            {
                player->AddQuestAndCheckCompletion(quest, nullptr);
            }
        }
        else
        {
            ObjectGuid::LowType guid = playerTarget->GetGUID().GetCounter();
            QueryResult result = CharacterDatabase.Query("SELECT 1 FROM character_queststatus WHERE guid = {} AND quest = {}", guid, entry);

            if (result)
            {
                handler->SendErrorMessage(LANG_COMMAND_QUEST_ACTIVE, quest->GetLogTitle().c_str(), entry);
                return false;
            }

            uint8 index = 0;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_QUESTSTATUS);
            stmt->SetData(index++, guid);
            stmt->SetData(index++, entry);
            stmt->SetData(index++, 1);
            stmt->SetData(index++, false);
            stmt->SetData(index, 0);

            CharacterDatabase.Execute(stmt);
        }

        handler->PSendSysMessage(LANG_COMMAND_QUEST_ADD, quest->GetLogTitle(), entry);
        handler->SetSentErrorMessage(false);
        return true;
    }

    static bool HandleQuestRemove(ChatHandler* handler, Quest const* quest, Optional<PlayerIdentifier> playerTarget)
    {
        if (!playerTarget)
        {
            playerTarget = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        uint32 entry = quest->GetQuestId();

        if (!quest)
        {
            handler->SendErrorMessage(LANG_COMMAND_QUEST_NOTFOUND, entry);
            return false;
        }

        if (Player* player = playerTarget->GetConnectedPlayer())
        {
            // remove all quest entries for 'entry' from quest log
            for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                uint32 logQuest = player->GetQuestSlotQuestId(slot);
                if (logQuest == entry)
                {
                    player->SetQuestSlot(slot, 0);

                    // we ignore unequippable quest items in this case, its' still be equipped
                    player->TakeQuestSourceItem(logQuest, false);

                    if (quest->HasFlag(QUEST_FLAGS_FLAGS_PVP))
                    {
                        player->pvpInfo.IsHostile = player->pvpInfo.IsInHostileArea || player->HasPvPForcingQuest();
                        player->UpdatePvPState();
                    }
                }
            }

            player->RemoveRewardedQuest(entry);
            player->RemoveActiveQuest(entry, false);
        }
        else
        {
            ObjectGuid::LowType guid = playerTarget->GetGUID().GetCounter();
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_REWARDED_BY_QUEST);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_BY_QUEST);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_OBJECTIVES_BY_QUEST);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);
            trans->Append(stmt);

            for (QuestObjective const& obj : quest->GetObjectives())
            {
                if (obj.Type != QUEST_OBJECTIVE_ITEM)
                    continue;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_INVENTORY_ITEM_BY_ENTRY_AND_OWNER);
                stmt->SetData(0, uint32(obj.ObjectID));
                stmt->SetData(1, guid);

                PreparedQueryResult result = CharacterDatabase.Query(stmt);

                if (result)
                {
                    Field* fields = result->Fetch();

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INVENTORY_BY_ITEM);
                    stmt->SetData(0, fields[0].Get<uint32>());
                    trans->Append(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                    stmt->SetData(0, fields[0].Get<uint32>());
                    trans->Append(stmt);
                }
            }

            CharacterDatabase.CommitTransaction(trans);
        }

        handler->PSendSysMessage(LANG_COMMAND_QUEST_REMOVED, quest->GetLogTitle(), entry);
        handler->SetSentErrorMessage(false);
        return true;
    }

    static bool HandleQuestComplete(ChatHandler* handler, Quest const* quest, Optional<PlayerIdentifier> playerTarget)
    {
        if (!playerTarget)
        {
            playerTarget = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        uint32 entry = quest->GetQuestId();

        if (Player* player = playerTarget->GetConnectedPlayer())
        {
            // If player doesn't have the quest
            if (player->GetQuestStatus(entry) == QUEST_STATUS_NONE)
            {
                handler->SendErrorMessage(LANG_COMMAND_QUEST_NOTFOUND, entry);
                return false;
            }

            for (QuestObjective const& obj : quest->GetObjectives())
            {
                switch (obj.Type)
                {
                    case QUEST_OBJECTIVE_ITEM:
                    {
                        // Add quest items for quests that require items
                        uint32 curItemCount = player->GetItemCount(obj.ObjectID, true);
                        if (curItemCount >= uint32(obj.Amount))
                            break;

                        ItemPosCountVec dest;
                        uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, obj.ObjectID, obj.Amount - curItemCount);
                        if (msg == EQUIP_ERR_OK)
                        {
                            Item* item = player->StoreNewItem(dest, obj.ObjectID, true);
                            player->SendNewItem(item, obj.Amount - curItemCount, true, false);
                        }
                        break;
                    }
                    case QUEST_OBJECTIVE_MONSTER:
                    {
                        // All creature slain (not required, but otherwise it will display "Creature slain 0/10")
                        if (CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(obj.ObjectID))
                            for (int32 z = 0; z < obj.Amount; ++z)
                                player->KilledMonster(creatureInfo, ObjectGuid::Empty);
                        break;
                    }
                    case QUEST_OBJECTIVE_GAMEOBJECT:
                    {
                        for (int32 z = 0; z < obj.Amount; ++z)
                            player->KillCreditGO(obj.ObjectID);
                        break;
                    }
                    case QUEST_OBJECTIVE_TALKTO:
                    {
                        for (int32 z = 0; z < obj.Amount; ++z)
                            player->TalkedToCreature(obj.ObjectID, ObjectGuid::Empty);
                        break;
                    }
                    case QUEST_OBJECTIVE_PLAYERKILLS:
                        player->KilledPlayerCreditForQuest(uint16(obj.Amount), quest);
                        break;
                    case QUEST_OBJECTIVE_MIN_REPUTATION:
                    {
                        uint32 curRep = player->GetReputationMgr().GetReputation(obj.ObjectID);
                        if (int32(curRep) < obj.Amount)
                            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(obj.ObjectID))
                                player->GetReputationMgr().SetReputation(factionEntry, static_cast<float>(obj.Amount));
                        break;
                    }
                    case QUEST_OBJECTIVE_MAX_REPUTATION:
                    {
                        int32 curRep = player->GetReputationMgr().GetReputation(obj.ObjectID);
                        if (curRep > obj.Amount)
                            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(obj.ObjectID))
                                player->GetReputationMgr().SetReputation(factionEntry, static_cast<float>(obj.Amount));
                        break;
                    }
                    case QUEST_OBJECTIVE_MONEY:
                        // If the quest requires money
                        player->ModifyMoney(obj.Amount);
                        break;
                    case QUEST_OBJECTIVE_AREATRIGGER:
                        if (obj.StorageIndex >= 0)
                            player->SetQuestObjectiveData(quest, obj.StorageIndex, 1);
                        break;
                    default:
                        break;
                }
            }

            player->CompleteQuest(entry);
        }
        else
        {
            ObjectGuid::LowType guid = playerTarget->GetGUID().GetCounter();
            QueryResult result = CharacterDatabase.Query("SELECT 1 FROM character_queststatus WHERE guid = {} AND quest = {}", guid, entry);

            if (!result)
            {
                handler->SendErrorMessage(LANG_COMMAND_QUEST_NOT_FOUND_IN_LOG, quest->GetLogTitle(), entry);
                return false;
            }

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

            typedef std::pair<uint32, uint32> items;
            std::vector<items> questItems;

            for (QuestObjective const& obj : quest->GetObjectives())
                if (obj.Type == QUEST_OBJECTIVE_ITEM && obj.Amount > 0)
                    questItems.emplace_back(uint32(obj.ObjectID), uint32(obj.Amount));

            if (!questItems.empty())
            {
                MailSender sender(MAIL_NORMAL, guid, MAIL_STATIONERY_GM);
                // fill mail
                MailDraft draft(quest->GetLogTitle(), std::string());

                for (auto const& itr : questItems)
                {
                    if (Item* item = Item::CreateItem(itr.first, itr.second))
                    {
                        item->SaveToDB(trans);
                        draft.AddItem(item);
                    }
                }

                draft.SendMailTo(trans, MailReceiver(nullptr, guid), sender);
            }

            uint8 index = 0;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_QUESTSTATUS);
            stmt->SetData(index++, guid);
            stmt->SetData(index++, entry);
            stmt->SetData(index++, 1);
            stmt->SetData(index++, quest->HasFlag(QUEST_FLAGS_EXPLORATION));
            stmt->SetData(index, 0);

            trans->Append(stmt);

            // creature/gameobject/talk/player kill objectives are stored as done; items are counted once looted from the mail
            for (QuestObjective const& obj : quest->GetObjectives())
            {
                if (obj.StorageIndex < 0 || obj.Type == QUEST_OBJECTIVE_ITEM)
                    continue;

                int32 data = 0;
                switch (obj.Type)
                {
                    case QUEST_OBJECTIVE_MONSTER:
                    case QUEST_OBJECTIVE_GAMEOBJECT:
                    case QUEST_OBJECTIVE_TALKTO:
                    case QUEST_OBJECTIVE_PLAYERKILLS:
                    case QUEST_OBJECTIVE_OBTAIN_CURRENCY:
                        data = obj.Amount;
                        break;
                    case QUEST_OBJECTIVE_AREATRIGGER:
                        data = 1;
                        break;
                    default:
                        break;
                }

                if (!data)
                    continue;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CHAR_QUESTSTATUS_OBJECTIVES);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                stmt->SetData(2, uint8(obj.StorageIndex));
                stmt->SetData(3, data);
                trans->Append(stmt);
            }

            // If the quest requires reputation to complete, set the player rep to the required amount.
            for (QuestObjective const& obj : quest->GetObjectives())
            {
                if (obj.Type != QUEST_OBJECTIVE_MIN_REPUTATION || !sFactionStore.LookupEntry(obj.ObjectID))
                    continue;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_REP_BY_FACTION);
                stmt->SetData(0, uint32(obj.ObjectID));
                stmt->SetData(1, guid);
                PreparedQueryResult repResult = CharacterDatabase.Query(stmt);

                if (repResult)
                {
                    Field* fields = repResult->Fetch();
                    int32 curRep = fields[0].Get<int32>();

                    if (curRep < obj.Amount)
                    {
                        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_REP_FACTION_CHANGE);
                        stmt->SetData(0, uint32(obj.ObjectID));
                        stmt->SetData(1, obj.Amount);
                        stmt->SetData(2, uint32(obj.ObjectID));
                        stmt->SetData(3, guid);
                        trans->Append(stmt);
                    }
                }
            }

            CharacterDatabase.CommitTransaction(trans);
        }

        // check if Quest Tracker is enabled
        if (sWorld->getBoolConfig(CONFIG_QUEST_ENABLE_QUEST_TRACKER))
        {
            // prepare Quest Tracker datas
            auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_QUEST_TRACK_GM_COMPLETE);
            stmt->SetData(0, entry);
            stmt->SetData(1, playerTarget->GetGUID().GetCounter());

            // add to Quest Tracker
            CharacterDatabase.Execute(stmt);
        }

        handler->PSendSysMessage(LANG_COMMAND_QUEST_COMPLETE, quest->GetLogTitle(), entry);
        handler->SetSentErrorMessage(false);
        return true;
    }

    static bool HandleQuestReward(ChatHandler* handler, Quest const* quest, Optional<PlayerIdentifier> playerTarget)
    {
        if (!playerTarget)
        {
            playerTarget = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        uint32 entry = quest->GetQuestId();

        if (Player* player = playerTarget->GetConnectedPlayer())
        {
            // If player doesn't have the quest
            if (player->GetQuestStatus(entry) != QUEST_STATUS_COMPLETE)
            {
                handler->SendErrorMessage(LANG_COMMAND_QUEST_NOTFOUND, entry);
                return false;
            }

            player->RewardQuest(quest, 0, player);
        }
        else
        {
            // Achievement criteria updates correctly the next time a quest is rewarded.
            // Titles are already awarded correctly the next time they login (only one quest awards title - 11549).
            // Rewarded talent points (Death Knights) and spells (e.g Druid forms) are also granted on login.
            // No reputation gains - too troublesome to calculate them when the player is offline.

            ObjectGuid::LowType guid = playerTarget->GetGUID().GetCounter();
            uint8 charLevel = sCharacterCache->GetCharacterLevelByGuid(ObjectGuid::Create<HighGuid::Player>(guid));
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            CharacterDatabasePreparedStatement* stmt;

            QueryResult result = CharacterDatabase.Query("SELECT 1 FROM character_queststatus WHERE guid = {} AND quest = {} AND status = 1", guid, entry);

            if (!result)
            {
                handler->SendErrorMessage(LANG_COMMAND_QUEST_NOT_COMPLETE);
                return false;
            }

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_OBJECTIVES_BY_QUEST);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);
            trans->Append(stmt);

            for (QuestObjective const& obj : quest->GetObjectives())
            {
                if (obj.Type != QUEST_OBJECTIVE_ITEM)
                    continue;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_INVENTORY_ITEM_BY_ENTRY_AND_OWNER);
                stmt->SetData(0, uint32(obj.ObjectID));
                stmt->SetData(1, guid);

                PreparedQueryResult result = CharacterDatabase.Query(stmt);

                if (result)
                {
                    Field* fields = result->Fetch();

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INVENTORY_BY_ITEM);
                    stmt->SetData(0, fields[0].Get<uint32>());
                    trans->Append(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                    stmt->SetData(0, fields[0].Get<uint32>());
                    trans->Append(stmt);
                }
            }

            for (uint32 const& sourceItem : quest->ItemDrop)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_INVENTORY_ITEM_BY_ENTRY_AND_OWNER);
                stmt->SetData(0, sourceItem);
                stmt->SetData(1, guid);

                PreparedQueryResult result = CharacterDatabase.Query(stmt);

                if (result)
                {
                    Field* fields = result->Fetch();

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INVENTORY_BY_ITEM);
                    stmt->SetData(0, fields[0].Get<uint32>());
                    trans->Append(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                    stmt->SetData(0, fields[0].Get<uint32>());
                    trans->Append(stmt);
                }
            }

            typedef std::pair<uint32, uint32> items;
            std::vector<items> questRewardItems;

            // the first choice item, as an online reward without a choice would give
            if (quest->GetRewChoiceItemsCount())
            {
                for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
                {
                    if (quest->RewardChoiceItemId[i])
                    {
                        questRewardItems.emplace_back(quest->RewardChoiceItemId[i], quest->RewardChoiceItemCount[i]);
                        break;
                    }
                }
            }

            for (uint32 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
                if (quest->RewardItemId[i])
                    questRewardItems.emplace_back(quest->RewardItemId[i], quest->RewardItemCount[i]);

            if (!questRewardItems.empty())
            {
                MailSender sender(MAIL_NORMAL, guid, MAIL_STATIONERY_GM);
                // fill mail
                MailDraft draft(quest->GetLogTitle(), "This quest has been manually rewarded to you. This mail contains your quest rewards.");

                for (auto const& itr : questRewardItems)
                {
                    if (!itr.first || !itr.second)
                    {
                        continue;
                    }

                    // Skip invalid items.
                    if (!sObjectMgr->GetItemTemplate(itr.first))
                    {
                        continue;
                    }

                    if (Item* item = Item::CreateItem(itr.first, itr.second))
                    {
                        item->SaveToDB(trans);
                        draft.AddItem(item);
                    }
                }

                draft.SendMailTo(trans, MailReceiver(nullptr, guid), sender);
            }

            // Send quest giver mail, if any.
            if (uint32 mail_template_id = quest->GetRewMailTemplateId())
            {
                if (quest->GetRewMailSenderEntry() != 0)
                {
                    MailDraft(mail_template_id).SendMailTo(trans, MailReceiver(nullptr, guid), quest->GetRewMailSenderEntry(), MAIL_CHECK_MASK_HAS_BODY, quest->GetRewMailDelaySecs());
                }
            }

            if (quest->IsDaily() || quest->IsDFQuest())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_DAILYQUESTSTATUS);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                stmt->SetData(2, GameTime::GetGameTime().count());
                trans->Append(stmt);
            }
            else if (quest->IsWeekly())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_WEEKLYQUESTSTATUS);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                trans->Append(stmt);
            }
            else if (quest->IsMonthly())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_MONTHLYQUESTSTATUS);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                trans->Append(stmt);
            }
            else if (quest->IsSeasonal())
            {
                // We can't know which event is the quest linked to, so we can't do anything about this.
                /* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_SEASONALQUESTSTATUS);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                stmt->SetData(2, event_id);
                trans->Append(stmt);*/
            }

            if (uint32 honor = quest->CalculateHonorGain(charLevel))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UDP_CHAR_HONOR_POINTS_ACCUMULATIVE);
                stmt->SetData(0, honor);
                stmt->SetData(1, guid);
                trans->Append(stmt);
            }

            if (quest->GetRewArenaPoints())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UDP_CHAR_ARENA_POINTS_ACCUMULATIVE);
                stmt->SetData(0, quest->GetRewArenaPoints());
                stmt->SetData(1, guid);
                trans->Append(stmt);
            }

            int32 rewMoney = 0;

            if (charLevel >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                rewMoney = quest->GetRewMoneyMaxLevel();
            }
            else
            {
                // Some experience might get lost on level up.
                uint32 xp = uint32(quest->XPValue(charLevel) * sWorld->getRate(RATE_XP_QUEST));
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_XP_ACCUMULATIVE);
                stmt->SetData(0, xp);
                stmt->SetData(1, guid);
                trans->Append(stmt);
            }

            rewMoney += int32(quest->MoneyValue(charLevel) * sWorld->getRate(RATE_REWARD_QUEST_MONEY));

            // Only reward money, don't subtract, let's not cause an overflow...
            if (rewMoney > 0)
            {
                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UDP_CHAR_MONEY_ACCUMULATIVE);
                stmt->SetData(0, rewMoney);
                stmt->SetData(1, guid);
                trans->Append(stmt);
            }

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_QUESTSTATUS_REWARDED);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_BY_QUEST);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);
            trans->Append(stmt);

            CharacterDatabase.CommitTransaction(trans);
        }

        handler->PSendSysMessage(LANG_COMMAND_QUEST_REWARDED, quest->GetLogTitle(), entry);
        handler->SetSentErrorMessage(false);
        return true;
    }

    static char const* QuestStatusToString(QuestStatus status)
    {
        switch (status)
        {
            case QUEST_STATUS_NONE:
                return "Not Taken";
            case QUEST_STATUS_COMPLETE:
                return "Complete";
            case QUEST_STATUS_INCOMPLETE:
                return "Incomplete";
            case QUEST_STATUS_FAILED:
                return "Failed";
            case QUEST_STATUS_REWARDED:
                return "Rewarded";
            default:
                return "Unknown";
        }
    }

    static bool HandleQuestStatus(ChatHandler* handler, Quest const* quest, Optional<PlayerIdentifier> playerTarget)
    {
        if (!playerTarget)
            playerTarget = PlayerIdentifier::FromTargetOrSelf(handler);

        if (!playerTarget)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        uint32 entry = quest->GetQuestId();

        if (Player* player = playerTarget->GetConnectedPlayer())
        {
            handler->PSendSysMessage(LANG_CMD_QUEST_STATUS, quest->GetLogTitle(), entry, QuestStatusToString(player->GetQuestStatus(entry)));

            if (!player->CanTakeQuest(quest, false))
            {
                handler->PSendSysMessage(LANG_CMD_QUEST_UNAVAILABLE, entry);

                if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, entry, player))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_DISABLED);

                if (!player->SatisfyQuestStatus(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_ALREADY_DONE);

                if (!player->SatisfyQuestClass(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_CLASS);

                if (!player->SatisfyQuestRace(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_RACE);

                if (int32(player->GetLevel()) < quest->GetMinLevel())
                    handler->PSendSysMessage(LANG_CMD_QUEST_STATUS_LOW_LEVEL, quest->GetMinLevel());

                if (quest->GetMaxLevel() > 0 && player->GetLevel() > quest->GetMaxLevel())
                    handler->PSendSysMessage(LANG_CMD_QUEST_STATUS_HIGH_LEVEL, quest->GetMaxLevel());

                if (!player->SatisfyQuestSkill(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_SKILL);

                if (!player->SatisfyQuestReputation(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_REPUTATION);

                if (!player->SatisfyQuestPreviousQuest(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_PREV_QUEST);

                if (!player->SatisfyQuestTimed(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_TIMED);

                if (!player->SatisfyQuestExclusiveGroup(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_EXCLUSIVE);

                if (!player->SatisfyQuestNextChain(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_NEXT_CHAIN);

                if (!player->SatisfyQuestPrevChain(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_PREV_CHAIN);

                if (!player->SatisfyQuestBreadcrumb(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_BREADCRUMB);

                if (!player->SatisfyQuestDay(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_DAY);

                if (!player->SatisfyQuestWeek(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_WEEK);

                if (!player->SatisfyQuestMonth(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_MONTH);

                if (!player->SatisfyQuestSeasonal(quest, false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_SEASONAL);

                if (!player->SatisfyQuestConditions(quest, false))
                {
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_CONDITION);

                    ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_QUEST_AVAILABLE, entry);
                    ConditionSourceInfo srcInfo = ConditionSourceInfo(player);
                    for (Condition* cond : conditions)
                    {
                        if (!cond->Meets(srcInfo))
                            handler->PSendSysMessage(LANG_CMD_QUEST_STATUS_COND_DETAIL, uint32(cond->ConditionType), cond->ConditionValue1, cond->ConditionValue2, cond->ConditionValue3);
                    }
                }

                if (!player->SatisfyQuestLog(false))
                    handler->SendSysMessage(LANG_CMD_QUEST_STATUS_LOG_FULL);
            }
        }
        else
        {
            ObjectGuid::LowType guid = playerTarget->GetGUID().GetCounter();
            char const* status;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_QUESTSTATUS_BY_QUEST);
            stmt->SetData(0, guid);
            stmt->SetData(1, entry);

            if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
                status = QuestStatusToString(QuestStatus(result->Fetch()[0].Get<uint8>()));
            else if (quest->IsSeasonal())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_QUESTSTATUS_SEASONAL_BY_QUEST);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                stmt->SetData(2, quest->GetEventIdForQuest());
                status = CharacterDatabase.Query(stmt) ? "Rewarded" : "Not Taken";
            }
            // rewarded repeatable quests report Not Taken, matching Player::GetQuestStatus
            else if (!quest->IsRepeatable())
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_QUESTSTATUS_REWARDED_BY_QUEST);
                stmt->SetData(0, guid);
                stmt->SetData(1, entry);
                status = CharacterDatabase.Query(stmt) ? "Rewarded" : "Not Taken";
            }
            else
                status = "Not Taken";

            handler->PSendSysMessage(LANG_CMD_QUEST_STATUS, quest->GetLogTitle(), entry, status);
        }

        return true;
    }
};

void AddSC_quest_commandscript()
{
    new quest_commandscript();
}
