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

#ifndef TC9GUID_H
#define TC9GUID_H

#include "ObjectGuid.h"

// The ToCloud9 sidecar speaks the 3.3.5 64 bit guid format, these convert to and from the 6.x 128 bit ObjectGuid.
// Players and items carry only their counter (realm bits 32-47 are dropped like the old uint32 LowType did),
// creatures and gameobjects carry high 0xF1x0, entry in bits 24-47 and a 24 bit counter.
namespace TC9Guid
{
    inline ObjectGuid Player(uint64 raw)
    {
        return raw ? ObjectGuid::Create<HighGuid::Player>(uint32(raw)) : ObjectGuid::Empty;
    }

    inline ObjectGuid Item(uint64 raw)
    {
        return raw ? ObjectGuid::Create<HighGuid::Item>(uint32(raw)) : ObjectGuid::Empty;
    }

    inline ObjectGuid WorldObject(uint64 raw, uint16 mapId)
    {
        uint32 entry = uint32((raw >> 24) & 0xFFFFFF);
        ObjectGuid::LowType counter = raw & 0xFFFFFF;
        switch (uint32(raw >> 48))
        {
            case 0xF110: return ObjectGuid::Create<HighGuid::GameObject>(mapId, entry, counter);
            case 0xF130: return ObjectGuid::Create<HighGuid::Creature>(mapId, entry, counter);
            case 0xF140: return ObjectGuid::Create<HighGuid::Pet>(mapId, entry, counter);
            case 0xF150: return ObjectGuid::Create<HighGuid::Vehicle>(mapId, entry, counter);
            default:     return ObjectGuid::Empty;
        }
    }

    inline uint64 ToRaw(ObjectGuid const& guid)
    {
        switch (guid.GetHigh())
        {
            case HighGuid::Player: return guid.GetCounter();
            case HighGuid::Item:   return (UI64LIT(0x4000) << 48) | guid.GetCounter();
            default:               return 0;
        }
    }
}

#endif
