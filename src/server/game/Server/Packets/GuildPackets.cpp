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
#include "Item.h"

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
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(PlayerGuid, { 7, 3, 4 });
    _worldPacket.ReadGuidMask(GuildGuid, { 3, 4 });
    _worldPacket.ReadGuidMask(PlayerGuid, { 2, 6 });
    _worldPacket.ReadGuidMask(GuildGuid, { 2, 5 });
    _worldPacket.ReadGuidMask(PlayerGuid, { 1, 5 });
    _worldPacket.ReadGuidMask(GuildGuid, { 7 });
    _worldPacket.ReadGuidMask(PlayerGuid, { 0 });
    _worldPacket.ReadGuidMask(GuildGuid, { 1, 6, 0 });
    _worldPacket.ReadGuidBytes(PlayerGuid, { 7 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 2, 4, 7 });
    _worldPacket.ReadGuidBytes(PlayerGuid, { 6, 0 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 6, 0, 3 });
    _worldPacket.ReadGuidBytes(PlayerGuid, { 2 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 5 });
    _worldPacket.ReadGuidBytes(PlayerGuid, { 3 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 1 });
    _worldPacket.ReadGuidBytes(PlayerGuid, { 4, 1, 5 });
}

WorldPackets::Guild::QueryGuildInfoResponse::QueryGuildInfoResponse()
        : ServerPacket(SMSG_QUERY_GUILD_INFO_RESPONSE) { }

WorldPacket const* WorldPackets::Guild::QueryGuildInfoResponse::Write()
{
    // 5.4.8 18414, from Project SkyFire
    ObjectGuid guid = GuildGuid;
    _worldPacket.WriteGuidMask(guid, { 5 });
    _worldPacket.WriteBit(true);                            // has guild info
    _worldPacket.WriteBits(Info.RankCount, 21);
    _worldPacket.WriteGuidMask(guid, { 5, 1, 4, 7 });
    for (uint32 i = 0; i < Info.RankCount; ++i)
        _worldPacket.WriteBits(Info.Ranks[i].length(), 7);
    _worldPacket.WriteGuidMask(guid, { 3, 2, 0, 6 });
    _worldPacket.WriteBits(Info.GuildName.length(), 7);
    _worldPacket.WriteGuidMask(guid, { 3, 7, 2, 1, 0, 4, 6 });
    _worldPacket.FlushBits();

    _worldPacket << uint32(Info.BorderStyle);
    _worldPacket << uint32(Info.EmblemStyle);
    _worldPacket.WriteGuidBytes(guid, { 2, 7 });
    _worldPacket << uint32(Info.EmblemColor);
    _worldPacket << uint32(VirtualRealmAddress);
    for (uint32 i = 0; i < Info.RankCount; ++i)
    {
        _worldPacket << uint32(Info.RankOrders[i]);
        _worldPacket << uint32(Info.RankIds[i]);
        _worldPacket.WriteString(Info.Ranks[i]);
    }
    _worldPacket.WriteString(Info.GuildName);
    _worldPacket << uint32(Info.BorderColor);
    _worldPacket.WriteGuidBytes(guid, { 5, 4 });
    _worldPacket << uint32(Info.BackgroundColor);
    _worldPacket.WriteGuidBytes(guid, { 1, 6, 0, 3 });
    _worldPacket.WriteGuidBytes(guid, { 2, 6, 4, 0, 7, 3, 5, 1 });

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
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(GuildGuid, { 5, 6, 0, 1, 3, 7, 4, 2 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 4, 6, 3, 0, 7, 5, 2, 1 });
}

void WorldPackets::Guild::GuildRequestMaxDailyXP::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(GuildGuid, { 0, 3, 5, 1, 4, 6, 7, 2 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 7, 4, 3, 5, 1, 2, 6, 0 });
}

