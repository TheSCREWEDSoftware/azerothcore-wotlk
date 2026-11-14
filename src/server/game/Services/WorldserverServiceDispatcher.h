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

// 6.x Battle.net services reached through the world connection, from TrinityCore 6.x (game/Services)

#ifndef ServiceRegistry_h__
#define ServiceRegistry_h__

#include "WorldSession.h"
#include "MessageBuffer.h"
#include "Log.h"
#include "Common.h"
#include "account_service.pb.h"
#include "authentication_service.pb.h"
#include "challenge_service.pb.h"
#include "channel_service.pb.h"
#include "connection_service.pb.h"
#include "friends_service.pb.h"
#include "game_utilities_service.pb.h"
#include "presence_service.pb.h"
#include "report_service.pb.h"
#include "resource_service.pb.h"
#include "user_manager_service.pb.h"

namespace Battlenet
{
    class WorldserverServiceDispatcher
    {
    public:
        void Dispatch(WorldSession* session, uint32 serviceHash, uint32 token, uint32 methodId, MessageBuffer buffer);

        static WorldserverServiceDispatcher& Instance();

    private:
        WorldserverServiceDispatcher();

        template<class Service>
        void AddService()
        {
            _dispatchers[Service::NameHash::value] = &WorldserverServiceDispatcher::Dispatch<Service>;
        }

        template<class Service>
        static void Dispatch(WorldSession* session, uint32 token, uint32 methodId, MessageBuffer buffer)
        {
            Service(session).CallServerMethod(token, methodId, std::forward<MessageBuffer>(buffer));
        }

        std::unordered_map<uint32, std::function<void(WorldSession*, uint32, uint32, MessageBuffer)>> _dispatchers;
    };
}

#define sServiceDispatcher Battlenet::WorldserverServiceDispatcher::Instance()

#endif // ServiceRegistry_h__
