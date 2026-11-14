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

#include "BlackMarketMgr.h"
#include "AccountMgr.h"
#include "BlackMarketPackets.h"
#include "CharacterCache.h"
#include "Containers.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Mail.h"
#include "Realm.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Language.h"

BlackMarketMgr::BlackMarketMgr()
{
}

BlackMarketMgr::~BlackMarketMgr()
{
    for (auto itr = _auctions.begin(); itr != _auctions.end(); ++itr)
        delete itr->second;

    for (auto itr = _templates.begin(); itr != _templates.end(); ++itr)
        delete itr->second;
}

BlackMarketMgr* BlackMarketMgr::Instance()
{
    static BlackMarketMgr instance;
    return &instance;
}

void BlackMarketMgr::LoadTemplates()
{
    uint32 oldMSTime = getMSTime();

    // Clear in case we are reloading
    if (!_templates.empty())
    {
        for (BlackMarketTemplateMap::iterator itr = _templates.begin(); itr != _templates.end(); ++itr)
            delete itr->second;

        _templates.clear();
    }

    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_BLACKMARKET_TEMPLATE));
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 black market templates. DB table `blackmarket_template` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        BlackMarketTemplate* templ = new BlackMarketTemplate();

        if (!templ->LoadFromDB(fields)) // Add checks
        {
            delete templ;
            continue;
        }

        AddTemplate(templ);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} black market templates in {} ms.", uint32(_templates.size()), GetMSTimeDiffToNow(oldMSTime));
}

void BlackMarketMgr::LoadAuctions()
{
    uint32 oldMSTime = getMSTime();

    // Clear in case we are reloading
    if (!_auctions.empty())
    {
        for (BlackMarketEntryMap::iterator itr = _auctions.begin(); itr != _auctions.end(); ++itr)
            delete itr->second;

        _auctions.clear();
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BLACKMARKET_AUCTIONS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 black market auctions. DB table `blackmarket_auctions` is empty.");
        return;
    }

    _lastUpdate = time(nullptr); //Set update time before loading

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    do
    {
        Field* fields = result->Fetch();
        BlackMarketEntry* auction = new BlackMarketEntry();

        if (!auction->LoadFromDB(fields))
        {
            auction->DeleteFromDB(trans);
            delete auction;
            continue;
        }

        if (auction->IsCompleted())
        {
            auction->DeleteFromDB(trans);
            delete auction;
            continue;
        }

        AddAuction(auction);
    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    LOG_INFO("server.loading", ">> Loaded {} black market auctions in {} ms.", uint32(_auctions.size()), GetMSTimeDiffToNow(oldMSTime));
}

void BlackMarketMgr::Update(bool updateTime)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    time_t now = time(nullptr);
    for (BlackMarketEntryMap::iterator itr = _auctions.begin(); itr != _auctions.end(); ++itr)
    {
        BlackMarketEntry* entry = itr->second;

        if (entry->IsCompleted() && entry->GetBidder())
            SendAuctionWonMail(entry, trans);

        if (updateTime)
            entry->Update(now);
    }

    if (updateTime)
        _lastUpdate = now;

    CharacterDatabase.CommitTransaction(trans);
}

void BlackMarketMgr::RefreshAuctions()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    // Delete completed auctions
    for (BlackMarketEntryMap::iterator itr = _auctions.begin(); itr != _auctions.end();)
    {
        BlackMarketEntry* entry = itr->second;
        if (!entry->IsCompleted())
        {
            ++itr;
            continue;
        }

        entry->DeleteFromDB(trans);
        itr = _auctions.erase(itr);
        delete entry;
    }

    CharacterDatabase.CommitTransaction(trans);
    trans = CharacterDatabase.BeginTransaction();

    std::list<BlackMarketTemplate const*> templates;
    for (auto const& pair : _templates)
    {
        if (GetAuctionByID(pair.second->MarketID))
            continue;
        if (!roll_chance_f(pair.second->Chance))
            continue;

        templates.push_back(pair.second);
    }

    Acore::Containers::RandomResize(templates, sWorld->getIntConfig(CONFIG_BLACK_MARKET_MAX_AUCTIONS));

    for (BlackMarketTemplate const* templat : templates)
    {
        BlackMarketEntry* entry = new BlackMarketEntry();
        entry->Initialize(templat->MarketID, templat->Duration);
        entry->SaveToDB(trans);
        AddAuction(entry);
    }

    CharacterDatabase.CommitTransaction(trans);

    Update(true);
}

bool BlackMarketMgr::IsEnabled() const
{
    return sWorld->getBoolConfig(CONFIG_BLACK_MARKET_OPEN);
}

void BlackMarketMgr::BuildItemsResponse(WorldPackets::BlackMarket::BlackMarketRequestItemsResult& packet, Player* player)
{
    packet.LastUpdateID = _lastUpdate;
    packet.Items.reserve(_auctions.size());
    for (auto itr = _auctions.begin(); itr != _auctions.end(); ++itr)
    {
        WorldPackets::BlackMarket::BlackMarketItem item;
        item.Initialize(itr->second, player);

        packet.Items.push_back(item);
    }
}