WorldPacket const* WorldPackets::Guild::GuildMaxDailyXP::Write()
{
    _worldPacket << uint64(Xp);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildXP::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << uint64(GuildCurrentXP);
    _worldPacket << uint64(WeeklyXP);
    _worldPacket << uint64(TotalXP);
    _worldPacket << uint64(GuildRemainingXP);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildReputationWeeklyCap::Write()
{
    _worldPacket << uint32(Cap);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildRoster::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteBits(MemberData.size(), 17);
    _worldPacket.WriteBits(WelcomeText.length(), 10);
    for (GuildRosterMemberData const& member : MemberData)
    {
        _worldPacket.WriteBits(member.OfficerNote.length(), 8);
        _worldPacket.WriteBit(member.Guid[5]);
        _worldPacket.WriteBit(member.SorEligible);
        _worldPacket.WriteBits(member.Note.length(), 8);
        _worldPacket.WriteBit(member.Guid[7]);
        _worldPacket.WriteBit(member.Guid[0]);
        _worldPacket.WriteBit(member.Guid[6]);
        _worldPacket.WriteBits(member.Name.length(), 6);
        _worldPacket.WriteBit(member.Authenticated);
        _worldPacket.WriteBit(member.Guid[3]);
        _worldPacket.WriteBit(member.Guid[4]);
        _worldPacket.WriteBit(member.Guid[1]);
        _worldPacket.WriteBit(member.Guid[2]);
    }

    _worldPacket.WriteBits(InfoText.length(), 11);
    _worldPacket.FlushBits();

    for (GuildRosterMemberData const& member : MemberData)
        _worldPacket << member;

    _worldPacket << uint32(NumAccounts);
    _worldPacket.AppendPackedTime(CreateDate);
    _worldPacket.WriteString(InfoText);
    _worldPacket << uint32(WeeklyRepCap);
    _worldPacket.WriteString(WelcomeText);
    _worldPacket << uint32(GuildFlags);

    return &_worldPacket;
}

// Cata guild profession tracking (see .agents/docs/porting-from-trinitycore.md).
ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Guild::GuildRosterProfessionData const& rosterProfessionData)
{
    data << int32(rosterProfessionData.DbID);
    data << int32(rosterProfessionData.Rank);
    data << int32(rosterProfessionData.Step);

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Guild::GuildRosterMemberData const& rosterMemberData)
{
    // 5.4.8 18414, from Project SkyFire
    data << uint8(rosterMemberData.ClassID);
    data << uint32(rosterMemberData.GuildReputation);
    data.WriteString(rosterMemberData.Name);
    data.WriteByteSeq(rosterMemberData.Guid[0]);
    for (WorldPackets::Guild::GuildRosterProfessionData const& profession : rosterMemberData.Profession)
        data << profession;
    data << uint8(rosterMemberData.Level);
    data << uint8(rosterMemberData.Status);
    data << uint32(rosterMemberData.AreaID);
    data << uint32(rosterMemberData.GuildRepToCap);
    data.WriteByteSeq(rosterMemberData.Guid[3]);
    data << uint64(rosterMemberData.TotalXP);
    data.WriteString(rosterMemberData.OfficerNote);
    data << float(rosterMemberData.LastSave);
    data << uint8(rosterMemberData.Gender);
    data << uint32(rosterMemberData.RankID);
    data << int32(rosterMemberData.VirtualRealmAddress);
    data.WriteByteSeq(rosterMemberData.Guid[5]);
    data.WriteByteSeq(rosterMemberData.Guid[7]);
    data.WriteString(rosterMemberData.Note);
    data.WriteByteSeq(rosterMemberData.Guid[4]);
    data << uint64(rosterMemberData.WeeklyXP);
    data << uint32(rosterMemberData.PersonalAchievementPoints);
    data.WriteByteSeq(rosterMemberData.Guid[6]);
    data.WriteByteSeq(rosterMemberData.Guid[1]);
    data.WriteByteSeq(rosterMemberData.Guid[2]);

    return data;
}

void WorldPackets::Guild::GuildQueryRanks::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(GuildGuid, { 0, 2, 5, 4, 3, 7, 6, 1 });
    _worldPacket.ReadGuidBytes(GuildGuid, { 6, 0, 1, 7, 3, 2, 5, 4 });
}

WorldPacket const* WorldPackets::Guild::GuildRanks::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteBits(Ranks.size(), 17);
    for (GuildRankData const& rank : Ranks)
        _worldPacket.WriteBits(rank.RankName.length(), 7);
    _worldPacket.FlushBits();

    for (GuildRankData const& rank : Ranks)
        _worldPacket << rank;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Guild::GuildRankData const& rankData)
{
    // 5.4.8 18414, from Project SkyFire
    data << uint32(rankData.RankOrder);
    data << uint32(rankData.WithdrawGoldLimit);
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; i++)
    {
        data << uint32(rankData.TabWithdrawItemLimit[i]);
        data << uint32(rankData.TabFlags[i]);
    }

    data.WriteString(rankData.RankName);
    data << uint32(rankData.RankID);
    data << uint32(rankData.Flags);

    return data;
}

void WorldPackets::Guild::GuildAssignMemberRank::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> RankOrder;
    _worldPacket.ReadGuidMask(Member, { 2, 3, 1, 6, 0, 4, 7, 5 });
    _worldPacket.ReadGuidBytes(Member, { 7, 3, 2, 5, 6, 0, 4, 1 });
}

void WorldPackets::Guild::GuildSwitchRank::Read()
{
    _worldPacket >> RankOrder;
    Up = _worldPacket.ReadBit();
}

void WorldPackets::Guild::GuildUpdateMotdText::Read()
{
    // 5.4.8 18414, from Project SkyFire
    uint32 length = _worldPacket.ReadBits(10);
    MotdText.ReadWithLength(_worldPacket, length);
}

WorldPacket const* WorldPackets::Guild::GuildCommandResult::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << uint32(Command);
    _worldPacket << uint32(Result);
    _worldPacket.WriteBits(Name.length(), 8);
    _worldPacket.FlushBits();
    _worldPacket.WriteString(Name);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildInviteByName::Read()
{
    // 5.4.8 18414, from Project SkyFire
    uint32 length = _worldPacket.ReadBits(9);
    Name.ReadWithLength(_worldPacket, length);
}

