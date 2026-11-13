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
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "Timer.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <sstream>
#include <vector>

// The Black Market Auction House, 5.4.8 18414, from Project SkyFire

BlackMarketTemplate const* BlackMarketAuction::GetTemplate() const
{
    return sBlackMarketMgr->GetTemplate(_templateId);
}

uint32 BlackMarketAuction::GetEndTime() const
{
    BlackMarketTemplate const* bmTemplate = GetTemplate();
    return _startTime + (bmTemplate ? bmTemplate->Duration : 0);
}

uint32 BlackMarketAuction::GetSecondsRemaining() const
{
    uint32 endTime = GetEndTime();
    uint32 now = uint32(GameTime::GetGameTime().count());
    return endTime > now ? endTime - now : 0;
}

bool BlackMarketAuction::IsActive() const
{
    return uint32(GameTime::GetGameTime().count()) >= _startTime;
}

bool BlackMarketAuction::IsExpired() const
{
    return GetEndTime() < uint32(GameTime::GetGameTime().count());
}

void BlackMarketAuction::PlaceBid(ObjectGuid::LowType bidder, uint64 bid)
{
    _currentBidder = bidder;
    _currentBid = bid;
    _minIncrement = bid / 20;                               // 5%
    ++_numBids;
}

void BlackMarketAuction::SaveToDB(CharacterDatabaseTransaction trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BLACKMARKET_AUCTION);
    stmt->SetData(0, _auctionId);
    stmt->SetData(1, _templateId);
    stmt->SetData(2, _startTime);
    stmt->SetData(3, _currentBidder);
    stmt->SetData(4, _currentBid);
    stmt->SetData(5, _minIncrement);
    stmt->SetData(6, _numBids);
    trans->Append(stmt);
}

void BlackMarketAuction::UpdateToDB(CharacterDatabaseTransaction trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_BLACKMARKET_AUCTION);
    stmt->SetData(0, _currentBidder);
    stmt->SetData(1, _currentBid);
    stmt->SetData(2, _minIncrement);
    stmt->SetData(3, _numBids);
    stmt->SetData(4, _auctionId);
    trans->Append(stmt);
}

void BlackMarketAuction::DeleteFromDB(CharacterDatabaseTransaction trans) const
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_BLACKMARKET_AUCTION);
    stmt->SetData(0, _auctionId);
    trans->Append(stmt);
}

std::string BlackMarketAuction::BuildMailSubject(BlackMarketMailResponse response) const
{
    BlackMarketTemplate const* bmTemplate = GetTemplate();
    std::ostringstream strm;
    strm << bmTemplate->ItemEntry << ":0:" << uint32(response) << ':' << _auctionId << ':' << bmTemplate->Quantity;
    return strm.str();
}

std::string BlackMarketAuction::BuildMailBody(ObjectGuid::LowType lowGuid) const
{
    std::ostringstream strm;
    strm.width(16);
    strm << std::right << std::hex << ObjectGuid::Create<HighGuid::Player>(lowGuid).GetRawValue();   // HIGHGUID_PLAYER always present, even for empty guids
    strm << std::dec << ':' << _currentBid << ':' << 0;
    strm << ':' << 0 << ':' << 0;
    return strm.str();
}

BlackMarketMgr::~BlackMarketMgr()
{
    for (auto const& [auctionId, auction] : _auctions)
        delete auction;
}

BlackMarketMgr* BlackMarketMgr::instance()
{
    static BlackMarketMgr instance;
    return &instance;
}

bool BlackMarketMgr::IsOpen() const
{
    return sWorld->getBoolConfig(CONFIG_BLACK_MARKET_OPEN);
}

