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

#include "GossipDef.h"
#include "Formulas.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestPackets.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
WorldPackets::Quest::QuestRewards BuildQuestRewards(Quest const* quest, Player* player, bool hidden)
{
    WorldPackets::Quest::QuestRewards rewards;
    if (!hidden)
    {
        rewards.ChoiceItemCount = quest->GetRewChoiceItemsCount();
        for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            rewards.ChoiceItemId[i] = quest->RewardChoiceItemId[i];
            rewards.ChoiceItemQuantity[i] = quest->RewardChoiceItemCount[i];
            if (ItemTemplate const* item = sObjectMgr->GetItemTemplate(rewards.ChoiceItemId[i]))
                rewards.ChoiceItemDisplay[i] = item->DisplayInfoID;
        }
        rewards.ItemCount = quest->GetRewItemsCount();
        for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        {
            rewards.ItemId[i] = quest->RewardItemId[i];
            rewards.ItemQuantity[i] = quest->RewardItemIdCount[i];
            if (ItemTemplate const* item = sObjectMgr->GetItemTemplate(rewards.ItemId[i]))
                rewards.ItemDisplay[i] = item->DisplayInfoID;
        }
        if (player && (player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) ||
            sScriptMgr->OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(player)))
            rewards.Money = quest->GetRewMoneyMaxLevel();
        rewards.Money += quest->GetRewOrReqMoney(player ? player->GetLevel() : 0);
        if (player && !sScriptMgr->OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(player))
            rewards.Experience = player->CalculateQuestRewardXP(quest);
        sScriptMgr->OnPlayerQuestComputeXP(player, quest, rewards.Experience);
    }
    rewards.Title = quest->GetCharTitleId();
    rewards.BonusTalents = quest->GetBonusTalents();
    rewards.FactionFlags = quest->RewardReputationMask;
    for (uint32 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        rewards.FactionId[i] = quest->RewardFactionId[i];
        rewards.FactionValue[i] = quest->RewardFactionValueId[i];
        rewards.FactionOverride[i] = quest->RewardFactionValueIdOverride[i];
    }
    rewards.DisplaySpell = quest->GetRewSpell();
    rewards.Spell = quest->GetRewSpellCast();
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        rewards.CurrencyId[i] = quest->RewardCurrencyId[i];
        rewards.CurrencyQuantity[i] = quest->RewardCurrencyCount[i];
    }
    return rewards;
}
}

GossipMenu::GossipMenu()
{
    _menuId = 0;
    _locale = DEFAULT_LOCALE;
    _senderGUID.Clear();
}

GossipMenu::~GossipMenu()
{
    ClearMenu();
}

void GossipMenu::AddMenuItem(int32 menuItemId, uint8 icon, std::string const& message, uint32 sender, uint32 action, std::string const& boxMessage, uint32 boxMoney, bool coded /*= false*/)
{
    ASSERT(_menuItems.size() <= GOSSIP_MAX_MENU_ITEMS);

    // Find a free new id - script case
    if (menuItemId == -1)
    {
        menuItemId = 0;
        if (!_menuItems.empty())
        {
            for (GossipMenuItemContainer::const_iterator itr = _menuItems.begin(); itr != _menuItems.end(); ++itr)
            {
                if (int32(itr->first) > menuItemId)
                    break;

                menuItemId = itr->first + 1;
            }
        }
    }

    GossipMenuItem& menuItem = _menuItems[menuItemId];

    menuItem.MenuItemIcon    = icon;
    menuItem.Message         = message;
    menuItem.IsCoded         = coded;
    menuItem.Sender          = sender;
    menuItem.OptionType      = action;
    menuItem.BoxMessage      = boxMessage;
    menuItem.BoxMoney        = boxMoney;
}

/**
 * @name AddMenuItem
 * @brief Adds a localized gossip menu item from db by menu id and menu item id.
 * @param menuId Gossip menu id.
 * @param menuItemId Gossip menu item id.
 * @param sender Identifier of the current menu.
 * @param action Custom action given to OnGossipHello.
 * @param boxMoney Custom price for pop-up box. If > 0, it will replace DB value.
 */
void GossipMenu::AddMenuItem(uint32 menuId, uint32 menuItemId, uint32 sender, uint32 action, uint32 boxMoney)
{
    /// Find items for given menu id.
    GossipMenuItemsMapBounds bounds = sObjectMgr->GetGossipMenuItemsMapBounds(menuId);
    /// Return if there are none.
    if (bounds.first == bounds.second)
        return;

    /// Iterate over each of them.
    for (GossipMenuItemsContainer::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        /// Find the one with the given menu item id.
        if (itr->second.OptionID != menuItemId)
            continue;

        /// Store texts for localization.
        std::string strOptionText, strBoxText;
        BroadcastText const* optionBroadcastText = sObjectMgr->GetBroadcastText(itr->second.OptionBroadcastTextID);
        BroadcastText const* boxBroadcastText = sObjectMgr->GetBroadcastText(itr->second.BoxBroadcastTextID);

        /// OptionText
        if (optionBroadcastText)
            ObjectMgr::GetLocaleString(optionBroadcastText->MaleText, GetLocale(), strOptionText);
        else
            strOptionText = itr->second.OptionText;

        /// BoxText
        if (boxBroadcastText)
            ObjectMgr::GetLocaleString(boxBroadcastText->MaleText, GetLocale(), strBoxText);
        else
            strBoxText = itr->second.BoxText;

        /// Check need of localization.
        if (GetLocale() != DEFAULT_LOCALE)
        {
            if (!optionBroadcastText)
            {
                /// Find localizations from database.
                if (GossipMenuItemsLocale const* gossipMenuLocale = sObjectMgr->GetGossipMenuItemsLocale(MAKE_PAIR32(menuId, menuItemId)))
                    ObjectMgr::GetLocaleString(gossipMenuLocale->OptionText, GetLocale(), strOptionText);
            }

            if (!boxBroadcastText)
            {
                /// Find localizations from database.
                if (GossipMenuItemsLocale const* gossipMenuLocale = sObjectMgr->GetGossipMenuItemsLocale(MAKE_PAIR32(menuId, menuItemId)))
                    ObjectMgr::GetLocaleString(gossipMenuLocale->BoxText, GetLocale(), strBoxText);
            }
        }

        /// Add menu item with existing method. Menu item id -1 is also used in ADD_GOSSIP_ITEM macro.
        AddMenuItem(-1, itr->second.OptionIcon, strOptionText, sender, action, strBoxText, boxMoney ? boxMoney : itr->second.BoxMoney, itr->second.BoxCoded);
    }
}

