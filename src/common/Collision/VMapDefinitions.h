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

#ifndef _VMAPDEFINITIONS_H
#define _VMAPDEFINITIONS_H

#define LIQUID_TILE_SIZE (533.333f / 128.f)

namespace VMAP
{
    char const VMAP_MAGIC[] = "VMAP_4.7";
    char const RAW_VMAP_MAGIC[] = "VMAP047";                // used in extracted vmap files with raw data
    char const GAMEOBJECT_MODELS[] = "GameObjectModels.dtree";

    // defined in TileAssembler.cpp currently...
    bool readChunk(FILE* rf, char* dest, char const* compare, uint32 len);
}
#endif
