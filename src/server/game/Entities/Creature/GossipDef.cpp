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
#include "NPCPackets.h"
#include "Opcodes.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestPackets.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

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

    WorldPackets::NPC::GossipMessage packet;
    packet.GossipGUID = objectGUID;
    packet.GossipID = _gossipMenu.GetMenuId();
    packet.TextID = titleTextId;

    packet.GossipOptions.reserve(_gossipMenu.GetMenuItems().size());
    for (auto const& [id, item] : _gossipMenu.GetMenuItems())
    {
        WorldPackets::NPC::ClientGossipOptions& opt = packet.GossipOptions.emplace_back();
        opt.ClientOption = id;
        opt.OptionNPC = item.MenuItemIcon;
        opt.OptionFlags = item.IsCoded;     // makes pop up box password
        opt.OptionCost = item.BoxMoney;     // money required to open menu, 2.0.3
        opt.Text = item.Message;            // text for gossip item
        opt.Confirm = item.BoxMessage;      // accept text (related to money) pop up box, 2.0.3
    }

    LocaleConstant const locale = _session->GetSessionDbLocaleIndex();
    bool const questLevelInTitle = sWorld->getBoolConfig(CONFIG_UI_QUESTLEVELS_IN_DIALOGS);

    packet.GossipText.reserve(_questMenu.GetMenuItemCount());
    for (uint32 i = 0; i < _questMenu.GetMenuItemCount(); ++i)
    {
        QuestMenuItem const& item = _questMenu.GetItem(i);
        Quest const* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;

        WorldPackets::NPC::ClientGossipText& text = packet.GossipText.emplace_back();
        text.QuestID = item.QuestId;
        text.QuestType = item.QuestIcon;
        text.QuestLevel = quest->GetQuestLevel();
        text.QuestFlags[0] = quest->GetFlags();
        text.QuestFlags[1] = quest->GetFlagsEx();
        text.Repeatable = quest->IsRepeatable() && !quest->IsDailyOrWeekly() && !quest->IsMonthly();

        std::string title = quest->GetLogTitle();
        if (locale != LOCALE_enUS)
            if (QuestTemplateLocale const* localeData = sObjectMgr->GetQuestLocale(item.QuestId))
                ObjectMgr::GetLocaleString(localeData->LogTitle, locale, title);

        if (questLevelInTitle)
            AddQuestLevelToTitle(title, quest->GetQuestLevel());

        text.QuestTitle = title;
    }

    _session->SendPacket(packet.Write());
}

