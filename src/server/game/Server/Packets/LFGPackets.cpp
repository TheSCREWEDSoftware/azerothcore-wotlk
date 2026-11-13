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

#include "LFGPackets.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"

// 5.4.8 18414 layouts, from Project SkyFire
void WorldPackets::LFG::LFGJoin::Read()
{
    _worldPacket >> PartyIndex;
    for (uint32& needs : Needs) // Needs count, hardcoded to 3 in client
        _worldPacket >> needs;

    _worldPacket >> Roles;
    Slots.resize(_worldPacket.ReadBits(22));
    uint32 commentLength = _worldPacket.ReadBits(8);
    QueueAsGroup = _worldPacket.ReadBit();

    for (uint32& slot : Slots)
        _worldPacket >> slot;

    Comment = _worldPacket.ReadString(commentLength);
}

void WorldPackets::LFG::LFGLeave::Read()
{
    _worldPacket >> Roles;
    _worldPacket >> Ticket.Time;
    uint32 type = 0;
    _worldPacket >> type;
    Ticket.Type = RideType(type);
    _worldPacket >> Ticket.Id;

    _worldPacket.ReadGuidMask(Ticket.RequesterGuid, { 6, 0, 2, 3, 1, 5, 4, 7 });
    _worldPacket.ReadGuidBytes(Ticket.RequesterGuid, { 2, 0, 4, 6, 3, 1, 5, 7 });
}

void WorldPackets::LFG::LFGProposalResponse::Read()
{
    _worldPacket >> ProposalID;
    _worldPacket >> Ticket.Id;
    uint32 type = 0;
    _worldPacket >> type;
    Ticket.Type = RideType(type);
    _worldPacket >> Ticket.Time;

    ObjectGuid instanceId;
    Accepted = _worldPacket.ReadBit();
    _worldPacket.ReadGuidMask(Ticket.RequesterGuid, { 6, 0, 2, 4 });
    _worldPacket.ReadGuidMask(instanceId, { 6, 7 });
    _worldPacket.ReadGuidMask(Ticket.RequesterGuid, { 3 });
    _worldPacket.ReadGuidMask(instanceId, { 4 });
    _worldPacket.ReadGuidMask(Ticket.RequesterGuid, { 7 });
    _worldPacket.ReadGuidMask(instanceId, { 1 });
    _worldPacket.ReadGuidMask(Ticket.RequesterGuid, { 5 });
    _worldPacket.ReadGuidMask(instanceId, { 0 });
    _worldPacket.ReadGuidMask(Ticket.RequesterGuid, { 1 });
    _worldPacket.ReadGuidMask(instanceId, { 2, 3, 5 });

    _worldPacket.ReadGuidBytes(Ticket.RequesterGuid, { 3, 6, 4, 1 });
    _worldPacket.ReadGuidBytes(instanceId, { 7, 0 });
    _worldPacket.ReadGuidBytes(Ticket.RequesterGuid, { 7 });
    _worldPacket.ReadGuidBytes(instanceId, { 6 });
    _worldPacket.ReadGuidBytes(Ticket.RequesterGuid, { 5 });
    _worldPacket.ReadGuidBytes(instanceId, { 3, 1, 5, 4 });
    _worldPacket.ReadGuidBytes(Ticket.RequesterGuid, { 0, 2 });
    _worldPacket.ReadGuidBytes(instanceId, { 2 });
    InstanceID = instanceId.GetRawValue();
}

void WorldPackets::LFG::LFGSetRoles::Read()
{
    _worldPacket >> RolesDesired;
    _worldPacket >> PartyIndex;
}

void WorldPackets::LFG::LFGSetComment::Read()
{
    _worldPacket >> Comment;
}

void WorldPackets::LFG::LFGBootPlayerVote::Read()
{
    _worldPacket >> Vote;
}

void WorldPackets::LFG::LFGTeleport::Read()
{
    // The direction is one bit after the party index; a few clients send it as a single byte.
    if (_worldPacket.size() >= 2)
    {
        _worldPacket.read_skip<uint8>();
        TeleportOut = _worldPacket.ReadBit();
    }
    else if (_worldPacket.size() == 1)
        TeleportOut = _worldPacket.read<uint8>() != 0;
}

