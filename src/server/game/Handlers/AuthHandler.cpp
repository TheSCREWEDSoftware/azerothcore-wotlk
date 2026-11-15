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
#include "Random.h"
#include "Realm.h"
#include "World.h"
#include "WorldSession.h"
#include <boost/asio/ip/address.hpp>

void WorldSession::SendAuthResponse(uint8 code, bool shortForm, uint32 queuePos)
{
    WorldPackets::Auth::AuthResponse packet(code);

    if (code == AUTH_OK)
    {
        WorldPackets::Auth::AuthSuccessInfo& successInfo = packet.SuccessInfo.emplace();
        successInfo.ActiveExpansionLevel = uint8(sWorld->getIntConfig(CONFIG_EXPANSION));
        successInfo.AccountExpansionLevel = Expansion();
    }

    if (!shortForm)
        packet.WaitInfo.emplace(WorldPackets::Auth::AuthWaitInfo{ queuePos, false });

    SendPacket(packet.Write());
}

void WorldSession::SendClientCacheVersion(uint32 version)
{
    WorldPacket data(SMSG_CLIENTCACHE_VERSION, 4);
    data << uint32(version);
    SendPacket(&data);
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
