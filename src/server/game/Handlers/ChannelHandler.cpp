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

#include "ChannelMgr.h"
#include "ChannelPackets.h"
#include "Language.h"
#include "ObjectMgr.h"                                      // for normalizePlayerName
#include "Player.h"
#include "TC9Sidecar.h"
#include <cctype>

// 6.x 19342: handlers ported from TrinityCore 6.x (Handlers/ChannelHandler.cpp)

void WorldSession::HandleJoinChannel(WorldPackets::Channel::JoinChannel& packet)
{
    LOG_DEBUG("chat.system", "CMSG_JOIN_CHANNEL {} ChatChannelId: {}, CreateVoiceSession: {}, Internal: {}, ChannelName: {}, Password: {}",
        GetPlayerInfo(), packet.ChatChannelId, packet.CreateVoiceSession, packet.Internal, packet.ChannelName, packet.Password);

    std::string channelName = packet.ChannelName;
    uint32 channelId = uint32(packet.ChatChannelId);

    if (channelId)
    {
        ChatChannelsEntry const* channel = sChatChannelsStore.LookupEntry(channelId);
        if (!channel)
            return;

        AreaTableEntry const* zone = sAreaTableStore.LookupEntry(GetPlayer()->GetZoneId());
        if (!zone || !GetPlayer()->CanJoinConstantChannelInZone(channel, zone))
            return;

        // Cluster mode rebuilds the localized channel name so nodes agree on it;
        // stock servers keep the client-supplied name.
        if (sToCloud9Sidecar->ClusterModeEnabled())
        {
            std::string const zoneName = zone->AreaName_lang;
            std::string const cityName = sObjectMgr->GetAcoreStringForDBCLocale(LANG_CHANNEL_CITY);
            char const* nameExt = (channel->Flags & CHANNEL_DBC_FLAG_CITY_ONLY) ? cityName.c_str() : zoneName.c_str();

            std::array<char, 128> buffer{};
            if (char const* pattern = channel->Name_lang)
            {
                std::snprintf(buffer.data(), buffer.size(), pattern, nameExt);
                channelName = buffer.data();
            }
        }
    }

    if (channelName.empty())
        return;

    if (isdigit(channelName[0]))
        return;

    if (channelName.size() >= 100 || !DisallowHyperlinksAndMaybeKick(channelName))
        return;

    if (ChannelMgr* cMgr = ChannelMgr::forTeam(GetPlayer()->GetTeamId()))
        if (Channel* channel = cMgr->GetJoinChannel(channelName, channelId))
            channel->JoinChannel(GetPlayer(), packet.Password);
}

void WorldSession::HandleLeaveChannel(WorldPackets::Channel::LeaveChannel& packet)
{
    LOG_DEBUG("chat.system", "CMSG_LEAVE_CHANNEL {} ChannelName: {}, ZoneChannelID: {}",
        GetPlayerInfo(), packet.ChannelName, packet.ZoneChannelID);

    if (packet.ChannelName.empty())
        return;

    if (ChannelMgr* cMgr = ChannelMgr::forTeam(GetPlayer()->GetTeamId()))
        if (Channel* channel = cMgr->GetChannel(packet.ChannelName, GetPlayer()))
            channel->LeaveChannel(GetPlayer(), true);
}

template<void(Channel::*CommandFunction)(Player const*)>
void WorldSession::HandleChannelCommand(WorldPackets::Channel::ChannelPlayerCommand& packet)
{
    LOG_DEBUG("chat.system", "{} {} ChannelName: {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())), GetPlayerInfo(), packet.ChannelName);

    if (ChannelMgr* cMgr = ChannelMgr::forTeam(GetPlayer()->GetTeamId()))
        if (Channel* channel = cMgr->GetChannel(packet.ChannelName, GetPlayer()))
            (channel->*CommandFunction)(GetPlayer());
}

template<void(Channel::*CommandFunction)(Player const*, std::string const&)>
void WorldSession::HandleChannelPlayerCommand(WorldPackets::Channel::ChannelPlayerCommand& packet)
{
    if (packet.Name.length() >= MAX_CHANNEL_NAME_STR)
    {
        LOG_DEBUG("chat.system", "{} {} ChannelName: {}, Name: {}, Name too long.",
            GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())), GetPlayerInfo(), packet.ChannelName, packet.Name);
        return;
    }

    LOG_DEBUG("chat.system", "{} {} ChannelName: {}, Name: {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())), GetPlayerInfo(), packet.ChannelName, packet.Name);

    // 6.x clients send "Name-Realm"; only the local realm is supported
    std::string name = packet.Name;
    if (std::size_t realmSeparator = name.find('-'); realmSeparator != std::string::npos)
        name.resize(realmSeparator);

    if (!normalizePlayerName(name))
        return;

    if (ChannelMgr* cMgr = ChannelMgr::forTeam(GetPlayer()->GetTeamId()))
        if (Channel* channel = cMgr->GetChannel(packet.ChannelName, GetPlayer()))
            (channel->*CommandFunction)(GetPlayer(), name);
}

template<>
void WorldSession::HandleChannelPlayerCommand<&Channel::Password>(WorldPackets::Channel::ChannelPlayerCommand& packet)
{
    if (packet.Name.length() > MAX_CHANNEL_PASS_STR)
    {
        LOG_DEBUG("chat.system", "{} {} ChannelName: {}, Password: {}, Password too long.",
            GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())), GetPlayerInfo(), packet.ChannelName, packet.Name);
        return;
    }

    LOG_DEBUG("chat.system", "{} {} ChannelName: {}, Password: {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())), GetPlayerInfo(), packet.ChannelName, packet.Name);

    if (ChannelMgr* cMgr = ChannelMgr::forTeam(GetPlayer()->GetTeamId()))
        if (Channel* channel = cMgr->GetChannel(packet.ChannelName, GetPlayer()))
            channel->Password(GetPlayer(), packet.Name);
}

template void WorldSession::HandleChannelCommand<&Channel::Announce>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::Ban>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelCommand<&Channel::DeclineInvite>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::Invite>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::Kick>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelCommand<&Channel::List>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelCommand<&Channel::ToggleModeration>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::SetModerator>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::SetMute>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelCommand<&Channel::SendWhoOwner>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::SetOwner>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::SilenceAll>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::SilenceVoice>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::UnBan>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::UnsetModerator>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::UnsetMute>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::UnsilenceAll>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelPlayerCommand<&Channel::UnsilenceVoice>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelCommand<&Channel::DeVoice>(WorldPackets::Channel::ChannelPlayerCommand&);
template void WorldSession::HandleChannelCommand<&Channel::Voice>(WorldPackets::Channel::ChannelPlayerCommand&);
