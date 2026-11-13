/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * You may redistribute it and/or modify it under the terms of the GNU General Public License
 * version 2 or, at your option, any later version.
 */

#include "SystemPackets.h"

WorldPacket const* WorldPackets::System::FeatureSystemStatus::Write()
{
    // 5.4.8 18414 (SkyFire_548 WorldSession::HandlePlayerLogin)
    _worldPacket << int32(ScrollOfResurrectionMaxRequestsPerDay);
    _worldPacket << int32(ScrollOfResurrectionRequestsRemaining);
    _worldPacket << int32(CfgRealmID);
    _worldPacket << int8(ComplaintStatus);
    _worldPacket << int32(CfgRealmRecID);

    _worldPacket.WriteBit(ItemRestorationButtonEnabled);
    _worldPacket.WriteBit(false);                           // in game shop enabled
    _worldPacket.WriteBit(false);
    _worldPacket.WriteBit(false);                           // recruit a friend
    _worldPacket.WriteBit(VoiceEnabled);
    _worldPacket.WriteBit(false);                           // in game shop icon
    _worldPacket.WriteBit(ScrollOfResurrectionEnabled);
    _worldPacket.WriteBit(SessionAlert.has_value());
    _worldPacket.WriteBit(false);                           // shop parental control
    _worldPacket.WriteBit(EuropaTicketSystemStatus.has_value());
    _worldPacket.FlushBits();

    if (SessionAlert)
    {
        _worldPacket << int32(SessionAlert->Period);
        _worldPacket << int32(SessionAlert->Delay);
        _worldPacket << int32(SessionAlert->DisplayTime);
    }

    if (EuropaTicketSystemStatus)
    {
        _worldPacket << int32(EuropaTicketSystemStatus->TryCount);
        _worldPacket << int32(EuropaTicketSystemStatus->LastResetTimeBeforeNow);
        _worldPacket << int32(EuropaTicketSystemStatus->MaxTries);
        _worldPacket << int32(EuropaTicketSystemStatus->PerMilliseconds);
    }

    return &_worldPacket;
}