WorldPacket const* WorldPackets::Guild::GuildInvite::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(GuildGuid, { 4 });
    _worldPacket.WriteBits(GuildName.length(), 7);
    _worldPacket.WriteGuidMask(OldGuildGuid, { 4 });
    _worldPacket.WriteGuidMask(GuildGuid, { 6 });
    _worldPacket.WriteGuidMask(OldGuildGuid, { 2, 1, 5, 7 });
    _worldPacket.WriteGuidMask(GuildGuid, { 0 });
    _worldPacket.WriteGuidMask(OldGuildGuid, { 3 });
    _worldPacket.WriteGuidMask(GuildGuid, { 5 });
    _worldPacket.WriteGuidMask(OldGuildGuid, { 6 });
    _worldPacket.WriteBits(InviterName.length(), 6);
    _worldPacket.WriteGuidMask(GuildGuid, { 1, 3 });
    _worldPacket.WriteGuidMask(OldGuildGuid, { 0 });
    _worldPacket.WriteGuidMask(GuildGuid, { 7 });
    _worldPacket.WriteBits(OldGuildName.length(), 7);
    _worldPacket.WriteGuidMask(GuildGuid, { 2 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(GuildGuid, { 1 });
    _worldPacket << uint32(BackgroundColor);
    _worldPacket.WriteGuidBytes(GuildGuid, { 4 });
    _worldPacket.WriteString(InviterName);
    _worldPacket << uint32(BorderStyle);
    _worldPacket.WriteGuidBytes(OldGuildGuid, { 7 });
    _worldPacket.WriteGuidBytes(GuildGuid, { 0, 2 });
    _worldPacket << uint32(EmblemColor);
    _worldPacket.WriteGuidBytes(OldGuildGuid, { 2, 5 });
    _worldPacket << uint32(GuildLevel);
    _worldPacket << uint32(OldGuildGuid ? VirtualRealmAddress : 0);
    _worldPacket.WriteGuidBytes(GuildGuid, { 7, 3 });
    _worldPacket.WriteGuidBytes(OldGuildGuid, { 4 });
    _worldPacket << uint32(BorderColor);
    _worldPacket.WriteString(GuildName);
    _worldPacket << uint32(VirtualRealmAddress);              // inviter realm
    _worldPacket << uint32(EmblemStyle);
    _worldPacket.WriteGuidBytes(OldGuildGuid, { 0 });
    _worldPacket.WriteString(OldGuildName);
    _worldPacket.WriteGuidBytes(GuildGuid, { 5 });
    _worldPacket << uint32(VirtualRealmAddress);              // guild realm
    _worldPacket.WriteGuidBytes(OldGuildGuid, { 1 });
    _worldPacket.WriteGuidBytes(GuildGuid, { 6 });
    _worldPacket.WriteGuidBytes(OldGuildGuid, { 3, 6 });

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
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteBits(Entry.size(), 21);
    for (GuildEventEntry const& entry : Entry)
    {
        _worldPacket.WriteGuidMask(entry.PlayerGUID, { 6, 1 });
        _worldPacket.WriteGuidMask(entry.OtherGUID, { 5, 1, 3, 0, 4 });
        _worldPacket.WriteGuidMask(entry.PlayerGUID, { 4 });
        _worldPacket.WriteGuidMask(entry.OtherGUID, { 7 });
        _worldPacket.WriteGuidMask(entry.PlayerGUID, { 0, 2, 7, 3, 5 });
        _worldPacket.WriteGuidMask(entry.OtherGUID, { 2, 6 });
    }

    _worldPacket.FlushBits();

    for (GuildEventEntry const& entry : Entry)
    {
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 5, 4 });
        _worldPacket.WriteGuidBytes(entry.OtherGUID, { 6 });
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 2 });
        _worldPacket.WriteGuidBytes(entry.OtherGUID, { 4 });
        _worldPacket << uint8(entry.TransactionType);
        _worldPacket.WriteGuidBytes(entry.OtherGUID, { 0 });
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 7, 3 });
        _worldPacket.WriteGuidBytes(entry.OtherGUID, { 5, 2 });
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 0 });
        _worldPacket << uint32(entry.TransactionDate);
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 1, 6 });
        _worldPacket.WriteGuidBytes(entry.OtherGUID, { 7, 1 });
        _worldPacket << uint8(entry.RankID);
        _worldPacket.WriteGuidBytes(entry.OtherGUID, { 3 });
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildPermissionsQueryResults::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << uint32(RankID);
    _worldPacket << uint32(WithdrawGoldLimit);
    _worldPacket << uint32(NumTabs);
    _worldPacket << uint32(Flags);
    _worldPacket.WriteBits(Tab.size(), 21);
    _worldPacket.FlushBits();

    for (GuildRankTabPermissions const& tab : Tab)
    {
        _worldPacket << int32(tab.WithdrawItemLimit);
        _worldPacket << int32(tab.Flags);
    }

    return &_worldPacket;
}