void BlackMarketMgr::LoadTemplates()
{
    uint32 oldMSTime = getMSTime();
    _templates.clear();

    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_BLACKMARKET_TEMPLATE));
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 Black Market templates. DB table `blackmarket_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        BlackMarketTemplate bmTemplate;
        bmTemplate.Id = fields[0].Get<uint32>();
        bmTemplate.MarketId = fields[1].Get<uint32>();
        bmTemplate.SellerNpcEntry = fields[2].Get<uint32>();
        bmTemplate.ItemEntry = fields[3].Get<uint32>();
        bmTemplate.Quantity = fields[4].Get<uint32>();
        bmTemplate.MinBid = fields[5].Get<uint64>();
        bmTemplate.Duration = fields[6].Get<uint32>();
        bmTemplate.Chance = fields[7].Get<float>();

        if (!sObjectMgr->GetCreatureTemplate(bmTemplate.SellerNpcEntry))
        {
            LOG_ERROR("sql.sql", "Table `blackmarket_template` (Id: {}) has a non-existing seller creature (Entry: {}), skipped.", bmTemplate.Id, bmTemplate.SellerNpcEntry);
            continue;
        }

        if (!sObjectMgr->GetItemTemplate(bmTemplate.ItemEntry))
        {
            LOG_ERROR("sql.sql", "Table `blackmarket_template` (Id: {}) has a non-existing item (Entry: {}), skipped.", bmTemplate.Id, bmTemplate.ItemEntry);
            continue;
        }

        if (!bmTemplate.Quantity)
        {
            LOG_ERROR("sql.sql", "Table `blackmarket_template` (Id: {}) has Quantity 0 for item {}, skipped.", bmTemplate.Id, bmTemplate.ItemEntry);
            continue;
        }

        _templates[bmTemplate.Id] = bmTemplate;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Black Market templates in {} ms", _templates.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void BlackMarketMgr::LoadAuctions()
{
    uint32 oldMSTime = getMSTime();

    for (auto const& [auctionId, auction] : _auctions)
        delete auction;
    _auctions.clear();

    PreparedQueryResult result = CharacterDatabase.Query(CharacterDatabase.GetPreparedStatement(CHAR_SEL_BLACKMARKET_AUCTIONS));
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 Black Market auctions. DB table `blackmarket_auctions` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    do
    {
        Field* fields = result->Fetch();

        BlackMarketAuction* auction = new BlackMarketAuction(fields[0].Get<uint32>(), fields[1].Get<uint32>(), fields[2].Get<uint32>(),
            fields[3].Get<uint32>(), fields[4].Get<uint64>(), fields[5].Get<uint64>(), fields[6].Get<uint32>());

        if (!auction->GetTemplate())
        {
            auction->DeleteFromDB(trans);
            delete auction;
            continue;
        }

        _auctions[auction->GetAuctionId()] = auction;
    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    LOG_INFO("server.loading", ">> Loaded {} Black Market auctions in {} ms", _auctions.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void BlackMarketMgr::Update()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Close the auctions that ran out
    for (auto itr = _auctions.begin(); itr != _auctions.end();)
    {
        BlackMarketAuction* auction = itr->second;
        if (!auction->IsExpired())
        {
            ++itr;
            continue;
        }

        if (auction->GetCurrentBidder())
            SendAuctionWon(auction, trans);

        auction->DeleteFromDB(trans);
        delete auction;
        itr = _auctions.erase(itr);
    }

    // Fill the market up again
    int32 missing = int32(sWorld->getIntConfig(CONFIG_BLACK_MARKET_MAX_AUCTIONS)) - int32(_auctions.size());
    if (missing > 0)
        CreateAuctions(uint32(missing), trans);

    CharacterDatabase.CommitTransaction(trans);
}

BlackMarketTemplate const* BlackMarketMgr::GetTemplate(uint32 templateId) const
{
    auto itr = _templates.find(templateId);
    return itr != _templates.end() ? &itr->second : nullptr;
}

BlackMarketAuction* BlackMarketMgr::GetAuction(uint32 auctionId) const
{
    auto itr = _auctions.find(auctionId);
    return itr != _auctions.end() ? itr->second : nullptr;
}

uint32 BlackMarketMgr::GetFreeAuctionId() const
{
    uint32 newId = 1;
    while (_auctions.count(newId))
        ++newId;
    return newId;
}

void BlackMarketMgr::CreateAuctions(uint32 count, CharacterDatabaseTransaction trans)
{
    if (_templates.empty())
        return;

    uint32 delay = sWorld->getIntConfig(CONFIG_BLACK_MARKET_AUCTION_DELAY);
    uint32 delayMod = sWorld->getIntConfig(CONFIG_BLACK_MARKET_AUCTION_DELAY_MOD);

    for (uint32 i = 0; i < count; ++i)
    {
        // Pick a template that passed its chance and is not on sale already
        uint32 roll = urand(1, 100);
        std::vector<uint32> candidates;
        for (auto const& [templateId, bmTemplate] : _templates)
        {
            if (bmTemplate.Chance < float(roll))
                continue;

            bool onSale = false;
            for (auto const& [auctionId, auction] : _auctions)
            {
                if (auction->GetTemplateId() == templateId)
                {
                    onSale = true;
                    break;
                }
            }

            if (!onSale)
                candidates.push_back(templateId);
        }

        if (candidates.empty())
            continue;

        BlackMarketTemplate const* bmTemplate = GetTemplate(Acore::Containers::SelectRandomContainerElement(candidates));
        if (!bmTemplate)
            continue;

        uint32 startTime = uint32(GameTime::GetGameTime().count()) + delay + urand(0, delayMod * 2) - delayMod / 2;

        BlackMarketAuction* auction = new BlackMarketAuction(GetFreeAuctionId(), bmTemplate->Id, startTime, 0, bmTemplate->MinBid, bmTemplate->MinBid / 20, 0);
        _auctions[auction->GetAuctionId()] = auction;
        auction->SaveToDB(trans);
    }
}

void BlackMarketMgr::PlaceBid(BlackMarketAuction* auction, Player* bidder, uint64 bid)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // the previous bidder gets the money back by mail
    if (auction->GetCurrentBidder())
        SendAuctionOutbid(auction, trans);

    auction->PlaceBid(bidder->GetGUID().GetCounter(), bid);
    auction->UpdateToDB(trans);

    CharacterDatabase.CommitTransaction(trans);
}

void BlackMarketMgr::BuildItemsResult(WorldPacket& data, ObjectGuid::LowType playerLowGuid) const
{
    uint32 count = 0;
    for (auto const& [auctionId, auction] : _auctions)
        if (auction->IsActive())
            ++count;

    data << uint32(1);                                      // last update id
    data.WriteBits(count, 18);
    for (auto const& [auctionId, auction] : _auctions)
        if (auction->IsActive())
            data.WriteBit(auction->GetCurrentBidder() == playerLowGuid);   // highest bidder
    data.FlushBits();

    for (auto const& [auctionId, auction] : _auctions)
    {
        if (!auction->IsActive())
            continue;

        BlackMarketTemplate const* bmTemplate = auction->GetTemplate();
        data << uint32(bmTemplate->MarketId);
        data << uint32(bmTemplate->Quantity);
        data << uint32(auction->GetAuctionId());
        data << uint32(auction->GetNumBids());
        data << uint32(bmTemplate->ItemEntry);
        data << uint64(bmTemplate->MinBid);
        data << uint64(auction->GetCurrentBid());
        data << uint64(auction->GetMinIncrement());
        data << uint32(bmTemplate->SellerNpcEntry);
        data << uint32(auction->GetSecondsRemaining());
    }
}

void BlackMarketMgr::SendAuctionOutbid(BlackMarketAuction const* auction, CharacterDatabaseTransaction trans)
{
    ObjectGuid bidderGuid = ObjectGuid::Create<HighGuid::Player>(auction->GetCurrentBidder());
    Player* bidder = ObjectAccessor::FindConnectedPlayer(bidderGuid);
    if (!bidder && !sCharacterCache->GetCharacterAccountIdByGuid(bidderGuid))
        return;

    BlackMarketTemplate const* bmTemplate = auction->GetTemplate();
    if (bidder)
    {
        WorldPacket data(SMSG_BLACKMARKET_OUT_BID, 12);
        data << uint32(bmTemplate->ItemEntry);
        data << uint32(1);
        data << uint32(1);
        bidder->SendDirectMessage(&data);
    }

    MailDraft(auction->BuildMailSubject(BlackMarketMailResponse::Outbid), auction->BuildMailBody(0))
        .AddMoney(auction->GetCurrentBid())
        .SendMailTo(trans, MailReceiver(bidder, auction->GetCurrentBidder()), MailSender(MAIL_BLACKMARKET, bmTemplate->SellerNpcEntry, MAIL_STATIONERY_AUCTION), MAIL_CHECK_MASK_COPIED);
}

void BlackMarketMgr::SendAuctionWon(BlackMarketAuction const* auction, CharacterDatabaseTransaction trans)
{
    ObjectGuid bidderGuid = ObjectGuid::Create<HighGuid::Player>(auction->GetCurrentBidder());
    Player* bidder = ObjectAccessor::FindConnectedPlayer(bidderGuid);
    if (!bidder && !sCharacterCache->GetCharacterAccountIdByGuid(bidderGuid))
        return;

    BlackMarketTemplate const* bmTemplate = auction->GetTemplate();
    if (bidder)
    {
        WorldPacket data(SMSG_BLACKMARKET_BID_WON, 12);
        data << uint32(1);
        data << uint32(bmTemplate->ItemEntry);
        data << uint32(1);
        bidder->SendDirectMessage(&data);
    }

    MailDraft draft(auction->BuildMailSubject(BlackMarketMailResponse::Won), auction->BuildMailBody(auction->GetCurrentBidder()));
    if (Item* item = Item::CreateItem(bmTemplate->ItemEntry, bmTemplate->Quantity, bidder))
    {
        item->SaveToDB(trans);
        draft.AddItem(item);
    }

    draft.SendMailTo(trans, MailReceiver(bidder, auction->GetCurrentBidder()), MailSender(MAIL_BLACKMARKET, bmTemplate->SellerNpcEntry, MAIL_STATIONERY_AUCTION), MAIL_CHECK_MASK_COPIED);
}
