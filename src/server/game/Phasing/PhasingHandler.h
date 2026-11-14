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

#ifndef PhasingHandler_h__
#define PhasingHandler_h__

#include "Player.h"

// TC master's PhasingHandler entry points that ported scripts call, mapped onto this core's
// conditional phases (phase_area, re-evaluated by Player::UpdatePhasesForArea).
class PhasingHandler
{
public:
    // Conditions changed (quest state, aura, ...): re-evaluate the area phases of the player
    static void OnConditionChange(WorldObject* object)
    {
        if (Player* player = object->ToPlayer())
            player->UpdatePhasesForArea(player->GetAreaId());
    }
};

#endif // PhasingHandler_h__
