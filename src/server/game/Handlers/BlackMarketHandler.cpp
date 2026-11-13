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
#include "Creature.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// The Black Market Auction House, 5.4.8 18414, from Project SkyFire

void WorldSession::HandleBlackMarketHelloOpcode(WorldPacket& recvData)
{
    ObjectGuid npcGuid;
    recvData.ReadGuidMask(npcGuid, { 4, 5, 2, 7, 0, 1, 3, 6 });
    recvData.ReadGuidBytes(npcGuid, { 4, 3, 0, 6, 2, 7, 5, 1 });

    if (!GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_BLACK_MARKET))
    {
        LOG_DEBUG("network", "WORLD: HandleBlackMarketHelloOpcode - {} not found or you can not interact with him.", npcGuid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendBlackMarketHello(npcGuid);
}

void WorldSession::SendBlackMarketHello(ObjectGuid npcGuid)
{
    WorldPacket data(SMSG_BLACKMARKET_HELLO, 9);
    data.WriteGuidMask(npcGuid, { 2, 0, 4, 1, 3, 6, 5, 7 });
    data.WriteBit(sBlackMarketMgr->IsOpen());
    data.FlushBits();
    data.WriteGuidBytes(npcGuid, { 6, 1, 2, 5, 0, 7, 4, 3 });
    SendPacket(&data);
}

void WorldSession::HandleBlackMarketRequestItemsOpcode(WorldPacket& recvData)
{
    ObjectGuid npcGuid;
    uint32 lastUpdateId;
    recvData >> lastUpdateId;
    recvData.ReadGuidMask(npcGuid, { 2, 6, 0, 3, 4, 5, 1, 7 });
    recvData.ReadGuidBytes(npcGuid, { 6, 2, 3, 5, 7, 4, 1, 0 });

    if (!GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_BLACK_MARKET))
        return;

    SendBlackMarketItemsResult();
}

void WorldSession::SendBlackMarketItemsResult()
{
    WorldPacket data(SMSG_BLACKMARKET_REQUEST_ITEMS_RESULT, 4 + 3 + 64);
    sBlackMarketMgr->BuildItemsResult(data, GetPlayer()->GetGUID().GetCounter());
    SendPacket(&data);
}

void WorldSession::HandleBlackMarketBidOpcode(WorldPacket& recvData)
{
    ObjectGuid npcGuid;
    uint32 itemId;
    uint32 auctionId;
    uint64 bid;

    recvData >> itemId >> auctionId >> bid;
    recvData.ReadGuidMask(npcGuid, { 0, 5, 4, 3, 7, 6, 1, 2 });
    recvData.ReadGuidBytes(npcGuid, { 4, 3, 6, 5, 7, 1, 0, 2 });

    LOG_DEBUG("network", "CMSG_BLACKMARKET_BID {} auction: {}, item: {}, bid: {}", GetPlayerInfo(), auctionId, itemId, bid);

    if (!bid || !sBlackMarketMgr->IsOpen())
        return;

    if (!GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_BLACK_MARKET))
        return;

    BlackMarketAuction* auction = sBlackMarketMgr->GetAuction(auctionId);
    if (!auction || !auction->IsActive() || auction->IsExpired() || auction->GetTemplate()->ItemEntry != itemId)
        return;

    // already the highest bidder
    if (auction->GetCurrentBidder() == GetPlayer()->GetGUID().GetCounter())
        return;

    if (bid < auction->GetRequiredBid() || bid > uint64(MAX_MONEY_AMOUNT))
        return;

    if (!GetPlayer()->HasEnoughMoney(bid))
        return;

    GetPlayer()->ModifyMoney(-int64(bid));
    sBlackMarketMgr->PlaceBid(auction, GetPlayer(), bid);

    WorldPacket data(SMSG_BLACKMARKET_BID_RESULT, 12);
    data << uint32(2);                                      // values as sent by SkyFire, meaning of the 2s not known
    data << uint32(itemId);
    data << uint32(2);
    SendPacket(&data);

    SendBlackMarketItemsResult();
}