void BlackMarketMgr::AddAuction(BlackMarketEntry* auction)
{
    _auctions[auction->GetMarketId()] = auction;
}

void BlackMarketMgr::AddTemplate(BlackMarketTemplate* templ)
{
    _templates[templ->MarketID] = templ;
}

void BlackMarketMgr::SendAuctionWonMail(BlackMarketEntry* entry, CharacterDatabaseTransaction trans)
{
    // Mail already sent
    if (entry->GetMailSent())
        return;

    uint32 bidderAccId = 0;
    ObjectGuid bidderGuid = ObjectGuid::Create<HighGuid::Player>(entry->GetBidder());
    Player* bidder = ObjectAccessor::FindConnectedPlayer(bidderGuid);
    // data for gm.log
    std::string bidderName;
    bool logGmTrade = false;

    if (bidder)
    {
        bidderAccId = bidder->GetSession()->GetAccountId();
        bidderName = bidder->GetName();
        logGmTrade = bidder->GetSession()->HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE);
    }
    else
    {
        bidderAccId = sCharacterCache->GetCharacterAccountIdByGuid(bidderGuid);
        if (!bidderAccId) // Account exists
            return;

        logGmTrade = AccountMgr::HasPermission(bidderAccId, rbac::RBAC_PERM_LOG_GM_TRADE, realm.Id.Realm);

        if (logGmTrade && !sCharacterCache->GetCharacterNameByGuid(bidderGuid, bidderName))
            bidderName = sObjectMgr->GetAcoreStringForDBCLocale(LANG_UNKNOWN);
    }

    // Create item
    BlackMarketTemplate const* templ = entry->GetTemplate();
    Item* item = Item::CreateItem(templ->Item.ItemID, templ->Quantity);
    if (!item)
        return;

    if (templ->Item.ItemBonus)
        for (int32 bonusList : templ->Item.ItemBonus->BonusListIDs)
            item->AddBonuses(bonusList);

    item->SetOwnerGUID(bidderGuid);

    item->SaveToDB(trans);

    // Log trade
    if (logGmTrade)
        sLog->outCommand(bidderAccId, "GM %s (Account: %u) won item in blackmarket auction: %s (Entry: %u Count: %u) and payed gold : %u.",
            bidderName.c_str(), bidderAccId, item->GetTemplate()->Name1, item->GetEntry(), item->GetCount(), entry->GetCurrentBid() / GOLD);

    if (bidder)
        bidder->GetSession()->SendBlackMarketWonNotification(entry, item);

    MailDraft(entry->BuildAuctionMailSubject(BMAH_AUCTION_WON), entry->BuildAuctionMailBody())
        .AddItem(item)
        .SendMailTo(trans, MailReceiver(bidder, entry->GetBidder()), entry, MAIL_CHECK_MASK_COPIED);

    entry->MailSent();
}

void BlackMarketMgr::SendAuctionOutbidMail(BlackMarketEntry* entry, CharacterDatabaseTransaction trans)
{
    ObjectGuid oldBidder_guid = ObjectGuid::Create<HighGuid::Player>(entry->GetBidder());
    Player* oldBidder = ObjectAccessor::FindConnectedPlayer(oldBidder_guid);

    uint32 oldBidder_accId = 0;
    if (!oldBidder)
        oldBidder_accId = sCharacterCache->GetCharacterAccountIdByGuid(oldBidder_guid);

    // old bidder exist
    if (!oldBidder && !oldBidder_accId)
        return;

    if (oldBidder)
        oldBidder->GetSession()->SendBlackMarketOutbidNotification(entry->GetTemplate());

    MailDraft(entry->BuildAuctionMailSubject(BMAH_AUCTION_OUTBID), entry->BuildAuctionMailBody())
        .AddMoney(entry->GetCurrentBid())
        .SendMailTo(trans, MailReceiver(oldBidder, entry->GetBidder()), entry, MAIL_CHECK_MASK_COPIED);
}

BlackMarketEntry* BlackMarketMgr::GetAuctionByID(int32 marketId) const
{
    BlackMarketEntryMap::const_iterator itr = _auctions.find(marketId);
    if (itr != _auctions.end())
        return itr->second;

    return nullptr;
}

BlackMarketTemplate const* BlackMarketMgr::GetTemplateByID(int32 marketId) const
{
    BlackMarketTemplateMap::const_iterator itr = _templates.find(marketId);
    if (itr != _templates.end())
        return itr->second;

    return nullptr;
}