void WorldPackets::Guild::GuildSetRankPermissions::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> OldRankID;
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; i++)
    {
        _worldPacket >> TabFlags[i];
        _worldPacket >> TabWithdrawItemLimit[i];
    }

    _worldPacket >> WithdrawGoldLimit;
    _worldPacket >> OldFlags;
    _worldPacket >> Flags;
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
    // 5.4.8 18414, from Project SkyFire
    uint32 length = _worldPacket.ReadBits(11);
    InfoText.ReadWithLength(_worldPacket, length);
}

void WorldPackets::Guild::GuildSetMemberNote::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(NoteeGUID, { 1 });
    uint32 noteLength = _worldPacket.ReadBits(8);
    _worldPacket.ReadGuidMask(NoteeGUID, { 4, 2 });
    IsPublic = _worldPacket.ReadBit();
    _worldPacket.ReadGuidMask(NoteeGUID, { 3, 5, 0, 6, 7 });
    _worldPacket.ReadGuidBytes(NoteeGUID, { 5, 1, 6 });
    Note.ReadWithLength(_worldPacket, noteLength);
    _worldPacket.ReadGuidBytes(NoteeGUID, { 0, 7, 4, 3, 2 });
}

void WorldPackets::Guild::GuildDemoteMember::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(Demotee, { 3, 6, 0, 2, 7, 5, 4, 1 });
    _worldPacket.ReadGuidBytes(Demotee, { 7, 4, 2, 5, 1, 3, 0, 6 });
}

void WorldPackets::Guild::GuildPromoteMember::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(Promotee, { 6, 0, 4, 3, 1, 7, 2, 5 });
    _worldPacket.ReadGuidBytes(Promotee, { 1, 7, 2, 5, 3, 4, 0, 6 });
}

void WorldPackets::Guild::GuildOfficerRemoveMember::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(Removee, { 7, 3, 4, 2, 5, 6, 1, 0 });
    _worldPacket.ReadGuidBytes(Removee, { 0, 2, 5, 6, 7, 1, 4, 3 });
}

void WorldPackets::Guild::GuildBankActivate::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(Banker, { 3 });
    FullUpdate = _worldPacket.ReadBit();
    _worldPacket.ReadGuidMask(Banker, { 0, 7, 1, 5, 2, 6, 4 });
    _worldPacket.ReadGuidBytes(Banker, { 7, 1, 0, 6, 4, 2, 5, 3 });
}

void WorldPackets::Guild::GuildBankBuyTab::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> BankTab;
    _worldPacket.ReadGuidMask(Banker, { 0, 1, 3, 7, 2, 6, 5, 4 });
    _worldPacket.ReadGuidBytes(Banker, { 1, 4, 6, 7, 3, 5, 2, 0 });
}

void WorldPackets::Guild::GuildBankUpdateTab::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> BankTab;
    _worldPacket.ReadGuidMask(Banker, { 5 });
    uint32 iconLength = _worldPacket.ReadBits(9);
    _worldPacket.ReadGuidMask(Banker, { 1, 4, 2, 7, 0, 6, 3 });
    uint32 nameLength = _worldPacket.ReadBits(7);
    _worldPacket.ReadGuidBytes(Banker, { 7, 4 });
    Icon.ReadWithLength(_worldPacket, iconLength);
    _worldPacket.ReadGuidBytes(Banker, { 5, 1, 0 });
    Name.ReadWithLength(_worldPacket, nameLength);
    _worldPacket.ReadGuidBytes(Banker, { 2, 3, 6 });
}

void WorldPackets::Guild::GuildBankDepositMoney::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> Money;
    _worldPacket.ReadGuidMask(Banker, { 2, 7, 6, 4, 0, 1, 5, 3 });
    _worldPacket.ReadGuidBytes(Banker, { 1, 4, 5, 0, 2, 7, 6, 3 });
}

void WorldPackets::Guild::GuildBankQueryTab::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> Tab;
    _worldPacket.ReadGuidMask(Banker, { 7, 3 });
    FullUpdate = _worldPacket.ReadBit();
    _worldPacket.ReadGuidMask(Banker, { 0, 2, 4, 1, 6, 5 });
    _worldPacket.ReadGuidBytes(Banker, { 3, 7, 6, 4, 2, 5, 0, 1 });
}

WorldPacket const* WorldPackets::Guild::GuildBankRemainingWithdrawMoney::Write()
{
    _worldPacket << int64(RemainingWithdrawMoney);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildBankWithdrawMoney::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> Money;
    _worldPacket.ReadGuidMask(Banker, { 1, 3, 7, 6, 5, 0, 4, 2 });
    _worldPacket.ReadGuidBytes(Banker, { 0, 7, 4, 2, 1, 6, 3, 5 });
}