void GossipMenu::AddGossipMenuItemData(uint32 menuItemId, uint32 gossipActionMenuId, uint32 gossipActionPoi)
{
    GossipMenuItemData& itemData = _menuItemData[menuItemId];

    itemData.GossipActionMenuId  = gossipActionMenuId;
    itemData.GossipActionPoi     = gossipActionPoi;
}

uint32 GossipMenu::GetMenuItemSender(uint32 menuItemId) const
{
    GossipMenuItemContainer::const_iterator itr = _menuItems.find(menuItemId);
    if (itr == _menuItems.end())
        return 0;

    return itr->second.Sender;
}

uint32 GossipMenu::GetMenuItemAction(uint32 menuItemId) const
{
    GossipMenuItemContainer::const_iterator itr = _menuItems.find(menuItemId);
    if (itr == _menuItems.end())
        return 0;

    return itr->second.OptionType;
}

bool GossipMenu::IsMenuItemCoded(uint32 menuItemId) const
{
    GossipMenuItemContainer::const_iterator itr = _menuItems.find(menuItemId);
    if (itr == _menuItems.end())
        return false;

    return itr->second.IsCoded;
}

void GossipMenu::ClearMenu()
{
    _menuItems.clear();
    _menuItemData.clear();
}

PlayerMenu::PlayerMenu(WorldSession* session) : _session(session)
{
    _gossipMenu.SetLocale(session->GetSessionDbLocaleIndex());
}

PlayerMenu::~PlayerMenu()
{
    ClearMenus();
}

void PlayerMenu::ClearMenus()
{
    _gossipMenu.ClearMenu();
    _questMenu.ClearMenu();
}

// 5.4.8 18414, from Project SkyFire (PlayerMenu::SendGossipMenu)
void PlayerMenu::SendGossipMenu(uint32 titleTextId, ObjectGuid objectGUID)
{
    _gossipMenu.SetSenderGUID(objectGUID);

    LocaleConstant const locale = _session->GetSessionDbLocaleIndex();
    std::vector<std::pair<QuestMenuItem const*, std::string>> quests;
    for (uint32 i = 0; i < _questMenu.GetMenuItemCount(); ++i)
    {
        QuestMenuItem const& item = _questMenu.GetItem(i);
        Quest const* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;

        std::string title = quest->GetTitle();
        if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(item.QuestId))
            ObjectMgr::GetLocaleString(localeData->Title, locale, title);
        quests.emplace_back(&item, std::move(title));
    }

    ObjectGuid const guid = objectGUID;
    WorldPacket data(SMSG_GOSSIP_MESSAGE, 24 + _gossipMenu.GetMenuItemCount() * 100 + quests.size() * 75);
    data.WriteBits(quests.size(), 19);
    for (auto const& [item, title] : quests)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(item->QuestId);
        data.WriteBit(quest->IsRepeatable() && !quest->IsDailyOrWeekly() && !quest->IsMonthly());
        data.WriteBits(title.size(), 9);
    }

    for (uint8 index : { 5, 7, 4, 0 })
        data.WriteBit(guid[index]);
    data.WriteBits(_gossipMenu.GetMenuItemCount(), 20);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[2]);

    for (auto const& [id, item] : _gossipMenu.GetMenuItems())
    {
        data.WriteBits(item.BoxMessage.size(), 12);
        data.WriteBits(item.Message.size(), 12);
    }

    data.WriteBit(guid[3]);
    data.WriteBit(guid[1]);
    data.FlushBits();

    for (auto const& [item, title] : quests)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(item->QuestId);
        data.WriteString(title);
        data << int32(quest->GetFlags());
        data << int32(quest->GetQuestLevel());
        data << int32(item->QuestIcon);
        data << int32(item->QuestId);
        data << int32(0);                                   // quest flags 2
    }

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[0]);

    for (auto const& [id, item] : _gossipMenu.GetMenuItems())
    {
        data << int32(item.BoxMoney);                       // money required to open menu
        data.WriteString(item.BoxMessage);                  // accept text (related to money) pop up box
        data << int32(id);
        data << int8(item.IsCoded);                         // makes pop up box password
        data.WriteString(item.Message);                     // text for gossip item
        data << int8(item.MenuItemIcon);
    }

    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[3]);
    data << int32(_gossipMenu.GetMenuId());
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[4]);
    data << int32(0);                                       // friendship faction
    data.WriteByteSeq(guid[7]);
    data << int32(titleTextId);

    _session->SendPacket(&data);
}

