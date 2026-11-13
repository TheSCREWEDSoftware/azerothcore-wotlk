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

void PlayerMenu::SendQuestGiverQuestDetails(Quest const* quest, ObjectGuid npcGUID, bool activateAccept) const
{
    std::string questTitle           = quest->GetTitle();
    std::string questDetails         = quest->GetDetails();
    std::string questObjectives      = quest->GetObjectives();
    std::string questAreaDescription = quest->GetAreaDescription();

    int32 locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
    {
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);
        ObjectMgr::GetLocaleString(localeData->Details, locale, questDetails);
        ObjectMgr::GetLocaleString(localeData->Objectives, locale, questObjectives);
        ObjectMgr::GetLocaleString(localeData->AreaDescription, locale, questAreaDescription);
    }

    WorldPacket data(SMSG_QUESTGIVER_QUEST_DETAILS, 500);   // guess size
    data << npcGUID;
    data << _session->GetPlayer()->GetDivider();
    data << uint32(quest->GetQuestId());
    data << questTitle;
    data << questDetails;
    data << questObjectives;
    // shortcut: portrait data stays empty until Cata portrait columns are imported.
    data << std::string() << std::string() << std::string() << std::string();
    data << uint32(0) << uint32(0);
    data << uint8(activateAccept ? 1 : 0);                  // auto finish
    data << uint32(quest->GetFlags());                      // 3.3.3 questFlags
    data << uint32(quest->GetSuggestedPlayers());
    data << uint8(0);                                       // IsFinished? value is sent back to server in quest accept packet
    data << uint8(0) << uint32(quest->GetRequiredSpell());   // DisplayPopup, RequiredSpellID

    BuildQuestRewards(quest, _session->GetPlayer(), quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS)).Write(data);

    data << uint32(QUEST_EMOTE_COUNT);
    for (uint32 i = 0; i < QUEST_EMOTE_COUNT; ++i)
    {
        data << uint32(quest->DetailsEmote[i]);
        data << uint32(quest->DetailsEmoteDelay[i]);       // DetailsEmoteDelay (in ms)
    }
    _session->SendPacket(&data);

    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_QUEST_DETAILS {}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}

