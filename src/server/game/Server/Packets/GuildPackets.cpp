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

#include "GuildPackets.h"

// Cata Guild Challenges. Field order matches TrinityCore's real wire layout exactly
// (ChallengeType, GoldAwarded, CurrentCount, XpAwarded, MaxCount) - it does not match the
// struct's declaration order above, which is cosmetic only.
WorldPacket const* WorldPackets::Guild::GuildChallengeCompleted::Write()
{
    _worldPacket << int32(ChallengeType);
    _worldPacket << int32(GoldAwarded);
    _worldPacket << int32(CurrentCount);
    _worldPacket << int32(XpAwarded);
    _worldPacket << int32(MaxCount);

    return &_worldPacket;
}

void WorldPackets::Guild::QueryGuildInfo::Read()
{
    _worldPacket >> GuildGuid;
    _worldPacket >> PlayerGuid;
}

WorldPackets::Guild::QueryGuildInfoResponse::QueryGuildInfoResponse()
        : ServerPacket(SMSG_QUERY_GUILD_INFO_RESPONSE) { }

WorldPacket const* WorldPackets::Guild::QueryGuildInfoResponse::Write()
{
    _worldPacket << GuildGuid;
    _worldPacket << Info.GuildName;

    for (std::string const& rankName : Info.Ranks)
        _worldPacket << rankName;

    for (uint32 rankOrder : Info.RankOrders)
        _worldPacket << uint32(rankOrder);

    for (uint32 rankId : Info.RankIds)
        _worldPacket << uint32(rankId);

    _worldPacket << uint32(Info.EmblemStyle);
    _worldPacket << uint32(Info.EmblemColor);
    _worldPacket << uint32(Info.BorderStyle);
    _worldPacket << uint32(Info.BorderColor);
    _worldPacket << uint32(Info.BackgroundColor);
    _worldPacket << uint32(Info.RankCount);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildCreate::Read()
{
    _worldPacket >> GuildName;
}

WorldPacket const* WorldPackets::Guild::GuildInfoResponse::Write()
{
    _worldPacket << GuildName;
    _worldPacket.AppendPackedTime(CreateDate);
    _worldPacket << int32(NumMembers);
    _worldPacket << int32(NumAccounts);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildXPQuery::Read()
{
    GuildGuid[2] = _worldPacket.ReadBit();
    GuildGuid[1] = _worldPacket.ReadBit();
    GuildGuid[0] = _worldPacket.ReadBit();
    GuildGuid[5] = _worldPacket.ReadBit();
    GuildGuid[4] = _worldPacket.ReadBit();
    GuildGuid[7] = _worldPacket.ReadBit();
    GuildGuid[6] = _worldPacket.ReadBit();
    GuildGuid[3] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(GuildGuid[7]);
    _worldPacket.ReadByteSeq(GuildGuid[2]);
    _worldPacket.ReadByteSeq(GuildGuid[3]);
    _worldPacket.ReadByteSeq(GuildGuid[6]);
    _worldPacket.ReadByteSeq(GuildGuid[1]);
    _worldPacket.ReadByteSeq(GuildGuid[5]);
    _worldPacket.ReadByteSeq(GuildGuid[0]);
    _worldPacket.ReadByteSeq(GuildGuid[4]);
}

void WorldPackets::Guild::GuildRequestMaxDailyXP::Read()
{
    GuildGuid[0] = _worldPacket.ReadBit();
    GuildGuid[3] = _worldPacket.ReadBit();
    GuildGuid[5] = _worldPacket.ReadBit();
    GuildGuid[1] = _worldPacket.ReadBit();
    GuildGuid[4] = _worldPacket.ReadBit();
    GuildGuid[6] = _worldPacket.ReadBit();
    GuildGuid[7] = _worldPacket.ReadBit();
    GuildGuid[2] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(GuildGuid[7]);
    _worldPacket.ReadByteSeq(GuildGuid[4]);
    _worldPacket.ReadByteSeq(GuildGuid[3]);
    _worldPacket.ReadByteSeq(GuildGuid[5]);
    _worldPacket.ReadByteSeq(GuildGuid[1]);
    _worldPacket.ReadByteSeq(GuildGuid[2]);
    _worldPacket.ReadByteSeq(GuildGuid[6]);
    _worldPacket.ReadByteSeq(GuildGuid[0]);
}

WorldPacket const* WorldPackets::Guild::GuildMaxDailyXP::Write()
{
    _worldPacket << uint64(Xp);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildXP::Write()
{
    _worldPacket << uint64(TotalXP);
    _worldPacket << uint64(GuildRemainingXP);
    _worldPacket << uint64(GuildTodayXP);
    _worldPacket << uint64(WeeklyXP);
    _worldPacket << uint64(GuildCurrentXP);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildRoster::Write()
{
    _worldPacket.WriteBits(WelcomeText.length(), 11);
    _worldPacket.WriteBits(MemberData.size(), 18);

    for (GuildRosterMemberData const& member : MemberData)
    {
        _worldPacket.WriteBit(member.Guid[3]);
        _worldPacket.WriteBit(member.Guid[4]);
        _worldPacket.WriteBit(member.Authenticated);
        _worldPacket.WriteBit(member.SorEligible);
        _worldPacket.WriteBits(member.Note.length(), 8);
        _worldPacket.WriteBits(member.OfficerNote.length(), 8);
        _worldPacket.WriteBit(member.Guid[0]);
        _worldPacket.WriteBits(member.Name.length(), 7);
        _worldPacket.WriteBit(member.Guid[1]);
        _worldPacket.WriteBit(member.Guid[2]);
        _worldPacket.WriteBit(member.Guid[6]);
        _worldPacket.WriteBit(member.Guid[5]);
        _worldPacket.WriteBit(member.Guid[7]);
    }

    _worldPacket.WriteBits(InfoText.length(), 12);
    _worldPacket.FlushBits();

    for (GuildRosterMemberData const& member : MemberData)
        _worldPacket << member;

    _worldPacket.WriteString(InfoText);
    _worldPacket.WriteString(WelcomeText);
    _worldPacket << uint32(NumAccounts);
    _worldPacket << uint32(WeeklyRepCap);
    _worldPacket.AppendPackedTime(CreateDate);
    _worldPacket << uint32(GuildFlags);

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Guild::GuildRosterMemberData const& rosterMemberData)
{
    data << uint8(rosterMemberData.ClassID);
    data << uint32(rosterMemberData.GuildReputation);
    data.WriteByteSeq(rosterMemberData.Guid[0]);
    data << uint64(rosterMemberData.WeeklyXP);
    data << uint32(rosterMemberData.RankID);
    data << uint32(rosterMemberData.PersonalAchievementPoints);

    for (WorldPackets::Guild::GuildRosterProfessionData const& profession : rosterMemberData.Profession)
    {
        data << uint32(profession.Step);
        data << uint32(profession.Rank);
        data << uint32(profession.DbID);
    }

    data.WriteByteSeq(rosterMemberData.Guid[2]);
    data << uint8(rosterMemberData.Status);
    data << uint32(rosterMemberData.AreaID);
    data << uint64(rosterMemberData.TotalXP);
    data.WriteByteSeq(rosterMemberData.Guid[7]);
    data << uint32(rosterMemberData.GuildRepToCap);
    data.WriteString(rosterMemberData.Note);
    data.WriteByteSeq(rosterMemberData.Guid[3]);
    data << uint8(rosterMemberData.Level);
    data << int32(rosterMemberData.VirtualRealmAddress);
    data.WriteByteSeq(rosterMemberData.Guid[5]);
    data.WriteByteSeq(rosterMemberData.Guid[4]);
    data << uint8(rosterMemberData.Gender);
    data.WriteByteSeq(rosterMemberData.Guid[1]);
    data << float(rosterMemberData.LastSave);
    data.WriteString(rosterMemberData.OfficerNote);
    data.WriteByteSeq(rosterMemberData.Guid[6]);
    data.WriteString(rosterMemberData.Name);

    return data;
}

void WorldPackets::Guild::GuildQueryRanks::Read()
{
    GuildGuid[2] = _worldPacket.ReadBit();
    GuildGuid[3] = _worldPacket.ReadBit();
    GuildGuid[0] = _worldPacket.ReadBit();
    GuildGuid[6] = _worldPacket.ReadBit();
    GuildGuid[4] = _worldPacket.ReadBit();
    GuildGuid[7] = _worldPacket.ReadBit();
    GuildGuid[5] = _worldPacket.ReadBit();
    GuildGuid[1] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(GuildGuid[3]);
    _worldPacket.ReadByteSeq(GuildGuid[4]);
    _worldPacket.ReadByteSeq(GuildGuid[5]);
    _worldPacket.ReadByteSeq(GuildGuid[7]);
    _worldPacket.ReadByteSeq(GuildGuid[1]);
    _worldPacket.ReadByteSeq(GuildGuid[0]);
    _worldPacket.ReadByteSeq(GuildGuid[6]);
    _worldPacket.ReadByteSeq(GuildGuid[2]);
}

WorldPacket const* WorldPackets::Guild::GuildRanks::Write()
{
    _worldPacket.WriteBits(Ranks.size(), 18);

    for (GuildRankData const& rank : Ranks)
        _worldPacket.WriteBits(rank.RankName.length(), 7);

    _worldPacket.FlushBits();

    for (GuildRankData const& rank : Ranks)
        _worldPacket << rank;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Guild::GuildRankData const& rankData)
{
    data << int32(rankData.RankID);

    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; i++)
    {
        data << int32(rankData.TabWithdrawItemLimit[i]);
        data << int32(rankData.TabFlags[i]);
    }

    data << int32(rankData.WithdrawGoldLimit);
    data << int32(rankData.Flags);
    data.WriteString(rankData.RankName);
    data << int32(rankData.RankOrder);

    return data;
}

void WorldPackets::Guild::GuildAssignMemberRank::Read()
{
    _worldPacket >> RankOrder;

    Member[1] = _worldPacket.ReadBit();
    Member[7] = _worldPacket.ReadBit();
    Setter[4] = _worldPacket.ReadBit();
    Setter[2] = _worldPacket.ReadBit();
    Member[4] = _worldPacket.ReadBit();
    Member[5] = _worldPacket.ReadBit();
    Member[6] = _worldPacket.ReadBit();
    Setter[1] = _worldPacket.ReadBit();
    Setter[7] = _worldPacket.ReadBit();
    Member[2] = _worldPacket.ReadBit();
    Member[3] = _worldPacket.ReadBit();
    Member[0] = _worldPacket.ReadBit();
    Setter[6] = _worldPacket.ReadBit();
    Setter[3] = _worldPacket.ReadBit();
    Setter[0] = _worldPacket.ReadBit();
    Setter[5] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(Member[0]);
    _worldPacket.ReadByteSeq(Setter[1]);
    _worldPacket.ReadByteSeq(Setter[3]);
    _worldPacket.ReadByteSeq(Setter[5]);
    _worldPacket.ReadByteSeq(Member[7]);
    _worldPacket.ReadByteSeq(Member[3]);
    _worldPacket.ReadByteSeq(Setter[0]);
    _worldPacket.ReadByteSeq(Member[1]);
    _worldPacket.ReadByteSeq(Setter[6]);
    _worldPacket.ReadByteSeq(Member[2]);
    _worldPacket.ReadByteSeq(Member[5]);
    _worldPacket.ReadByteSeq(Member[4]);
    _worldPacket.ReadByteSeq(Setter[2]);
    _worldPacket.ReadByteSeq(Setter[4]);
    _worldPacket.ReadByteSeq(Member[6]);
    _worldPacket.ReadByteSeq(Setter[7]);
}

void WorldPackets::Guild::GuildSwitchRank::Read()
{
    _worldPacket >> RankOrder;
    Up = _worldPacket.ReadBit();
}

void WorldPackets::Guild::GuildUpdateMotdText::Read()
{
    uint32 length = _worldPacket.ReadBits(11);
    MotdText.ReadWithLength(_worldPacket, length);
}

WorldPacket const* WorldPackets::Guild::GuildCommandResult::Write()
{
    _worldPacket << uint32(Command);
    _worldPacket << Name;
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildInviteByName::Read()
{
    uint32 length = _worldPacket.ReadBits(7);
    Name.ReadWithLength(_worldPacket, length);
}

WorldPacket const* WorldPackets::Guild::GuildInvite::Write()
{
    _worldPacket << uint32(GuildLevel);
    _worldPacket << uint32(BorderStyle);
    _worldPacket << uint32(BorderColor);
    _worldPacket << uint32(EmblemStyle);
    _worldPacket << uint32(BackgroundColor);
    _worldPacket << uint32(EmblemColor);

    _worldPacket.WriteBit(GuildGuid[3]);
    _worldPacket.WriteBit(GuildGuid[2]);
    _worldPacket.WriteBits(OldGuildName.length(), 8);
    _worldPacket.WriteBit(GuildGuid[1]);
    _worldPacket.WriteBit(OldGuildGuid[6]);
    _worldPacket.WriteBit(OldGuildGuid[4]);
    _worldPacket.WriteBit(OldGuildGuid[1]);
    _worldPacket.WriteBit(OldGuildGuid[5]);
    _worldPacket.WriteBit(OldGuildGuid[7]);
    _worldPacket.WriteBit(OldGuildGuid[2]);
    _worldPacket.WriteBit(GuildGuid[7]);
    _worldPacket.WriteBit(GuildGuid[0]);
    _worldPacket.WriteBit(GuildGuid[6]);
    _worldPacket.WriteBits(GuildName.length(), 8);
    _worldPacket.WriteBit(OldGuildGuid[3]);
    _worldPacket.WriteBit(OldGuildGuid[0]);
    _worldPacket.WriteBit(GuildGuid[5]);
    _worldPacket.WriteBits(InviterName.length(), 7);
    _worldPacket.WriteBit(GuildGuid[4]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(GuildGuid[1]);
    _worldPacket.WriteByteSeq(OldGuildGuid[3]);
    _worldPacket.WriteByteSeq(GuildGuid[6]);
    _worldPacket.WriteByteSeq(OldGuildGuid[2]);
    _worldPacket.WriteByteSeq(OldGuildGuid[1]);
    _worldPacket.WriteByteSeq(GuildGuid[0]);
    _worldPacket.WriteString(OldGuildName);
    _worldPacket.WriteByteSeq(GuildGuid[7]);
    _worldPacket.WriteByteSeq(GuildGuid[2]);
    _worldPacket.WriteString(InviterName);
    _worldPacket.WriteByteSeq(OldGuildGuid[7]);
    _worldPacket.WriteByteSeq(OldGuildGuid[6]);
    _worldPacket.WriteByteSeq(OldGuildGuid[5]);
    _worldPacket.WriteByteSeq(OldGuildGuid[0]);
    _worldPacket.WriteByteSeq(GuildGuid[4]);
    _worldPacket.WriteString(GuildName);
    _worldPacket.WriteByteSeq(GuildGuid[5]);
    _worldPacket.WriteByteSeq(GuildGuid[3]);
    _worldPacket.WriteByteSeq(OldGuildGuid[4]);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildEvent::Write()
{
    _worldPacket << uint8(Type);
    _worldPacket << uint8(Params.size());
    for (std::string_view param : Params)
        _worldPacket << param;

    // 4.3.4 always sends the guid
    _worldPacket << Guid;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildEventLogQueryResults::Write()
{
    _worldPacket.WriteBits(Entry.size(), 23);

    for (GuildEventEntry const& entry : Entry)
    {
        _worldPacket.WriteBit(entry.PlayerGUID[2]);
        _worldPacket.WriteBit(entry.PlayerGUID[4]);
        _worldPacket.WriteBit(entry.OtherGUID[7]);
        _worldPacket.WriteBit(entry.OtherGUID[6]);
        _worldPacket.WriteBit(entry.PlayerGUID[3]);
        _worldPacket.WriteBit(entry.OtherGUID[3]);
        _worldPacket.WriteBit(entry.OtherGUID[5]);
        _worldPacket.WriteBit(entry.PlayerGUID[7]);
        _worldPacket.WriteBit(entry.PlayerGUID[5]);
        _worldPacket.WriteBit(entry.PlayerGUID[0]);
        _worldPacket.WriteBit(entry.OtherGUID[4]);
        _worldPacket.WriteBit(entry.OtherGUID[2]);
        _worldPacket.WriteBit(entry.OtherGUID[0]);
        _worldPacket.WriteBit(entry.OtherGUID[1]);
        _worldPacket.WriteBit(entry.PlayerGUID[1]);
        _worldPacket.WriteBit(entry.PlayerGUID[6]);
    }

    _worldPacket.FlushBits();

    for (GuildEventEntry const& entry : Entry)
    {
        _worldPacket.WriteByteSeq(entry.OtherGUID[3]);
        _worldPacket.WriteByteSeq(entry.OtherGUID[2]);
        _worldPacket.WriteByteSeq(entry.OtherGUID[5]);
        _worldPacket << uint8(entry.RankID);
        _worldPacket.WriteByteSeq(entry.OtherGUID[4]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[0]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[4]);
        _worldPacket << uint32(entry.TransactionDate);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[7]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[3]);
        _worldPacket.WriteByteSeq(entry.OtherGUID[0]);
        _worldPacket.WriteByteSeq(entry.OtherGUID[6]);
        _worldPacket.WriteByteSeq(entry.OtherGUID[7]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[5]);
        _worldPacket << uint8(entry.TransactionType);
        _worldPacket.WriteByteSeq(entry.OtherGUID[1]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[2]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[6]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[1]);
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildPermissionsQueryResults::Write()
{
    _worldPacket << uint32(RankID);
    _worldPacket << uint32(NumTabs);
    _worldPacket << uint32(Flags);
    _worldPacket << uint32(WithdrawGoldLimit);
    _worldPacket.WriteBits(Tab.size(), 23);

    for (GuildRankTabPermissions const& tab : Tab)
    {
        _worldPacket << int32(tab.Flags);
        _worldPacket << int32(tab.WithdrawItemLimit);
    }

    return &_worldPacket;
}

void WorldPackets::Guild::GuildSetRankPermissions::Read()
{
    _worldPacket >> OldRankID;
    _worldPacket >> OldFlags;
    _worldPacket >> Flags;

    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; i++)
    {
        _worldPacket >> TabFlags[i];
        _worldPacket >> TabWithdrawItemLimit[i];
    }

    _worldPacket >> WithdrawGoldLimit;
    _worldPacket >> RankID;

    uint32 length = _worldPacket.ReadBits(7);
    RankName.ReadWithLength(_worldPacket, length);
}

void WorldPackets::Guild::GuildAddRank::Read()
{
    uint32 rankOrder;
    _worldPacket >> rankOrder;

    uint32 length = _worldPacket.ReadBits(7);
    Name.ReadWithLength(_worldPacket, length);
}

void WorldPackets::Guild::GuildDeleteRank::Read()
{
    _worldPacket >> RankOrder;
}

void WorldPackets::Guild::GuildUpdateInfoText::Read()
{
    uint32 length = _worldPacket.ReadBits(12);
    InfoText.ReadWithLength(_worldPacket, length);
}

void WorldPackets::Guild::GuildSetMemberNote::Read()
{
    NoteeGUID[1] = _worldPacket.ReadBit();
    NoteeGUID[4] = _worldPacket.ReadBit();
    NoteeGUID[5] = _worldPacket.ReadBit();
    NoteeGUID[3] = _worldPacket.ReadBit();
    NoteeGUID[0] = _worldPacket.ReadBit();
    NoteeGUID[7] = _worldPacket.ReadBit();
    IsPublic = _worldPacket.ReadBit();
    NoteeGUID[6] = _worldPacket.ReadBit();
    uint32 noteLength = _worldPacket.ReadBits(8);
    NoteeGUID[2] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(NoteeGUID[4]);
    _worldPacket.ReadByteSeq(NoteeGUID[5]);
    _worldPacket.ReadByteSeq(NoteeGUID[0]);
    _worldPacket.ReadByteSeq(NoteeGUID[3]);
    _worldPacket.ReadByteSeq(NoteeGUID[1]);
    _worldPacket.ReadByteSeq(NoteeGUID[6]);
    _worldPacket.ReadByteSeq(NoteeGUID[7]);
    Note.ReadWithLength(_worldPacket, noteLength);
    _worldPacket.ReadByteSeq(NoteeGUID[2]);
}

void WorldPackets::Guild::GuildDemoteMember::Read()
{
    Demotee[7] = _worldPacket.ReadBit();
    Demotee[1] = _worldPacket.ReadBit();
    Demotee[5] = _worldPacket.ReadBit();
    Demotee[6] = _worldPacket.ReadBit();
    Demotee[2] = _worldPacket.ReadBit();
    Demotee[3] = _worldPacket.ReadBit();
    Demotee[0] = _worldPacket.ReadBit();
    Demotee[4] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(Demotee[1]);
    _worldPacket.ReadByteSeq(Demotee[2]);
    _worldPacket.ReadByteSeq(Demotee[7]);
    _worldPacket.ReadByteSeq(Demotee[5]);
    _worldPacket.ReadByteSeq(Demotee[6]);
    _worldPacket.ReadByteSeq(Demotee[0]);
    _worldPacket.ReadByteSeq(Demotee[4]);
    _worldPacket.ReadByteSeq(Demotee[3]);
}

void WorldPackets::Guild::GuildPromoteMember::Read()
{
    Promotee[7] = _worldPacket.ReadBit();
    Promotee[2] = _worldPacket.ReadBit();
    Promotee[5] = _worldPacket.ReadBit();
    Promotee[6] = _worldPacket.ReadBit();
    Promotee[1] = _worldPacket.ReadBit();
    Promotee[0] = _worldPacket.ReadBit();
    Promotee[3] = _worldPacket.ReadBit();
    Promotee[4] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(Promotee[0]);
    _worldPacket.ReadByteSeq(Promotee[5]);
    _worldPacket.ReadByteSeq(Promotee[2]);
    _worldPacket.ReadByteSeq(Promotee[3]);
    _worldPacket.ReadByteSeq(Promotee[6]);
    _worldPacket.ReadByteSeq(Promotee[4]);
    _worldPacket.ReadByteSeq(Promotee[1]);
    _worldPacket.ReadByteSeq(Promotee[7]);
}

void WorldPackets::Guild::GuildOfficerRemoveMember::Read()
{
    Removee[6] = _worldPacket.ReadBit();
    Removee[5] = _worldPacket.ReadBit();
    Removee[4] = _worldPacket.ReadBit();
    Removee[0] = _worldPacket.ReadBit();
    Removee[1] = _worldPacket.ReadBit();
    Removee[3] = _worldPacket.ReadBit();
    Removee[7] = _worldPacket.ReadBit();
    Removee[2] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(Removee[2]);
    _worldPacket.ReadByteSeq(Removee[6]);
    _worldPacket.ReadByteSeq(Removee[5]);
    _worldPacket.ReadByteSeq(Removee[7]);
    _worldPacket.ReadByteSeq(Removee[1]);
    _worldPacket.ReadByteSeq(Removee[4]);
    _worldPacket.ReadByteSeq(Removee[3]);
    _worldPacket.ReadByteSeq(Removee[0]);
}

void WorldPackets::Guild::GuildBankActivate::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> FullUpdate;
}

void WorldPackets::Guild::GuildBankBuyTab::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> BankTab;
}

void WorldPackets::Guild::GuildBankUpdateTab::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> BankTab;
    _worldPacket >> Name;
    _worldPacket >> Icon;
}

void WorldPackets::Guild::GuildBankDepositMoney::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> Money;
}

void WorldPackets::Guild::GuildBankQueryTab::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> Tab;
    _worldPacket >> FullUpdate;
}

WorldPacket const* WorldPackets::Guild::GuildBankRemainingWithdrawMoney::Write()
{
    _worldPacket << int64(RemainingWithdrawMoney);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildBankWithdrawMoney::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> Money;
}

WorldPacket const* WorldPackets::Guild::GuildBankQueryResults::Write()
{
    _worldPacket.WriteBit(FullUpdate);
    _worldPacket.WriteBits(ItemInfo.size(), 20);
    _worldPacket.WriteBits(TabInfo.size(), 22);

    for (GuildBankItemInfo const& item : ItemInfo)
    {
        _worldPacket.WriteBit(item.Locked);
        _worldPacket.WriteBits(item.SocketEnchant.size(), 23);
    }

    for (GuildBankTabInfo const& tab : TabInfo)
    {
        _worldPacket.WriteBits(tab.Icon.length(), 9);
        _worldPacket.WriteBits(tab.Name.length(), 7);
    }

    _worldPacket.FlushBits();

    for (GuildBankTabInfo const& tab : TabInfo)
    {
        _worldPacket.WriteString(tab.Icon);
        _worldPacket << uint32(tab.TabIndex);
        _worldPacket.WriteString(tab.Name);
    }

    _worldPacket << uint64(Money);

    for (GuildBankItemInfo const& item : ItemInfo)
    {
        for (GuildBankSocketEnchant const& socketEnchant : item.SocketEnchant)
        {
            _worldPacket << uint32(socketEnchant.SocketIndex);
            _worldPacket << uint32(socketEnchant.SocketEnchantID);
        }

        _worldPacket << uint32(item.EnchantmentID);
        _worldPacket << uint32(item.ReforgeEnchantmentID);
        _worldPacket << uint32(item.OnUseEnchantmentID);
        _worldPacket << uint32(item.Count);
        _worldPacket << uint32(item.Slot);
        _worldPacket << uint32(item.Flags);
        _worldPacket << uint32(item.ItemID);
        _worldPacket << uint32(item.RandomPropertiesID);
        _worldPacket << uint32(item.Charges);
        _worldPacket << uint32(item.RandomPropertiesSeed);
    }

    _worldPacket << uint32(Tab);
    _withdrawalsRemainingPos = _worldPacket.wpos();
    _worldPacket << int32(WithdrawalsRemaining);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildBankQueryResults::SetWithdrawalsRemaining(int32 withdrawalsRemaining)
{
    WithdrawalsRemaining = withdrawalsRemaining;
    _worldPacket.put<int32>(_withdrawalsRemainingPos, withdrawalsRemaining);
}

void WorldPackets::Guild::GuildBankSwapItems::Read()
{
    _worldPacket >> Banker;
    _worldPacket >> BankOnly;

    if (BankOnly)
    {
        // dest
        _worldPacket >> BankTab;
        _worldPacket >> BankSlot;
        _worldPacket >> ItemID;

        // src
        _worldPacket >> BankTab1;
        _worldPacket >> BankSlot1;
        _worldPacket >> ItemID1;

        _worldPacket >> ToSlot;
        _worldPacket >> StackCount;
    }
    else
    {
        _worldPacket >> BankTab;
        _worldPacket >> BankSlot;
        _worldPacket >> ItemID;

        _worldPacket >> AutoStore;
        if (AutoStore)
        {
            _worldPacket >> StackCount;
            _worldPacket >> ToSlot;
            _worldPacket >> BankItemCount;
        }
        else
        {
            _worldPacket >> ContainerSlot;
            _worldPacket >> ContainerItemSlot;
            _worldPacket >> ToSlot;
            _worldPacket >> StackCount;
        }
    }
}

void WorldPackets::Guild::GuildBankLogQuery::Read()
{
    _worldPacket >> Tab;
}

WorldPacket const* WorldPackets::Guild::GuildBankLogQueryResults::Write()
{
    _worldPacket.WriteBit(WeeklyBonusMoney.has_value());
    _worldPacket.WriteBits(Entry.size(), 23);

    for (GuildBankLogEntry const& entry : Entry)
    {
        _worldPacket.WriteBit(entry.Money.has_value());
        _worldPacket.WriteBit(entry.PlayerGUID[4]);
        _worldPacket.WriteBit(entry.PlayerGUID[1]);
        _worldPacket.WriteBit(entry.ItemID.has_value());
        _worldPacket.WriteBit(entry.Count.has_value());
        _worldPacket.WriteBit(entry.PlayerGUID[2]);
        _worldPacket.WriteBit(entry.PlayerGUID[5]);
        _worldPacket.WriteBit(entry.PlayerGUID[3]);
        _worldPacket.WriteBit(entry.PlayerGUID[6]);
        _worldPacket.WriteBit(entry.PlayerGUID[0]);
        _worldPacket.WriteBit(entry.OtherTab.has_value());
        _worldPacket.WriteBit(entry.PlayerGUID[7]);
    }

    _worldPacket.FlushBits();

    for (GuildBankLogEntry const& entry : Entry)
    {
        _worldPacket.WriteByteSeq(entry.PlayerGUID[6]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[1]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[5]);

        if (entry.Count)
            _worldPacket << int32(*entry.Count);

        _worldPacket << int8(entry.EntryType);

        _worldPacket.WriteByteSeq(entry.PlayerGUID[2]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[4]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[0]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[7]);
        _worldPacket.WriteByteSeq(entry.PlayerGUID[3]);

        if (entry.ItemID)
            _worldPacket << int32(*entry.ItemID);

        _worldPacket << uint32(entry.TimeOffset);

        if (entry.Money)
            _worldPacket << uint64(*entry.Money);

        if (entry.OtherTab)
            _worldPacket << int8(*entry.OtherTab);
    }

    _worldPacket << int32(Tab);

    if (WeeklyBonusMoney)
        _worldPacket << uint64(*WeeklyBonusMoney);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildBankTextQuery::Read()
{
    uint32 tab;
    _worldPacket >> tab;
    Tab = uint8(tab);
}

WorldPacket const* WorldPackets::Guild::GuildBankTextQueryResult::Write()
{
    _worldPacket.WriteBits(Text.length(), 14);
    _worldPacket.FlushBits();

    _worldPacket << uint32(Tab);
    _worldPacket.WriteString(Text);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildBankSetTabText::Read()
{
    uint32 tab;
    _worldPacket >> tab;
    Tab = uint8(tab);

    uint32 length = _worldPacket.ReadBits(14);
    TabText.ReadWithLength(_worldPacket, length);
}

void WorldPackets::Guild::GuildSetGuildMaster::Read()
{
    uint32 length = _worldPacket.ReadBits(7);
    IsDethrone = _worldPacket.ReadBit();
    NewMasterName = _worldPacket.ReadString(length);
}

void WorldPackets::Guild::SaveGuildEmblem::Read()
{
    _worldPacket >> Vendor;
    _worldPacket >> EStyle;
    _worldPacket >> EColor;
    _worldPacket >> BStyle;
    _worldPacket >> BColor;
    _worldPacket >> Bg;
}

WorldPacket const* WorldPackets::Guild::PlayerSaveGuildEmblem::Write()
{
    _worldPacket << int32(Error);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildChallengeUpdate::Write()
{
    for (int32 xp : Xp)
        _worldPacket << int32(xp);

    for (int32 gold : MaxLevelGold)
        _worldPacket << int32(gold);

    for (int32 count : MaxCount)
        _worldPacket << int32(count);

    for (int32 gold : Gold)
        _worldPacket << int32(gold);

    for (int32 count : CurrentCount)
        _worldPacket << int32(count);

    return &_worldPacket;
}