void PlayerMenu::SendCloseGossip()
{
    _gossipMenu.SetSenderGUID(ObjectGuid::Empty);

    WorldPacket data(SMSG_GOSSIP_COMPLETE, 0);
    _session->SendPacket(&data);
}

void PlayerMenu::SendPointOfInterest(uint32 poiId) const
{
    PointOfInterest const* poi = sObjectMgr->GetPointOfInterest(poiId);
    if (!poi)
    {
        LOG_ERROR("sql.sql", "Request to send non-existing POI (Id: {}), ignored.", poiId);
        return;
    }

    std::string name = poi->Name;
    int32 locale = _session->GetSessionDbLocaleIndex();
    if (PointOfInterestLocale const* localeData = sObjectMgr->GetPointOfInterestLocale(poiId))
        ObjectMgr::GetLocaleString(localeData->Name, locale, name);

    WorldPacket data(SMSG_GOSSIP_POI, 4 + 4 + 4 + 4 + 4 + 20);  // guess size
    data << uint32(poi->Flags);
    data << float(poi->PositionX);
    data << float(poi->PositionY);
    data << uint32(poi->Icon);
    data << uint32(poi->Importance);
    data << name;

    _session->SendPacket(&data);
}

/*********************************************************/
/***                    QUEST SYSTEM                   ***/
/*********************************************************/

QuestMenu::QuestMenu()
{
    _questMenuItems.reserve(16);                                   // can be set for max from most often sizes to speedup push_back and less memory use
}

QuestMenu::~QuestMenu()
{
    ClearMenu();
}

void QuestMenu::AddMenuItem(uint32 QuestId, uint8 Icon)
{
    if (!sObjectMgr->GetQuestTemplate(QuestId))
        return;

    ASSERT(_questMenuItems.size() <= GOSSIP_MAX_MENU_ITEMS);

    QuestMenuItem questMenuItem;

    questMenuItem.QuestId        = QuestId;
    questMenuItem.QuestIcon      = Icon;

    _questMenuItems.push_back(questMenuItem);
}

bool QuestMenu::HasItem(uint32 questId) const
{
    for (QuestMenuItemList::const_iterator i = _questMenuItems.begin(); i != _questMenuItems.end(); ++i)
        if (i->QuestId == questId)
            return true;

    return false;
}

void QuestMenu::ClearMenu()
{
    _questMenuItems.clear();
}

// 5.4.8 18414, from Project SkyFire (PlayerMenu::SendQuestGiverQuestList)
void PlayerMenu::SendQuestGiverQuestList(QEmote const& eEmote, std::string const& Title, ObjectGuid guid)
{
    std::string greeting = Title;
    uint32 emoteDelay = eEmote._Delay;
    uint32 emoteType = eEmote._Emote;
    LocaleConstant const locale = _session->GetSessionDbLocaleIndex();

    if (QuestGreeting const* questGreeting = sObjectMgr->GetQuestGreeting(guid.GetTypeId(), guid.GetEntry()))
    {
        greeting.clear();
        // Check if greeting exists. Blizzlike that some creatures have empty greeting
        if (!questGreeting->Greeting.empty())
            greeting = questGreeting->Greeting.size() > size_t(locale) ? questGreeting->Greeting[locale] : questGreeting->Greeting[DEFAULT_LOCALE];
        emoteDelay = questGreeting->EmoteDelay;
        emoteType = questGreeting->EmoteType;
    }

    WorldPacket data(SMSG_QUESTGIVER_QUEST_LIST, 100);
    ByteBuffer questData;

    data << uint32(emoteType);                              // NPC emote
    data << uint32(emoteDelay);                             // player emote
    data.WriteBit(guid[2]);
    data.WriteBits(greeting.size(), 11);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[0]);

    uint32 count = 0;
    std::size_t const countPos = data.bitwpos();
    data.WriteBits(0, 19);

    for (uint32 i = 0; i < _questMenu.GetMenuItemCount(); ++i)
    {
        QuestMenuItem const& questMenuItem = _questMenu.GetItem(i);
        uint32 questID = questMenuItem.QuestId;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questID);
        if (!quest)
            continue;

        ++count;
        std::string title = quest->GetTitle();
        if (locale != LOCALE_enUS)
            if (QuestLocale const* questTemplateLocale = sObjectMgr->GetQuestLocale(questID))
                ObjectMgr::GetLocaleString(questTemplateLocale->Title, locale, title);

        data.WriteBit(quest->IsRepeatable() && !quest->IsDailyOrWeekly() && !quest->IsMonthly());
        data.WriteBits(title.size(), 9);

        questData << uint32(quest->GetFlags());
        questData << uint32(questID);
        questData.WriteString(title);
        questData << uint32(0);                             // quest flags 2
        questData << uint32(questMenuItem.QuestIcon);
        questData << int32(quest->GetQuestLevel());
    }

    for (uint8 index : { 1, 3, 4, 5, 7 })
        data.WriteBit(guid[index]);

    data.FlushBits();
    data.PutBits(countPos, count, 19);

    for (uint8 index : { 1, 0, 6, 7 })
        data.WriteByteSeq(guid[index]);
    data.append(questData);
    for (uint8 index : { 5, 3, 2 })
        data.WriteByteSeq(guid[index]);
    data.WriteString(greeting);
    data.WriteByteSeq(guid[4]);

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_QUEST_LIST (QuestGiver: {})", guid.ToString());
}

