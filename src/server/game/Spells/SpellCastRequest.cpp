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

#include "SpellCastRequest.h"
#include "ByteBuffer.h"
#include "Object.h"
#include "Spell.h"
#include "SpellPackets.h"

// 5.4.8 18414 CMSG_CAST_SPELL, CMSG_USE_ITEM and CMSG_PET_CAST_SPELL, from Project SkyFire
// (SpellHandler.cpp ReadCastSpellRequest/ReadUseItemRequest, PetHandler.cpp ReadPetCastSpellRequest).
// The caster's movement block is read and dropped, as SkyFire does.

void SpellCastRequest::FillTargets(SpellCastTargets& targets, WorldObject* caster) const
{
    // a location the client did not send is the caster's, like SpellCastTargets::Read does
    ObjectGuid casterTransport = caster->GetTransGUID();
    Position casterPos = casterTransport
        ? Position(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO())
        : Position(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), caster->GetOrientation());

    bool hasSrc = targetMask & TARGET_FLAG_SOURCE_LOCATION;
    bool hasDst = targetMask & TARGET_FLAG_DEST_LOCATION;
    targets.Initialize(caster, targetMask, targetGuid, itemTargetGuid,
        hasSrc ? srcTransportGuid : casterTransport, hasSrc ? srcPos : casterPos,
        hasDst ? destTransportGuid : casterTransport, hasDst ? destPos : casterPos, targetString);
    targets.SetElevation(elevation);
    targets.SetSpeed(missileSpeed);
}

SpellCastRequest SpellCastRequest::FromPacket(WorldPackets::Spells::SpellCastRequest const& cast)
{
    SpellCastRequest request;
    request.castCount = cast.CastID;
    request.castFlags = cast.SendCastFlags;
    request.spellId = uint32(cast.SpellID);
    request.glyphIndex = uint32(cast.Misc[0]);
    request.targetMask = cast.Target.Flags;
    request.elevation = cast.MissileTrajectory.Pitch;
    request.missileSpeed = cast.MissileTrajectory.Speed;
    request.targetGuid = cast.Target.Unit;
    request.itemTargetGuid = cast.Target.Item;
    request.targetString = cast.Target.Name;

    if (cast.Target.SrcLocation)
    {
        request.srcTransportGuid = cast.Target.SrcLocation->Transport;
        request.srcPos.Relocate(cast.Target.SrcLocation->Location);
    }

    if (cast.Target.DstLocation)
    {
        request.destTransportGuid = cast.Target.DstLocation->Transport;
        request.destPos.Relocate(cast.Target.DstLocation->Location);
    }

    if (cast.Target.Orientation)
        request.destPos.SetOrientation(*cast.Target.Orientation);

    // archaeology: keystones (type 1, item) and fragments (type 2, currency) used for a project
    for (WorldPackets::Spells::SpellWeight const& weight : cast.Weight)
    {
        switch (weight.Type)
        {
            case 1:
                request.researchData.KeyId = uint32(weight.ID);
                request.researchData.KeyCount = weight.Quantity;
                request.hasResearchData = true;
                break;
            case 2:
                request.researchData.FragId = uint32(weight.ID);
                request.researchData.FragCount = weight.Quantity;
                request.hasResearchData = true;
                break;
            default:
                break;
        }
    }

    return request;
}

