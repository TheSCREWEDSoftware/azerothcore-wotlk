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

#include "CombatPackets.h"
#include "UnitDefines.h"

void WorldPackets::Combat::SetSheathed::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> CurrentSheathState;
    if (!_worldPacket.ReadBit())                            // has data
        CurrentSheathState = MAX_SHEATH_STATE;
}

WorldPacket const* WorldPackets::Combat::SAttackStop::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(Victim, { 5, 6 });
    _worldPacket.WriteGuidMask(Attacker, { 3, 6, 7, 2, 5 });
    _worldPacket.WriteGuidMask(Victim, { 4 });
    _worldPacket.WriteBit(NowDead);
    _worldPacket.WriteGuidMask(Victim, { 3, 0, 2, 7 });
    _worldPacket.WriteGuidMask(Attacker, { 4, 1, 0 });
    _worldPacket.WriteGuidMask(Victim, { 1 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(Victim, { 0, 3, 5, 2 });
    _worldPacket.WriteGuidBytes(Attacker, { 0, 6, 3 });
    _worldPacket.WriteGuidBytes(Victim, { 4 });
    _worldPacket.WriteGuidBytes(Attacker, { 1, 4 });
    _worldPacket.WriteGuidBytes(Victim, { 6 });
    _worldPacket.WriteGuidBytes(Attacker, { 5, 7, 2 });
    _worldPacket.WriteGuidBytes(Victim, { 1, 7 });

    return &_worldPacket;
}
