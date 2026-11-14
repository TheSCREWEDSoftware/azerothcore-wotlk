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

#ifndef AZEROTHCORE_SPELL_CAST_REQUEST_H
#define AZEROTHCORE_SPELL_CAST_REQUEST_H

#include "Archaeology.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <string>

class SpellCastTargets;
class WorldObject;

namespace WorldPackets::Spells
{
    struct SpellCastRequest;
}

// The cast data the cast, use item and pet cast requests carry
struct SpellCastRequest
{
    uint8 castCount = 0;
    uint8 castFlags = 0;
    uint32 spellId = 0;
    uint32 glyphIndex = 0;
    uint32 targetMask = 0;
    float elevation = 0.0f;
    float missileSpeed = 0.0f;
    ObjectGuid targetGuid;
    ObjectGuid itemTargetGuid;
    ObjectGuid destTransportGuid;
    ObjectGuid srcTransportGuid;
    Position srcPos;
    Position destPos;
    std::string targetString;
    ArchData researchData;
    bool hasResearchData = false;

    // CMSG_PET_CAST_SPELL
    ObjectGuid petGuid;

    void FillTargets(SpellCastTargets& targets, WorldObject* caster) const;

    // the 6.x cast data (CMSG_CAST_SPELL, CMSG_USE_ITEM, CMSG_PET_CAST_SPELL)
    static SpellCastRequest FromPacket(WorldPackets::Spells::SpellCastRequest const& cast);
};

#endif