namespace WorldPackets::Spells
{
    ::SpellCastRequest ReadUseItem(ByteBuffer& recvPacket, WorldObject* caster)
    {
        ::SpellCastRequest request;
        uint32 targetStringLength = 0;
        uint8 researchTypes[4] = { 0, 0, 0, 0 };

        // Movement data
        MovementInfo movementInfo;
        ObjectGuid movementTransportGuid;
        ObjectGuid movementGuid;
        bool hasTransport = false;
        bool hasTransportTime2 = false;
        bool hasTransportTime3 = false;
        bool hasFallData = false;
        bool hasFallDirection = false;
        bool hasTimestamp = false;
        bool hasSplineElevation = false;
        bool hasPitch = false;
        bool hasOrientation = false;
        bool hasUnkMovementField = false;
        uint32 unkMovementLoopCounter = 0;

        recvPacket >> request.slot >> request.bagIndex;

        bool hasElevation = !recvPacket.ReadBit();
        request.itemGuid[6] = recvPacket.ReadBit();
        bool hasTargetString = !recvPacket.ReadBit();
        request.itemGuid[1] = recvPacket.ReadBit();
        bool hasCastFlags = !recvPacket.ReadBit();
        bool hasDestLocation = recvPacket.ReadBit();
        request.itemGuid[2] = recvPacket.ReadBit();
        request.itemGuid[7] = recvPacket.ReadBit();
        request.itemGuid[0] = recvPacket.ReadBit();
        bool hasTargetMask = !recvPacket.ReadBit();
        bool hasMissileSpeed = !recvPacket.ReadBit();
        bool hasMovement = recvPacket.ReadBit();
        bool hasCastCount = !recvPacket.ReadBit();
        bool hasSpellId = !recvPacket.ReadBit();
        recvPacket.ReadBit();
        bool hasGlyphIndex = !recvPacket.ReadBit();
        recvPacket.ReadBit();
        request.itemGuid[4] = recvPacket.ReadBit();
        bool hasSrcLocation = recvPacket.ReadBit();
        request.itemGuid[3] = recvPacket.ReadBit();
        request.itemGuid[5] = recvPacket.ReadBit();
        uint8 researchDataCount = recvPacket.ReadBits(2);

        for (uint8 i = 0; i < researchDataCount; ++i)
            researchTypes[i] = recvPacket.ReadBits(2);

        if (hasMovement)
        {
            hasPitch = !recvPacket.ReadBit();
            hasTransport = recvPacket.ReadBit();
            recvPacket.ReadBit();

            if (hasTransport)
            {
                movementTransportGuid[7] = recvPacket.ReadBit();
                movementTransportGuid[2] = recvPacket.ReadBit();
                movementTransportGuid[4] = recvPacket.ReadBit();
                movementTransportGuid[5] = recvPacket.ReadBit();
                movementTransportGuid[6] = recvPacket.ReadBit();
                movementTransportGuid[0] = recvPacket.ReadBit();
                movementTransportGuid[1] = recvPacket.ReadBit();
                hasTransportTime3 = recvPacket.ReadBit();
                movementTransportGuid[4] = recvPacket.ReadBit();
                hasTransportTime2 = recvPacket.ReadBit();
            }

            movementGuid[6] = recvPacket.ReadBit();
            movementGuid[2] = recvPacket.ReadBit();
            movementGuid[1] = recvPacket.ReadBit();
            unkMovementLoopCounter = recvPacket.ReadBits(22);
            recvPacket.ReadBit();
            bool hasMovementFlags2 = !recvPacket.ReadBit();
            hasFallData = recvPacket.ReadBit();
            movementGuid[5] = recvPacket.ReadBit();
            hasSplineElevation = !recvPacket.ReadBit();
            recvPacket.ReadBit();
            movementGuid[7] = recvPacket.ReadBit();
            movementGuid[0] = recvPacket.ReadBit();

            if (hasFallData)
                hasFallDirection = recvPacket.ReadBit();

            hasOrientation = !recvPacket.ReadBit();
            movementGuid[4] = recvPacket.ReadBit();
            movementGuid[3] = recvPacket.ReadBit();
            hasTimestamp = !recvPacket.ReadBit();
            hasUnkMovementField = !recvPacket.ReadBit();
            bool hasMovementFlags = !recvPacket.ReadBit();

            if (hasMovementFlags2)
                movementInfo.flags2 = recvPacket.ReadBits(13);

            // Only present when the flags bit says so: reading it unconditionally misaligns the rest of the packet
            // whenever the item is used while moving or swimming without movement flags.
            if (hasMovementFlags)
                movementInfo.flags = recvPacket.ReadBits(30);
        }

        if (hasSrcLocation)
        {
            request.srcTransportGuid[3] = recvPacket.ReadBit();
            request.srcTransportGuid[1] = recvPacket.ReadBit();
            request.srcTransportGuid[7] = recvPacket.ReadBit();
            request.srcTransportGuid[4] = recvPacket.ReadBit();
            request.srcTransportGuid[2] = recvPacket.ReadBit();
            request.srcTransportGuid[0] = recvPacket.ReadBit();
            request.srcTransportGuid[6] = recvPacket.ReadBit();
            request.srcTransportGuid[5] = recvPacket.ReadBit();
        }

        if (hasDestLocation)
        {
            request.destTransportGuid[2] = recvPacket.ReadBit();
            request.destTransportGuid[4] = recvPacket.ReadBit();
            request.destTransportGuid[1] = recvPacket.ReadBit();
            request.destTransportGuid[7] = recvPacket.ReadBit();
            request.destTransportGuid[6] = recvPacket.ReadBit();
            request.destTransportGuid[0] = recvPacket.ReadBit();
            request.destTransportGuid[3] = recvPacket.ReadBit();
            request.destTransportGuid[5] = recvPacket.ReadBit();
        }

        if (hasTargetString)
            targetStringLength = recvPacket.ReadBits(7);

        request.targetGuid[1] = recvPacket.ReadBit();
        request.targetGuid[0] = recvPacket.ReadBit();
        request.targetGuid[5] = recvPacket.ReadBit();
        request.targetGuid[3] = recvPacket.ReadBit();
        request.targetGuid[6] = recvPacket.ReadBit();
        request.targetGuid[4] = recvPacket.ReadBit();
        request.targetGuid[7] = recvPacket.ReadBit();
        request.targetGuid[2] = recvPacket.ReadBit();

        request.itemTargetGuid[4] = recvPacket.ReadBit();
        request.itemTargetGuid[5] = recvPacket.ReadBit();
        request.itemTargetGuid[0] = recvPacket.ReadBit();
        request.itemTargetGuid[1] = recvPacket.ReadBit();
        request.itemTargetGuid[3] = recvPacket.ReadBit();
        request.itemTargetGuid[7] = recvPacket.ReadBit();
        request.itemTargetGuid[6] = recvPacket.ReadBit();
        request.itemTargetGuid[2] = recvPacket.ReadBit();

        if (hasCastFlags)
            request.castFlags = recvPacket.ReadBits(5);

        if (hasTargetMask)
            request.targetMask = recvPacket.ReadBits(20);

        recvPacket.ReadByteSeq(request.itemGuid[0]);
        recvPacket.ReadByteSeq(request.itemGuid[5]);
        recvPacket.ReadByteSeq(request.itemGuid[6]);
        recvPacket.ReadByteSeq(request.itemGuid[3]);
        recvPacket.ReadByteSeq(request.itemGuid[4]);
        recvPacket.ReadByteSeq(request.itemGuid[2]);
        recvPacket.ReadByteSeq(request.itemGuid[1]);

        for (uint8 i = 0; i < researchDataCount; ++i)
        {
            // Client sends (usedCount, entryId); type 1 = keystone item, type 2 = fragment currency.
            uint32 usedCount = 0;
            uint32 entry = 0;
            recvPacket >> usedCount;
            recvPacket >> entry;

            switch (researchTypes[i])
            {
                case 1: // Keystones
                    request.researchData.KeyId = entry;
                    request.researchData.KeyCount = usedCount;
                    request.hasResearchData = true;
                    break;
                case 2: // Fragments
                    request.researchData.FragId = entry;
                    request.researchData.FragCount = usedCount;
                    request.hasResearchData = true;
                    break;
                default:
                    break;
            }
        }

        recvPacket.ReadByteSeq(request.itemGuid[7]);

        if (hasMovement)
        {
            for (uint8 i = 0; i != unkMovementLoopCounter; i++)
                recvPacket.read_skip<uint32>();

            if (hasTransport)
            {
                recvPacket >> movementInfo.transport.pos.m_positionY;
                recvPacket >> movementInfo.transport.pos.m_positionZ;
                recvPacket.ReadByteSeq(movementTransportGuid[1]);

                if (hasTransportTime3)
                    recvPacket >> movementInfo.transport.vehicleId;

                recvPacket.ReadByteSeq(movementTransportGuid[7]);
                recvPacket.ReadByteSeq(movementTransportGuid[5]);
                recvPacket.ReadByteSeq(movementTransportGuid[2]);
                recvPacket.ReadByteSeq(movementTransportGuid[4]);
                recvPacket >> movementInfo.transport.pos.m_positionX;
                movementInfo.transport.pos.SetOrientation(recvPacket.read<float>());
                recvPacket.ReadByteSeq(movementTransportGuid[0]);
                recvPacket >> movementInfo.transport.seat;
                recvPacket >> movementInfo.transport.time;
                recvPacket.ReadByteSeq(movementTransportGuid[6]);
                recvPacket.ReadByteSeq(movementTransportGuid[3]);

                if (hasTransportTime2)
                    recvPacket >> movementInfo.transport.time2;
            }

            if (hasFallData)
            {
                recvPacket >> movementInfo.jump.zspeed;

                if (hasFallDirection)
                {
                    recvPacket >> movementInfo.jump.sinAngle;
                    recvPacket >> movementInfo.jump.cosAngle;
                    recvPacket >> movementInfo.jump.xyspeed;
                }

                recvPacket >> movementInfo.fallTime;
            }

            recvPacket.ReadByteSeq(movementGuid[3]);
            recvPacket.ReadByteSeq(movementGuid[7]);
            recvPacket.ReadByteSeq(movementGuid[6]);
            recvPacket.ReadByteSeq(movementGuid[1]);
            recvPacket >> movementInfo.pos.m_positionY;

            if (hasSplineElevation)
                recvPacket >> movementInfo.splineElevation;

            if (hasUnkMovementField)
                recvPacket.read_skip<uint32>();

            if (hasOrientation)
                movementInfo.pos.SetOrientation(recvPacket.read<float>());

            recvPacket.ReadByteSeq(movementGuid[2]);
            recvPacket >> movementInfo.pos.m_positionZ;

            if (hasTimestamp)
                recvPacket >> movementInfo.time;

            recvPacket >> movementInfo.pos.m_positionX;
            recvPacket.ReadByteSeq(movementGuid[5]);
            recvPacket.ReadByteSeq(movementGuid[0]);

            if (hasPitch)
                movementInfo.pitch = recvPacket.read<float>();

            recvPacket.ReadByteSeq(movementGuid[4]);
        }

        if (hasDestLocation)
        {
            float x, y, z;

            recvPacket.ReadByteSeq(request.destTransportGuid[7]);
            recvPacket >> x;
            recvPacket.ReadByteSeq(request.destTransportGuid[0]);
            recvPacket.ReadByteSeq(request.destTransportGuid[6]);
            recvPacket.ReadByteSeq(request.destTransportGuid[1]);
            recvPacket.ReadByteSeq(request.destTransportGuid[3]);
            recvPacket >> y;
            recvPacket.ReadByteSeq(request.destTransportGuid[5]);
            recvPacket >> z;
            recvPacket.ReadByteSeq(request.destTransportGuid[4]);
            recvPacket.ReadByteSeq(request.destTransportGuid[2]);

            request.destPos.Relocate(x, y, z);
        }
        else
        {
            request.destTransportGuid = caster->GetTransGUID();

            if (request.destTransportGuid)
                request.destPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
            else
                request.destPos.Relocate(caster);
        }

        recvPacket.ReadByteSeq(request.itemTargetGuid[6]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[7]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[2]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[0]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[3]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[4]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[1]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[5]);

        if (hasSrcLocation)
        {
            float x, y, z;

            recvPacket.ReadByteSeq(request.srcTransportGuid[7]);
            recvPacket >> x;
            recvPacket.ReadByteSeq(request.srcTransportGuid[1]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[5]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[4]);
            recvPacket >> z;
            recvPacket.ReadByteSeq(request.srcTransportGuid[6]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[0]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[3]);
            recvPacket >> y;
            recvPacket.ReadByteSeq(request.srcTransportGuid[2]);

            request.srcPos.Relocate(x, y, z);
        }
        else
        {
            request.srcTransportGuid = caster->GetTransGUID();
            if (request.srcTransportGuid)
                request.srcPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
            else
                request.srcPos.Relocate(caster);
        }

        if (hasSpellId)
            recvPacket >> request.spellId;

        recvPacket.ReadByteSeq(request.targetGuid[1]);
        recvPacket.ReadByteSeq(request.targetGuid[4]);
        recvPacket.ReadByteSeq(request.targetGuid[3]);
        recvPacket.ReadByteSeq(request.targetGuid[6]);
        recvPacket.ReadByteSeq(request.targetGuid[2]);
        recvPacket.ReadByteSeq(request.targetGuid[0]);
        recvPacket.ReadByteSeq(request.targetGuid[7]);
        recvPacket.ReadByteSeq(request.targetGuid[5]);

        if (hasTargetString)
            request.targetString = recvPacket.ReadString(targetStringLength);

        if (hasElevation)
            recvPacket >> request.elevation;

        if (hasGlyphIndex)
            recvPacket >> request.glyphIndex;

        if (hasMissileSpeed)
            recvPacket >> request.missileSpeed;

        if (hasCastCount)
            recvPacket >> request.castCount;

        return request;
    }

