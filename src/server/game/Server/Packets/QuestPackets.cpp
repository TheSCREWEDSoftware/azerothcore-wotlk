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

#include "QuestPackets.h"

void WorldPackets::Quest::QuestRewards::Write(ByteBuffer& data) const
{
    data << ChoiceItemCount;
    for (uint32 value : ChoiceItemId)
        data << value;
    for (uint32 value : ChoiceItemQuantity)
        data << value;
    for (uint32 value : ChoiceItemDisplay)
        data << value;
    data << ItemCount;
    for (uint32 value : ItemId)
        data << value;
    for (uint32 value : ItemQuantity)
        data << value;
    for (uint32 value : ItemDisplay)
        data << value;
    data << Money << Experience << Title << uint32(0) << float(0) << BonusTalents << uint32(0) << FactionFlags;
    for (uint32 value : FactionId)
        data << value;
    for (int32 value : FactionValue)
        data << value;
    for (uint32 value : FactionOverride)
        data << value;
    data << DisplaySpell << Spell;
    for (uint32 value : CurrencyId)
        data << value;
    for (uint32 value : CurrencyQuantity)
        data << value;
    data << SkillLine << SkillUps;
}

WorldPacket const* WorldPackets::Quest::QuestUpdateComplete::Write()
{
    _worldPacket << QuestId;

    return &_worldPacket;
}

// 5.4.8 18414, from Project SkyFire (Player::SendQuestReward)
WorldPacket const* WorldPackets::Quest::QuestGiverQuestComplete::Write()
{
    _worldPacket.WriteBit(LaunchGossip);
    _worldPacket.WriteBit(UseQuestReward);
    _worldPacket.FlushBits();
    _worldPacket << RewardTalents;
    _worldPacket << RewardMoney;
    _worldPacket << QuestId;
    _worldPacket << RewardSkillLine;
    _worldPacket << Experience;
    _worldPacket << RewardSkillUps;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Quest::QuestGiverQuestFailed::Write()
{
    _worldPacket << QuestId;
    _worldPacket << FailureReason;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Quest::QuestUpdateFailedTimer::Write()
{
    _worldPacket << QuestId;

    return &_worldPacket;
}

// 5.4.8 18414, from Project SkyFire (Player::SendCanTakeQuestResponse)
WorldPacket const* WorldPackets::Quest::QuestGiverQuestInvalid::Write()
{
    _worldPacket.WriteBit(true);
    _worldPacket.FlushBits();
    _worldPacket << FailureReason;

    return &_worldPacket;
}

// 5.4.8 18414, from Project SkyFire (Player::SendQuestConfirmAccept)
WorldPacket const* WorldPackets::Quest::QuestConfirmAccept::Write()
{
    _worldPacket.WriteBit(PlayerGuid[0]);
    _worldPacket.WriteBit(PlayerGuid[3]);
    _worldPacket.WriteBit(false);                           // !has quest title
    _worldPacket.WriteBit(PlayerGuid[2]);
    _worldPacket.WriteBit(PlayerGuid[5]);
    _worldPacket.WriteBit(PlayerGuid[6]);
    _worldPacket.WriteBit(PlayerGuid[4]);
    _worldPacket.WriteBit(PlayerGuid[1]);
    _worldPacket.WriteBits(QuestTitle.size(), 10);
    _worldPacket.WriteBit(PlayerGuid[7]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(PlayerGuid[6]);
    _worldPacket.WriteString(QuestTitle);
    for (uint8 index : { 0, 5, 3, 1, 4, 2, 7 })
        _worldPacket.WriteByteSeq(PlayerGuid[index]);
    _worldPacket << QuestId;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Quest::QuestPushResult::Write()
{
    // 5.4.8 18414, from Project SkyFire (Player::SendPushToPartyResponse)
    for (uint8 index : { 3, 0, 1, 4, 7, 5, 6, 2 })
        _worldPacket.WriteBit(PlayerGuid[index]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(PlayerGuid[4]);
    _worldPacket << uint8(QuestShareMessage);
    for (uint8 index : { 1, 5, 3, 7, 6, 2, 0 })
        _worldPacket.WriteByteSeq(PlayerGuid[index]);

    return &_worldPacket;
}

// 5.4.8 18414 SMSG_QUESTUPDATE_ADD_CREDIT, from Project SkyFire (Player::SendQuestUpdateAddCredit)
WorldPacket const* WorldPackets::Quest::QuestUpdateAddKill::Write()
{
    _worldPacket << uint16(CurrentCount);
    _worldPacket << uint8(ObjectiveType);
    _worldPacket << QuestId;
    _worldPacket << uint16(RequiredCount);
    _worldPacket << CreatureEntry;

    for (uint8 index : { 0, 4, 2, 6, 1, 5, 7, 3 })
        _worldPacket.WriteBit(ObjectiveGuid[index]);
    _worldPacket.FlushBits();
    for (uint8 index : { 2, 7, 3, 0, 4, 5, 1, 6 })
        _worldPacket.WriteByteSeq(ObjectiveGuid[index]);

    return &_worldPacket;
}

// 5.4.8 18414, from Project SkyFire (Player::SendQuestUpdateAddPlayer)
WorldPacket const* WorldPackets::Quest::QuestUpdateAddPvPKill::Write()
{
    _worldPacket << QuestId;
    _worldPacket << CurrentCount;

    return &_worldPacket;
}

void WorldPackets::Quest::QuestPushResultClient::Read()
{
    // 5.4.8 18414, from Project SkyFire (WorldSession::HandleQuestPushResult)
    _worldPacket >> QuestId;
    uint8 Message;
    _worldPacket >> Message;
    for (uint8 index : { 5, 3, 0, 6, 1, 2, 7, 4 })
        PlayerGuid[index] = _worldPacket.ReadBit();
    for (uint8 index : { 1, 2, 0, 5, 6, 4, 7, 3 })
        _worldPacket.ReadByteSeq(PlayerGuid[index]);
    QuestShareMessage = static_cast<QuestShareMessages>(Message);
}

void WorldPackets::Quest::QuestLogSwapQuest::Read()
{
    _worldPacket >> Slot1;
    _worldPacket >> Slot2;
}

void WorldPackets::Quest::QuestLogRemoveQuest::Read()
{
    _worldPacket >> Slot;
}

void WorldPackets::Quest::QuestConfirmAcceptClient::Read()
{
    _worldPacket >> QuestId;
}

void WorldPackets::Quest::PushQuestToParty::Read()
{
    _worldPacket >> QuestId;
}
