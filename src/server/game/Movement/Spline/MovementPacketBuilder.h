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

#ifndef AC_PACKET_BUILDER_H
#define AC_PACKET_BUILDER_H

#include "Define.h"
#include "ObjectGuid.h"

class ByteBuffer;
namespace G3D
{
    class Vector3;
}

namespace Movement
{
    using G3D::Vector3;

    class MoveSpline;
    class PacketBuilder
    {
    public:
        static void WriteMonsterMove(MoveSpline const& mov, ByteBuffer& data, ObjectGuid const& guid,
            ObjectGuid const& transport, Vector3 const& transportOffset, int8 seat, bool hasSeat, Vector3 const& start);
        static void WriteStopMovement(uint32 splineId, ByteBuffer& data, ObjectGuid const& guid,
            ObjectGuid const& transport, Vector3 const& transportOffset, int8 seat, bool hasSeat, Vector3 const& position);
        static void WriteCreateBits(MoveSpline const& mov, ByteBuffer& data);
        static void WriteCreateData(MoveSpline const& mov, ByteBuffer& data);
        static void WriteFacingTargetPart(MoveSpline const& mov, ByteBuffer& data);

    private:
        static uint8 GetMonsterMoveType(MoveSpline const& mov);
    };
}
#endif // AC_PACKET_BUILDER_H
