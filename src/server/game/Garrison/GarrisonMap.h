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

#ifndef GarrisonMap_h__
#define GarrisonMap_h__

#include "Map.h"

class Garrison;
class Player;

// 6.x garrison map: one instance per owner, its plots are spawned from the owner's garrison (TC 6.x)
class GarrisonMap : public Map
{
public:
    GarrisonMap(uint32 id, uint32 instanceId, Map* parent, ObjectGuid const& owner);

    void OnGridObjectsLoaded(MapGridType& grid) override;
    Garrison* GetGarrison();

    void InitVisibilityDistance() override;

    bool AddPlayerToMap(Player* player) override;

private:
    ObjectGuid _owner;
    Player* _loadingPlayer; ///< @workaround Player is not registered in ObjectAccessor during login
};

#endif // GarrisonMap_h__
