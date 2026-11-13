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

#include "MovementPacketBuilder.h"
#include "ByteBuffer.h"
#include "MoveSpline.h"
#include "ObjectGuid.h"

namespace Movement
{
    inline void operator << (ByteBuffer& b, Vector3 const& v)
    {
        b << v.x << v.y << v.z;
    }

    inline void operator >> (ByteBuffer& b, Vector3& v)
    {
        b >> v.x >> v.y >> v.z;
    }

    enum MonsterMoveType
    {
        MonsterMoveNormal       = 0,
        MonsterMoveStop         = 1,
        MonsterMoveFacingSpot   = 2,
        MonsterMoveFacingTarget = 3,
        MonsterMoveFacingAngle  = 4
    };

    // 5.4.8 SMSG_ON_MONSTER_MOVE, from Project SkyFire (PacketBuilder::WriteMonsterMove).
    // The mover and its transport are part of the packet, there is no separate transport opcode.
    static void WriteMonsterMoveTransportBits(ByteBuffer& data, ObjectGuid const& transport)
    {
        for (uint8 index : { 7, 1, 3, 0, 6, 4, 5, 2 })
            data.WriteBit(transport[index]);
    }

    static void WriteMonsterMoveTransportBytes(ByteBuffer& data, ObjectGuid const& transport)
    {
        for (uint8 index : { 6, 4, 1, 7, 0, 3, 5, 2 })
            data.WriteByteSeq(transport[index]);
    }

    static void WriteLinearPath(Spline<int32> const& spline, ByteBuffer& data)
    {
        uint32 lastIndex = spline.getPointCount() - 3;
        Vector3 const* realPath = &spline.getPoint(1);
        Vector3 middle = (realPath[0] + realPath[lastIndex]) / 2.f;
        for (uint32 i = 0; i < lastIndex; ++i)
        {
            Vector3 offset = middle - realPath[i];
            data.appendPackXYZ(offset.x, offset.y, offset.z);
        }
    }

    void PacketBuilder::WriteMonsterMove(MoveSpline const& moveSpline, ByteBuffer& data, ObjectGuid const& guid,
        ObjectGuid const& transport, Vector3 const& transportOffset, int8 seat, bool hasSeat, Vector3 const& start)
    {
        uint8 type = GetMonsterMoveType(moveSpline);
        MoveSplineFlag const& splineFlags = moveSpline.splineflags;
        uint32 flags = splineFlags & uint32(~MoveSplineFlag::Mask_No_Monster_Move);
        bool const hasParabolicSpeed = splineFlags.parabolic && moveSpline.effect_start_time < moveSpline.Duration();
        bool const hasParabolicTime = splineFlags.parabolic || splineFlags.animation;
        bool const uncompressed = splineFlags.uncompressedPath;
        Spline<int32> const& spline = moveSpline.spline;
        uint32 uncompressedCount = uncompressed ? (splineFlags.cyclic ? spline.getPointCount() - 3 : spline.getPointCount() - 2) : 1;
        uint32 compressedCount = uncompressed ? 0 : spline.getPointCount() - 3;

        data << float(start.z);
        data << float(start.x);
        data << uint32(moveSpline.GetId());
        data << float(start.y);
        data << float(transportOffset.y);
        data << float(transportOffset.z);
        data << float(transportOffset.x);

        data.WriteBit(!hasParabolicSpeed);
        data.WriteBit(guid[0]);
        data.WriteBits(type, 3);

        ObjectGuid const targetGuid(type == MonsterMoveFacingTarget ? moveSpline.facing.target : 0);
        if (type == MonsterMoveFacingTarget)
            for (uint8 index : { 6, 4, 3, 0, 5, 7, 1, 2 })
                data.WriteBit(targetGuid[index]);

        data.WriteBit(!hasParabolicTime);
        data.WriteBit(1);
        data.WriteBit(!hasSeat);
        data.WriteBits(uncompressedCount, 20);
        data.WriteBit(!flags);
        data.WriteBit(guid[3]);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(!moveSpline.Duration());
        data.WriteBit(guid[7]);
        data.WriteBit(guid[4]);
        data.WriteBit(1);
        data.WriteBit(guid[5]);
        data.WriteBits(compressedCount, 22);
        data.WriteBit(guid[6]);
        data.WriteBit(0);
        WriteMonsterMoveTransportBits(data, transport);
        data.WriteBit(0);                                   // no spline filter
        data.WriteBit(0);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[1]);
        data.FlushBits();

        if (compressedCount)
            WriteLinearPath(spline, data);

        data.WriteByteSeq(guid[1]);
        WriteMonsterMoveTransportBytes(data, transport);

        if (uncompressed)
        {
            if (splineFlags.cyclic)
            {
                // Fake point, the client erases it from the spline after the first cycle
                data << spline.getPoint(1).y << spline.getPoint(1).x << spline.getPoint(1).z;
                for (int32 i = 1; i < spline.getPointCount() - 3; ++i)
                    data << spline.getPoint(i).y << spline.getPoint(i).x << spline.getPoint(i).z;
            }
            else
            {
                for (int32 i = 1; i < spline.getPointCount() - 1; ++i)
                    data << spline.getPoint(i).y << spline.getPoint(i).x << spline.getPoint(i).z;
            }
        }
        else
        {
            Vector3 const& point = spline.getPoint(spline.getPointCount() - 2);
            data << point.y << point.x << point.z;
        }

        if (type == MonsterMoveFacingTarget)
            for (uint8 index : { 5, 7, 0, 4, 3, 2, 6, 1 })
                data.WriteByteSeq(targetGuid[index]);

        data.WriteByteSeq(guid[5]);

        if (hasParabolicSpeed)
            data << float(moveSpline.vertical_acceleration);

