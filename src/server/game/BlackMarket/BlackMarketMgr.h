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

#ifndef BlackMarketMgr_h__
#define BlackMarketMgr_h__

#include "DatabaseEnvFwd.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <map>
#include <string>

class Player;
class WorldPacket;

// The Black Market Auction House, 5.4.8 18414, from Project SkyFire
enum class BlackMarketMailResponse : uint32
{
    Outbid = 0,
    Won    = 1
};

struct BlackMarketTemplate
{
    uint32 Id = 0;
    uint32 MarketId = 0;
    uint32 SellerNpcEntry = 0;
    uint32 ItemEntry = 0;
    uint32 Quantity = 0;
    uint64 MinBid = 0;
    uint32 Duration = 0;
    float Chance = 0.0f;
};

class BlackMarketAuction
{
public:
    BlackMarketAuction(uint32 auctionId, uint32 templateId, uint32 startTime, ObjectGuid::LowType currentBidder, uint64 currentBid, uint64 minIncrement, uint32 numBids)
        : _auctionId(auctionId), _templateId(templateId), _startTime(startTime), _currentBidder(currentBidder), _currentBid(currentBid),
        _minIncrement(minIncrement), _numBids(numBids) { }

    uint32 GetAuctionId() const { return _auctionId; }
    uint32 GetTemplateId() const { return _templateId; }
    uint32 GetStartTime() const { return _startTime; }
    ObjectGuid::LowType GetCurrentBidder() const { return _currentBidder; }
    uint64 GetCurrentBid() const { return _currentBid; }
    uint64 GetMinIncrement() const { return _minIncrement; }
    uint32 GetNumBids() const { return _numBids; }

    BlackMarketTemplate const* GetTemplate() const;

    uint32 GetEndTime() const;
    uint32 GetSecondsRemaining() const;
    bool IsActive() const;
    bool IsExpired() const;

    // The bid a new bidder has to reach: the starting price for the first bid, then the current bid plus the increment
    uint64 GetRequiredBid() const { return _numBids ? _currentBid + _minIncrement : _currentBid; }
    void PlaceBid(ObjectGuid::LowType bidder, uint64 bid);

    void SaveToDB(CharacterDatabaseTransaction trans) const;
    void UpdateToDB(CharacterDatabaseTransaction trans) const;
    void DeleteFromDB(CharacterDatabaseTransaction trans) const;

    std::string BuildMailSubject(BlackMarketMailResponse response) const;
    std::string BuildMailBody(ObjectGuid::LowType lowGuid) const;

private:
    uint32 _auctionId;
    uint32 _templateId;
    uint32 _startTime;
    ObjectGuid::LowType _currentBidder;
    uint64 _currentBid;
    uint64 _minIncrement;
    uint32 _numBids;
};

class BlackMarketMgr
{
    BlackMarketMgr() = default;
    ~BlackMarketMgr();

public:
    BlackMarketMgr(BlackMarketMgr const&) = delete;
    BlackMarketMgr& operator=(BlackMarketMgr const&) = delete;

    static BlackMarketMgr* instance();

    void LoadTemplates();
    void LoadAuctions();
    void Update();

    bool IsOpen() const;

    BlackMarketTemplate const* GetTemplate(uint32 templateId) const;
    BlackMarketAuction* GetAuction(uint32 auctionId) const;

    void PlaceBid(BlackMarketAuction* auction, Player* bidder, uint64 bid);
    void BuildItemsResult(WorldPacket& data, ObjectGuid::LowType playerLowGuid) const;

private:
    uint32 GetFreeAuctionId() const;
    void CreateAuctions(uint32 count, CharacterDatabaseTransaction trans);
    void SendAuctionWon(BlackMarketAuction const* auction, CharacterDatabaseTransaction trans);
    void SendAuctionOutbid(BlackMarketAuction const* auction, CharacterDatabaseTransaction trans);

    std::map<uint32, BlackMarketTemplate> _templates;
    std::map<uint32, BlackMarketAuction*> _auctions;
};

#define sBlackMarketMgr BlackMarketMgr::instance()

#endif // BlackMarketMgr_h__