// 5.4.8 18414, from Project SkyFire (PlayerMenu::SendQuestGiverStatus)
void PlayerMenu::SendQuestGiverStatus(uint32 questStatus, ObjectGuid npcGUID) const
{
    WorldPacket data(SMSG_QUESTGIVER_STATUS, 1 + 8 + 4);
    for (uint8 index : { 1, 7, 4, 2, 5, 3, 6, 0 })
        data.WriteBit(npcGUID[index]);
    data.FlushBits();

    data.WriteByteSeq(npcGUID[7]);
    data << uint32(questStatus);
    for (uint8 index : { 4, 6, 1, 5, 2, 0, 3 })
        data.WriteByteSeq(npcGUID[index]);

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_STATUS NPC {}, status={}", npcGUID.ToString(), questStatus);
}

// 5.4.8 18414, from Project SkyFire (PlayerMenu::SendQuestGiverQuestDetails)
void PlayerMenu::SendQuestGiverQuestDetails(Quest const* quest, ObjectGuid npcGUID, bool activateAccept) const
{
    std::string questTitle      = quest->GetTitle();
    std::string questDetails    = quest->GetDetails();
    std::string questObjectives = quest->GetObjectives();
    std::string const questGiverTextWindow;
    std::string const questGiverTargetName;
    std::string const questTurnTextWindow;
    std::string const questTurnTargetName;

    LocaleConstant locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
    {
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);
        ObjectMgr::GetLocaleString(localeData->Details, locale, questDetails);
        ObjectMgr::GetLocaleString(localeData->Objectives, locale, questObjectives);
    }

    Player* player = _session->GetPlayer();
    WorldPackets::Quest::QuestRewards const rewards = BuildQuestRewards(quest, player, quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS));
    std::vector<Quest::ClientObjective> const objectives = quest->BuildClientObjectives(locale);

    ObjectGuid const dividerGuid = player->GetDivider();
    ObjectGuid const giverGuid = npcGUID;

    WorldPacket data(SMSG_QUESTGIVER_QUEST_DETAILS, 500);
    data << uint32(rewards.ItemQuantity[3]);
    data << uint32(rewards.ChoiceItemDisplay[4]);
    data << uint32(rewards.ChoiceItemId[2]);
    for (uint8 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        data << uint32(rewards.CurrencyQuantity[i]);
        data << uint32(rewards.CurrencyId[i]);
    }
    data << uint32(rewards.ChoiceItemCount);
    data << uint32(rewards.ChoiceItemQuantity[2]);
    data << uint32(rewards.ItemQuantity[1]);
    data << uint32(rewards.ChoiceItemDisplay[5]);
    data << uint32(rewards.ItemQuantity[0]);
    data << uint32(rewards.ItemDisplay[3]);
    data << uint32(rewards.ChoiceItemId[0]);
    data << uint32(rewards.ChoiceItemQuantity[3]);
    data << uint32(0);                                      // quest giver portrait
    data << uint32(rewards.ChoiceItemDisplay[3]);
    data << uint32(rewards.ItemId[0]);
    data << uint32(quest->GetQuestId());
    data << uint32(quest->GetSuggestedPlayers());
    data << uint32(rewards.ChoiceItemDisplay[0]);
    data << uint32(rewards.ChoiceItemQuantity[4]);
    data << uint32(rewards.ChoiceItemQuantity[5]);
    data << uint32(rewards.BonusTalents);
    data << uint32(rewards.ChoiceItemQuantity[1]);
    data << uint32(rewards.ChoiceItemDisplay[2]);
    for (uint8 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        data << int32(rewards.FactionValue[i]);
        data << uint32(rewards.FactionOverride[i]);
        data << uint32(rewards.FactionId[i]);
    }
    data << uint32(rewards.ItemId[3]);
    data << uint32(rewards.SkillLine);
    data << uint32(rewards.Experience);
    data << uint32(rewards.FactionFlags);
    data << uint32(rewards.ItemDisplay[2]);
    data << uint32(rewards.ItemId[1]);
    data << uint32(rewards.ChoiceItemId[1]);
    data << uint32(rewards.ChoiceItemId[5]);
    data << uint32(rewards.DisplaySpell);                   // spell completion id
    data << uint32(quest->GetFlags());
    data << uint32(rewards.Title);
    data << uint32(rewards.ItemId[2]);
    data << uint32(rewards.Money);
    data << uint32(rewards.ItemQuantity[2]);
    data << uint32(0);                                      // quest flags 2
    data << uint32(rewards.DisplaySpell);
    data << uint32(rewards.ChoiceItemId[3]);
    data << uint32(rewards.ItemCount);
    data << uint32(rewards.SkillUps);
    data << uint32(rewards.ItemDisplay[0]);
    data << uint32(rewards.ChoiceItemId[4]);
    data << uint32(0);                                      // reward package item
    data << uint32(rewards.ChoiceItemQuantity[0]);
    data << uint32(rewards.ItemDisplay[1]);
    data << uint32(rewards.ChoiceItemDisplay[1]);
    data << uint32(0);                                      // quest turn in portrait

    data.WriteBit(dividerGuid[7]);
    data.WriteBit(giverGuid[1]);
    data.WriteBits(questTurnTargetName.size(), 8);
    data.WriteBit(giverGuid[2]);
    data.WriteBits(questGiverTextWindow.size(), 10);
    data.WriteBit(false);                                   // display popup
    data.WriteBit(dividerGuid[2]);
    data.WriteBits(questTitle.size(), 9);
    data.WriteBits(QUEST_EMOTE_COUNT, 21);
    data.WriteBit(dividerGuid[0]);
    data.WriteBit(giverGuid[6]);
    data.WriteBit(giverGuid[5]);
    data.WriteBits(questGiverTargetName.size(), 8);
    data.WriteBit(giverGuid[3]);
    data.WriteBit(dividerGuid[1]);
    data.WriteBit(giverGuid[0]);
    data.WriteBit(false);                                   // start cheat
    data.WriteBit(giverGuid[4]);
    data.WriteBit(dividerGuid[3]);
    data.WriteBit(dividerGuid[5]);
    data.WriteBit(dividerGuid[4]);
    data.WriteBits(questTurnTextWindow.size(), 10);
    data.WriteBit(activateAccept);
    data.WriteBit(dividerGuid[6]);
    data.WriteBit(giverGuid[7]);
    data.WriteBits(questDetails.size(), 12);
    data.WriteBits(0, 22);                                  // learned spells
    data.WriteBits(objectives.size(), 20);
    data.WriteBits(questObjectives.size(), 12);
    data.FlushBits();

    data.WriteByteSeq(dividerGuid[0]);
    data.WriteString(questGiverTargetName);
    data.WriteString(questTurnTextWindow);
    data.WriteString(questTitle);
    data.WriteByteSeq(giverGuid[6]);
    data.WriteString(questObjectives);
    data.WriteByteSeq(dividerGuid[2]);
    data.WriteString(questGiverTextWindow);

    for (Quest::ClientObjective const& objective : objectives)
    {
        data << uint8(objective.Type);
        data << uint32(objective.Id);
        data << int32(objective.Amount);
        data << uint32(objective.ObjectId);
    }

    data.WriteString(questTurnTargetName);
    data.WriteString(questDetails);
    data.WriteByteSeq(dividerGuid[5]);
    data.WriteByteSeq(dividerGuid[7]);
    data.WriteByteSeq(giverGuid[7]);
    data.WriteByteSeq(giverGuid[3]);
    data.WriteByteSeq(giverGuid[0]);

    for (uint32 i = 0; i < QUEST_EMOTE_COUNT; ++i)
    {
        data << uint32(quest->DetailsEmoteDelay[i]);        // in ms
        data << uint32(quest->DetailsEmote[i]);
    }

    data.WriteByteSeq(dividerGuid[4]);
    data.WriteByteSeq(dividerGuid[3]);
    data.WriteByteSeq(giverGuid[5]);
    data.WriteByteSeq(giverGuid[1]);
    data.WriteByteSeq(giverGuid[2]);
    data.WriteByteSeq(dividerGuid[1]);
    data.WriteByteSeq(dividerGuid[6]);
    data.WriteByteSeq(giverGuid[4]);

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_QUEST_DETAILS {}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}

