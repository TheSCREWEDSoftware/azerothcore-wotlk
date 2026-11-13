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

void WorldPackets::Pet::PetAbandon::Read()
{
    _worldPacket >> PetGUID;
}

void WorldPackets::Pet::PetStopAttack::Read()
{
    _worldPacket >> PetGUID;
}

void WorldPackets::Pet::PetSpellAutocast::Read()
{
    _worldPacket >> PetGUID;
    _worldPacket >> SpellID;
    _worldPacket >> AutocastEnabled;
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
    _worldPacket << UnitGUID;
    _worldPacket << int32(Action);
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
