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

#include "PetPackets.h"

void WorldPackets::Pet::DismissCritter::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(CritterGUID, { 4, 6, 7, 5, 1, 0, 2, 3 });
    _worldPacket.ReadGuidBytes(CritterGUID, { 2, 4, 5, 0, 1, 7, 3, 6 });
}

// 5.4.8 18414, from Project SkyFire
void WorldPackets::Pet::PetAbandon::Read()
{
    _worldPacket.ReadGuidMask(PetGUID, { 7, 3, 4, 2, 5, 6, 1, 0 });
    _worldPacket.ReadGuidBytes(PetGUID, { 0, 2, 5, 6, 7, 1, 4, 3 });
}

void WorldPackets::Pet::PetStopAttack::Read()
{
    _worldPacket.ReadGuidMask(PetGUID, { 7, 5, 1, 6, 0, 2, 4, 3 });
    _worldPacket.ReadGuidBytes(PetGUID, { 2, 5, 0, 4, 1, 7, 6, 3 });
}

void WorldPackets::Pet::PetSpellAutocast::Read()
{
    _worldPacket >> SpellID;
    _worldPacket.ReadGuidMask(PetGUID, { 0, 4, 2, 6, 1, 5, 3, 7 });
    AutocastEnabled = _worldPacket.ReadBit();
    _worldPacket.ReadGuidBytes(PetGUID, { 5, 0, 4, 1, 7, 2, 3, 6 });
}

WorldPacket const* WorldPackets::Pet::PetLearnedSpell::Write()
{
    _worldPacket << uint32(SpellID);
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Pet::PetUnlearnedSpell::Write()
{
    _worldPacket << uint32(SpellID);
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Pet::PetActionSound::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(UnitGUID, { 2, 7, 6, 0, 5, 1, 3, 4 });
    _worldPacket.FlushBits();
    _worldPacket.WriteGuidBytes(UnitGUID, { 7, 4, 6, 1 });
    _worldPacket << int32(Action);
    _worldPacket.WriteGuidBytes(UnitGUID, { 2, 3, 5, 0 });
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Pet::PetDismissSound::Write()
{
    _worldPacket << int32(ModelId);
    _worldPacket << float(ModelPosition.x);
    _worldPacket << float(ModelPosition.y);
    _worldPacket << float(ModelPosition.z);
    return &_worldPacket;
}
