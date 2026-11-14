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

ByteBuffer& operator>>(ByteBuffer& data, WorldPackets::LFG::RideTicket& ticket)
{
    data >> ticket.RequesterGuid;
    data >> ticket.Id;
    ticket.Type = data.read<WorldPackets::LFG::RideType>();
    data >> ticket.Time;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::RideTicket const& ticket)
{
    data << ticket.RequesterGuid;
    data << uint32(ticket.Id);
    data << uint32(ticket.Type);
    data << int32(ticket.Time);

    return data;
}

void WorldPackets::LFG::DFJoin::Read()
{
    QueueAsGroup = _worldPacket.ReadBit();
    uint32 commentLength = _worldPacket.ReadBits(8);
    _worldPacket >> PartyIndex;
    _worldPacket >> Roles;
    Slots.resize(_worldPacket.read<uint32>());
    for (uint32& need : Needs)
        _worldPacket >> need;

    Comment = _worldPacket.ReadString(commentLength);
    for (uint32& slot : Slots)
        _worldPacket >> slot;
}

void WorldPackets::LFG::DFLeave::Read()
{
    _worldPacket >> Ticket;
}

void WorldPackets::LFG::DFProposalResponse::Read()
{
    _worldPacket >> Ticket;
    _worldPacket >> InstanceID;
    _worldPacket >> ProposalID;
    Accepted = _worldPacket.ReadBit();
}

void WorldPackets::LFG::DFSetRoles::Read()
{
    _worldPacket >> RolesDesired;
    _worldPacket >> PartyIndex;
}

void WorldPackets::LFG::DFBootPlayerVote::Read()
{
    Vote = _worldPacket.ReadBit();
}

void WorldPackets::LFG::DFTeleport::Read()
{
    TeleportOut = _worldPacket.ReadBit();
}

void WorldPackets::LFG::DFGetSystemInfo::Read()
{
    Player = _worldPacket.ReadBit();
    _worldPacket >> PartyIndex;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LFGBlackListSlot const& blackListSlot)
{
    data << uint32(blackListSlot.Slot);
    data << uint32(blackListSlot.Reason);
    data << int32(blackListSlot.SubReason1);
    data << int32(blackListSlot.SubReason2);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LFGBlackList const& blackList)
{
    data.WriteBit(blackList.PlayerGuid.has_value());
    data.FlushBits();
    data << uint32(blackList.Slot.size());
    if (blackList.PlayerGuid)
        data << *blackList.PlayerGuid;

    for (WorldPackets::LFG::LFGBlackListSlot const& slot : blackList.Slot)
        data << slot;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LfgPlayerQuestReward const& playerQuestReward)
{
    data << uint32(playerQuestReward.Mask);
    data << int32(playerQuestReward.RewardMoney);
    data << int32(playerQuestReward.RewardXP);
    data << uint32(playerQuestReward.Item.size());
    data << uint32(playerQuestReward.Currency.size());
    data << uint32(playerQuestReward.BonusCurrency.size());

    for (WorldPackets::LFG::LfgPlayerQuestRewardItem const& item : playerQuestReward.Item)
        data << int32(item.ItemID) << int32(item.Quantity);

    for (WorldPackets::LFG::LfgPlayerQuestRewardCurrency const& currency : playerQuestReward.Currency)
        data << int32(currency.CurrencyID) << int32(currency.Quantity);

    for (WorldPackets::LFG::LfgPlayerQuestRewardCurrency const& currency : playerQuestReward.BonusCurrency)
        data << int32(currency.CurrencyID) << int32(currency.Quantity);

    data.WriteBit(playerQuestReward.RewardSpellID.has_value());
    data.FlushBits();
    if (playerQuestReward.RewardSpellID)
        data << int32(*playerQuestReward.RewardSpellID);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LfgPlayerDungeonInfo const& playerDungeonInfo)
{
    data << uint32(playerDungeonInfo.Slot);
    data << int32(playerDungeonInfo.CompletionQuantity);
    data << int32(playerDungeonInfo.CompletionLimit);
    data << int32(playerDungeonInfo.CompletionCurrencyID);
    data << int32(playerDungeonInfo.SpecificQuantity);
    data << int32(playerDungeonInfo.SpecificLimit);
    data << int32(playerDungeonInfo.OverallQuantity);
    data << int32(playerDungeonInfo.OverallLimit);
    data << int32(playerDungeonInfo.PurseWeeklyQuantity);
    data << int32(playerDungeonInfo.PurseWeeklyLimit);
    data << int32(playerDungeonInfo.PurseQuantity);
    data << int32(playerDungeonInfo.PurseLimit);
    data << int32(playerDungeonInfo.Quantity);
    data << uint32(playerDungeonInfo.CompletedMask);
    data << uint32(playerDungeonInfo.ShortageReward.size());
    data << playerDungeonInfo.Rewards;
    for (WorldPackets::LFG::LfgPlayerQuestReward const& shortageReward : playerDungeonInfo.ShortageReward)
        data << shortageReward;

    data.WriteBit(playerDungeonInfo.FirstReward);
    data.WriteBit(playerDungeonInfo.ShortageEligible);
    data.FlushBits();

    return data;
}

