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

#include "CombatLogPackets.h"

WorldPacket const* WorldPackets::CombatLog::EnvironmentalDamageLog::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(Victim, { 5, 7, 1, 4, 2, 0 });
    _worldPacket.WriteBit(false);                           // power data
    _worldPacket.WriteGuidMask(Victim, { 6, 3 });
    _worldPacket.FlushBits();

    _worldPacket << uint32(Absorbed);
    _worldPacket.WriteGuidBytes(Victim, { 0, 7 });
    _worldPacket << uint8(Type);
    _worldPacket.WriteGuidBytes(Victim, { 6, 3, 5 });
    _worldPacket << uint32(Resisted);
    _worldPacket.WriteGuidBytes(Victim, { 1, 2, 4 });
    _worldPacket << uint32(Amount);

    return &_worldPacket;
}