void PlayerMenu::SendQuestQueryResponse(Quest const* quest) const
{
    std::string questTitle           = quest->GetTitle();
    std::string questDetails         = quest->GetDetails();
    std::string questObjectives      = quest->GetObjectives();
    std::string questAreaDescription = quest->GetAreaDescription();
    std::string questCompletedText   = quest->GetCompletedText();

    std::string questObjectiveText[QUEST_OBJECTIVES_COUNT];
    for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        questObjectiveText[i] = quest->ObjectiveText[i];

    int32 locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
    {
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);
        ObjectMgr::GetLocaleString(localeData->Details, locale, questDetails);
        ObjectMgr::GetLocaleString(localeData->Objectives, locale, questObjectives);
        ObjectMgr::GetLocaleString(localeData->AreaDescription, locale, questAreaDescription);
        ObjectMgr::GetLocaleString(localeData->CompletedText, locale, questCompletedText);

        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            ObjectMgr::GetLocaleString(localeData->ObjectiveText[i], locale, questObjectiveText[i]);
    }

    WorldPacket data(SMSG_QUEST_QUERY_RESPONSE, 100);       // guess size

    data << uint32(quest->GetQuestId());                    // quest id
    data << uint32(quest->GetQuestMethod());                // Accepted values: 0, 1 or 2. 0 == IsAutoComplete() (skip objectives/details)
    data << uint32(quest->GetQuestLevel());                 // may be -1, static data, in other cases must be used dynamic level: Player::GetQuestLevel (0 is not known, but assuming this is no longer valid for quest intended for client)
    data << uint32(quest->GetMinLevel());                   // min level
    data << uint32(quest->GetZoneOrSort());                 // zone or sort to display in quest log

    data << uint32(quest->GetType());                       // quest type
    data << uint32(quest->GetSuggestedPlayers());           // suggested players count

    data << uint32(quest->GetRepObjectiveFaction());        // shown in quest log as part of quest objective
    data << uint32(quest->GetRepObjectiveValue());          // shown in quest log as part of quest objective

    data << uint32(quest->GetRepObjectiveFaction2());       // shown in quest log as part of quest objective OPPOSITE faction
    data << uint32(quest->GetRepObjectiveValue2());         // shown in quest log as part of quest objective OPPOSITE faction

    data << uint32(quest->GetNextQuestInChain());           // client will request this quest from NPC, if not 0
    data << uint32(quest->GetXPId());                       // used for calculating rewarded experience

    if (quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
        data << uint32(0);                                  // Hide money rewarded
    else
    {
        uint32 moneyRew = 0;
        Player* player = _session->GetPlayer();
        if (player && (player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) || sScriptMgr->OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(player)))
        {
            moneyRew = quest->GetRewMoneyMaxLevel();
        }
        moneyRew += quest->GetRewOrReqMoney(player ? player->GetLevel() : 0); // reward money (below max lvl)
        data << moneyRew;
    }

    data << quest->RewardBonusMoney;
    data << uint32(quest->GetRewSpell());                   // reward spell, this spell will display (icon) (cast if RewSpellCast == 0)
    data << int32(quest->GetRewSpellCast());                // cast spell

    // rewarded honor points
    data << uint32(quest->GetRewHonorAddition());
    data << float(quest->GetRewHonorMultiplier());
    data << uint32(quest->GetSrcItemId());                  // source item id
    data << uint32(quest->GetFlags());             // quest flags
    data << uint32(0);                                      // MinimapTargetMark
    data << uint32(quest->GetCharTitleId());                // CharTitleId, new 2.4.0, player gets this title (id from CharTitles)
    data << uint32(quest->GetPlayersSlain());               // players slain
    data << uint32(quest->GetBonusTalents());               // bonus talents
    data << uint32(quest->GetRewArenaPoints());             // bonus arena points
    data << uint32(0) << uint32(0);                          // RewardSkillLineID, RewardNumSkillUps
    data << quest->RewardReputationMask;
    data << uint32(0) << uint32(0);                          // PortraitGiver, PortraitTurnIn

    if (quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
    {
        for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            data << uint32(0) << uint32(0);
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
            data << uint32(0) << uint32(0);
    }
    else
    {
        for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        {
            data << uint32(quest->RewardItemId[i]);
            data << uint32(quest->RewardItemIdCount[i]);
        }
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            data << uint32(quest->RewardChoiceItemId[i]);
            data << uint32(quest->RewardChoiceItemCount[i]);
        }
    }

    for (uint8 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // reward factions ids
        data << uint32(quest->RewardFactionId[i]);

    for (uint8 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // columnid+1 QuestFactionReward.dbc?
        data << int32(quest->RewardFactionValueId[i]);

    for (uint8 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // unk (0)
        data << int32(quest->RewardFactionValueIdOverride[i]);

    data << uint32(quest->GetPOIContinent());
    data << float(quest->GetPOIx());
    data << float(quest->GetPOIy());
    data << uint32(quest->GetPointOpt());

    data << questTitle;
    data << questObjectives;
    data << questDetails;
    data << questAreaDescription;
    data << questCompletedText;                                 // display in quest objectives window once all objectives are completed

    for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
    {
        if (quest->RequiredNpcOrGo[i] < 0)
            data << uint32((quest->RequiredNpcOrGo[i] * (-1)) | 0x80000000);    // client expects gameobject template id in form (id|0x80000000)
        else
            data << uint32(quest->RequiredNpcOrGo[i]);

        data << uint32(quest->RequiredNpcOrGoCount[i]);
        data << uint32(quest->ItemDrop[i]);
        data << uint32(quest->ItemDropQuantity[i]);
    }

    for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        data << uint32(quest->RequiredItemId[i]);
        data << uint32(quest->RequiredItemCount[i]);
    }

    data << uint32(quest->GetRequiredSpell());              // RequiredSpell
    for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        data << questObjectiveText[i];
    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
        data << uint32(quest->RewardCurrencyId[i]) << uint32(quest->RewardCurrencyCount[i]);
    for (uint32 i = 0; i < QUEST_REQUIRED_CURRENCY_COUNT; ++i)
        data << uint32(quest->RequiredCurrencyId[i]) << uint32(quest->RequiredCurrencyCount[i]);
    data << std::string() << std::string() << std::string() << std::string();
    data << quest->SoundAccept << quest->SoundTurnIn;

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUEST_QUERY_RESPONSE questid={}", quest->GetQuestId());
}

void PlayerMenu::SendQuestGiverOfferReward(Quest const* quest, ObjectGuid npcGUID, bool enableNext) const
{
    std::string questTitle = quest->GetTitle();
    std::string RewardText = quest->GetOfferRewardText();

    int32 locale = _session->GetSessionDbLocaleIndex();
    if (QuestLocale const* localeData = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(localeData->Title, locale, questTitle);

    if (QuestOfferRewardLocale const* questOfferRewardLocale = sObjectMgr->GetQuestOfferRewardLocale(quest->GetQuestId()))
        ObjectMgr::GetLocaleString(questOfferRewardLocale->RewardText, locale, RewardText);

    WorldPacket data(SMSG_QUESTGIVER_OFFER_REWARD, 400);    // guess size
    data << npcGUID;
    data << uint32(quest->GetQuestId());
    data << questTitle;
    data << RewardText;

    data << std::string() << std::string() << std::string() << std::string();
    data << uint32(0) << uint32(0);                         // PortraitGiver, PortraitTurnIn

    data << uint8(enableNext ? 1 : 0);                      // Auto Finish
    data << uint32(quest->GetFlags());                      // 3.3.3 questFlags
    data << uint32(quest->GetSuggestedPlayers());           // SuggestedGroupNum

    uint32 emoteCount = 0;
    for (uint32 i = 0; i < QUEST_EMOTE_COUNT; ++i)
    {
        if (quest->OfferRewardEmote[i] <= 0)
            break;
        ++emoteCount;
    }

    data << emoteCount;                                     // Emote Count
    for (uint32 i = 0; i < emoteCount; ++i)
    {
        data << uint32(quest->OfferRewardEmoteDelay[i]);    // Delay Emote
        data << uint32(quest->OfferRewardEmote[i]);
    }

    BuildQuestRewards(quest, _session->GetPlayer(), false).Write(data);

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

    WorldPacket data(SMSG_QUESTGIVER_REQUEST_ITEMS, 300);   // guess size
    data << npcGUID;
    data << uint32(quest->GetQuestId());
    data << questTitle;
    data << requestItemsText;

    data << uint32(0x00);                                   // unknown

    if (canComplete)
        data << quest->GetCompleteEmote();
    else
        data << quest->GetIncompleteEmote();

    // Close Window after cancel
    if (closeOnCancel)
        data << uint32(0x01);
    else
        data << uint32(0x00);

    data << uint32(quest->GetFlags());                      // 3.3.3 questFlags
    data << uint32(quest->GetSuggestedPlayers());           // SuggestedGroupNum

    // Required Money
    data << uint32(quest->GetRewOrReqMoney() < 0 ? -quest->GetRewOrReqMoney() : 0);

    data << uint32(quest->GetReqItemsCount());
    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
    {
        if (!quest->RequiredItemId[i])
            continue;

        data << uint32(quest->RequiredItemId[i]);
        data << uint32(quest->RequiredItemCount[i]);

        if (ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(quest->RequiredItemId[i]))
            data << uint32(itemTemplate->DisplayInfoID);
        else
            data << uint32(0);
    }

    uint32 requiredCurrencyCount = 0;
    for (uint32 i = 0; i < QUEST_REQUIRED_CURRENCY_COUNT; ++i)
        if (quest->RequiredCurrencyId[i])
            ++requiredCurrencyCount;
    data << uint32(requiredCurrencyCount);               // required currency count
    for (uint32 i = 0; i < QUEST_REQUIRED_CURRENCY_COUNT; ++i)
        if (quest->RequiredCurrencyId[i])
            data << uint32(quest->RequiredCurrencyId[i]) << uint32(quest->RequiredCurrencyCount[i]);

    // 4.3.4 status flags: kill credit done, items collected, then three fixed flags
    bool killCreditComplete = true;
    bool collectComplete = true;
    auto statusItr = _session->GetPlayer()->getQuestStatusMap().find(quest->GetQuestId());
    if (statusItr != _session->GetPlayer()->getQuestStatusMap().end())
    {
        QuestStatusData const& statusData = statusItr->second;
        for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
            if (quest->RequiredItemId[i] && statusData.ItemCount[i] < quest->RequiredItemCount[i]
                && !_session->GetPlayer()->HasItemCount(quest->RequiredItemId[i], quest->RequiredItemCount[i]))
                collectComplete = false;

        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            if (quest->RequiredNpcOrGo[i] && statusData.CreatureOrGOCount[i] != quest->RequiredNpcOrGoCount[i])
                killCreditComplete = false;
    }

    data << uint32(killCreditComplete ? 0x02 : 0x00);      // QUEST_STATUS_FLAG_KILL_OBJECTIVE_COMPLETE
    data << uint32(collectComplete ? 0x04 : 0x00);         // QUEST_STATUS_FLAG_COLLECT_OBJECTIVE_COMPLETE
    data << uint32(0x08);
    data << uint32(0x10);
    data << uint32(0x40);

    _session->SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_REQUEST_ITEMS {}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}