WorldPacket const* WorldPackets::Guild::GuildBankQueryResults::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << uint32(Tab);
    _worldPacket << uint64(Money);
    _withdrawalsRemainingPos = _worldPacket.wpos();
    _worldPacket << int32(WithdrawalsRemaining);
    _worldPacket.WriteBit(FullUpdate);
    _worldPacket.WriteBits(TabInfo.size(), 21);
    _worldPacket.WriteBits(ItemInfo.size(), 18);
    for (GuildBankTabInfo const& tab : TabInfo)
    {
        _worldPacket.WriteBits(tab.Icon.length(), 9);
        _worldPacket.WriteBits(tab.Name.length(), 7);
    }

    ByteBuffer itemData;
    for (GuildBankItemInfo const& item : ItemInfo)
    {
        // enchantment id and enchantment slot pairs
        std::vector<std::pair<int32, uint32>> enchants;
        if (item.EnchantmentID)
            enchants.emplace_back(item.EnchantmentID, uint32(PERM_ENCHANTMENT_SLOT));
        for (GuildBankSocketEnchant const& socketEnchant : item.SocketEnchant)
            enchants.emplace_back(socketEnchant.SocketEnchantID, uint32(SOCK_ENCHANTMENT_SLOT) + socketEnchant.SocketIndex);
        if (item.ReforgeEnchantmentID)
            enchants.emplace_back(item.ReforgeEnchantmentID, uint32(REFORGE_ENCHANTMENT_SLOT));

        _worldPacket.WriteBit(item.Locked);
        _worldPacket.WriteBits(enchants.size(), 21);

        itemData << uint32(0);
        itemData << uint32(0);
        for (auto const& [enchantId, enchantSlot] : enchants)
        {
            itemData << uint32(enchantId);
            itemData << uint32(enchantSlot);
        }

        itemData << uint32(0);
        itemData << uint32(0);                              // item modifiers size
        itemData << uint32(item.ItemID);
        itemData << uint32(item.Charges);
        itemData << uint32(item.Count);
        itemData << uint32(item.Slot);
        itemData << int32(item.RandomPropertiesID);
        itemData << int32(item.RandomPropertiesSeed);
    }

    _worldPacket.FlushBits();
    _worldPacket.append(itemData);

    for (GuildBankTabInfo const& tab : TabInfo)
    {
        _worldPacket << uint32(tab.TabIndex);
        _worldPacket.WriteString(tab.Icon);
        _worldPacket.WriteString(tab.Name);
    }

    return &_worldPacket;
}

void WorldPackets::Guild::GuildBankQueryResults::SetWithdrawalsRemaining(int32 withdrawalsRemaining)
{
    WithdrawalsRemaining = withdrawalsRemaining;
    _worldPacket.put<int32>(_withdrawalsRemainingPos, withdrawalsRemaining);
}

void WorldPackets::Guild::GuildBankSwapItems::Read()
{
    // 5.4.8 18414, from Project SkyFire
    uint32 stackCount;
    uint32 itemId;
    _worldPacket >> stackCount;
    _worldPacket >> BankSlot;
    _worldPacket >> ToSlot;
    _worldPacket >> itemId;
    _worldPacket >> BankTab;
    StackCount = int32(stackCount);
    ItemID = itemId;

    Banker[5] = _worldPacket.ReadBit();
    bool hasBankTab1 = !_worldPacket.ReadBit();
    Banker[1] = _worldPacket.ReadBit();
    bool hasContainerSlot = !_worldPacket.ReadBit();
    AutoStore = _worldPacket.ReadBit();
    Banker[0] = _worldPacket.ReadBit();
    bool hasItemID1 = !_worldPacket.ReadBit();
    bool hasBankSlot1 = !_worldPacket.ReadBit();
    Banker[2] = _worldPacket.ReadBit();
    BankOnly = _worldPacket.ReadBit();
    Banker[4] = _worldPacket.ReadBit();
    Banker[7] = _worldPacket.ReadBit();
    Banker[3] = _worldPacket.ReadBit();
    bool hasContainerItemSlot = !_worldPacket.ReadBit();
    Banker[6] = _worldPacket.ReadBit();
    bool hasBankItemCount = !_worldPacket.ReadBit();
    _worldPacket.ReadGuidBytes(Banker, { 2, 6, 5, 4, 0, 3, 1, 7 });

    ContainerSlot = NULL_BAG;
    ContainerItemSlot = NULL_SLOT;
    if (hasItemID1)
        _worldPacket >> ItemID1;
    if (hasContainerSlot)
        _worldPacket >> ContainerSlot;
    if (hasContainerItemSlot)
        _worldPacket >> ContainerItemSlot;
    if (hasBankSlot1)
        _worldPacket >> BankSlot1;
    if (hasBankItemCount)
        _worldPacket >> BankItemCount;
    if (hasBankTab1)
        _worldPacket >> BankTab1;
}

void WorldPackets::Guild::GuildBankLogQuery::Read()
{
    _worldPacket >> Tab;
}

