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

#ifndef __UPDATEDATA_H
#define __UPDATEDATA_H

#include "ByteBuffer.h"
#include "ObjectGuid.h"

class WorldPacket;

enum OBJECT_UPDATE_TYPE
{
    UPDATETYPE_VALUES               = 0,
    UPDATETYPE_CREATE_OBJECT        = 1,
    UPDATETYPE_CREATE_OBJECT2       = 2,
    UPDATETYPE_OUT_OF_RANGE_OBJECTS = 3,                    // not a block type in 6.x, the guids precede the blocks
};

class UpdateData
{
public:
    // The map is part of every SMSG_UPDATE_OBJECT; require it so a sender cannot leave it at 0 by default.
    explicit UpdateData(uint16 map);

    void AddOutOfRangeGUID(ObjectGuid guid);
    void AddDestroyObject(ObjectGuid guid);
    void AddUpdateBlock(ByteBuffer const& block);
    void AddUpdateBlock(UpdateData const& block);
    bool BuildPacket(WorldPacket& packet);
    [[nodiscard]] bool HasData() const { return m_blockCount > 0 || !m_outOfRangeGUIDs.empty() || !m_destroyGUIDs.empty(); }
    void Clear();

protected:
    uint16 m_map{0};
    uint32 m_blockCount;
    GuidVector m_destroyGUIDs;
    GuidVector m_outOfRangeGUIDs;
    ByteBuffer m_data;
};
#endif