    ::SpellCastRequest ReadCastSpell(ByteBuffer& recvPacket, WorldObject* caster)
    {
        ::SpellCastRequest request;
        uint32 targetStringLength = 0;
        uint8 researchTypes[4] = { 0, 0, 0, 0 };

        // Movement data
        MovementInfo movementInfo;
        ObjectGuid movementTransportGuid;
        ObjectGuid movementGuid;
        bool hasTransport = false;
        bool hasTransportTime2 = false;
        bool hasTransportTime3 = false;
        bool hasFallData = false;
        bool hasFallDirection = false;
        bool hasTimestamp = false;
        bool hasSplineElevation = false;
        bool hasPitch = false;
        bool hasOrientation = false;
        bool hasUnkMovementField = false;
        uint32 unkMovementLoopCounter = 0;

        recvPacket.ReadBit(); // Fake bit
        bool hasTargetString = !recvPacket.ReadBit();
        recvPacket.ReadBit(); // Fake Bit
        bool hasCastCount = !recvPacket.ReadBit();
        bool hasSrcLocation = recvPacket.ReadBit();
        bool hasDestLocation = recvPacket.ReadBit();
        bool hasSpellId = !recvPacket.ReadBit();
        uint8 researchDataCount = recvPacket.ReadBits(2);
        bool hasTargetMask = !recvPacket.ReadBit();
        bool hasMissileSpeed = !recvPacket.ReadBit();

        for (uint8 i = 0; i < researchDataCount; ++i)
            researchTypes[i] = recvPacket.ReadBits(2);

        bool hasGlyphIndex = !recvPacket.ReadBit();
        bool hasMovement = recvPacket.ReadBit();
        bool hasElevation = !recvPacket.ReadBit();
        bool hasCastFlags = !recvPacket.ReadBit();

        request.targetGuid[5] = recvPacket.ReadBit();
        request.targetGuid[4] = recvPacket.ReadBit();
        request.targetGuid[2] = recvPacket.ReadBit();
        request.targetGuid[7] = recvPacket.ReadBit();
        request.targetGuid[1] = recvPacket.ReadBit();
        request.targetGuid[6] = recvPacket.ReadBit();
        request.targetGuid[3] = recvPacket.ReadBit();
        request.targetGuid[0] = recvPacket.ReadBit();

        if (hasDestLocation)
        {
            request.destTransportGuid[1] = recvPacket.ReadBit();
            request.destTransportGuid[3] = recvPacket.ReadBit();
            request.destTransportGuid[5] = recvPacket.ReadBit();
            request.destTransportGuid[0] = recvPacket.ReadBit();
            request.destTransportGuid[2] = recvPacket.ReadBit();
            request.destTransportGuid[6] = recvPacket.ReadBit();
            request.destTransportGuid[7] = recvPacket.ReadBit();
            request.destTransportGuid[4] = recvPacket.ReadBit();
        }

        if (hasMovement)
        {
            unkMovementLoopCounter = recvPacket.ReadBits(22);
            recvPacket.ReadBit();
            movementGuid[4] = recvPacket.ReadBit();
            hasTransport = recvPacket.ReadBit();

            if (hasTransport)
            {
                hasTransportTime2 = recvPacket.ReadBit();
                movementTransportGuid[7] = recvPacket.ReadBit();
                movementTransportGuid[4] = recvPacket.ReadBit();
                movementTransportGuid[1] = recvPacket.ReadBit();
                movementTransportGuid[0] = recvPacket.ReadBit();
                movementTransportGuid[6] = recvPacket.ReadBit();
                movementTransportGuid[3] = recvPacket.ReadBit();
                movementTransportGuid[5] = recvPacket.ReadBit();
                hasTransportTime3 = recvPacket.ReadBit();
                movementTransportGuid[2] = recvPacket.ReadBit();
            }

            recvPacket.ReadBit();
            movementGuid[7] = recvPacket.ReadBit();
            hasOrientation = !recvPacket.ReadBit();
            movementGuid[6] = recvPacket.ReadBit();
            hasSplineElevation = !recvPacket.ReadBit();
            hasPitch = !recvPacket.ReadBit();
            movementGuid[0] = recvPacket.ReadBit();
            recvPacket.ReadBit();
            bool hasMovementFlags = !recvPacket.ReadBit();
            hasTimestamp = !recvPacket.ReadBit();
            hasUnkMovementField = !recvPacket.ReadBit();

            if (hasMovementFlags)
                movementInfo.flags = recvPacket.ReadBits(30);

            movementGuid[1] = recvPacket.ReadBit();
            movementGuid[3] = recvPacket.ReadBit();
            movementGuid[2] = recvPacket.ReadBit();
            movementGuid[5] = recvPacket.ReadBit();
            hasFallData = recvPacket.ReadBit();

            if (hasFallData)
                hasFallDirection = recvPacket.ReadBit();

            bool hasMovementFlags2 = !recvPacket.ReadBit();

            if (hasMovementFlags2)
                movementInfo.flags2 = recvPacket.ReadBits(13);
        }

        request.itemTargetGuid[1] = recvPacket.ReadBit();
        request.itemTargetGuid[0] = recvPacket.ReadBit();
        request.itemTargetGuid[7] = recvPacket.ReadBit();
        request.itemTargetGuid[4] = recvPacket.ReadBit();
        request.itemTargetGuid[6] = recvPacket.ReadBit();
        request.itemTargetGuid[5] = recvPacket.ReadBit();
        request.itemTargetGuid[3] = recvPacket.ReadBit();
        request.itemTargetGuid[2] = recvPacket.ReadBit();

        if (hasSrcLocation)
        {
            request.srcTransportGuid[4] = recvPacket.ReadBit();
            request.srcTransportGuid[5] = recvPacket.ReadBit();
            request.srcTransportGuid[3] = recvPacket.ReadBit();
            request.srcTransportGuid[0] = recvPacket.ReadBit();
            request.srcTransportGuid[7] = recvPacket.ReadBit();
            request.srcTransportGuid[1] = recvPacket.ReadBit();
            request.srcTransportGuid[6] = recvPacket.ReadBit();
            request.srcTransportGuid[2] = recvPacket.ReadBit();
        }

        if (hasTargetMask)
            request.targetMask = recvPacket.ReadBits(20);

        if (hasCastFlags)
            request.castFlags = recvPacket.ReadBits(5);

        if (hasTargetString)
            targetStringLength = recvPacket.ReadBits(7);

        for (uint8 i = 0; i < researchDataCount; ++i)
        {
            // Client sends (usedCount, entryId); type 1 = keystone item, type 2 = fragment currency.
            uint32 usedCount = 0;
            uint32 entry = 0;
            recvPacket >> usedCount;
            recvPacket >> entry;

            switch (researchTypes[i])
            {
                case 1: // Keystones
                    request.researchData.KeyId = entry;
                    request.researchData.KeyCount = usedCount;
                    request.hasResearchData = true;
                    break;
                case 2: // Fragments
                    request.researchData.FragId = entry;
                    request.researchData.FragCount = usedCount;
                    request.hasResearchData = true;
                    break;
                default:
                    break;
            }
        }

        if (hasMovement)
        {
            recvPacket >> movementInfo.pos.m_positionX;
            recvPacket.ReadByteSeq(movementGuid[0]);

            if (hasTransport)
            {
                recvPacket.ReadByteSeq(movementTransportGuid[2]);
                recvPacket >> movementInfo.transport.seat;
                recvPacket.ReadByteSeq(movementTransportGuid[3]);
                recvPacket.ReadByteSeq(movementTransportGuid[7]);
                recvPacket >> movementInfo.transport.pos.m_positionX;
                recvPacket.ReadByteSeq(movementTransportGuid[5]);

                if (hasTransportTime3)
                    recvPacket >> movementInfo.transport.vehicleId;

                recvPacket >> movementInfo.transport.pos.m_positionZ;
                recvPacket >> movementInfo.transport.pos.m_positionY;

                recvPacket.ReadByteSeq(movementTransportGuid[6]);
                recvPacket.ReadByteSeq(movementTransportGuid[1]);
                movementInfo.transport.pos.SetOrientation(recvPacket.read<float>());

                recvPacket.ReadByteSeq(movementTransportGuid[4]);

                if (hasTransportTime2)
                    recvPacket >> movementInfo.transport.time2;

                recvPacket.ReadByteSeq(movementTransportGuid[0]);
                recvPacket >> movementInfo.transport.time;
            }

            recvPacket.ReadByteSeq(movementGuid[5]);

            if (hasFallData)
            {
                recvPacket >> movementInfo.fallTime;
                recvPacket >> movementInfo.jump.zspeed;

                if (hasFallDirection)
                {
                    recvPacket >> movementInfo.jump.sinAngle;
                    recvPacket >> movementInfo.jump.xyspeed;
                    recvPacket >> movementInfo.jump.cosAngle;
                }
            }

            if (hasSplineElevation)
                recvPacket >> movementInfo.splineElevation;

            recvPacket.ReadByteSeq(movementGuid[6]);

            if (hasUnkMovementField)
                recvPacket.read_skip<uint32>();

            recvPacket.ReadByteSeq(movementGuid[4]);

            if (hasOrientation)
                movementInfo.pos.SetOrientation(recvPacket.read<float>());

            if (hasTimestamp)
                recvPacket >> movementInfo.time;

            recvPacket.ReadByteSeq(movementGuid[1]);

            if (hasPitch)
                movementInfo.pitch = recvPacket.read<float>();

            recvPacket.ReadByteSeq(movementGuid[3]);

            for (uint8 i = 0; i != unkMovementLoopCounter; i++)
                recvPacket.read_skip<uint32>();

            recvPacket >> movementInfo.pos.m_positionY;
            recvPacket.ReadByteSeq(movementGuid[7]);
            recvPacket >> movementInfo.pos.m_positionZ;
            recvPacket.ReadByteSeq(movementGuid[2]);
        }

        recvPacket.ReadByteSeq(request.itemTargetGuid[4]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[2]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[1]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[5]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[7]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[3]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[6]);
        recvPacket.ReadByteSeq(request.itemTargetGuid[0]);

        if (hasDestLocation)
        {
            float x, y, z;
            recvPacket.ReadByteSeq(request.destTransportGuid[2]);
            recvPacket >> x;
            recvPacket.ReadByteSeq(request.destTransportGuid[4]);
            recvPacket.ReadByteSeq(request.destTransportGuid[1]);
            recvPacket.ReadByteSeq(request.destTransportGuid[0]);
            recvPacket.ReadByteSeq(request.destTransportGuid[3]);
            recvPacket >> y;
            recvPacket.ReadByteSeq(request.destTransportGuid[7]);
            recvPacket >> z;
            recvPacket.ReadByteSeq(request.destTransportGuid[5]);
            recvPacket.ReadByteSeq(request.destTransportGuid[6]);
            request.destPos.Relocate(x, y, z);
        }
        else
        {
            request.destTransportGuid = caster->GetTransGUID();
            if (request.destTransportGuid)
                request.destPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
            else
                request.destPos.Relocate(caster);
        }

        recvPacket.ReadByteSeq(request.targetGuid[3]);
        recvPacket.ReadByteSeq(request.targetGuid[4]);
        recvPacket.ReadByteSeq(request.targetGuid[7]);
        recvPacket.ReadByteSeq(request.targetGuid[6]);
        recvPacket.ReadByteSeq(request.targetGuid[2]);
        recvPacket.ReadByteSeq(request.targetGuid[0]);
        recvPacket.ReadByteSeq(request.targetGuid[1]);
        recvPacket.ReadByteSeq(request.targetGuid[5]);

        if (hasSrcLocation)
        {
            float x, y, z;
            recvPacket >> y;
            recvPacket.ReadByteSeq(request.srcTransportGuid[5]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[1]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[7]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[6]);
            recvPacket >> x;
            recvPacket.ReadByteSeq(request.srcTransportGuid[3]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[2]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[0]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[4]);
            recvPacket >> z;
            request.srcPos.Relocate(x, y, z);
        }
        else
        {
            request.srcTransportGuid = caster->GetTransGUID();
            if (request.srcTransportGuid)
                request.srcPos.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
            else
                request.srcPos.Relocate(caster);
        }

        if (hasTargetString)
            request.targetString = recvPacket.ReadString(targetStringLength);

        if (hasMissileSpeed)
            recvPacket >> request.missileSpeed;

        if (hasElevation)
            recvPacket >> request.elevation;

        if (hasCastCount)
            recvPacket >> request.castCount;

        if (hasSpellId)
            recvPacket >> request.spellId;

        if (hasGlyphIndex)
            recvPacket >> request.glyphIndex;

        return request;
    }

