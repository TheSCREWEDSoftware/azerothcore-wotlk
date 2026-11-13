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

#include "TotemPackets.h"

void WorldPackets::Totem::TotemDestroyed::Read()
{
    // 5.4.8 18414, from Project SkyFire; the totem GUID is not used
    ObjectGuid totemGuid;
    _worldPacket >> Slot;
    _worldPacket.ReadGuidMask(totemGuid, { 4, 2, 1, 3, 0, 6, 7, 5 });
    _worldPacket.ReadGuidBytes(totemGuid, { 6, 2, 4, 1, 5, 0, 3, 7 });
}

WorldPacket const* WorldPackets::Totem::TotemCreated::Write()
{
    _worldPacket << Slot;
    _worldPacket << Totem;
    _worldPacket << Duration;
    _worldPacket << SpellID;

    return &_worldPacket;
}