// 5.4.8 18414, from Project SkyFire (PlayerMenu::SendQuestQueryResponse)
void PlayerMenu::SendQuestQueryResponse(Quest const* quest) const
{
    std::string questTitle         = quest->GetTitle();
    std::string questDetails       = quest->GetDetails();
    std::string questObjectives    = quest->GetObjectives();
    std::string questEndText       = quest->GetAreaDescription();
    std::string questCompletedText = quest->GetCompletedText();
    std::string const questGiverTextWindow;
    std::string const questGiverTargetName;
    std::string const questTurnTextWindow;
    std::string const questTurnTargetName;

    LocaleConstant locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
    {
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);
        ObjectMgr::GetLocaleString(localeData->Details, locale, questDetails);
        ObjectMgr::GetLocaleString(localeData->Objectives, locale, questObjectives);
        ObjectMgr::GetLocaleString(localeData->AreaDescription, locale, questEndText);
        ObjectMgr::GetLocaleString(localeData->CompletedText, locale, questCompletedText);
    }

    std::vector<Quest::ClientObjective> const objectives = quest->BuildClientObjectives(locale);
    bool const hidden = quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS);
    auto rewardItem = [hidden](uint32 value) { return hidden ? 0 : value; };

    WorldPacket data(SMSG_QUEST_QUERY_RESPONSE, 500);
    data << uint32(quest->GetQuestId());
    data.WriteBit(true);                                    // has data
    data.WriteBits(questTurnTextWindow.size(), 10);
    data.WriteBits(questTitle.size(), 9);
    data.WriteBits(questCompletedText.size(), 11);
    data.WriteBits(questDetails.size(), 12);
    data.WriteBits(questTurnTargetName.size(), 8);
    data.WriteBits(questGiverTargetName.size(), 8);
    data.WriteBits(questGiverTextWindow.size(), 10);
    data.WriteBits(questEndText.size(), 9);
    data.WriteBits(objectives.size(), 19);
    data.WriteBits(questObjectives.size(), 12);

    ByteBuffer objectiveData;
    for (Quest::ClientObjective const& objective : objectives)
    {
        data.WriteBits(objective.Description.size(), 8);
        data.WriteBits(0, 22);                              // visual effects

        objectiveData << int32(objective.Amount);
        objectiveData << uint32(objective.Id);
        objectiveData.WriteString(objective.Description);
        objectiveData << uint32(objective.Flags);
        objectiveData << uint8(objective.Index);
        objectiveData << uint8(objective.Type);
        objectiveData << uint32(objective.ObjectId);
    }
    data.FlushBits();
    data.append(objectiveData);

    data << uint32(quest->ItemDrop[0]);
    data << uint32(rewardItem(quest->RewardChoiceItemId[4]));
    data << uint32(rewardItem(quest->RewardItemId[3]));
    data << uint32(rewardItem(quest->RewardItemIdCount[1]));
    data << uint32(rewardItem(quest->RewardChoiceItemCount[2]));
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        data << uint32(quest->RewardCurrencyId[i]);
        data << uint32(quest->RewardCurrencyCount[i]);
    }
    data << uint32(quest->GetBonusTalents());
    data << float(quest->GetPOIy());
    data << uint32(0);                                      // sound turn in
    for (uint32 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        data << int32(quest->RewardFactionValueId[i]);
        data << int32(quest->RewardFactionValueIdOverride[i]);
        data << uint32(quest->RewardFactionId[i]);
    }
    data << uint32(hidden ? 0 : std::max(0, quest->GetRewOrReqMoney()));
    data << uint32(rewardItem(quest->RewardChoiceItemCount[4]));
    data << uint32(rewardItem(quest->RewardChoiceItemCount[1]));
    data << uint32(0);                                      // quest flags 2
    data.WriteString(questEndText);
    data << uint32(rewardItem(quest->RewardChoiceItemId[1]));
    data << uint32(quest->GetRewMoneyMaxLevel());           // used in XP calculation at client
    data << uint32(rewardItem(quest->RewardItemId[0]));
    data.WriteString(questCompletedText);
    data << uint32(rewardItem(quest->RewardChoiceItemId[3]));
    data << uint32(quest->GetRewHonorAddition());
    data.WriteString(questGiverTextWindow);
    data.WriteString(questObjectives);
    data << uint32(0);                                      // reward skill points
    data << uint32(rewardItem(quest->RewardChoiceItemId[5]));
    data << uint32(quest->GetSuggestedPlayers());
    data << uint32(quest->GetQuestId());
    data << uint32(quest->ItemDrop[1]);
    data << uint32(rewardItem(quest->RewardItemId[1]));
    data << int32(quest->GetMinLevel());
    data << uint32(quest->RewardReputationMask);
    data << uint32(quest->GetPointOpt());
    data << int32(quest->GetQuestLevel());
    data << uint32(quest->GetQuestMethod());
    data << uint32(quest->ItemDropQuantity[2]);
    data << uint32(quest->GetXPId());
    data.WriteString(questDetails);
    data << uint32(rewardItem(quest->RewardItemIdCount[0]));
    data << uint32(rewardItem(quest->RewardChoiceItemCount[5]));
    data << uint32(rewardItem(quest->RewardItemIdCount[2]));
    data << int32(quest->GetRewSpellCast());
    data << uint32(0);
    data.WriteString(questTurnTargetName);
    data << uint32(0);
    data << uint32(quest->ItemDropQuantity[1]);
    data << uint32(quest->ItemDrop[2]);
    data << uint32(0);                                      // quest turn in portrait
    data.WriteString(questTitle);
    data << uint32(quest->GetType());
    data << uint32(quest->GetXPId());
    data << uint32(0);
    data << uint32(0);
    data << uint32(quest->GetPOIContinent());
    data << uint32(quest->GetNextQuestInChain());           // client will request this quest from NPC, if not 0
    data << uint32(rewardItem(quest->RewardChoiceItemId[0]));
    data.WriteString(questGiverTargetName);
    data << uint32(0);
    data << uint32(quest->ItemDrop[3]);
    data << float(quest->GetPOIx());
    data << uint32(rewardItem(quest->RewardChoiceItemId[2]));
    data << uint32(0);
    data << uint32(rewardItem(quest->RewardItemIdCount[3]));
    data << uint32(0);                                      // sound accept
    data << uint32(rewardItem(quest->RewardItemId[2]));
    data << float(quest->GetRewHonorMultiplier());
    data << uint32(quest->GetCharTitleId());
    data.WriteString(questTurnTextWindow);
    data << uint32(rewardItem(quest->RewardChoiceItemCount[3]));
    data << uint32(quest->ItemDropQuantity[0]);
    data << int32(quest->GetZoneOrSort());
    data << uint32(0);                                      // reward skill id
    data << uint32(rewardItem(quest->RewardChoiceItemCount[0]));
    data << uint32(quest->GetRewSpell());
    data << uint32(0);                                      // quest giver portrait
    data << uint32(0);
    data << uint32(quest->ItemDropQuantity[3]);
    data << uint32(quest->GetFlags());
    data << uint32(0);                                      // reward package item
    data << uint32(quest->GetSrcItemId());

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUEST_QUERY_RESPONSE questid={}", quest->GetQuestId());
}

