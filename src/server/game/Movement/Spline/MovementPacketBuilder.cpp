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

    // Cata build 15595 SMSG_ON_MONSTER_MOVE / SMSG_ON_MONSTER_MOVE_TRANSPORT spline-data body
    // (splineId, facing, flags, duration, [animation], [jump], path). Caller writes the mover
    // GUID / transport info / position ahead of this.
    void PacketBuilder::WriteMonsterMove(MoveSpline const& move_spline, ByteBuffer& data)
    {
        data << move_spline.GetId();

        MoveSplineFlag splineflags = move_spline.splineflags;
        switch (splineflags & MoveSplineFlag::Mask_Final_Facing)
        {
            case MoveSplineFlag::Final_Target:
                data << uint8(MonsterMoveFacingTarget);
                data << move_spline.facing.target;
                break;
            case MoveSplineFlag::Final_Angle:
                data << uint8(MonsterMoveFacingAngle);
                data << move_spline.facing.angle;
                break;
            case MoveSplineFlag::Final_Point:
                data << uint8(MonsterMoveFacingSpot);
                data << move_spline.facing.f.x << move_spline.facing.f.y << move_spline.facing.f.z;
                break;
            default:
                data << uint8(MonsterMoveNormal);
                break;
        }

        data << uint32(splineflags & uint32(~MoveSplineFlag::Mask_No_Monster_Move));

        if (splineflags.animation)
        {
            data << splineflags.animId;
            data << move_spline.effect_start_time;
        }

        data << move_spline.Duration();

        if (splineflags.parabolic)
        {
            data << move_spline.vertical_acceleration;
            data << move_spline.effect_start_time;
        }

        Spline<int32> const& spline = move_spline.spline;
        std::vector<Vector3> const& array = spline.getPoints();
        uint32 cyclicOffset = splineflags.cyclic ? 4 : 3;

        if (splineflags.uncompressedPath)
        {
            uint32 count = spline.getPointCount() - cyclicOffset;
            data << count;
            for (uint32 i = 0; i < count; ++i)
                data << array[i + 2];
        }
        else
        {
            uint32 last_idx = spline.getPointCount() - cyclicOffset;
            Vector3 const* real_path = &spline.getPoint(1);

            data << last_idx;
            data << real_path[last_idx];   // destination
            if (last_idx > 1)
            {
                Vector3 middle = (real_path[0] + real_path[last_idx]) / 2.f;
                // first and last points already appended
                for (uint32 i = 1; i < last_idx; ++i)
                {
                    Vector3 offset = middle - real_path[i];
                    data.appendPackXYZ(offset.x, offset.y, offset.z);
                }
            }
        }
    }

    void PacketBuilder::WriteStopMovement(uint32 splineId, ByteBuffer& data)
    {
        data << splineId;
        data << uint8(MonsterMoveStop);
        data << uint32(0);   // flags
        data << uint32(0);   // duration
        data << uint32(0);   // path count
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
