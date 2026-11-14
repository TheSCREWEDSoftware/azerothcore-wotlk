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

#include "LootPackets.h"

void WorldPackets::Loot::LootUnit::Read()
{
    _worldPacket >> Unit;
}

WorldPacket const* WorldPackets::Loot::LootResponse::Write()
{
    _worldPacket << LootObj;
    _worldPacket << Owner;
    _worldPacket << FailureReason;
    _worldPacket << AcquireReason;
    _worldPacket << LootMethod;
    _worldPacket << Threshold;
    _worldPacket << Coins;
    _worldPacket << uint32(Items.size());
    _worldPacket << uint32(Currencies.size());

    for (LootItemData const& item : Items)
    {
        _worldPacket.WriteBits(item.Type, 2);
        _worldPacket.WriteBits(item.UIType, 3);
        _worldPacket.WriteBit(item.CanTradeToTapList);
        _worldPacket.FlushBits();

        _worldPacket << item.Quantity;
        _worldPacket << item.LootItemType;
        _worldPacket << item.LootListID;
        _worldPacket << item.Loot; // WorldPackets::Item::ItemInstance
    }

    for (LootCurrency const& currency : Currencies)
    {
        _worldPacket << currency.CurrencyID;
        _worldPacket << currency.Quantity;
        _worldPacket << currency.LootListID;
        _worldPacket.WriteBits(currency.UIType, 3);
        _worldPacket.FlushBits();
    }

    _worldPacket.WriteBit(Acquired);
    _worldPacket.WriteBit(AELooting);
    _worldPacket.WriteBit(PersonalLooting);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void WorldPackets::Loot::LootItem::Read()
{
    uint32 Count;
    _worldPacket >> Count;

    Loot.resize(Count);
    for (uint32 i = 0; i < Count; ++i)
    {
        _worldPacket >> Loot[i].Object;
        _worldPacket >> Loot[i].LootListID;
    }
}

WorldPacket const* WorldPackets::Loot::LootRemoved::Write()
{
    _worldPacket << Owner;
    _worldPacket << LootObj;
    _worldPacket << LootListID;

    return &_worldPacket;
}

void WorldPackets::Loot::LootRelease::Read()
{
    _worldPacket >> Unit;
}

WorldPacket const* WorldPackets::Loot::LootMoneyNotify::Write()
{
    _worldPacket << Money;
    _worldPacket.WriteBit(SoleLooter);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Loot::CoinRemoved::Write()
{
    _worldPacket << LootObj;

    return &_worldPacket;
}

void WorldPackets::Loot::LootRoll::Read()
{
    _worldPacket >> LootObj;
    _worldPacket >> LootListID;
    _worldPacket >> RollType;
}

WorldPacket const* WorldPackets::Loot::LootReleaseResponse::Write()
{
    _worldPacket << LootObj;
    _worldPacket << Owner;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Loot::LootList::Write()
{
    _worldPacket << Owner;

    _worldPacket.WriteBit(Master.has_value());
    _worldPacket.WriteBit(RoundRobinWinner.has_value());

    _worldPacket.FlushBits();

    if (Master)
        _worldPacket << *Master;

    if (RoundRobinWinner)
        _worldPacket << *RoundRobinWinner;

    return &_worldPacket;
}

void WorldPackets::Loot::SetLootSpecialization::Read()
{
    _worldPacket >> SpecID;
}

static void WriteLootItemData(ByteBuffer& data, WorldPackets::Loot::LootItemData const& item)
{
    data.WriteBits(item.Type, 2);
    data.WriteBits(item.UIType, 3);
    data.WriteBit(item.CanTradeToTapList);
    data.FlushBits();
    data << uint32(item.Quantity);
    data << uint8(item.LootItemType);
    data << uint8(item.LootListID);
    data << item.Loot; // WorldPackets::Item::ItemInstance
}

WorldPacket const* WorldPackets::Loot::StartLootRoll::Write()
{
    _worldPacket << LootObj;
    _worldPacket << int32(MapID);
    WriteLootItemData(_worldPacket, Item);
    _worldPacket << uint32(RollTime);
    _worldPacket << uint8(ValidRolls);
    _worldPacket << uint8(Method);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Loot::LootRollBroadcast::Write()
{
    _worldPacket << LootObj;
    _worldPacket << Player;
    WriteLootItemData(_worldPacket, Item);
    _worldPacket << int32(Roll);
    _worldPacket << uint8(RollType);
    _worldPacket.WriteBit(Autopassed);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Loot::LootRollWon::Write()
{
    _worldPacket << LootObj;
    WriteLootItemData(_worldPacket, Item);
    _worldPacket << Winner;
    _worldPacket << int32(Roll);
    _worldPacket << uint8(RollType);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Loot::LootAllPassed::Write()
{
    _worldPacket << LootObj;
    WriteLootItemData(_worldPacket, Item);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Loot::LootRollsComplete::Write()
{
    _worldPacket << LootObj;
    _worldPacket << uint8(LootListID);

    return &_worldPacket;
}

void WorldPackets::Loot::MasterLootItem::Read()
{
    uint32 count;
    _worldPacket >> count;
    _worldPacket >> Target;

    if (count > 1000)
        throw PacketArrayMaxCapacityException(count, 1000);

    Loot.resize(count);
    for (LootRequest& request : Loot)
    {
        _worldPacket >> request.Object;
        _worldPacket >> request.LootListID;
    }
}

WorldPacket const* WorldPackets::Loot::MasterLootCandidateList::Write()
{
    _worldPacket << LootObj;
    _worldPacket << uint32(Players.size());
    for (ObjectGuid const& player : Players)
        _worldPacket << player;

    return &_worldPacket;
}