// 5.4.8 18414, from Project SkyFire (PlayerMenu::SendQuestGiverOfferReward)
void PlayerMenu::SendQuestGiverOfferReward(Quest const* quest, ObjectGuid npcGUID, bool enableNext) const
{
    std::string questTitle = quest->GetTitle();
    std::string rewardText = quest->GetOfferRewardText();
    std::string const questGiverTextWindow;
    std::string const questGiverTargetName;
    std::string const questTurnTextWindow;
    std::string const questTurnTargetName;

    LocaleConstant locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);

    if (QuestOfferRewardLocale const* questOfferRewardLocale = sObjectMgr->GetQuestOfferRewardLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(questOfferRewardLocale->RewardText, locale, rewardText);

    uint32 questTakerEntry = 0;
    if (npcGUID)
        if (Object* object = ObjectAccessor::GetObjectByTypeMask(*_session->GetPlayer(), npcGUID, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM))
            if (object->hasInvolvedQuest(quest->GetQuestId()))
                questTakerEntry = object->GetEntry();

    uint32 emoteCount = 0;
    for (uint32 i = 0; i < QUEST_EMOTE_COUNT; ++i)
    {
        if (quest->OfferRewardEmote[i] <= 0)
            break;
        ++emoteCount;
    }

    WorldPackets::Quest::QuestRewards const rewards = BuildQuestRewards(quest, _session->GetPlayer(), false);
    ObjectGuid const giverGuid = npcGUID;

    WorldPacket data(SMSG_QUESTGIVER_OFFER_REWARD, 400);
    data << uint32(rewards.ItemQuantity[2]);
    data << uint32(quest->GetQuestId());
    data << uint32(rewards.ItemId[3]);
    data << uint32(rewards.ChoiceItemDisplay[2]);
    for (uint8 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        data << uint32(rewards.FactionId[i]);
        data << int32(rewards.FactionValue[i]);
        data << uint32(rewards.FactionOverride[i]);
    }
    data << uint32(rewards.ItemQuantity[0]);
    data << uint32(rewards.ItemQuantity[3]);
    data << uint32(rewards.ItemDisplay[3]);
    data << uint32(rewards.ItemId[1]);
    data << uint32(rewards.ChoiceItemId[3]);
    data << uint32(rewards.ChoiceItemDisplay[3]);
    data << uint32(rewards.ChoiceItemCount);
    data << uint32(rewards.Spell);
    data << uint32(rewards.ItemDisplay[1]);
    data << uint32(rewards.ChoiceItemQuantity[5]);
    data << uint32(rewards.ChoiceItemDisplay[4]);
    data << uint32(rewards.ChoiceItemQuantity[1]);
    data << uint32(rewards.ChoiceItemDisplay[0]);
    data << uint32(rewards.ItemDisplay[0]);
    data << uint32(0);                                      // reward package item
    data << uint32(0);                                      // quest turn in portrait
    data << uint32(rewards.ItemQuantity[1]);
    data << uint32(rewards.FactionFlags);
    data << uint32(rewards.ChoiceItemId[0]);
    data << uint32(rewards.ChoiceItemQuantity[3]);
    data << uint32(rewards.ChoiceItemQuantity[4]);
    data << uint32(rewards.ChoiceItemId[1]);
    data << uint32(rewards.BonusTalents);
    data << uint32(rewards.SkillLine);
    for (uint8 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        data << uint32(rewards.CurrencyId[i]);
        data << uint32(rewards.CurrencyQuantity[i]);
    }
    data << uint32(quest->GetFlags());
    data << uint32(0);                                      // quest flags 2
    data << uint32(rewards.Experience);
    data << uint32(rewards.Title);
    data << uint32(rewards.ChoiceItemId[2]);
    data << uint32(rewards.ItemCount);
    data << uint32(quest->GetSuggestedPlayers());
    data << uint32(rewards.ChoiceItemId[4]);
    data << uint32(questTakerEntry);                        // ender NPC or GO entry
    data << uint32(rewards.ItemId[2]);
    data << uint32(rewards.ChoiceItemQuantity[0]);
    data << uint32(rewards.ChoiceItemDisplay[5]);
    data << uint32(0);                                      // quest giver portrait
    data << uint32(rewards.Money);
    data << uint32(rewards.ChoiceItemId[5]);
    data << uint32(rewards.ChoiceItemDisplay[1]);
    data << uint32(rewards.ChoiceItemQuantity[2]);
    data << uint32(rewards.ItemDisplay[2]);
    data << uint32(rewards.DisplaySpell);                   // spell completion id
    data << uint32(rewards.ItemId[0]);
    data << uint32(rewards.SkillUps);

    data.WriteBits(questTurnTextWindow.size(), 10);
    data.WriteBits(questGiverTargetName.size(), 8);
    data.WriteBit(giverGuid[6]);
    data.WriteBits(emoteCount, 21);
    data.WriteBit(giverGuid[3]);
    data.WriteBit(giverGuid[7]);
    data.WriteBits(questTitle.size(), 9);
    data.WriteBit(giverGuid[4]);
    data.WriteBits(questTurnTargetName.size(), 8);
    data.WriteBits(questGiverTextWindow.size(), 10);
    data.WriteBits(rewardText.size(), 12);
    data.WriteBit(giverGuid[1]);
    data.WriteBit(giverGuid[2]);
    data.WriteBit(giverGuid[0]);
    data.WriteBit(giverGuid[5]);
    data.WriteBit(enableNext);
    data.FlushBits();

    data.WriteString(questGiverTargetName);
    data.WriteString(questTitle);
    for (uint32 i = 0; i < emoteCount; ++i)
    {
        data << uint32(quest->OfferRewardEmoteDelay[i]);
        data << uint32(quest->OfferRewardEmote[i]);
    }
    data.WriteByteSeq(giverGuid[2]);
    data.WriteString(rewardText);
    data.WriteString(questTurnTextWindow);
    data.WriteString(questTurnTargetName);
    data.WriteByteSeq(giverGuid[5]);
    data.WriteByteSeq(giverGuid[1]);
    data.WriteString(questGiverTextWindow);
    for (uint8 index : { 0, 7, 6, 4, 3 })
        data.WriteByteSeq(giverGuid[index]);

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_OFFER_REWARD {}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}