void PlayerMenu::SendCloseGossip()
{
    _gossipMenu.SetSenderGUID(ObjectGuid::Empty);

    WorldPackets::NPC::GossipComplete packet;
    _session->SendPacket(packet.Write());
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
    LocaleConstant locale = _session->GetSessionDbLocaleIndex();
    if (locale != LOCALE_enUS)
        if (PointOfInterestLocale const* localeData = sObjectMgr->GetPointOfInterestLocale(poiId))
            ObjectMgr::GetLocaleString(localeData->Name, locale, name);

    WorldPackets::NPC::GossipPOI packet;
    packet.Flags = poi->Flags;
    packet.Pos = G3D::Vector2(poi->PositionX, poi->PositionY);
    packet.Icon = poi->Icon;
    packet.Importance = poi->Importance;
    packet.Name = name;

    _session->SendPacket(packet.Write());
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

void PlayerMenu::SendQuestGiverQuestList(QEmote const& eEmote, std::string const& Title, ObjectGuid guid)
{
    WorldPackets::Quest::QuestGiverQuestList questList;
    questList.QuestGiverGUID = guid;
    questList.Greeting = Title;
    questList.GreetEmoteDelay = eEmote._Delay;
    questList.GreetEmoteType = eEmote._Emote;

    LocaleConstant const locale = _session->GetSessionDbLocaleIndex();

    if (QuestGreeting const* questGreeting = sObjectMgr->GetQuestGreeting(guid.GetTypeId(), guid.GetEntry()))
    {
        questList.Greeting.clear();
        // Check if greeting exists. Blizzlike that some creatures have empty greeting
        if (!questGreeting->Greeting.empty())
            questList.Greeting = questGreeting->Greeting.size() > size_t(locale) ? questGreeting->Greeting[locale] : questGreeting->Greeting[DEFAULT_LOCALE];
        questList.GreetEmoteDelay = questGreeting->EmoteDelay;
        questList.GreetEmoteType = questGreeting->EmoteType;
    }

    // Store this instead of checking the Singleton every loop iteration
    bool questLevelInTitle = sWorld->getBoolConfig(CONFIG_UI_QUESTLEVELS_IN_DIALOGS);

    for (uint32 i = 0; i < _questMenu.GetMenuItemCount(); ++i)
    {
        QuestMenuItem const& questMenuItem = _questMenu.GetItem(i);

        uint32 questID = questMenuItem.QuestId;

        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questID))
        {
            std::string title = quest->GetLogTitle();

            if (locale != LOCALE_enUS)
                if (QuestTemplateLocale const* questTemplateLocale = sObjectMgr->GetQuestLocale(questID))
                    ObjectMgr::GetLocaleString(questTemplateLocale->LogTitle, locale, title);

            if (questLevelInTitle)
                AddQuestLevelToTitle(title, quest->GetQuestLevel());

            bool repeatable = quest->IsRepeatable() && !quest->IsDailyOrWeekly() && !quest->IsMonthly();

            questList.GossipTexts.push_back(WorldPackets::Quest::GossipTextData(questID, questMenuItem.QuestIcon, quest->GetQuestLevel(), quest->GetFlags(), quest->GetFlagsEx(), repeatable, title));
        }
    }

    _session->SendPacket(questList.Write());
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUEST_GIVER_QUEST_LIST_MESSAGE NPC={}", guid.ToString());
}

void PlayerMenu::SendQuestGiverStatus(uint32 questStatus, ObjectGuid npcGUID) const
{
    WorldPackets::Quest::QuestGiverStatus packet;
    packet.QuestGiver.Guid = npcGUID;
    packet.QuestGiver.Status = questStatus;

    _session->SendPacket(packet.Write());
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_STATUS NPC={}, status={}", npcGUID.ToString(), questStatus);
}

void PlayerMenu::SendQuestGiverQuestDetails(Quest const* quest, ObjectGuid npcGUID, bool activateAccept) const
{
    std::string questLogTitle        = quest->GetLogTitle();
    std::string questLogDescription  = quest->GetLogDescription();
    std::string questDescription     = quest->GetQuestDescription();
    std::string portraitGiverText    = quest->GetPortraitGiverText();
    std::string portraitGiverName    = quest->GetPortraitGiverName();
    std::string portraitTurnInText   = quest->GetPortraitTurnInText();
    std::string portraitTurnInName   = quest->GetPortraitTurnInName();

    LocaleConstant localeConstant = _session->GetSessionDbLocaleIndex();
    if (localeConstant != LOCALE_enUS)
    {
        if (QuestTemplateLocale const* questTemplateLocale = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        {
            ObjectMgr::GetLocaleString(questTemplateLocale->LogTitle,           localeConstant, questLogTitle);
            ObjectMgr::GetLocaleString(questTemplateLocale->LogDescription,     localeConstant, questLogDescription);
            ObjectMgr::GetLocaleString(questTemplateLocale->QuestDescription,   localeConstant, questDescription);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitGiverText,  localeConstant, portraitGiverText);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitGiverName,  localeConstant, portraitGiverName);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitTurnInText, localeConstant, portraitTurnInText);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitTurnInName, localeConstant, portraitTurnInName);
        }
    }

    if (sWorld->getBoolConfig(CONFIG_UI_QUESTLEVELS_IN_DIALOGS))
        AddQuestLevelToTitle(questLogTitle, quest->GetQuestLevel());

    WorldPackets::Quest::QuestGiverQuestDetails packet;
    packet.QuestGiverGUID = npcGUID;
    packet.InformUnit = _session->GetPlayer()->GetDivider();
    packet.QuestID = quest->GetQuestId();
    packet.QuestTitle = questLogTitle;
    packet.LogDescription = questLogDescription;
    packet.DescriptionText = questDescription;
    packet.PortraitGiverText = portraitGiverText;
    packet.PortraitGiverName = portraitGiverName;
    packet.PortraitTurnInText = portraitTurnInText;
    packet.PortraitTurnInName = portraitTurnInName;
    packet.PortraitGiver = quest->GetQuestGiverPortrait();
    packet.PortraitTurnIn = quest->GetQuestTurnInPortrait();
    packet.DisplayPopup = activateAccept;
    packet.QuestFlags[0] = quest->GetFlags();
    packet.QuestFlags[1] = quest->GetFlagsEx();
    packet.SuggestedPartyMembers = quest->GetSuggestedPlayers();

    if (quest->GetSrcSpell())
        packet.LearnSpells.push_back(quest->GetSrcSpell());

    quest->BuildQuestRewards(packet.Rewards, _session->GetPlayer());

    packet.DescEmotes.resize(QUEST_EMOTE_COUNT);
    for (uint32 i = 0; i < QUEST_EMOTE_COUNT; ++i)
    {
        packet.DescEmotes[i].Type = quest->DetailsEmote[i];
        packet.DescEmotes[i].Delay = quest->DetailsEmoteDelay[i];
    }

    QuestObjectives const& objs = quest->GetObjectives();
    packet.Objectives.resize(objs.size());
    for (uint32 i = 0; i < objs.size(); ++i)
    {
        packet.Objectives[i].ID = objs[i].ID;
        packet.Objectives[i].ObjectID = objs[i].ObjectID;
        packet.Objectives[i].Amount = objs[i].Amount;
        packet.Objectives[i].Type = objs[i].Type;
    }

    _session->SendPacket(packet.Write());

    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_QUEST_DETAILS NPC={}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}

