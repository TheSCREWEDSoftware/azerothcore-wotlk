/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// 6.x Battle.net requests through the world connection, from TrinityCore 6.x (BattlenetHandler.cpp)

#include "BattlenetPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "Log.h"
#include "WorldserverServiceDispatcher.h"
#include "WorldSession.h"

void WorldSession::HandleBattlenetRequest(WorldPackets::Battlenet::Request& request)
{
    sServiceDispatcher.Dispatch(this, request.Method.GetServiceHash(), request.Method.Token, request.Method.GetMethodId(), std::move(request.Data));
}

void WorldSession::HandleBattlenetRequestRealmListTicket(WorldPackets::Battlenet::RequestRealmListTicket& requestRealmListTicket)
{
    SetRealmListSecret(requestRealmListTicket.Secret);

    WorldPackets::Battlenet::RealmListTicket realmListTicket;
    realmListTicket.Token = requestRealmListTicket.Token;
    realmListTicket.Allow = true;
    realmListTicket.Ticket << "WorldserverRealmListTicket";

    SendPacket(realmListTicket.Write());
}

void WorldSession::SendBattlenetResponse(uint32 serviceHash, uint32 methodId, uint32 token, google::protobuf::Message const* response)
{
    WorldPackets::Battlenet::Response bnetResponse;
    bnetResponse.BnetStatus = ERROR_OK;
    bnetResponse.Method.Type = (uint64(serviceHash) << 32) | uint64(methodId);
    bnetResponse.Method.ObjectId = 1;
    bnetResponse.Method.Token = token;

    if (response->ByteSize())
    {
        bnetResponse.Data.resize(response->ByteSize());
        response->SerializePartialToArray(bnetResponse.Data.contents(), response->ByteSize());
    }

    SendPacket(bnetResponse.Write());
}

void WorldSession::SendBattlenetResponse(uint32 serviceHash, uint32 methodId, uint32 token, uint32 status)
{
    WorldPackets::Battlenet::Response bnetResponse;
    bnetResponse.BnetStatus = BattlenetRpcErrorCode(status);
    bnetResponse.Method.Type = (uint64(serviceHash) << 32) | uint64(methodId);
    bnetResponse.Method.ObjectId = 1;
    bnetResponse.Method.Token = token;

    SendPacket(bnetResponse.Write());
}

void WorldSession::SendBattlenetRequest(uint32 serviceHash, uint32 methodId, google::protobuf::Message const* request, std::function<void(MessageBuffer)> callback)
{
    _battlenetResponseCallbacks[_battlenetRequestToken] = std::move(callback);
    SendBattlenetRequest(serviceHash, methodId, request);
}

void WorldSession::SendBattlenetRequest(uint32 serviceHash, uint32 methodId, google::protobuf::Message const* request)
{
    WorldPackets::Battlenet::Notification notification;
    notification.Method.Type = (uint64(serviceHash) << 32) | uint64(methodId);
    notification.Method.ObjectId = 1;
    notification.Method.Token = _battlenetRequestToken++;

    if (request->ByteSize())
    {
        notification.Data.resize(request->ByteSize());
        request->SerializePartialToArray(notification.Data.contents(), request->ByteSize());
    }

    SendPacket(notification.Write());
}