    ::SpellCastRequest ReadPetCastSpell(ByteBuffer& recvPacket)
    {
        ::SpellCastRequest request;
        ObjectGuid transportGuid;
        ObjectGuid movementGuid;

        bool hasUnkMovementField = false;
        bool hasTransport = false;
        bool hasTransportTime2 = false;
        bool hasTransportTime3 = false;
        bool hasMovementFlags = false;
        bool hasMovementFlags2 = false;
        bool hasFallDirection = false;
        bool hasFallData = false;
        bool hasPitch = false;
        bool hasOrientation = false;
        bool hasSplineElevation = false;
        bool hasTimestamp = false;

        uint32 targetStrLength = 0;
        uint32 movementForcesCount = 0;
        uint32 archeologyCount = 0;

        bool hasDestLocation = recvPacket.ReadBit();
        request.petGuid[7] = recvPacket.ReadBit();
        bool hasMissileSpeed = !recvPacket.ReadBit();
        bool hasSrcLocation = recvPacket.ReadBit();
        request.petGuid[1] = recvPacket.ReadBit();
        archeologyCount = recvPacket.ReadBits(2);
        bool hasTargetMask = !recvPacket.ReadBit();
        request.petGuid[4] = recvPacket.ReadBit();
        recvPacket.ReadBit();
        request.petGuid[6] = recvPacket.ReadBit();
        bool hasTargetString = !recvPacket.ReadBit();
        recvPacket.ReadBit();
        bool hasMovement = recvPacket.ReadBit();
        bool hasCastFlags = !recvPacket.ReadBit();
        bool hasSpellId = !recvPacket.ReadBit();
        request.petGuid[0] = recvPacket.ReadBit();
        request.petGuid[5] = recvPacket.ReadBit();
        request.petGuid[2] = recvPacket.ReadBit();
        for (uint32 i = 0; i < archeologyCount; ++i)
            recvPacket.ReadBits(2);                            // archeologyType
        request.petGuid[3] = recvPacket.ReadBit();
        bool hasGlyphIndex = !recvPacket.ReadBit();
        bool hasCastCount = !recvPacket.ReadBit();
        bool hasElevation = !recvPacket.ReadBit();

        if (hasMovement)
        {
            hasOrientation = !recvPacket.ReadBit();
            hasSplineElevation = !recvPacket.ReadBit();
            recvPacket.ReadBit();
            movementGuid[5] = recvPacket.ReadBit();
            movementGuid[7] = recvPacket.ReadBit();
            hasMovementFlags2 = !recvPacket.ReadBit();
            hasTimestamp = !recvPacket.ReadBit();
            hasFallData = recvPacket.ReadBit();
            hasMovementFlags = !recvPacket.ReadBit();
            hasUnkMovementField = !recvPacket.ReadBit();

            if (hasMovementFlags)
                recvPacket.ReadBits(30);                       // Movement Flags

            recvPacket.ReadBit();
            movementGuid[6] = recvPacket.ReadBit();
            hasTransport = recvPacket.ReadBit();
            movementGuid[0] = recvPacket.ReadBit();
            movementForcesCount = recvPacket.ReadBits(22);

            if (hasTransport)
            {
                hasTransportTime2 = recvPacket.ReadBit();
                hasTransportTime3 = recvPacket.ReadBit();
                transportGuid[5] = recvPacket.ReadBit();
                transportGuid[6] = recvPacket.ReadBit();
                transportGuid[4] = recvPacket.ReadBit();
                transportGuid[0] = recvPacket.ReadBit();
                transportGuid[1] = recvPacket.ReadBit();
                transportGuid[2] = recvPacket.ReadBit();
                transportGuid[7] = recvPacket.ReadBit();
                transportGuid[3] = recvPacket.ReadBit();
            }

            movementGuid[1] = recvPacket.ReadBit();

            if (hasMovementFlags2)
                recvPacket.ReadBits(13);                       // Movement Flags 2

            movementGuid[3] = recvPacket.ReadBit();
            movementGuid[2] = recvPacket.ReadBit();
            recvPacket.ReadBit();
            hasPitch = !recvPacket.ReadBit();
            movementGuid[4] = recvPacket.ReadBit();

            if (hasFallData)
                hasFallDirection = recvPacket.ReadBit();
        }

        if (hasDestLocation)
            recvPacket.ReadGuidMask(request.destTransportGuid, { 2, 0, 1, 4, 5, 6, 3, 7 });

        if (hasCastFlags)
            request.castFlags = recvPacket.ReadBits(5);        // Cast Flags

        recvPacket.ReadGuidMask(request.itemTargetGuid, { 2, 4, 7, 0, 6, 1, 5, 3 });

        if (hasTargetMask)
            request.targetMask = recvPacket.ReadBits(20);     // Target Flags

        if (hasTargetString)
            targetStrLength = recvPacket.ReadBits(7);          // Target String

        if (hasSrcLocation)
            recvPacket.ReadGuidMask(request.srcTransportGuid, { 2, 0, 3, 1, 6, 7, 4, 5 });

        recvPacket.ReadGuidMask(request.targetGuid, { 6, 0, 3, 4, 2, 1, 5, 7 });

        recvPacket.ReadGuidBytes(request.petGuid, { 2, 6, 3 });

        for (uint32 i = 0; i < archeologyCount; ++i)
        {
            recvPacket.read_skip<uint32>();                    // unk1
            recvPacket.read_skip<uint32>();                    // unk2
        }
        recvPacket.ReadGuidBytes(request.petGuid, { 1, 7, 0, 4, 5 });

        if (hasDestLocation)
        {
            recvPacket.ReadByteSeq(request.destTransportGuid[4]);
            recvPacket.ReadByteSeq(request.destTransportGuid[1]);
            recvPacket.ReadByteSeq(request.destTransportGuid[7]);
            request.destPos.m_positionZ = recvPacket.read<float>(); // Position Z
            request.destPos.m_positionY = recvPacket.read<float>(); // Position Y
            recvPacket.ReadByteSeq(request.destTransportGuid[6]);
            recvPacket.ReadByteSeq(request.destTransportGuid[3]);
            request.destPos.m_positionX = recvPacket.read<float>(); // Position X
            recvPacket.ReadByteSeq(request.destTransportGuid[2]);
            recvPacket.ReadByteSeq(request.destTransportGuid[5]);
            recvPacket.ReadByteSeq(request.destTransportGuid[0]);
        }

        if (hasMovement)
        {
            if (hasPitch)
                recvPacket.read_skip<float>();                 // Pitch

            if (hasTransport)
            {
                if (hasTransportTime3)
                    recvPacket.read_skip<int32>();             // Transport Time 3

                if (hasTransportTime2)
                    recvPacket.read_skip<int32>();             // Transport Time 2

                recvPacket.read_skip<uint8>();                 // Transport Seat
                recvPacket.read_skip<float>();                 // Orientation
                recvPacket.read_skip<float>();                 // Position Z
                recvPacket.ReadByteSeq(transportGuid[2]);
                recvPacket.read_skip<int32>();                 // Transport Time
                recvPacket.ReadByteSeq(transportGuid[3]);
                recvPacket.read_skip<float>();                 // Position X
                recvPacket.ReadByteSeq(transportGuid[6]);
                recvPacket.ReadByteSeq(transportGuid[5]);
                recvPacket.ReadByteSeq(transportGuid[7]);
                recvPacket.ReadByteSeq(transportGuid[0]);
                recvPacket.read_skip<float>();                 // Position Y
                recvPacket.ReadByteSeq(transportGuid[4]);
                recvPacket.ReadByteSeq(transportGuid[1]);
            }

            if (hasUnkMovementField)
                recvPacket.read_skip<int32>();

            for (uint32 i = 0; i < movementForcesCount; ++i)
                recvPacket.read_skip<uint32>();                // Movement Forces

            recvPacket.ReadByteSeq(movementGuid[3]);

            if (hasOrientation)
                recvPacket.read_skip<float>();                 // Orientation

            recvPacket.ReadByteSeq(movementGuid[5]);

            if (hasFallData)
            {
                recvPacket.read_skip<float>();                 // Z Speed

                if (hasFallDirection)
                {
                    recvPacket.read_skip<float>();             // CosAngle
                    recvPacket.read_skip<float>();             // XY Speed
                    recvPacket.read_skip<float>();             // SinAngle
                }
                recvPacket.read_skip<int32>();                 // FallTime
            }

            if (hasTimestamp)
                recvPacket.read_skip<int32>();                 // Timestamp

            recvPacket.ReadByteSeq(movementGuid[6]);
            recvPacket.read_skip<float>();                     // Position X
            recvPacket.ReadByteSeq(movementGuid[1]);
            recvPacket.read_skip<float>();                     // Position Z
            recvPacket.ReadByteSeq(movementGuid[2]);
            recvPacket.ReadByteSeq(movementGuid[7]);
            recvPacket.ReadByteSeq(movementGuid[0]);
            recvPacket.read_skip<float>();                     // Position Y
            recvPacket.ReadByteSeq(movementGuid[4]);

            if (hasSplineElevation)
                recvPacket.read_skip<float>();                 // Spline Elevation
        }

        if (hasSrcLocation)
        {
            recvPacket.ReadByteSeq(request.srcTransportGuid[3]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[4]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[2]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[1]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[0]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[7]);
            request.srcPos.m_positionZ = recvPacket.read<float>(); // Position Z
            recvPacket.ReadByteSeq(request.srcTransportGuid[6]);
            recvPacket.ReadByteSeq(request.srcTransportGuid[5]);
            request.srcPos.m_positionX = recvPacket.read<float>(); // Position X
            request.srcPos.m_positionY = recvPacket.read<float>(); // Position Y
        }

        if (hasMissileSpeed)
            request.missileSpeed = recvPacket.read<float>();   // missileSpeed

        recvPacket.ReadGuidBytes(request.itemTargetGuid, { 1, 2, 5, 7, 4, 6, 3, 0 });

        recvPacket.ReadGuidBytes(request.targetGuid, { 1, 5, 7, 3, 0, 2, 4, 6 });

        if (hasElevation)
            request.elevation = recvPacket.read<float>();      // Elevation

        if (hasCastCount)
            request.castCount = recvPacket.read<uint8>();      // Cast Count

        if (hasTargetString)
            recvPacket.ReadString(targetStrLength);

        if (hasGlyphIndex)
            recvPacket.read_skip<int32>();                     // Glyph Index

        if (hasSpellId)
            request.spellId = recvPacket.read<uint32>();

        return request;
    }
}
