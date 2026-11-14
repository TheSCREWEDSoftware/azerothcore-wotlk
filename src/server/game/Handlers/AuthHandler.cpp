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

#include "AuthenticationPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "Random.h"
#include "Realm.h"
#include "World.h"
#include "WorldSession.h"
#include <boost/asio/ip/address.hpp>

namespace
{
    // 6.2.4 playable races and classes with the expansion that unlocks them
    // (TrinityCore 6.x race_expansion_requirement / class_expansion_requirement)
    WorldPackets::Auth::ExpansionRequirementContainer const RaceExpansions =
    {
        { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 }, { 9, 3 }, { 10, 1 }, { 11, 1 },
        { 22, 3 }, { 24, 4 }, { 25, 4 }, { 26, 4 }
    };

    WorldPackets::Auth::ExpansionRequirementContainer const ClassExpansions =
    {
        { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 6, 2 }, { 7, 0 }, { 8, 0 }, { 9, 0 }, { 10, 4 }, { 11, 0 }
    };

    std::string NormalizeRealmName(std::string name)
    {
        name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
        return name;
    }
}

uint32 WorldSession::GetVirtualRealmAddress() const
{
    return realm.Id.GetAddress();
}

void WorldSession::SendAuthResponse(uint32 code, bool queued, uint32 queuePos)
{
    WorldPackets::Auth::AuthResponse response;
    response.Result = code;

    if (queued)
    {
        response.WaitInfo.emplace();
        response.WaitInfo->WaitCount = queuePos;
    }
    else if (code == ERROR_OK)
    {
        WorldPackets::Auth::AuthResponse::AuthSuccessInfo& successInfo = response.SuccessInfo.emplace();
        successInfo.AccountExpansionLevel = Expansion();
        successInfo.ActiveExpansionLevel = uint8(sWorld->getIntConfig(CONFIG_EXPANSION));
        successInfo.VirtualRealmAddress = GetVirtualRealmAddress();

        // the home realm; there is no need to send it again in realm queries
        successInfo.VirtualRealms.emplace_back(GetVirtualRealmAddress(), true, false, realm.Name, NormalizeRealmName(realm.Name));

        successInfo.AvailableClasses = &ClassExpansions;
        successInfo.AvailableRaces = &RaceExpansions;
    }

    SendPacket(response.Write());
}

void WorldSession::SendConnectToInstance(WorldPackets::Auth::ConnectToSerial serial)
{
    boost::system::error_code ignoredError;
    boost::asio::ip::tcp::endpoint instanceAddress = realm.GetAddressForClient(boost::asio::ip::make_address(GetRemoteAddress(), ignoredError));
    instanceAddress.port(uint16(sWorld->getIntConfig(CONFIG_PORT_INSTANCE)));

    _instanceConnectKey.Fields.AccountId = GetAccountId();
    _instanceConnectKey.Fields.ConnectionType = CONNECTION_TYPE_INSTANCE;
    _instanceConnectKey.Fields.Key = urand(0, 0x7FFFFFFF);

    WorldPackets::Auth::ConnectTo connectTo;
    connectTo.Key = _instanceConnectKey.Raw;
    connectTo.Serial = serial;
    connectTo.Payload.Where = instanceAddress;
    connectTo.Con = CONNECTION_TYPE_INSTANCE;

    SendPacket(connectTo.Write());
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPacket data(SMSG_CACHE_VERSION, 4);
    data << uint32(version);
    SendPacket(&data);
}