void WorldPackets::LFG::LFGGetSystemInfo::Read()
{
    _worldPacket >> PartyIndex;
    Player = _worldPacket.ReadBit();
}

namespace
{
    void WriteLockBlock(ByteBuffer& data, std::vector<WorldPackets::LFG::LFGBlackListSlot> const& slots)
    {
        for (WorldPackets::LFG::LFGBlackListSlot const& slot : slots)
        {
            data << uint32(slot.Slot);
            data << uint32(slot.Reason);
            data << int32(slot.SubReason1);
            data << int32(slot.SubReason2);
        }
    }

    void WriteRewardBits(ByteBuffer& data, WorldPackets::LFG::LfgPlayerDungeonInfo const& info, uint32 shortageCount)
    {
        data.WriteBit(info.FirstReward);
        data.WriteBit(info.ShortageEligible);
        data.WriteBits(0, 21);                                  // bonus currency count
        data.WriteBits(shortageCount, 19);
        data.WriteBits(info.Rewards.Item.size(), 20);

        for (uint32 i = 0; i < shortageCount; ++i)
        {
            data.WriteBits(0, 21);                              // shortage currency count
            data.WriteBits(0, 20);                              // shortage item count
            data.WriteBits(0, 21);                              // shortage bonus currency count
        }

        data.WriteBits(info.Rewards.Currency.size(), 21);
    }

    void WriteRewardData(ByteBuffer& data, WorldPackets::LFG::LfgPlayerDungeonInfo const& info, uint32 shortageCount)
    {
        data << int32(info.Rewards.RewardXP);

        for (uint32 i = 0; i < shortageCount; ++i)
        {
            data << int32(i == 0 ? info.ShortageReward.RewardMoney : 0);
            data << uint32(info.ShortageRoleMask);
            data << int32(i == 0 ? info.ShortageReward.RewardXP : 0);
        }

        data << uint32(info.SpecificQuantity);
        data << uint32(info.PurseLimit);
        data << int32(info.Rewards.RewardMoney);

        for (WorldPackets::LFG::LfgPlayerQuestRewardItem const& item : info.Rewards.Item)
        {
            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item.ItemID);
            data << int32(item.Quantity);
            data << int32(item.ItemID);
            data << int32(itemTemplate ? itemTemplate->DisplayInfoID : 0);
        }

        data << uint32(info.OverallQuantity);
        data << uint32(info.PurseWeeklyQuantity);
        data << uint32(info.OverallLimit);
        data << uint32(info.Quantity);
        data << uint32(info.CompletionCurrencyID);
        data << uint32(info.Slot);

        for (WorldPackets::LFG::LfgPlayerQuestRewardCurrency const& currency : info.Rewards.Currency)
        {
            data << int32(currency.CurrencyID);
            data << int32(currency.Quantity);
        }

        data << uint32(info.PurseWeeklyLimit);
        data << uint32(0);                                      // mask
        data << uint32(info.PurseQuantity);
        data << uint32(info.CompletionLimit);
        data << uint32(info.SpecificLimit);
        data << uint32(info.CompletedMask);
        data << uint32(info.CompletionQuantity);
    }
}

