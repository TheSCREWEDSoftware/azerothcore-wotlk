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

#include "GarrisonMap.h"
#include "GameObject.h"
#include "Garrison.h"
#include "GridDefines.h"
#include "MapGrid.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "World.h"

GarrisonMap::GarrisonMap(uint32 id, uint32 instanceId, Map* parent, ObjectGuid const& owner)
    : Map(id, instanceId, DIFFICULTY_NORMAL, parent), _owner(owner), _loadingPlayer(nullptr)
{
    GarrisonMap::InitVisibilityDistance();
}

void GarrisonMap::OnGridObjectsLoaded(MapGridType& grid)
{
    Garrison* garrison = GetGarrison();
    if (!garrison)
        return;

    uint32 gameObjects = 0;
    for (Garrison::Plot* plot : garrison->GetPlots())
    {
        Position const& spawn = plot->PacketInfo.PlotPos;
        GridCoord gridCoord = Acore::ComputeGridCoord(spawn.GetPositionX(), spawn.GetPositionY());
        if (gridCoord.x_coord != grid.GetX() || gridCoord.y_coord != grid.GetY())
            continue;

        GameObject* go = plot->CreateGameObject(this, garrison->GetFaction());
        if (!go)
            continue;

        if (!AddToMap(go))
        {
            delete go;
            continue;
        }

        ++gameObjects;
    }

    LOG_DEBUG("maps", "{} garrison GameObjects loaded for grid {} on map {}", gameObjects, grid.GetId(), GetId());
}

Garrison* GarrisonMap::GetGarrison()
{
    if (_loadingPlayer)
        return _loadingPlayer->GetGarrison();

    if (Player* owner = ObjectAccessor::FindConnectedPlayer(_owner))
        return owner->GetGarrison();

    return nullptr;
}

void GarrisonMap::InitVisibilityDistance()
{
    //init visibility distance for instances
    m_VisibleDistance = World::GetMaxVisibleDistanceInBGArenas();
}

bool GarrisonMap::AddPlayerToMap(Player* player)
{
    if (player->GetGUID() == _owner)
        _loadingPlayer = player;

    bool result = Map::AddPlayerToMap(player);

    if (player->GetGUID() == _owner)
        _loadingPlayer = nullptr;

    return result;
}