WorldPacket const* WorldPackets::Guild::GuildBankLogQueryResults::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << int32(Tab);
    _worldPacket.WriteBit(WeeklyBonusMoney.has_value());
    _worldPacket.WriteBits(Entry.size(), 21);
    for (GuildBankLogEntry const& entry : Entry)
    {
        _worldPacket.WriteBit(entry.Money.has_value());
        _worldPacket.WriteGuidMask(entry.PlayerGUID, { 0, 2, 3, 6, 5, 4 });
        _worldPacket.WriteBit(entry.Count.has_value());
        _worldPacket.WriteBit(entry.ItemID.has_value());
        _worldPacket.WriteGuidMask(entry.PlayerGUID, { 7, 1 });
        _worldPacket.WriteBit(entry.OtherTab.has_value());
    }

    _worldPacket.FlushBits();

    for (GuildBankLogEntry const& entry : Entry)
    {
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 1, 7 });
        if (entry.OtherTab)
            _worldPacket << int8(*entry.OtherTab);
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 2 });
        _worldPacket << uint32(entry.TimeOffset);
        _worldPacket << int8(entry.EntryType);
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 0, 4 });
        if (entry.ItemID)
            _worldPacket << int32(*entry.ItemID);
        if (entry.Money)
            _worldPacket << uint64(*entry.Money);
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 6 });
        if (entry.Count)
            _worldPacket << int32(*entry.Count);
        _worldPacket.WriteGuidBytes(entry.PlayerGUID, { 5, 3 });
    }

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
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << uint32(Tab);
    _worldPacket.WriteBits(Text.length(), 14);
    _worldPacket.FlushBits();
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
    // 5.4.8 18414, from Project SkyFire
    uint32 length = _worldPacket.ReadBits(9);
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