        if (hasParabolicTime)
            data << uint32(moveSpline.effect_start_time);

        if (type == MonsterMoveFacingAngle)
            data << float(moveSpline.facing.angle);

        data.WriteByteSeq(guid[3]);

        if (flags)
            data << uint32(flags);

        data.WriteByteSeq(guid[6]);

        if (type == MonsterMoveFacingSpot)
            data << moveSpline.facing.f.x << moveSpline.facing.f.y << moveSpline.facing.f.z;

        data.WriteByteSeq(guid[0]);

        if (hasSeat)
            data << int8(seat);

        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[2]);
        data.WriteByteSeq(guid[4]);

        if (moveSpline.Duration())
            data << uint32(moveSpline.Duration());
    }

    void PacketBuilder::WriteStopMovement(uint32 splineId, ByteBuffer& data, ObjectGuid const& guid,
        ObjectGuid const& transport, Vector3 const& transportOffset, int8 seat, bool hasSeat, Vector3 const& position)
    {
        data << float(position.z);
        data << float(position.x);
        data << uint32(splineId);
        data << float(position.y);
        data << float(transportOffset.y);
        data << float(transportOffset.z);
        data << float(transportOffset.x);

        data.WriteBit(1);                                   // no parabolic speed
        data.WriteBit(guid[0]);
        data.WriteBits(MonsterMoveStop, 3);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(!hasSeat);
        data.WriteBits(0, 20);
        data.WriteBit(1);
        data.WriteBit(guid[3]);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(1);
        data.WriteBit(guid[7]);
        data.WriteBit(guid[4]);
        data.WriteBit(1);
        data.WriteBit(guid[5]);
        data.WriteBits(0, 22);
        data.WriteBit(guid[6]);
        data.WriteBit(0);
        WriteMonsterMoveTransportBits(data, transport);
        data.WriteBit(0);
        data.WriteBit(0);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[1]);
        data.FlushBits();

        data.WriteByteSeq(guid[1]);
        WriteMonsterMoveTransportBytes(data, transport);
        data.WriteByteSeq(guid[5]);
        data.WriteByteSeq(guid[3]);
        data.WriteByteSeq(guid[6]);
        data.WriteByteSeq(guid[0]);

        if (hasSeat)
            data << int8(seat);

        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[2]);
        data.WriteByteSeq(guid[4]);
    }

    uint8 PacketBuilder::GetMonsterMoveType(MoveSpline const& moveSpline)
    {
        switch (moveSpline.splineflags & MoveSplineFlag::Mask_Final_Facing)
        {
            case MoveSplineFlag::Final_Target:
                return MonsterMoveFacingTarget;
            case MoveSplineFlag::Final_Angle:
                return MonsterMoveFacingAngle;
            case MoveSplineFlag::Final_Point:
                return MonsterMoveFacingSpot;
            default:
                return MonsterMoveNormal;
        }
    }

    // 5.4.8 SMSG_UPDATE_OBJECT spline block, from Project SkyFire.
    void PacketBuilder::WriteCreateBits(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if (!data.WriteBit(!moveSpline.Finalized()))
            return;

        data.WriteBit(moveSpline.splineflags.parabolic || moveSpline.splineflags.animation);
        data.WriteBit(moveSpline.splineflags.parabolic && moveSpline.effect_start_time < moveSpline.Duration());
        data.WriteBit(0);                                   // spline filter, not used
        data.WriteBits(moveSpline.getPath().size(), 20);
        data.WriteBits(uint8(moveSpline.spline.mode()), 2);
        data.WriteBits(moveSpline.splineflags.raw(), 25);
    }

    void PacketBuilder::WriteCreateData(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if (!moveSpline.Finalized())
        {
            MoveSplineFlag const& splineFlags = moveSpline.splineflags;
            uint8 type = GetMonsterMoveType(moveSpline);

            data << moveSpline.timePassed();
            data << float(1.f);                             // splineInfo.duration_mod_next
            data << float(1.f);                             // splineInfo.duration_mod

            uint32 nodes = moveSpline.getPath().size();
            for (uint32 i = 0; i < nodes; ++i)
            {
                data << float(moveSpline.getPath()[i].x);
                data << float(moveSpline.getPath()[i].z);
                data << float(moveSpline.getPath()[i].y);
            }

            if (splineFlags.parabolic || splineFlags.animation)
                data << moveSpline.effect_start_time;

            data << uint8(type);

            if (type == MonsterMoveFacingAngle)
                data << float(moveSpline.facing.angle);

            if (type == MonsterMoveFacingSpot)
                data << moveSpline.facing.f.x << moveSpline.facing.f.z << moveSpline.facing.f.y;

            if (splineFlags.parabolic && moveSpline.effect_start_time < moveSpline.Duration())
                data << float(moveSpline.vertical_acceleration);

            data << moveSpline.Duration();
        }

        Vector3 destination = moveSpline.isCyclic() ? Vector3::zero() : moveSpline.FinalDestination();

        data << float(destination.x);
        data << float(destination.z);
        data << moveSpline.GetId();
        data << float(destination.y);
    }

    void PacketBuilder::WriteFacingTargetPart(MoveSpline const& moveSpline, ByteBuffer& data)
    {
        if (GetMonsterMoveType(moveSpline) != MonsterMoveFacingTarget || moveSpline.Finalized())
            return;

        ObjectGuid facingGuid(moveSpline.facing.target);
        for (uint8 index : { 4, 7, 0, 5, 1, 2, 3, 6 })
            data.WriteBit(facingGuid[index]);

        data.FlushBits();

        for (uint8 index : { 4, 2, 0, 5, 6, 3, 1, 7 })
            data.WriteByteSeq(facingGuid[index]);
    }
}