WorldPacket const* WorldPackets::LFG::LFGPlayerInfo::Write()
{
    ObjectGuid playerGuid = BlackList.PlayerGuid.value_or(ObjectGuid::Empty);

    _worldPacket.WriteBits(BlackList.Slot.size(), 20);
    _worldPacket.WriteBit(BlackList.PlayerGuid.has_value());
    _worldPacket.WriteBits(Dungeon.size(), 17);

    ByteBuffer rewardData;
    for (LfgPlayerDungeonInfo const& playerDungeonInfo : Dungeon)
    {
        uint32 shortageCount = playerDungeonInfo.ShortageEligible ? 3 : 0;
        WriteRewardBits(_worldPacket, playerDungeonInfo, shortageCount);
        WriteRewardData(rewardData, playerDungeonInfo, shortageCount);
    }

    if (BlackList.PlayerGuid)
        _worldPacket.WriteGuidMask(playerGuid, { 5, 1, 2, 7, 3, 0, 6, 4 });

    _worldPacket.FlushBits();

    if (BlackList.PlayerGuid)
        _worldPacket.WriteGuidBytes(playerGuid, { 7, 2, 3, 0, 4, 5, 6, 1 });

    _worldPacket.append(rewardData);
    WriteLockBlock(_worldPacket, BlackList.Slot);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGPartyInfo::Write()
{
    _worldPacket.WriteBits(Player.size(), 22);
    for (WorldPackets::LFG::LFGBlackList const& player : Player)
    {
        ObjectGuid playerGuid = player.PlayerGuid.value_or(ObjectGuid::Empty);
        _worldPacket.WriteBit(player.PlayerGuid.has_value());
        _worldPacket.WriteGuidMask(playerGuid, { 3, 6, 0, 5, 2, 7, 4, 1 });
        _worldPacket.WriteBits(player.Slot.size(), 20);
        _worldPacket.FlushBits();

        for (WorldPackets::LFG::LFGBlackListSlot const& slot : player.Slot)
        {
            _worldPacket << int32(slot.SubReason1);
            _worldPacket << uint32(slot.Slot);
            _worldPacket << int32(slot.SubReason2);
            _worldPacket << uint32(slot.Reason);
        }

        _worldPacket.WriteGuidBytes(playerGuid, { 0, 3, 1, 4, 6, 2, 5, 7 });
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGUpdateStatus::Write()
{
    ObjectGuid const& guid = Ticket.RequesterGuid;

    _worldPacket.WriteBits(Comment.length(), 8);
    _worldPacket.WriteBit(IsParty);
    _worldPacket.WriteBit(Joined);
    _worldPacket.WriteBits(Slots.size(), 22);
    _worldPacket.WriteGuidMask(guid, { 2, 3, 1 });
    _worldPacket.WriteBit(LfgJoined);
    _worldPacket.WriteGuidMask(guid, { 7, 6, 0 });
    _worldPacket.WriteBit(NotifyUI);
    _worldPacket.WriteBit(Queued);
    _worldPacket.WriteBits(SuspendedPlayers.size(), 24);
    _worldPacket.WriteGuidMask(guid, { 5 });
    for (ObjectGuid const& suspended : SuspendedPlayers)
        _worldPacket.WriteGuidMask(suspended, { 0, 1, 2, 3, 4, 5, 6, 7 });
    _worldPacket.WriteGuidMask(guid, { 4 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(guid, { 3 });
    for (uint8 i = 0; i < 3; ++i)
        _worldPacket << uint8(0);                               // needs

    _worldPacket.WriteGuidBytes(guid, { 4 });
    for (ObjectGuid const& suspended : SuspendedPlayers)
        _worldPacket.WriteGuidBytes(suspended, { 0, 1, 2, 3, 4, 5, 6, 7 });
    _worldPacket.WriteGuidBytes(guid, { 6 });
    _worldPacket << uint8(Reason);
    _worldPacket << uint32(RequestedRoles);
    _worldPacket << uint32(Ticket.Id);
    _worldPacket.WriteGuidBytes(guid, { 5 });
    _worldPacket.WriteString(Comment);
    _worldPacket.WriteGuidBytes(guid, { 2 });
    for (uint32 slot : Slots)
        _worldPacket << uint32(slot);
    _worldPacket.WriteGuidBytes(guid, { 0, 1 });
    _worldPacket << uint32(Ticket.Time);
    _worldPacket << uint8(SubType);
    _worldPacket << uint32(Ticket.Type);
    _worldPacket.WriteGuidBytes(guid, { 7 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::RoleChosen::Write()
{
    _worldPacket.WriteGuidMask(Player, { 6, 2, 1, 7, 0 });
    _worldPacket.WriteBit(Accepted);
    _worldPacket.WriteGuidMask(Player, { 3, 5, 4 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(Player, { 0, 3, 6 });
    _worldPacket << uint32(RoleMask);
    _worldPacket.WriteGuidBytes(Player, { 5, 1, 4, 2, 7 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGRoleCheckUpdate::Write()
{
    _worldPacket << uint8(RoleCheckStatus);
    _worldPacket << uint8(PartyIndex);
    _worldPacket.WriteBits(Members.size(), 21);
    for (LFGRoleCheckUpdateMember const& member : Members)
    {
        _worldPacket.WriteBit(member.RoleCheckComplete);
        _worldPacket.WriteGuidMask(member.Guid, { 3, 0, 5, 2, 7, 1, 4, 6 });
    }

    _worldPacket.WriteGuidMask(RequesterGuid, { 3, 5 });
    _worldPacket.WriteBits(JoinSlots.size(), 22);
    _worldPacket.WriteGuidMask(RequesterGuid, { 0, 7, 6, 1, 4, 2 });
    _worldPacket.WriteBit(IsBeginning);
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(RequesterGuid, { 0 });
    for (LFGRoleCheckUpdateMember const& member : Members)
    {
        _worldPacket << uint8(member.Level);
        _worldPacket.WriteGuidBytes(member.Guid, { 3, 6 });
        _worldPacket << uint32(member.RolesDesired);
        _worldPacket.WriteGuidBytes(member.Guid, { 2, 4, 0, 1, 5, 7 });
    }

    _worldPacket.WriteGuidBytes(RequesterGuid, { 1, 7, 6, 4, 3, 2, 5 });
    for (uint32 slot : JoinSlots)
        _worldPacket << uint32(slot);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGJoinResult::Write()
{
    ObjectGuid const& guid = Ticket.RequesterGuid;

    _worldPacket.WriteGuidMask(guid, { 7, 6, 3, 0 });
    _worldPacket.WriteBits(BlackList.size(), 22);
    for (LFGJoinBlackList const& blackList : BlackList)
    {
        _worldPacket.WriteGuidMask(blackList.Guid, { 3 });
        _worldPacket.WriteBits(blackList.Slots.size(), 20);
        _worldPacket.WriteGuidMask(blackList.Guid, { 6, 1, 4, 7, 2, 0, 5 });
    }

    _worldPacket.WriteGuidMask(guid, { 5, 1, 4, 2 });
    _worldPacket.FlushBits();
    _worldPacket << uint8(Result);

    for (LFGJoinBlackList const& blackList : BlackList)
    {
        _worldPacket.WriteGuidBytes(blackList.Guid, { 4 });
        for (LFGJoinBlackListSlot const& slot : blackList.Slots)
        {
            _worldPacket << int32(slot.SubReason2);
            _worldPacket << int32(slot.SubReason1);
            _worldPacket << int32(slot.Reason);
            _worldPacket << int32(slot.Slot);
        }

        _worldPacket.WriteGuidBytes(blackList.Guid, { 1, 0, 5, 7, 3, 6, 2 });
    }

    _worldPacket << uint8(ResultDetail);
    _worldPacket.WriteGuidBytes(guid, { 2 });
    _worldPacket << uint32(Ticket.Time);
    _worldPacket << uint32(Ticket.Id);
    _worldPacket << uint32(Ticket.Type);
    _worldPacket.WriteGuidBytes(guid, { 6, 4, 1, 0, 5, 7, 3 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGQueueStatus::Write()
{
    ObjectGuid const& guid = Ticket.RequesterGuid;

    _worldPacket.WriteGuidMask(guid, { 4, 3, 5, 1, 2, 0, 6, 7 });
    _worldPacket.FlushBits();
    _worldPacket << uint32(Ticket.Type);
    _worldPacket.WriteGuidBytes(guid, { 0 });
    _worldPacket << uint32(QueuedTime);
    _worldPacket.WriteGuidBytes(guid, { 4 });
    _worldPacket << int32(AvgWaitTimeMe);
    for (uint8 i = 0; i < 3; ++i)
    {
        _worldPacket << int32(AvgWaitTimeByRole[i]);
        _worldPacket << uint8(LastNeeded[i]);
    }

    _worldPacket << uint32(Ticket.Time);
    _worldPacket << uint32(Ticket.Id);
    _worldPacket.WriteGuidBytes(guid, { 1 });
    _worldPacket << int32(AvgWaitTime);
    _worldPacket.WriteGuidBytes(guid, { 7, 2 });
    _worldPacket << uint32(Slot);
    _worldPacket.WriteGuidBytes(guid, { 5, 3, 6 });

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LFGPlayerRewards const& lfgPlayerRewards)
{
    ItemTemplate const* item = !lfgPlayerRewards.IsCurrency ? sObjectMgr->GetItemTemplate(lfgPlayerRewards.RewardItem) : nullptr;
    data << uint32(lfgPlayerRewards.RewardItem);
    data << uint32(item ? item->DisplayInfoID : 0);
    data << uint32(lfgPlayerRewards.RewardItemQuantity);
    data << uint8(lfgPlayerRewards.IsCurrency);

    return data;
}

WorldPacket const* WorldPackets::LFG::LFGPlayerReward::Write()
{
    _worldPacket << uint32(QueuedSlot);
    _worldPacket << uint32(ActualSlot);
    _worldPacket << uint32(RewardMoney);
    _worldPacket << uint32(AddedXP);
    _worldPacket << uint8(Rewards.size());

    for (LFGPlayerRewards const& reward : Rewards)
        _worldPacket << reward;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LfgBootPlayer::Write()
{
    bool hasReason = !Info.Reason.empty();

    _worldPacket.WriteBit(hasReason);
    _worldPacket.WriteGuidMask(Info.Target, { 3 });
    _worldPacket.WriteBit(Info.MyVoteCompleted);
    _worldPacket.WriteBit(Info.VotePassed);
    _worldPacket.WriteBit(Info.MyVote);
    _worldPacket.WriteGuidMask(Info.Target, { 6 });
    if (hasReason)
        _worldPacket.WriteBits(Info.Reason.length(), 8);
    _worldPacket.WriteBit(Info.VoteInProgress);
    _worldPacket.WriteGuidMask(Info.Target, { 1, 7, 5, 2, 0, 4 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(Info.Target, { 2, 4, 3, 6 });
    _worldPacket << uint32(Info.VotesNeeded);
    _worldPacket << int32(Info.TimeLeft);
    if (hasReason)
        _worldPacket.WriteString(Info.Reason);
    _worldPacket.WriteGuidBytes(Info.Target, { 5, 0 });
    _worldPacket << uint32(Info.BootVotes);
    _worldPacket.WriteGuidBytes(Info.Target, { 7 });
    _worldPacket << uint32(Info.TotalVotes);
    _worldPacket.WriteGuidBytes(Info.Target, { 1 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGProposalUpdate::Write()
{
    ObjectGuid const& queueGuid = Ticket.RequesterGuid;
    ObjectGuid slotGuid = ObjectGuid(InstanceID);

    _worldPacket.WriteGuidMask(slotGuid, { 6, 0 });
    _worldPacket.WriteGuidMask(queueGuid, { 1, 7, 5 });
    _worldPacket.WriteGuidMask(slotGuid, { 5 });
    _worldPacket.WriteGuidMask(queueGuid, { 4 });
    _worldPacket.WriteBit(ProposalSilent);
    _worldPacket.WriteGuidMask(slotGuid, { 2 });
    _worldPacket.WriteGuidMask(queueGuid, { 6 });
    _worldPacket.WriteGuidMask(slotGuid, { 3, 7 });
    _worldPacket.WriteGuidMask(queueGuid, { 3 });
    _worldPacket.WriteBits(Players.size(), 21);
    for (LFGProposalUpdatePlayer const& player : Players)
    {
        _worldPacket.WriteBit(player.MyParty);
        _worldPacket.WriteBit(player.Me);
        _worldPacket.WriteBit(player.Responded);
        _worldPacket.WriteBit(player.Accepted);
        _worldPacket.WriteBit(player.SameParty);
    }

    _worldPacket.WriteGuidMask(queueGuid, { 2 });
    _worldPacket.WriteGuidMask(slotGuid, { 4 });
    _worldPacket.WriteBit(false);                               // unknown
    _worldPacket.WriteGuidMask(queueGuid, { 0 });
    _worldPacket.WriteGuidMask(slotGuid, { 1 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(slotGuid, { 1 });
    _worldPacket.WriteGuidBytes(queueGuid, { 4 });
    _worldPacket.WriteGuidBytes(slotGuid, { 4 });
    _worldPacket.WriteGuidBytes(queueGuid, { 7, 2, 0 });
    _worldPacket << uint32(Slot);
    _worldPacket << uint8(State);
    _worldPacket << uint32(Ticket.Id);
    _worldPacket.WriteGuidBytes(slotGuid, { 6 });
    _worldPacket << uint32(ProposalID);
    _worldPacket.WriteGuidBytes(queueGuid, { 5, 3 });
    _worldPacket << uint32(Ticket.Time);
    _worldPacket.WriteGuidBytes(slotGuid, { 5 });
    _worldPacket.WriteGuidBytes(queueGuid, { 6 });
    for (LFGProposalUpdatePlayer const& player : Players)
        _worldPacket << uint32(player.Roles);
    _worldPacket << uint32(CompletedMask);
    _worldPacket.WriteGuidBytes(slotGuid, { 7 });
    _worldPacket.WriteGuidBytes(queueGuid, { 1 });
    _worldPacket.WriteGuidBytes(slotGuid, { 0, 2 });
    _worldPacket << uint32(Ticket.Type);
    _worldPacket.WriteGuidBytes(slotGuid, { 3 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGOfferContinue::Write()
{
    _worldPacket << uint32(Slot);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGTeleportDenied::Write()
{
    _worldPacket.WriteBits(Reason, 4);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGUpdateSearch::Write()
{
    _worldPacket << uint8(Update);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGSearchResults::Write()
{
    _worldPacket.WriteBit(Ticket.RequesterGuid[3]);
    _worldPacket.WriteBits(Parties.size(), 21);

    for (ClientLFGSearchResultParty const& party : Parties)
    {
        _worldPacket.WriteBit(party.InstanceID[6]);
        _worldPacket.WriteBit(party.Guid[3]);
        _worldPacket.WriteBit(party.Guid[1]);
        _worldPacket.WriteBits(party.Comment.length(), 9);
        _worldPacket.WriteBit(party.InstanceID[0]);
        _worldPacket.WriteBit(party.Guid[5]);
        _worldPacket.WriteBit(party.Guid[2]);
        _worldPacket.WriteBit(party.InstanceID[1]);
        _worldPacket.WriteBit(party.InstanceID[4]);
        _worldPacket.WriteBit(party.Guid[4]);
        _worldPacket.WriteBit(party.InstanceID[7]);
        _worldPacket.WriteBit(party.Guid[7]);
        _worldPacket.WriteBit(party.Guid[0]);
        _worldPacket.WriteBit(party.InstanceID[3]);
        _worldPacket.WriteBit(party.Guid[6]);
        _worldPacket.WriteBit(party.InstanceID[2]);
        _worldPacket.WriteBit(party.InstanceID[5]);
    }

    _worldPacket.WriteBit(Ticket.RequesterGuid[6]);
    _worldPacket.WriteBit(Incremental);
    _worldPacket.WriteBit(Ticket.RequesterGuid[7]);
    _worldPacket.WriteBit(Ticket.RequesterGuid[5]);
    _worldPacket.WriteBit(Ticket.RequesterGuid[2]);
    _worldPacket.WriteBit(Ticket.RequesterGuid[4]);
    _worldPacket.WriteBits(Players.size(), 18);

    for (ClientLFGSearchResultPlayer const& player : Players)
    {
        _worldPacket.WriteBits(player.Comment.length(), 9);
        _worldPacket.WriteBit(player.PartyGuid[3]);
        _worldPacket.WriteBit(player.Guid[7]);
        _worldPacket.WriteBit(player.PartyGuid[1]);
        _worldPacket.WriteBit(player.InstanceID[7]);
        _worldPacket.WriteBit(player.InstanceID[4]);
        _worldPacket.WriteBit(player.Guid[3]);
        _worldPacket.WriteBit(player.Guid[1]);
        _worldPacket.WriteBit(player.InstanceID[6]);
        _worldPacket.WriteBit(player.PartyGuid[2]);
        _worldPacket.WriteBit(player.IsLeader);
        _worldPacket.WriteBit(player.InstanceID[0]);
        _worldPacket.WriteBit(player.PartyGuid[0]);
        _worldPacket.WriteBit(player.Guid[2]);
        _worldPacket.WriteBit(player.PartyGuid[5]);
        _worldPacket.WriteBit(player.InstanceID[3]);
        _worldPacket.WriteBit(player.Guid[4]);
        _worldPacket.WriteBit(player.Guid[6]);
        _worldPacket.WriteBit(player.InstanceID[5]);
        _worldPacket.WriteBit(player.Guid[0]);
        _worldPacket.WriteBit(player.PartyGuid[7]);
        _worldPacket.WriteBit(player.InstanceID[1]);
        _worldPacket.WriteBit(player.PartyGuid[4]);
        _worldPacket.WriteBit(player.PartyGuid[6]);
        _worldPacket.WriteBit(player.InstanceID[2]);
        _worldPacket.WriteBit(player.Guid[5]);
    }

    _worldPacket.WriteBits(Removes.size(), 26);

    for (ObjectGuid const& remove : Removes)
    {
        _worldPacket.WriteBit(remove[7]);
        _worldPacket.WriteBit(remove[3]);
        _worldPacket.WriteBit(remove[6]);
        _worldPacket.WriteBit(remove[2]);
        _worldPacket.WriteBit(remove[1]);
        _worldPacket.WriteBit(remove[4]);
        _worldPacket.WriteBit(remove[0]);
        _worldPacket.WriteBit(remove[5]);
    }

    _worldPacket.WriteBit(Ticket.RequesterGuid[0]);
    _worldPacket.WriteBit(Ticket.RequesterGuid[1]);
    _worldPacket.FlushBits();

    for (ObjectGuid const& remove : Removes)
    {
        _worldPacket.WriteByteSeq(remove[3]);
        _worldPacket.WriteByteSeq(remove[6]);
        _worldPacket.WriteByteSeq(remove[4]);
        _worldPacket.WriteByteSeq(remove[2]);
        _worldPacket.WriteByteSeq(remove[1]);
        _worldPacket.WriteByteSeq(remove[0]);
        _worldPacket.WriteByteSeq(remove[5]);
        _worldPacket.WriteByteSeq(remove[7]);
    }

    for (ClientLFGSearchResultPlayer const& player : Players)
    {
        _worldPacket << int32(player.CritRanged);
        _worldPacket << int8(player.Talents[0]);
        _worldPacket << float(player.GearRating);

        _worldPacket.WriteByteSeq(player.PartyGuid[4]);
        _worldPacket.WriteByteSeq(player.InstanceID[5]);

        _worldPacket << int32(player.DefenseRating);

        _worldPacket.WriteByteSeq(player.InstanceID[7]);
        _worldPacket.WriteString(player.Comment);
        _worldPacket.WriteByteSeq(player.InstanceID[4]);

        _worldPacket << float(player.Mp5InCombat);
        _worldPacket << int32(player.DodgeRating);
        _worldPacket << int8(player.Level);
        _worldPacket << int32(player.FreeTalentPoints);

        _worldPacket.WriteByteSeq(player.PartyGuid[6]);

        _worldPacket << int32(player.HasteRating);
        _worldPacket << int32(player.Expertise);
        _worldPacket << int32(player.ParryRating);
        _worldPacket << int8(player.Race);

        _worldPacket.WriteByteSeq(player.PartyGuid[5]);

        _worldPacket << float(player.AvgItemLevel);
        _worldPacket << int32(player.MaxHealth);
        _worldPacket << int32(player.CritSpell);

        _worldPacket.WriteByteSeq(player.PartyGuid[0]);
        _worldPacket.WriteByteSeq(player.Guid[4]);
        _worldPacket.WriteByteSeq(player.PartyGuid[7]);

        _worldPacket << int32(player.Armor);
        _worldPacket << int8(0); // unk?

        _worldPacket.WriteByteSeq(player.Guid[6]);
        _worldPacket.WriteByteSeq(player.PartyGuid[2]);

        _worldPacket << int8(player.Talents[1]);
        _worldPacket << int32(player.Agility);

        _worldPacket.WriteByteSeq(player.InstanceID[2]);

        _worldPacket << int8(player.Talents[2]);

        _worldPacket.WriteByteSeq(player.InstanceID[1]);

        _worldPacket << int8(player.ChrClass);

        _worldPacket.WriteByteSeq(player.Guid[0]);
        _worldPacket.WriteByteSeq(player.PartyGuid[1]);
        _worldPacket.WriteByteSeq(player.Guid[7]);
        _worldPacket.WriteByteSeq(player.Guid[2]);

        _worldPacket << int32(player.BlockRating);
        _worldPacket << float(player.Mp5);

        _worldPacket.WriteByteSeq(player.InstanceID[0]);

        _worldPacket << int8(0); // unk?

        _worldPacket << int32(player.CritMelee);
        _worldPacket << int32(player.ChangeMask);
        _worldPacket << int32(player.SpellDamage);

        _worldPacket.WriteByteSeq(player.InstanceID[6]);

        _worldPacket << int32(player.MaxMana);

        _worldPacket.WriteByteSeq(player.Guid[1]);
        _worldPacket.WriteByteSeq(player.Guid[3]);

        _worldPacket << int32(player.InstanceCompletedMask);
        _worldPacket << int32(player.PlusHealing);

        _worldPacket.WriteByteSeq(player.PartyGuid[3]);
        _worldPacket.WriteByteSeq(player.Guid[5]);
        _worldPacket.WriteByteSeq(player.InstanceID[3]);

        _worldPacket << int32(0); // Unk
        _worldPacket << int32(0); // Unk
    }

    _worldPacket << int32(SlotID);

    for (ClientLFGSearchResultParty const& party : Parties)
    {
        _worldPacket.WriteString(party.Comment);
        _worldPacket << int32(party.ChangeMask);

        _worldPacket.WriteByteSeq(party.Guid[6]);

        _worldPacket << int32(party.InstanceCompletedMask);

        _worldPacket.WriteByteSeq(party.InstanceID[6]);
        _worldPacket.WriteByteSeq(party.Guid[2]);
        _worldPacket.WriteByteSeq(party.InstanceID[4]);

        _worldPacket << int8(party.Needs[0]);
        _worldPacket << int8(party.Needs[1]);
        _worldPacket << int8(party.Needs[2]);

        _worldPacket.WriteByteSeq(party.Guid[4]);
        _worldPacket.WriteByteSeq(party.InstanceID[7]);
        _worldPacket.WriteByteSeq(party.Guid[7]);
        _worldPacket.WriteByteSeq(party.Guid[3]);
        _worldPacket.WriteByteSeq(party.InstanceID[3]);
        _worldPacket.WriteByteSeq(party.InstanceID[5]);
        _worldPacket.WriteByteSeq(party.InstanceID[0]);
        _worldPacket.WriteByteSeq(party.Guid[5]);
        _worldPacket.WriteByteSeq(party.InstanceID[1]);
        _worldPacket.WriteByteSeq(party.Guid[1]);
        _worldPacket.WriteByteSeq(party.InstanceID[2]);
        _worldPacket.WriteByteSeq(party.Guid[0]);
    }

    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[7]);
    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[1]);

    _worldPacket << int32(CountTotalParties);
    _worldPacket << int32(Ticket.Type);

    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[4]);
    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[2]);

    _worldPacket << int32(Ticket.Id);

    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[6]);
    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[5]);

    _worldPacket << int32(CountTotalPlayers);
    _worldPacket << int32(Ticket.Time);

    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[0]);

    _worldPacket << int32(SlotType);

    _worldPacket.WriteByteSeq(Ticket.RequesterGuid[3]);

    return &_worldPacket;
}