WorldPacket const* WorldPackets::Guild::GuildMemberUpdateNote::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(Member, { 2 });
    _worldPacket.WriteBits(Note.length(), 8);
    _worldPacket.WriteBit(IsPublic);
    _worldPacket.WriteGuidMask(Member, { 5, 0, 4, 3, 1, 6, 7 });
    _worldPacket.FlushBits();
    _worldPacket.WriteGuidBytes(Member, { 7, 5, 0, 1 });
    _worldPacket.WriteString(Note);
    _worldPacket.WriteGuidBytes(Member, { 3, 6, 4, 2 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildRanksUpdate::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(Other, { 5, 6 });
    _worldPacket.WriteGuidMask(Officer, { 0, 1 });
    _worldPacket.WriteGuidMask(Other, { 3 });
    _worldPacket.WriteGuidMask(Officer, { 4 });
    _worldPacket.WriteGuidMask(Other, { 2 });
    _worldPacket.WriteGuidMask(Officer, { 6, 3, 7 });
    _worldPacket.WriteGuidMask(Other, { 4, 0, 1 });
    _worldPacket.WriteGuidMask(Officer, { 2 });
    _worldPacket.WriteGuidMask(Other, { 7 });
    _worldPacket.WriteBit(Promote);
    _worldPacket.WriteGuidMask(Officer, { 5 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(Other, { 2 });
    _worldPacket.WriteGuidBytes(Officer, { 1 });
    _worldPacket.WriteGuidBytes(Other, { 6, 1, 5 });
    _worldPacket.WriteGuidBytes(Officer, { 0 });
    _worldPacket << int32(RankID);
    _worldPacket.WriteGuidBytes(Officer, { 3, 7 });
    _worldPacket.WriteGuidBytes(Other, { 7 });
    _worldPacket.WriteGuidBytes(Officer, { 2 });
    _worldPacket.WriteGuidBytes(Other, { 3, 4 });
    _worldPacket.WriteGuidBytes(Officer, { 6, 5 });
    _worldPacket.WriteGuidBytes(Other, { 0 });
    _worldPacket.WriteGuidBytes(Officer, { 4 });

    return &_worldPacket;
}

void WorldPackets::Guild::GuildRequestPartyState::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(GuildGUID, { 1, 5, 7, 2, 6, 3, 0, 4 });
    _worldPacket.ReadGuidBytes(GuildGUID, { 2, 5, 4, 6, 1, 0, 7, 3 });
}

WorldPacket const* WorldPackets::Guild::GuildPartyState::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket << uint32(NumRequired);
    _worldPacket << float(GuildXPEarnedMult);
    _worldPacket << uint32(NumMembers);
    _worldPacket.WriteBit(InGuildParty);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void WorldPackets::Guild::AutoDeclineGuildInvites::Read()
{
    // 5.4.8 18414, from Project SkyFire
    Allow = _worldPacket.ReadBit();
}

void WorldPackets::Guild::GuildQueryMembersForRecipe::Read()
{
    _worldPacket >> SpellID;
    _worldPacket >> SkillLineID;
    _worldPacket >> UniqueBit;

    GuildGUID[4] = _worldPacket.ReadBit();
    GuildGUID[1] = _worldPacket.ReadBit();
    GuildGUID[0] = _worldPacket.ReadBit();
    GuildGUID[3] = _worldPacket.ReadBit();
    GuildGUID[6] = _worldPacket.ReadBit();
    GuildGUID[7] = _worldPacket.ReadBit();
    GuildGUID[5] = _worldPacket.ReadBit();
    GuildGUID[2] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(GuildGUID[1]);
    _worldPacket.ReadByteSeq(GuildGUID[6]);
    _worldPacket.ReadByteSeq(GuildGUID[5]);
    _worldPacket.ReadByteSeq(GuildGUID[0]);
    _worldPacket.ReadByteSeq(GuildGUID[3]);
    _worldPacket.ReadByteSeq(GuildGUID[7]);
    _worldPacket.ReadByteSeq(GuildGUID[2]);
    _worldPacket.ReadByteSeq(GuildGUID[4]);
}

WorldPacket const* WorldPackets::Guild::GuildMembersWithRecipe::Write()
{
    _worldPacket.WriteBits(Members.size(), 26);

    // TrinityCore writes guid[3] twice here; guid[5] is the bit that belongs in the second slot
    for (ObjectGuid const& guid : Members)
    {
        _worldPacket.WriteBit(guid[2]);
        _worldPacket.WriteBit(guid[5]);
        _worldPacket.WriteBit(guid[1]);
        _worldPacket.WriteBit(guid[6]);
        _worldPacket.WriteBit(guid[0]);
        _worldPacket.WriteBit(guid[7]);
        _worldPacket.WriteBit(guid[4]);
        _worldPacket.WriteBit(guid[3]);
    }

    _worldPacket.FlushBits();

    for (ObjectGuid const& guid : Members)
    {
        _worldPacket.WriteByteSeq(guid[1]);
        _worldPacket.WriteByteSeq(guid[5]);
        _worldPacket.WriteByteSeq(guid[6]);
        _worldPacket.WriteByteSeq(guid[7]);
        _worldPacket.WriteByteSeq(guid[2]);
        _worldPacket.WriteByteSeq(guid[3]);
        _worldPacket.WriteByteSeq(guid[0]);
        _worldPacket.WriteByteSeq(guid[4]);
    }

    _worldPacket << int32(SpellID);
    _worldPacket << uint32(SkillLineID);

    return &_worldPacket;
}

void WorldPackets::Guild::GuildQueryMemberRecipes::Read()
{
    _worldPacket >> SkillLineID;

    GuildMember[2] = _worldPacket.ReadBit();
    GuildGUID[1] = _worldPacket.ReadBit();
    GuildMember[1] = _worldPacket.ReadBit();
    GuildGUID[0] = _worldPacket.ReadBit();
    GuildGUID[6] = _worldPacket.ReadBit();
    GuildMember[7] = _worldPacket.ReadBit();
    GuildGUID[4] = _worldPacket.ReadBit();
    GuildGUID[3] = _worldPacket.ReadBit();
    GuildGUID[7] = _worldPacket.ReadBit();
    GuildMember[5] = _worldPacket.ReadBit();
    GuildMember[0] = _worldPacket.ReadBit();
    GuildGUID[5] = _worldPacket.ReadBit();
    GuildMember[3] = _worldPacket.ReadBit();
    GuildMember[6] = _worldPacket.ReadBit();
    GuildGUID[2] = _worldPacket.ReadBit();
    GuildMember[4] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(GuildMember[2]);
    _worldPacket.ReadByteSeq(GuildMember[6]);
    _worldPacket.ReadByteSeq(GuildGUID[4]);
    _worldPacket.ReadByteSeq(GuildGUID[2]);
    _worldPacket.ReadByteSeq(GuildMember[1]);
    _worldPacket.ReadByteSeq(GuildGUID[7]);
    _worldPacket.ReadByteSeq(GuildGUID[3]);
    _worldPacket.ReadByteSeq(GuildGUID[1]);
    _worldPacket.ReadByteSeq(GuildMember[3]);
    _worldPacket.ReadByteSeq(GuildMember[0]);
    _worldPacket.ReadByteSeq(GuildGUID[0]);
    _worldPacket.ReadByteSeq(GuildMember[7]);
    _worldPacket.ReadByteSeq(GuildMember[4]);
    _worldPacket.ReadByteSeq(GuildGUID[5]);
    _worldPacket.ReadByteSeq(GuildGUID[6]);
    _worldPacket.ReadByteSeq(GuildMember[5]);
}

WorldPacket const* WorldPackets::Guild::GuildMemberRecipes::Write()
{
    _worldPacket.WriteBit(Member[0]);
    _worldPacket.WriteBit(Member[3]);
    _worldPacket.WriteBit(Member[7]);
    _worldPacket.WriteBit(Member[4]);
    _worldPacket.WriteBit(Member[6]);
    _worldPacket.WriteBit(Member[2]);
    _worldPacket.WriteBit(Member[1]);
    _worldPacket.WriteBit(Member[5]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(Member[0]);
    _worldPacket.WriteByteSeq(Member[1]);
    _worldPacket.WriteByteSeq(Member[7]);
    _worldPacket.WriteByteSeq(Member[4]);
    _worldPacket.WriteByteSeq(Member[5]);

    _worldPacket << int32(SkillRank);

    for (uint8 bits : SkillLineBitArray)
        _worldPacket << uint8(bits);

    _worldPacket.WriteByteSeq(Member[3]);
    _worldPacket.WriteByteSeq(Member[6]);
    _worldPacket.WriteByteSeq(Member[2]);

    _worldPacket << int32(SkillLineID);
    _worldPacket << int32(SkillStep);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildChallengeUpdate::Write()
{
    // 5.4.8 18414, from Project SkyFire
    for (int32 count : MaxCount)
        _worldPacket << int32(count);

    for (int32 gold : Gold)
        _worldPacket << int32(gold);

    for (int32 gold : MaxLevelGold)
        _worldPacket << int32(gold);

    for (int32 xp : Xp)
        _worldPacket << int32(xp);

    for (int32 count : CurrentCount)
        _worldPacket << int32(count);

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Guild::GuildRewardItem const& rewardItem)
{
    // 5.4.8 18414, from Project SkyFire
    if (rewardItem.AchievementRequired)
        data << uint32(rewardItem.AchievementRequired);
    data << int32(rewardItem.RaceMask);
    data << uint32(rewardItem.ItemID);
    data << uint32(0);                                      // min guild level
    data << uint32(rewardItem.MinGuildRep);
    data << uint64(rewardItem.Cost);

    return data;
}

void WorldPackets::Guild::RequestGuildRewardsList::Read()
{
    _worldPacket >> CurrentVersion;
}

WorldPacket const* WorldPackets::Guild::GuildRewardList::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteBits(RewardItems.size(), 19);
    for (GuildRewardItem const& item : RewardItems)
        _worldPacket.WriteBits(item.AchievementRequired ? 1 : 0, 22);
    _worldPacket.FlushBits();

    for (GuildRewardItem const& item : RewardItems)
        _worldPacket << item;

    _worldPacket << int32(Version);

    return &_worldPacket;
}

// Cata guild profession/recipe tracking (see .agents/docs/porting-from-trinitycore.md).
WorldPacket const* WorldPackets::Guild::GuildKnownRecipes::Write()
{
    _worldPacket.WriteBits(Recipes.size(), 16);
    _worldPacket.FlushBits();

    for (GuildRecipesData const& recipe : Recipes)
    {
        _worldPacket << int32(recipe.SkillLineID);
        for (uint8 bits : recipe.SkillLineBitArray)
            _worldPacket << uint8(bits);
    }

    return &_worldPacket;
}

void WorldPackets::Guild::GuildQueryNews::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.rfinish();
}

void WorldPackets::Guild::GuildNewsUpdateSticky::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> NewsID;
    _worldPacket.ReadGuidMask(GuildGUID, { 6, 0 });
    Sticky = _worldPacket.ReadBit();
    _worldPacket.ReadGuidMask(GuildGUID, { 2, 7, 5, 4, 3, 1 });
    _worldPacket.ReadGuidBytes(GuildGUID, { 5, 4, 0, 1, 6, 2, 3, 7 });
}

WorldPacket const* WorldPackets::Guild::GuildNews::Write()
{
    // 5.4.8 18414, from Project SkyFire. The member list (guild achievements) is not sent.
    _worldPacket.WriteBits(NewsEvents.size(), 19);
    for (GuildNewsEvent const& newsEvent : NewsEvents)
    {
        _worldPacket.WriteGuidMask(newsEvent.MemberGuid, { 3 });
        _worldPacket.WriteBits(0, 24);
        _worldPacket.WriteGuidMask(newsEvent.MemberGuid, { 4, 5, 6, 0, 7, 2, 1 });
    }

    _worldPacket.FlushBits();

    for (GuildNewsEvent const& newsEvent : NewsEvents)
    {
        _worldPacket.WriteGuidBytes(newsEvent.MemberGuid, { 2 });
        _worldPacket << int32(newsEvent.Data[0]);
        _worldPacket.WriteGuidBytes(newsEvent.MemberGuid, { 1, 7, 4, 3, 5 });
        _worldPacket << int32(newsEvent.Data[1]);
        _worldPacket << int32(newsEvent.Type);
        _worldPacket << uint32(newsEvent.Flags);
        _worldPacket.WriteGuidBytes(newsEvent.MemberGuid, { 6 });
        _worldPacket.AppendPackedTime(newsEvent.CompletedDate);
        _worldPacket << int32(newsEvent.Id);
        _worldPacket.WriteGuidBytes(newsEvent.MemberGuid, { 0 });
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Guild::GuildNameChanged::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(GuildGUID, { 0, 7, 3, 1, 5, 6 });
    _worldPacket.WriteBits(GuildName.length(), 7);
    _worldPacket.WriteGuidMask(GuildGUID, { 2, 4 });
    _worldPacket.FlushBits();
    _worldPacket.WriteString(GuildName);
    _worldPacket.WriteGuidBytes(GuildGUID, { 3, 2, 6, 7, 5, 0, 1, 4 });

    return &_worldPacket;
}
