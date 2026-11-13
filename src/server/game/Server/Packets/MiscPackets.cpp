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

#include "MiscPackets.h"

WorldPacket const* WorldPackets::Misc::SetCurrency::Write()
{
    _worldPacket.WriteBit(WeeklyQuantity.has_value());
    _worldPacket.WriteBit(TrackedQuantity.has_value());
    _worldPacket.WriteBit(SuppressChatLog);
    _worldPacket.FlushBits();

    if (TrackedQuantity.has_value())
        _worldPacket << int32(*TrackedQuantity);

    _worldPacket << int32(Quantity);
    _worldPacket << int32(Type);

    if (WeeklyQuantity.has_value())
        _worldPacket << int32(*WeeklyQuantity);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::SetupCurrency::Write()
{
    _worldPacket.WriteBits(Data.size(), 23);

    for (Record const& record : Data)
    {
        _worldPacket.WriteBit(record.WeeklyQuantity.has_value());
        _worldPacket.WriteBits(record.Flags, 4);
        _worldPacket.WriteBit(record.MaxWeeklyQuantity.has_value());
        _worldPacket.WriteBit(record.TrackedQuantity.has_value());
    }

    _worldPacket.FlushBits();

    for (Record const& record : Data)
    {
        _worldPacket << uint32(record.Quantity);

        if (record.MaxWeeklyQuantity)
            _worldPacket << uint32(*record.MaxWeeklyQuantity);
        if (record.TrackedQuantity)
            _worldPacket << uint32(*record.TrackedQuantity);

        _worldPacket << uint32(record.Type);

        if (record.WeeklyQuantity)
            _worldPacket << uint32(*record.WeeklyQuantity);
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::SetAnimTier::Write()
{
    _worldPacket << Unit.WriteAsPacked();
    _worldPacket << int32(Tier);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::SetPlayHoverAnim::Write()
{
    _worldPacket.WriteBit(UnitGUID[4]);
    _worldPacket.WriteBit(UnitGUID[0]);
    _worldPacket.WriteBit(UnitGUID[1]);
    _worldPacket.WriteBit(PlayHoverAnim);
    _worldPacket.WriteBit(UnitGUID[3]);
    _worldPacket.WriteBit(UnitGUID[7]);
    _worldPacket.WriteBit(UnitGUID[5]);
    _worldPacket.WriteBit(UnitGUID[2]);
    _worldPacket.WriteBit(UnitGUID[6]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(UnitGUID[3]);
    _worldPacket.WriteByteSeq(UnitGUID[2]);
    _worldPacket.WriteByteSeq(UnitGUID[1]);
    _worldPacket.WriteByteSeq(UnitGUID[7]);
    _worldPacket.WriteByteSeq(UnitGUID[0]);
    _worldPacket.WriteByteSeq(UnitGUID[5]);
    _worldPacket.WriteByteSeq(UnitGUID[4]);
    _worldPacket.WriteByteSeq(UnitGUID[6]);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::BindPointUpdate::Write()
{
    _worldPacket << X;
    _worldPacket << Y;
    _worldPacket << Z;
    _worldPacket << MapID;
    _worldPacket << AreaID;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::WorldServerInfo::Write()
{
    _worldPacket.WriteBit(RestrictedAccountMaxLevel.has_value());
    _worldPacket.WriteBit(RestrictedAccountMaxMoney.has_value());
    _worldPacket.WriteBit(IneligibleForLootMask.has_value());
    _worldPacket.FlushBits();

    if (IneligibleForLootMask)
        _worldPacket << *IneligibleForLootMask;

    _worldPacket << IsTournamentRealm;

    if (RestrictedAccountMaxMoney)
        _worldPacket << *RestrictedAccountMaxMoney;

    if (RestrictedAccountMaxLevel)
        _worldPacket << *RestrictedAccountMaxLevel;

    _worldPacket << WeeklyReset;
    _worldPacket << DifficultyID;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::LoginSetTimeSpeed::Write()
{
    _worldPacket.AppendPackedTime(GameTime);
    _worldPacket << NewSpeed;
    _worldPacket << GameTimeHolidayOffset;

    return &_worldPacket;
}

WorldPackets::Misc::Weather::Weather() : ServerPacket(SMSG_WEATHER, 4 + 4 + 1) { }

WorldPackets::Misc::Weather::Weather(WeatherState weatherID, float intensity /*= 0.0f*/, bool abrupt /*= false*/)
    : ServerPacket(SMSG_WEATHER, 4 + 4 + 1), Abrupt(abrupt), Intensity(intensity), WeatherID(weatherID) { }

WorldPacket const* WorldPackets::Misc::Weather::Write()
{
    _worldPacket << uint32(WeatherID);
    _worldPacket << float(Intensity);
    _worldPacket << uint8(Abrupt);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::LevelUpInfo::Write()
{
    _worldPacket << int32(Level);
    _worldPacket << int32(HealthDelta);

    for (int32 power : PowerDelta)
        _worldPacket << power;

    for (int32 stat : StatDelta)
        _worldPacket << stat;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::PlayMusic::Write()
{
    _worldPacket << SoundKitID;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::PlayObjectSound::Write()
{
    _worldPacket << SoundKitID;
    _worldPacket << SourceObjectGUID;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::Playsound::Write()
{
    _worldPacket << SoundKitID;

    return &_worldPacket;
}

void WorldPackets::Misc::MinimapPingClient::Read()
{
    _worldPacket >> MapX;
    _worldPacket >> MapY;
}

WorldPacket const* WorldPackets::Misc::MinimapPing::Write()
{
    _worldPacket << SourceGuid;
    _worldPacket << float(MapX);
    _worldPacket << float(MapY);

    return &_worldPacket;
}

void WorldPackets::Misc::RandomRollClient::Read()
{
    _worldPacket >> Min;
    _worldPacket >> Max;
}

WorldPacket const* WorldPackets::Misc::RandomRoll::Write()
{
    _worldPacket << uint32(Min);
    _worldPacket << uint32(Max);
    _worldPacket << uint32(Result);
    _worldPacket << Roller;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::StartMirrorTimer::Write()
{
    _worldPacket << uint32(Timer);
    _worldPacket << uint32(Value);
    _worldPacket << uint32(MaxValue);
    _worldPacket << int32(Scale);
    _worldPacket << uint8(Paused);
    _worldPacket << uint32(SpellID);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::PauseMirrorTimer::Write()
{
    _worldPacket << uint32(Timer);
    _worldPacket << uint8(Paused);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::StopMirrorTimer::Write()
{
    _worldPacket << uint32(Timer);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::CrossedInebriationThreshold::Write()
{
    _worldPacket << Guid;
    _worldPacket << uint32(Threshold);
    _worldPacket << uint32(ItemID);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::UITime::Write()
{
    _worldPacket << uint32(Time);

    return &_worldPacket;
}

void WorldPackets::Misc::Complain::Read()
{
    _worldPacket >> SpamType; // 0 - mail, 1 - chat
    _worldPacket >> SpammerGuid;
    switch (SpamType)
    {
    case 0:
        _worldPacket >> Unk1; // const 0
        _worldPacket >> MailIdOrMessageType; // probably mail id
        _worldPacket >> ChannelId; // const 0
        break;
    case 1:
        _worldPacket >> Unk1; // probably language
        _worldPacket >> MailIdOrMessageType;
        _worldPacket >> ChannelId;
        _worldPacket >> SecondsSinceMessage;
        _worldPacket >> Description; // spam description string (messagetype, channel name, player name, message)
        break;
    }
}

WorldPacket const* WorldPackets::Misc::ComplainResult::Write()
{
    _worldPacket << uint8(Result);
    _worldPacket << uint8(ComplaintType);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::AnimKitPacket::Write()
{
    _worldPacket << Unit.WriteAsPacked();
    _worldPacket << uint16(AnimKitID);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::PlayTimeWarning::Write()
{
    _worldPacket << Flag;
    _worldPacket << PlayTimeRemaining;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::PhaseShiftChange::Write()
{
    _worldPacket.WriteBit(Client[2]);
    _worldPacket.WriteBit(Client[3]);
    _worldPacket.WriteBit(Client[1]);
    _worldPacket.WriteBit(Client[6]);
    _worldPacket.WriteBit(Client[4]);
    _worldPacket.WriteBit(Client[5]);
    _worldPacket.WriteBit(Client[0]);
    _worldPacket.WriteBit(Client[7]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(Client[7]);
    _worldPacket.WriteByteSeq(Client[4]);
    _worldPacket << uint32(UiMapPhaseIDs.size() * 2); // UiMapPhaseIDs byte size
    for (uint16 uiMapPhaseId : UiMapPhaseIDs)
        _worldPacket << uiMapPhaseId;
    _worldPacket.WriteByteSeq(Client[1]);
    _worldPacket << uint32(PhaseShiftFlags);
    _worldPacket.WriteByteSeq(Client[2]);
    _worldPacket.WriteByteSeq(Client[6]);
    _worldPacket << uint32(PreloadMapIDs.size() * 2); // PreloadMapIDs byte size
    for (uint16 preloadMapId : PreloadMapIDs)
        _worldPacket << preloadMapId;
    _worldPacket << uint32(Phases.size() * 2); // Phases byte size
    for (uint16 phaseId : Phases)
        _worldPacket << phaseId;
    _worldPacket.WriteByteSeq(Client[3]);
    _worldPacket.WriteByteSeq(Client[0]);
    _worldPacket << uint32(VisibleMapIDs.size() * 2); // VisibleMapIDs byte size
    for (uint16 visibleMapId : VisibleMapIDs)
        _worldPacket << visibleMapId;
    _worldPacket.WriteByteSeq(Client[5]);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Misc::LoadCufProfiles::Write()
{
    _worldPacket.WriteBits(CUFProfiles.size(), 20);

    for (CUFProfile const* cufProfile : CUFProfiles)
    {
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_UNK_157]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_10_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_5_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_25_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_HEAL_PREDICTION]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_PVE]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_HORIZONTAL_GROUPS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_40_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_3_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_AGGRO_HIGHLIGHT]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_BORDER]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_2_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_NON_BOSS_DEBUFFS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_MAIN_TANK_AND_ASSIST]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_UNK_156]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_SPEC_2]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_USE_CLASS_COLORS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_POWER_BAR]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_SPEC_1]);
        _worldPacket.WriteBits(cufProfile->ProfileName.size(), 8);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_ONLY_DISPELLABLE_DEBUFFS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_KEEP_GROUPS_TOGETHER]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_UNK_145]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_15_PLAYERS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_DISPLAY_PETS]);
        _worldPacket.WriteBit(cufProfile->BoolOptions[CUF_AUTO_ACTIVATE_PVP]);
    }

    _worldPacket.FlushBits();

    for (CUFProfile const* cufProfile : CUFProfiles)
    {
        _worldPacket << uint16(cufProfile->LeftOffset);
        _worldPacket << uint16(cufProfile->FrameHeight);
        _worldPacket << uint16(cufProfile->BottomOffset);
        _worldPacket << uint8(cufProfile->BottomPoint);
        _worldPacket << uint16(cufProfile->TopOffset);
        _worldPacket << uint8(cufProfile->TopPoint);
        _worldPacket << uint8(cufProfile->HealthText);
        _worldPacket << uint8(cufProfile->SortBy);
        _worldPacket << uint16(cufProfile->FrameWidth);
        _worldPacket << uint8(cufProfile->LeftPoint);
        _worldPacket.WriteString(cufProfile->ProfileName);
    }

    return &_worldPacket;
}