void PlayerMenu::SendQuestGiverRequestItems(Quest const* quest, ObjectGuid npcGUID, bool canComplete, bool closeOnCancel) const
{
    // We can always call to RequestItems, but this packet only goes out if there are actually
    // items.  Otherwise, we'll skip straight to the OfferReward

    std::string questTitle = quest->GetTitle();
    std::string requestItemsText = quest->GetRequestItemsText();

    int32 locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);

    if (QuestRequestItemsLocale const* questRequestItemsLocale = sObjectMgr->GetQuestRequestItemsLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(questRequestItemsLocale->CompletionText, locale, requestItemsText);

    if (!quest->GetReqItemsCount() && !quest->HasRequiredCurrency() && canComplete)
    {
        SendQuestGiverOfferReward(quest, npcGUID, true);
        return;
    }

    // Xinef: recheck completion on reward display
    Player* _player = _session->GetPlayer();
    QuestStatusMap::iterator qsitr = _player->getQuestStatusMap().find(quest->GetQuestId());
    if (qsitr != _player->getQuestStatusMap().end() && qsitr->second.Status == QUEST_STATUS_INCOMPLETE)
    {
        for (uint8 i = 0; i < 6; ++i)
            if (quest->RequiredItemId[i] && qsitr->second.ItemCount[i] < quest->RequiredItemCount[i])
                if (_player->GetItemCount(quest->RequiredItemId[i], false) >= quest->RequiredItemCount[i])
                    qsitr->second.ItemCount[i] = quest->RequiredItemCount[i];

        if (_player->CanCompleteQuest(quest->GetQuestId()))
        {
            _player->CompleteQuest(quest->GetQuestId());
            canComplete = true;
        }
    }

    // 5.4.8 18414, from Project SkyFire (PlayerMenu::SendQuestGiverRequestItems)
    uint32 questTakerEntry = 0;
    if (npcGUID)
        if (Object* object = ObjectAccessor::GetObjectByTypeMask(*_player, npcGUID, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM))
            if (object->hasInvolvedQuest(quest->GetQuestId()))
                questTakerEntry = object->GetEntry();

    ByteBuffer currencyData;
    ByteBuffer itemData;
    uint32 currencyCount = 0;
    uint32 itemCount = 0;
    for (uint8 i = 0; i < QUEST_REQUIRED_CURRENCY_COUNT; ++i)
    {
        if (!quest->RequiredCurrencyId[i])
            continue;

        currencyData << uint32(quest->RequiredCurrencyId[i]);
        currencyData << uint32(quest->RequiredCurrencyCount[i]);
        ++currencyCount;
    }

    for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        if (!quest->RequiredItemId[i])
            continue;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(quest->RequiredItemId[i]);
        itemData << uint32(itemTemplate ? itemTemplate->DisplayInfoID : 0);
        itemData << uint32(quest->RequiredItemId[i]);
        itemData << uint32(quest->RequiredItemCount[i]);
        ++itemCount;
    }

    ObjectGuid const giverGuid = npcGUID;
    WorldPacket data(SMSG_QUESTGIVER_REQUEST_ITEMS, 300);
    data << uint32(quest->GetSuggestedPlayers());
    data << uint32(quest->GetFlags());
    data << uint32(quest->OfferRewardEmoteDelay[0]);
    data << uint32(canComplete ? 0x5F : 0x5B);              // status flags
    data << uint32(quest->GetRewOrReqMoney() < 0 ? -quest->GetRewOrReqMoney() : 0);
    data << uint32(questTakerEntry);
    data << uint32(0);                                      // quest flags 2
    data << uint32(canComplete ? quest->GetCompleteEmote() : quest->GetIncompleteEmote());
    data << uint32(quest->GetQuestId());

    data.WriteBits(currencyCount, 21);
    data.WriteBit(closeOnCancel);
    data.WriteBit(giverGuid[2]);
    data.WriteBit(giverGuid[5]);
    data.WriteBit(giverGuid[1]);
    data.WriteBits(questTitle.size(), 9);
    data.WriteBits(requestItemsText.size(), 12);
    data.WriteBit(giverGuid[6]);
    data.WriteBit(giverGuid[0]);
    data.WriteBits(itemCount, 20);
    data.WriteBit(giverGuid[4]);
    data.WriteBit(giverGuid[7]);
    data.WriteBit(giverGuid[3]);
    data.FlushBits();

    data.WriteByteSeq(giverGuid[0]);
    data.WriteByteSeq(giverGuid[2]);
    data.WriteString(questTitle);
    data.append(currencyData);
    data.append(itemData);
    data.WriteByteSeq(giverGuid[3]);
    data.WriteByteSeq(giverGuid[1]);
    data.WriteString(requestItemsText);
    for (uint8 index : { 4, 5, 7, 6 })
        data.WriteByteSeq(giverGuid[index]);

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_REQUEST_ITEMS {}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}