WorldPacket const* WorldPackets::LFG::LFGPlayerInfo::Write()
{
    _worldPacket << uint32(Dungeon.size());
    _worldPacket << BlackList;
    for (LfgPlayerDungeonInfo const& playerDungeonInfo : Dungeon)
        _worldPacket << playerDungeonInfo;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGPartyInfo::Write()
{
    _worldPacket << uint32(Player.size());
    for (LFGBlackList const& blackList : Player)
        _worldPacket << blackList;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGUpdateStatus::Write()
{
    _worldPacket << Ticket;
    _worldPacket << uint8(SubType);
    _worldPacket << uint8(Reason);
    for (uint8 need : Needs)
        _worldPacket << uint8(need);

    _worldPacket << uint32(Slots.size());
    _worldPacket << uint32(RequestedRoles);
    _worldPacket << uint32(SuspendedPlayers.size());

    for (uint32 slot : Slots)
        _worldPacket << uint32(slot);

    for (ObjectGuid const& suspendedPlayer : SuspendedPlayers)
        _worldPacket << suspendedPlayer;

    _worldPacket.WriteBit(NotifyUI);
    _worldPacket.WriteBit(IsParty);
    _worldPacket.WriteBit(Joined);
    _worldPacket.WriteBit(LfgJoined);
    _worldPacket.WriteBit(Queued);
    _worldPacket.WriteBits(Comment.length(), 8);
    _worldPacket.FlushBits();
    _worldPacket.WriteString(Comment);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::RoleChosen::Write()
{
    _worldPacket << Player;
    _worldPacket << uint32(RoleMask);
    _worldPacket.WriteBit(Accepted);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LFGRoleCheckUpdateMember const& lfgRoleCheckUpdateMember)
{
    data << lfgRoleCheckUpdateMember.Guid;
    data << uint32(lfgRoleCheckUpdateMember.RolesDesired);
    data << uint8(lfgRoleCheckUpdateMember.Level);
    data.WriteBit(lfgRoleCheckUpdateMember.RoleCheckComplete);
    data.FlushBits();

    return data;
}

WorldPacket const* WorldPackets::LFG::LFGRoleCheckUpdate::Write()
{
    _worldPacket << uint8(PartyIndex);
    _worldPacket << uint8(RoleCheckStatus);
    _worldPacket << uint32(JoinSlots.size());
    _worldPacket << uint64(BgQueueID);
    _worldPacket << int32(GroupFinderActivityID);
    _worldPacket << uint32(Members.size());

    for (uint32 slot : JoinSlots)
        _worldPacket << uint32(slot);

    for (LFGRoleCheckUpdateMember const& member : Members)
        _worldPacket << member;

    _worldPacket.WriteBit(IsBeginning);
    _worldPacket.WriteBit(ShowRoleCheck);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGJoinResult::Write()
{
    _worldPacket << Ticket;
    _worldPacket << uint8(Result);
    _worldPacket << uint8(ResultDetail);
    _worldPacket << uint32(BlackList.size());
    for (LFGJoinBlackList const& blackList : BlackList)
    {
        _worldPacket << blackList.Guid;
        _worldPacket << uint32(blackList.Slots.size());
        for (LFGJoinBlackListSlot const& slot : blackList.Slots)
        {
            _worldPacket << int32(slot.Slot);
            _worldPacket << int32(slot.Reason);
            _worldPacket << int32(slot.SubReason1);
            _worldPacket << int32(slot.SubReason2);
        }
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::LFG::LFGQueueStatus::Write()
{
    _worldPacket << Ticket;
    _worldPacket << uint32(Slot);
    _worldPacket << uint32(AvgWaitTime);
    _worldPacket << uint32(QueuedTime);

    for (uint32 i = 0; i < 3; ++i)
    {
        _worldPacket << uint32(AvgWaitTimeByRole[i]);
        _worldPacket << uint8(LastNeeded[i]);
    }

    _worldPacket << uint32(AvgWaitTimeMe);

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LFGPlayerRewards const& lfgPlayerRewards)
{
    data << int32(lfgPlayerRewards.RewardItem);
    data << uint32(lfgPlayerRewards.RewardItemQuantity);
    data << int32(lfgPlayerRewards.BonusCurrency);
    data.WriteBit(lfgPlayerRewards.IsCurrency);
    data.FlushBits();

    return data;
}

WorldPacket const* WorldPackets::LFG::LFGPlayerReward::Write()
{
    _worldPacket << uint32(QueuedSlot);
    _worldPacket << uint32(ActualSlot);
    _worldPacket << int32(RewardMoney);
    _worldPacket << int32(AddedXP);
    _worldPacket << uint32(Rewards.size());

    for (LFGPlayerRewards const& reward : Rewards)
        _worldPacket << reward;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LfgBootInfo const& boot)
{
    data.WriteBit(boot.VoteInProgress);
    data.WriteBit(boot.VotePassed);
    data.WriteBit(boot.MyVoteCompleted);
    data.WriteBit(boot.MyVote);
    data.WriteBits(boot.Reason.length(), 8);
    data.FlushBits();
    data << boot.Target;
    data << uint32(boot.TotalVotes);
    data << uint32(boot.BootVotes);
    data << int32(boot.TimeLeft);
    data << uint32(boot.VotesNeeded);
    data.WriteString(boot.Reason);

    return data;
}

WorldPacket const* WorldPackets::LFG::LfgBootPlayer::Write()
{
    _worldPacket << Info;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::LFG::LFGProposalUpdatePlayer const& proposalUpdatePlayer)
{
    data << uint32(proposalUpdatePlayer.Roles);
    data.WriteBit(proposalUpdatePlayer.Me);
    data.WriteBit(proposalUpdatePlayer.SameParty);
    data.WriteBit(proposalUpdatePlayer.MyParty);
    data.WriteBit(proposalUpdatePlayer.Responded);
    data.WriteBit(proposalUpdatePlayer.Accepted);
    data.FlushBits();

    return data;
}

WorldPacket const* WorldPackets::LFG::LFGProposalUpdate::Write()
{
    _worldPacket << Ticket;
    _worldPacket << uint64(InstanceID);
    _worldPacket << uint32(ProposalID);
    _worldPacket << uint32(Slot);
    _worldPacket << int8(State);
    _worldPacket << uint32(CompletedMask);
    _worldPacket << uint32(Players.size());
    for (LFGProposalUpdatePlayer const& player : Players)
        _worldPacket << player;

    _worldPacket.WriteBit(ValidCompletedMask);
    _worldPacket.WriteBit(ProposalSilent);
    _worldPacket.FlushBits();

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