void PlayerMenu::SendQuestQueryResponse(Quest const* quest) const
{
    LocaleConstant localeConstant = _session->GetSessionDbLocaleIndex();

    std::string questLogTitle       = quest->GetLogTitle();
    std::string questLogDescription = quest->GetLogDescription();
    std::string questDescription    = quest->GetQuestDescription();
    std::string areaDescription     = quest->GetAreaDescription();
    std::string questCompletionLog  = quest->GetQuestCompletionLog();
    std::string portraitGiverText   = quest->GetPortraitGiverText();
    std::string portraitGiverName   = quest->GetPortraitGiverName();
    std::string portraitTurnInText  = quest->GetPortraitTurnInText();
    std::string portraitTurnInName  = quest->GetPortraitTurnInName();

    if (localeConstant != LOCALE_enUS)
    {
        if (QuestTemplateLocale const* questTemplateLocale = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        {
            ObjectMgr::GetLocaleString(questTemplateLocale->LogTitle,           localeConstant, questLogTitle);
            ObjectMgr::GetLocaleString(questTemplateLocale->LogDescription,     localeConstant, questLogDescription);
            ObjectMgr::GetLocaleString(questTemplateLocale->QuestDescription,   localeConstant, questDescription);
            ObjectMgr::GetLocaleString(questTemplateLocale->AreaDescription,    localeConstant, areaDescription);
            ObjectMgr::GetLocaleString(questTemplateLocale->QuestCompletionLog, localeConstant, questCompletionLog);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitGiverText,  localeConstant, portraitGiverText);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitGiverName,  localeConstant, portraitGiverName);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitTurnInText, localeConstant, portraitTurnInText);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitTurnInName, localeConstant, portraitTurnInName);
        }
    }

    WorldPackets::Quest::QueryQuestInfoResponse packet;

    packet.Allow = true;
    packet.QuestID = quest->GetQuestId();

    packet.Info.QuestID = quest->GetQuestId();
    packet.Info.QuestType = quest->GetQuestType();
    packet.Info.QuestLevel = quest->GetQuestLevel();
    packet.Info.QuestPackageID = quest->GetQuestPackageID();
    packet.Info.QuestMinLevel = quest->GetMinLevel();
    packet.Info.QuestSortID = quest->GetZoneOrSort();
    packet.Info.QuestInfoID = quest->GetQuestInfoID();
    packet.Info.SuggestedGroupNum = quest->GetSuggestedPlayers();
    packet.Info.RewardNextQuest = quest->GetNextQuestInChain();
    packet.Info.RewardXPDifficulty = quest->GetXPDifficulty();
    packet.Info.RewardXPMultiplier = quest->GetXPMultiplier();

    if (!quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
        packet.Info.RewardMoney = quest->RewardMoney < 0 ? quest->RewardMoney : _session->GetPlayer()->GetQuestMoneyReward(quest);

    packet.Info.RewardMoneyDifficulty = quest->GetRewMoneyDifficulty();
    packet.Info.RewardMoneyMultiplier = quest->GetMoneyMultiplier();
    packet.Info.RewardBonusMoney = quest->GetRewMoneyMaxLevel();
    packet.Info.RewardDisplaySpell = quest->GetRewDisplaySpell();
    packet.Info.RewardSpell = quest->GetRewSpell();

    packet.Info.RewardHonor = quest->GetRewHonor();
    packet.Info.RewardKillHonor = quest->GetRewKillHonor();

    packet.Info.StartItem = quest->GetSrcItemId();
    packet.Info.Flags = quest->GetFlags();
    packet.Info.FlagsEx = quest->GetFlagsEx();
    packet.Info.RewardTitle = quest->GetRewTitle();
    packet.Info.RewardTalents = quest->GetBonusTalents();
    packet.Info.RewardArenaPoints = quest->GetRewArenaPoints();
    packet.Info.RewardSkillLineID = quest->GetRewardSkillId();
    packet.Info.RewardNumSkillUps = quest->GetRewardSkillPoints();
    packet.Info.RewardFactionFlags = quest->GetRewardReputationMask();
    packet.Info.PortraitGiver = quest->GetQuestGiverPortrait();
    packet.Info.PortraitTurnIn = quest->GetQuestTurnInPortrait();

    for (uint8 i = 0; i < QUEST_ITEM_DROP_COUNT; ++i)
    {
        packet.Info.ItemDrop[i] = quest->ItemDrop[i];
        packet.Info.ItemDropQuantity[i] = quest->ItemDropQuantity[i];
    }

    if (!quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
    {
        for (uint8 i = 0; i < QUEST_REWARD_ITEM_COUNT; ++i)
        {
            packet.Info.RewardItems[i] = quest->RewardItemId[i];
            packet.Info.RewardAmount[i] = quest->RewardItemCount[i];
        }
        for (uint8 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
        {
            packet.Info.UnfilteredChoiceItems[i].ItemID = quest->RewardChoiceItemId[i];
            packet.Info.UnfilteredChoiceItems[i].Quantity = quest->RewardChoiceItemCount[i];
        }
    }

    for (uint8 i = 0; i < QUEST_REWARD_REPUTATIONS_COUNT; ++i)
    {
        packet.Info.RewardFactionID[i] = quest->RewardFactionId[i];
        packet.Info.RewardFactionValue[i] = quest->RewardFactionValue[i];
        packet.Info.RewardFactionOverride[i] = quest->RewardFactionOverride[i];
    }

    packet.Info.POIContinent = quest->GetPOIContinent();
    packet.Info.POIx = quest->GetPOIx();
    packet.Info.POIy = quest->GetPOIy();
    packet.Info.POIPriority = quest->GetPOIPriority();

    if (sWorld->getBoolConfig(CONFIG_UI_QUESTLEVELS_IN_DIALOGS))
        AddQuestLevelToTitle(questLogTitle, quest->GetQuestLevel());

    packet.Info.LogTitle = questLogTitle;
    packet.Info.LogDescription = questLogDescription;
    packet.Info.QuestDescription = questDescription;
    packet.Info.AreaDescription = areaDescription;
    packet.Info.QuestCompletionLog = questCompletionLog;
    packet.Info.AllowableRaces = quest->GetAllowableRaces();

    for (QuestObjective const& questObjective : quest->GetObjectives())
    {
        packet.Info.Objectives.push_back(questObjective);

        if (localeConstant != LOCALE_enUS)
        {
            if (QuestObjectivesLocale const* questObjectivesLocale = sObjectMgr->GetQuestObjectivesLocale(questObjective.ID))
                ObjectMgr::GetLocaleString(questObjectivesLocale->Description, localeConstant, packet.Info.Objectives.back().Description);
        }
    }

    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
    {
        packet.Info.RewardCurrencyID[i] = quest->RewardCurrencyId[i];
        packet.Info.RewardCurrencyQty[i] = quest->RewardCurrencyCount[i];
    }

    packet.Info.PortraitGiverText = portraitGiverText;
    packet.Info.PortraitGiverName = portraitGiverName;
    packet.Info.PortraitTurnInText = portraitTurnInText;
    packet.Info.PortraitTurnInName = portraitTurnInName;

    packet.Info.AcceptedSoundKitID = quest->GetSoundAccept();
    packet.Info.CompleteSoundKitID = quest->GetSoundTurnIn();
    packet.Info.AreaGroupID = quest->GetAreaGroupID();
    packet.Info.TimeAllowed = quest->GetLimitTime();

    _session->SendPacket(packet.Write());

    LOG_DEBUG("network", "WORLD: Sent SMSG_QUEST_QUERY_RESPONSE questid={}", quest->GetQuestId());
}

void PlayerMenu::SendQuestGiverOfferReward(Quest const* quest, ObjectGuid npcGUID, bool enableNext) const
{
    std::string questTitle              = quest->GetLogTitle();
    std::string questOfferRewardText    = quest->GetOfferRewardText();
    std::string portraitGiverText       = quest->GetPortraitGiverText();
    std::string portraitGiverName       = quest->GetPortraitGiverName();
    std::string portraitTurnInText      = quest->GetPortraitTurnInText();
    std::string portraitTurnInName      = quest->GetPortraitTurnInName();

    LocaleConstant locale = _session->GetSessionDbLocaleIndex();
    if (locale != LOCALE_enUS)
    {
        if (QuestTemplateLocale const* questTemplateLocale = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
        {
            ObjectMgr::GetLocaleString(questTemplateLocale->LogTitle,           locale, questTitle);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitGiverText,  locale, portraitGiverText);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitGiverName,  locale, portraitGiverName);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitTurnInText, locale, portraitTurnInText);
            ObjectMgr::GetLocaleString(questTemplateLocale->PortraitTurnInName, locale, portraitTurnInName);
        }
    }

    if (locale != LOCALE_enUS)
        if (QuestOfferRewardLocale const* questOfferRewardLocale = sObjectMgr->GetQuestOfferRewardLocale(quest->GetQuestId()))
            ObjectMgr::GetLocaleString(questOfferRewardLocale->RewardText, locale, questOfferRewardText);

    if (sWorld->getBoolConfig(CONFIG_UI_QUESTLEVELS_IN_DIALOGS))
        AddQuestLevelToTitle(questTitle, quest->GetQuestLevel());

    WorldPackets::Quest::QuestGiverOfferRewardMessage packet;
    WorldPackets::Quest::QuestGiverOfferReward& offer = packet.QuestData;

    quest->BuildQuestRewards(offer.Rewards, _session->GetPlayer());
    offer.QuestGiverGUID = npcGUID;

    // Is there a better way? what about game objects?
    if (Creature const* creature = ObjectAccessor::GetCreature(*_session->GetPlayer(), npcGUID))
        offer.QuestGiverCreatureID = creature->GetCreatureTemplate()->Entry;

    offer.QuestID = quest->GetQuestId();
    offer.AutoLaunched = enableNext;
    offer.SuggestedPartyMembers = quest->GetSuggestedPlayers();

    for (uint32 i = 0; i < QUEST_EMOTE_COUNT && quest->OfferRewardEmote[i]; ++i)
        offer.Emotes.push_back(WorldPackets::Quest::QuestDescEmote(quest->OfferRewardEmote[i], quest->OfferRewardEmoteDelay[i]));

    offer.QuestFlags[0] = quest->GetFlags();
    offer.QuestFlags[1] = quest->GetFlagsEx();

    packet.QuestTitle = questTitle;
    packet.RewardText = questOfferRewardText;
    packet.PortraitTurnIn = quest->GetQuestTurnInPortrait();
    packet.PortraitGiver = quest->GetQuestGiverPortrait();
    packet.PortraitGiverText = portraitGiverText;
    packet.PortraitGiverName = portraitGiverName;
    packet.PortraitTurnInText = portraitTurnInText;
    packet.PortraitTurnInName = portraitTurnInName;
    packet.QuestPackageID = quest->GetQuestPackageID();

    _session->SendPacket(packet.Write());
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_OFFER_REWARD NPC={}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}