bool BlackMarketTemplate::LoadFromDB(Field* fields)
{
    MarketID = fields[0].Get<int32>();
    SellerNPC = fields[1].Get<int32>();
    Item.ItemID = fields[2].Get<uint32>();
    Quantity = fields[3].Get<int32>();
    MinBid = fields[4].Get<uint64>();
    Duration = static_cast<time_t>(fields[5].Get<uint32>());
    Chance = fields[6].Get<float>();

    std::vector<int32> bonusListIDs;
    for (std::string_view token : Acore::Tokenize(fields[7].Get<std::string_view>(), ' ', false))
        if (Optional<int32> bonusListID = Acore::StringTo<int32>(token))
            bonusListIDs.push_back(*bonusListID);

    if (!bonusListIDs.empty())
    {
        Item.ItemBonus.emplace();
        Item.ItemBonus->BonusListIDs = bonusListIDs;
    }

    if (!sObjectMgr->GetCreatureTemplate(SellerNPC))
    {
        LOG_ERROR("misc", "Black market template {} does not have a valid seller. (Entry: {})", MarketID, SellerNPC);
        return false;
    }

    if (!sObjectMgr->GetItemTemplate(Item.ItemID))
    {
        LOG_ERROR("misc", "Black market template {} does not have a valid item. (Entry: {})", MarketID, Item.ItemID);
        return false;
    }

    return true;
}

void BlackMarketEntry::Update(time_t newTimeOfUpdate)
{
    _secondsRemaining = _secondsRemaining - (newTimeOfUpdate - sBlackMarketMgr->GetLastUpdate());
}

BlackMarketTemplate const* BlackMarketEntry::GetTemplate() const
{
    return sBlackMarketMgr->GetTemplateByID(_marketId);
}

uint32 BlackMarketEntry::GetSecondsRemaining() const
{
    return _secondsRemaining - (time(nullptr) - sBlackMarketMgr->GetLastUpdate());
}

time_t BlackMarketEntry::GetExpirationTime() const
{
    return time(nullptr) + GetSecondsRemaining();
}

bool BlackMarketEntry::IsCompleted() const
{
    return GetSecondsRemaining() <= 0;
}

bool BlackMarketEntry::LoadFromDB(Field* fields)
{
    _marketId = fields[0].Get<int32>();

    // Invalid MarketID
    BlackMarketTemplate const* templ = sBlackMarketMgr->GetTemplateByID(_marketId);
    if (!templ)
    {
        LOG_ERROR("misc", "Black market auction {} does not have a valid id.", _marketId);
        return false;
    }

    _currentBid = fields[1].Get<uint64>();
    _secondsRemaining =  static_cast<time_t>(fields[2].Get<int32>()) - sBlackMarketMgr->GetLastUpdate();
    _numBids = fields[3].Get<int32>();
    _bidder = fields[4].Get<uint64>();

    // Either no bidder or existing player
    if (_bidder && !sCharacterCache->GetCharacterAccountIdByGuid(ObjectGuid::Create<HighGuid::Player>(_bidder))) // Probably a better way to check if player exists
    {
        LOG_ERROR("misc", "Black market auction {} does not have a valid bidder (GUID: {} ).", _marketId, _bidder);
        return false;
    }

    return true;
}

void BlackMarketEntry::SaveToDB(CharacterDatabaseTransaction trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BLACKMARKET_AUCTIONS);

    stmt->SetData(0, _marketId);
    stmt->SetData(1, _currentBid);
    stmt->SetData(2, GetExpirationTime());
    stmt->SetData(3, _numBids);
    stmt->SetData(4, _bidder);

    trans->Append(stmt);
}

void BlackMarketEntry::DeleteFromDB(CharacterDatabaseTransaction trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_BLACKMARKET_AUCTIONS);
    stmt->SetData(0, _marketId);
    trans->Append(stmt);
}

bool BlackMarketEntry::ValidateBid(uint64 bid) const
{
    if (bid <= _currentBid)
        return false;

    if (bid < _currentBid + GetMinIncrement())
        return false;

    if (bid >= BMAH_MAX_BID)
        return false;

    return true;
}

void BlackMarketEntry::PlaceBid(uint64 bid, Player* player, CharacterDatabaseTransaction trans)   //Updated
{
    if (bid < _currentBid)
        return;

    _currentBid = bid;
    ++_numBids;

    if (GetSecondsRemaining() < 30 * MINUTE)
        _secondsRemaining += 30 * MINUTE;

    _bidder = player->GetGUID().GetCounter();

    player->ModifyMoney(-static_cast<int64>(bid));

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_BLACKMARKET_AUCTIONS);

    stmt->SetData(0, _currentBid);
    stmt->SetData(1, GetExpirationTime());
    stmt->SetData(2, _numBids);
    stmt->SetData(3, _bidder);
    stmt->SetData(4, _marketId);

    trans->Append(stmt);

    sBlackMarketMgr->Update(true);
}

std::string BlackMarketEntry::BuildAuctionMailSubject(BMAHMailAuctionAnswers response) const
{
    std::ostringstream strm;
    strm << GetTemplate()->Item.ItemID << ":0:" << response << ':' << GetMarketId() << ':' << GetTemplate()->Quantity;
    return strm.str();
}

std::string BlackMarketEntry::BuildAuctionMailBody()
{
    std::ostringstream strm;
    strm << GetTemplate()->SellerNPC << ':' << _currentBid;

    return strm.str();
}