void PlayerMenu::SendQuestGiverRequestItems(Quest const* quest, ObjectGuid npcGUID, bool canComplete, bool closeOnCancel) const
{
    // We can always call to RequestItems, but this packet only goes out if there are actually
    // items.  Otherwise, we'll skip straight to the OfferReward

    std::string questTitle = quest->GetLogTitle();
    std::string requestItemsText = quest->GetRequestItemsText();

    LocaleConstant locale = _session->GetSessionDbLocaleIndex();
    if (locale != LOCALE_enUS)
    {
        if (QuestTemplateLocale const* questTemplateLocale = sObjectMgr->GetQuestLocale(quest->GetQuestId()))
            ObjectMgr::GetLocaleString(questTemplateLocale->LogTitle, locale, questTitle);

        if (QuestRequestItemsLocale const* questRequestItemsLocale = sObjectMgr->GetQuestRequestItemsLocale(quest->GetQuestId()))
            ObjectMgr::GetLocaleString(questRequestItemsLocale->CompletionText, locale, requestItemsText);
    }

    if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER) && canComplete)
    {
        SendQuestGiverOfferReward(quest, npcGUID, true);
        return;
    }

    if (sWorld->getBoolConfig(CONFIG_UI_QUESTLEVELS_IN_DIALOGS))
        AddQuestLevelToTitle(questTitle, quest->GetQuestLevel());

    WorldPackets::Quest::QuestGiverRequestItems packet;
    packet.QuestGiverGUID = npcGUID;

    // Is there a better way? what about game objects?
    if (Creature const* creature = ObjectAccessor::GetCreature(*_session->GetPlayer(), npcGUID))
        packet.QuestGiverCreatureID = creature->GetCreatureTemplate()->Entry;

    packet.QuestID = quest->GetQuestId();

    if (canComplete)
    {
        packet.CompEmoteDelay = quest->EmoteOnCompleteDelay;
        packet.CompEmoteType = quest->EmoteOnComplete;
    }
    else
    {
        packet.CompEmoteDelay = quest->EmoteOnIncompleteDelay;
        packet.CompEmoteType = quest->EmoteOnIncomplete;
    }

    packet.QuestFlags[0] = quest->GetFlags();
    packet.QuestFlags[1] = quest->GetFlagsEx();
    packet.SuggestPartyMembers = quest->GetSuggestedPlayers();
    packet.StatusFlags = 0xDF; // Unk, send common value

    packet.MoneyToGet = 0;
    for (QuestObjective const& obj : quest->GetObjectives())
    {
        switch (obj.Type)
        {
            case QUEST_OBJECTIVE_ITEM:
                packet.Collect.push_back(WorldPackets::Quest::QuestObjectiveCollect(obj.ObjectID, obj.Amount));
                break;
            case QUEST_OBJECTIVE_CURRENCY:
                packet.Currency.push_back(WorldPackets::Quest::QuestCurrency(obj.ObjectID, obj.Amount));
                break;
            case QUEST_OBJECTIVE_MONEY:
                packet.MoneyToGet += obj.Amount;
                break;
            default:
                break;
        }
    }

    packet.AutoLaunched = closeOnCancel;
    packet.QuestTitle = questTitle;
    packet.CompletionText = requestItemsText;

    _session->SendPacket(packet.Write());
    LOG_DEBUG("network", "WORLD: Sent SMSG_QUESTGIVER_REQUEST_ITEMS NPC={}, questid={}", npcGUID.ToString(), quest->GetQuestId());
}

void PlayerMenu::AddQuestLevelToTitle(std::string &title, int32 level)
{
    // Adds the quest level to the front of the quest title
    // example: [13] Westfall Stew

    std::stringstream questTitlePretty;
    questTitlePretty << "[" << level << "] " << title;
    title = questTitlePretty.str();
}
