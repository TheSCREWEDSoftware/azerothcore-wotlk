/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * You may redistribute it and/or modify it under the terms of the GNU General Public License
 * version 2 or, at your option, any later version.
 */

#include "MovementPackets.h"
#include "Object.h"
#include "UnitDefines.h"
#include <G3D/Vector3.h>

uint32 WorldPackets::Movement::MovementFlagsToClient(uint32 flags)
{
    // AC keeps its internal flag positions. Cata moved transport and spline presence out of this field.
    return (flags & 0x000001FF) | ((flags & 0x07FFFC00) >> 1) | ((flags & 0xF0000000) >> 2);
}

uint16 WorldPackets::Movement::ExtraMovementFlagsToClient(uint16 flags)
{
    return (flags & 0x03C3) | ((flags & 0x0038) >> 1) | ((flags & 0x0004) << 3) |
        ((flags & 0x1C00) << 3) | ((flags & 0xE000) >> 3);
}

WorldPacket const* WorldPackets::Movement::TransferPending::Write()
{
    _worldPacket.WriteBit(TransferSpellID.has_value());
    _worldPacket.WriteBit(Ship.has_value());
    // AC's ByteBuffer does not flush pending bits on scalar writes like TC's append does.
    _worldPacket.FlushBits();
    if (Ship)
    {
        _worldPacket << int32(Ship->OriginMapID);
        _worldPacket << uint32(Ship->ID);
    }
    if (TransferSpellID)
        _worldPacket << int32(*TransferSpellID);
    _worldPacket << int32(MapID);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::SuspendToken::Write()
{
    _worldPacket << uint32(SequenceIndex);
    _worldPacket.WriteBit(Reason);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::NewWorld::Write()
{
    _worldPacket << Pos.GetPositionX();
    _worldPacket << Pos.GetOrientation();
    _worldPacket << Pos.GetPositionZ();
    _worldPacket << int32(MapID);
    _worldPacket << Pos.GetPositionY();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::MoveSetActiveMover::Write()
{
    _worldPacket.WriteBit(MoverGUID[5]);
    _worldPacket.WriteBit(MoverGUID[7]);
    _worldPacket.WriteBit(MoverGUID[3]);
    _worldPacket.WriteBit(MoverGUID[6]);
    _worldPacket.WriteBit(MoverGUID[0]);
    _worldPacket.WriteBit(MoverGUID[4]);
    _worldPacket.WriteBit(MoverGUID[1]);
    _worldPacket.WriteBit(MoverGUID[2]);

    _worldPacket.WriteByteSeq(MoverGUID[6]);
    _worldPacket.WriteByteSeq(MoverGUID[2]);
    _worldPacket.WriteByteSeq(MoverGUID[3]);
    _worldPacket.WriteByteSeq(MoverGUID[0]);
    _worldPacket.WriteByteSeq(MoverGUID[5]);
    _worldPacket.WriteByteSeq(MoverGUID[7]);
    _worldPacket.WriteByteSeq(MoverGUID[1]);
    _worldPacket.WriteByteSeq(MoverGUID[4]);

    return &_worldPacket;
}

using namespace WorldPackets::Movement;

static MovementStatusElements const MovementStartBackward[] =
{

    MSEPositionY,

    MSEPositionZ,

    MSEPositionX,

    MSEHasTimestamp,

    MSEHasOrientation,

    MSEHasGuidByte7,

    MSEHasGuidByte2,

    MSEForcesCount,

    MSEHasFallData,

    MSEZeroBit,

    MSEHasGuidByte5,

    MSEHasGuidByte3,

    MSEHasGuidByte6,

    MSEHasSplineElevation,

    MSEHasGuidByte4,

    MSEHasTransportData,

    MSEHasGuidByte0,

    MSEHasMovementFlags,

    MSEHasPitch,

    MSEHasCounter,

    MSEHasMovementFlags2,

    MSEZeroBit,

    MSEHasGuidByte1,

    MSEZeroBit,

    MSEHasTransportGuidByte1,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte7,

    MSEHasVehicleId,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte4,

    MSEMovementFlags2,

    MSEMovementFlags,

    MSEHasFallDirection,

    MSEForces,

    MSEGuidByte1,

    MSEGuidByte3,

    MSEGuidByte5,

    MSEGuidByte2,

    MSEGuidByte0,

    MSEGuidByte4,

    MSEGuidByte7,

    MSEGuidByte6,

    MSEOptionalCounter,

    MSETransportTime,

    MSETransportGuidByte4,

    MSETransportGuidByte1,

    MSETransportGuidByte5,

    MSETransportGuidByte3,

    MSETransportGuidByte6,

    MSETransportSeat,

    MSETransportOrientation,

    MSETransportPositionX,

    MSETransportGuidByte0,

    MSETransportPositionY,

    MSETransportVehicleId,

    MSETransportGuidByte7,

    MSETransportTime2,

    MSETransportPositionZ,

    MSETransportGuidByte2,

    MSEOrientation,

    MSEFallTime,

    MSEFallSinAngle,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallVerticalSpeed,

    MSEPitch,

    MSETimestamp,

    MSESplineElevation,

    MSEEnd,
};

static MovementStatusElements const MovementStartForward[] =
{

    MSEPositionZ,

    MSEPositionX,

    MSEPositionY,

    MSEHasMovementFlags2,

    MSEZeroBit,

    MSEHasCounter,

    MSEZeroBit,

    MSEHasGuidByte0,

    MSEHasOrientation,

    MSEHasFallData,

    MSEForcesCount,

    MSEHasGuidByte4,

    MSEHasGuidByte1,

    MSEHasTimestamp,

    MSEHasGuidByte7,

    MSEHasPitch,

    MSEHasTransportData,

    MSEHasGuidByte5,

    MSEHasMovementFlags,

    MSEHasGuidByte3,

    MSEHasSplineElevation,

    MSEHasGuidByte2,

    MSEHasGuidByte6,

    MSEZeroBit,

    MSEHasTransportGuidByte1,

    MSEHasVehicleId,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte6,

    MSEHasTransportTime2,

    MSEHasFallDirection,

    MSEMovementFlags2,

    MSEMovementFlags,

    MSEGuidByte1,

    MSEGuidByte6,

    MSEGuidByte7,

    MSEForces,

    MSEGuidByte5,

    MSEGuidByte0,

    MSEGuidByte3,

    MSEGuidByte2,

    MSEGuidByte4,

    MSETransportGuidByte3,

    MSETransportGuidByte1,

    MSETransportGuidByte6,

    MSETransportPositionZ,

    MSETransportGuidByte4,

    MSETransportVehicleId,

    MSETransportSeat,

    MSETransportGuidByte7,

    MSETransportOrientation,

    MSETransportTime2,

    MSETransportGuidByte5,

    MSETransportGuidByte2,

    MSETransportPositionX,

    MSETransportGuidByte0,

    MSETransportPositionY,

    MSETransportTime,

    MSEFallCosAngle,

    MSEFallSinAngle,

    MSEFallHorizontalSpeed,

    MSEFallTime,

    MSEFallVerticalSpeed,

    MSETimestamp,

    MSEPitch,

    MSESplineElevation,

    MSEOrientation,

    MSEOptionalCounter,

    MSEEnd,
};

static MovementStatusElements const MovementStartStrafeLeft[] =
{

    MSEPositionY,

    MSEPositionZ,

    MSEPositionX,

    MSEHasGuidByte0,

    MSEHasTimestamp,

    MSEHasGuidByte3,

    MSEHasMovementFlags2,

    MSEHasPitch,

    MSEZeroBit,

    MSEHasGuidByte2,

    MSEZeroBit,

    MSEHasTransportData,

    MSEHasFallData,

    MSEHasGuidByte5,

    MSEForcesCount,

    MSEZeroBit,

    MSEHasGuidByte4,

    MSEHasOrientation,

    MSEHasSplineElevation,

    MSEHasGuidByte7,

    MSEHasCounter,

    MSEHasGuidByte1,

    MSEHasGuidByte6,

    MSEHasMovementFlags,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte5,

    MSEHasVehicleId,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte4,

    MSEMovementFlags,

    MSEHasFallDirection,

    MSEMovementFlags2,

    MSEGuidByte0,

    MSEGuidByte2,

    MSEForces,

    MSEGuidByte3,

    MSEGuidByte5,

    MSEGuidByte1,

    MSEGuidByte7,

    MSEGuidByte4,

    MSEGuidByte6,

    MSETransportGuidByte2,

    MSETransportPositionZ,

    MSETransportVehicleId,

    MSETransportGuidByte6,

    MSETransportGuidByte3,

    MSETransportOrientation,

    MSETransportGuidByte5,

    MSETransportTime2,

    MSETransportGuidByte1,

    MSETransportPositionY,

    MSETransportGuidByte4,

    MSETransportTime,

    MSETransportSeat,

    MSETransportPositionX,

    MSETransportGuidByte0,

    MSETransportGuidByte7,

    MSEPitch,

    MSETimestamp,

    MSEFallTime,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallSinAngle,

    MSEFallVerticalSpeed,

    MSEOptionalCounter,

    MSESplineElevation,

    MSEOrientation,

    MSEEnd,
};

static MovementStatusElements const MovementStartStrafeRight[] =
{

    MSEPositionY,

    MSEPositionX,

    MSEPositionZ,

    MSEHasGuidByte0,

    MSEHasFallData,

    MSEForcesCount,

    MSEHasGuidByte7,

    MSEHasGuidByte6,

    MSEHasGuidByte4,

    MSEHasMovementFlags,

    MSEHasGuidByte5,

    MSEHasSplineElevation,

    MSEHasGuidByte3,

    MSEZeroBit,

    MSEHasTransportData,

    MSEHasCounter,

    MSEHasGuidByte1,

    MSEZeroBit,

    MSEHasGuidByte2,

    MSEHasPitch,

    MSEHasMovementFlags2,

    MSEHasOrientation,

    MSEZeroBit,

    MSEHasTimestamp,

    MSEHasFallDirection,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte4,

    MSEHasVehicleId,

    MSEHasTransportGuidByte7,

    MSEHasTransportTime2,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEGuidByte6,

    MSEGuidByte7,

    MSEGuidByte0,

    MSEGuidByte4,

    MSEGuidByte1,

    MSEForces,

    MSEGuidByte2,

    MSEGuidByte3,

    MSEGuidByte5,

    MSEPitch,

    MSETransportGuidByte1,

    MSETransportSeat,

    MSETransportGuidByte3,

    MSETransportTime2,

    MSETransportGuidByte7,

    MSETransportVehicleId,

    MSETransportGuidByte5,

    MSETransportGuidByte6,

    MSETransportGuidByte2,

    MSETransportGuidByte0,

    MSETransportTime,

    MSETransportOrientation,

    MSETransportPositionY,

    MSETransportPositionZ,

    MSETransportGuidByte4,

    MSETransportPositionX,

    MSETimestamp,

    MSEFallVerticalSpeed,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallSinAngle,

    MSEFallTime,

    MSEOrientation,

    MSEOptionalCounter,

    MSESplineElevation,

    MSEEnd,
};

static MovementStatusElements const MovementStartTurnLeft[] =
{

    MSEPositionZ,

    MSEPositionX,

    MSEPositionY,

    MSEHasOrientation,

    MSEHasGuidByte4,

    MSEHasGuidByte5,

    MSEZeroBit,

    MSEHasTimestamp,

    MSEZeroBit,

    MSEZeroBit,

    MSEHasCounter,

    MSEHasGuidByte3,

    MSEHasGuidByte1,

    MSEHasMovementFlags2,

    MSEHasMovementFlags,

    MSEHasGuidByte0,

    MSEHasGuidByte2,

    MSEForcesCount,

    MSEHasTransportData,

    MSEHasGuidByte7,

    MSEHasPitch,

    MSEHasSplineElevation,

    MSEHasFallData,

    MSEHasGuidByte6,

    MSEHasVehicleId,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte7,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte1,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEHasFallDirection,

    MSEGuidByte7,

    MSEGuidByte3,

    MSEGuidByte6,

    MSEGuidByte4,

    MSEGuidByte1,

    MSEForces,

    MSEGuidByte5,

    MSEGuidByte0,

    MSEGuidByte2,

    MSEFallTime,

    MSEFallHorizontalSpeed,

    MSEFallCosAngle,

    MSEFallSinAngle,

    MSEFallVerticalSpeed,

    MSEPitch,

    MSETransportPositionY,

    MSETransportGuidByte3,

    MSETransportPositionX,

    MSETransportOrientation,

    MSETransportGuidByte5,

    MSETransportTime2,

    MSETransportPositionZ,

    MSETransportGuidByte2,

    MSETransportGuidByte1,

    MSETransportGuidByte7,

    MSETransportGuidByte4,

    MSETransportGuidByte0,

    MSETransportVehicleId,

    MSETransportSeat,

    MSETransportGuidByte6,

    MSETransportTime,

    MSEOrientation,

    MSESplineElevation,

    MSEOptionalCounter,

    MSETimestamp,

    MSEEnd,
};

static MovementStatusElements const MovementStartTurnRight[] =
{

    MSEPositionX,

    MSEPositionZ,

    MSEPositionY,

    MSEZeroBit,

    MSEZeroBit,

    MSEHasGuidByte1,

    MSEHasGuidByte0,

    MSEHasMovementFlags,

    MSEHasFallData,

    MSEHasPitch,

    MSEHasCounter,

    MSEForcesCount,

    MSEHasSplineElevation,

    MSEHasMovementFlags2,

    MSEHasOrientation,

    MSEHasGuidByte2,

    MSEHasTimestamp,

    MSEHasGuidByte4,

    MSEHasGuidByte6,

    MSEHasGuidByte5,

    MSEHasGuidByte3,

    MSEZeroBit,

    MSEHasTransportData,

    MSEHasGuidByte7,

    MSEHasTransportGuidByte2,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte4,

    MSEHasVehicleId,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte1,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEHasFallDirection,

    MSEGuidByte5,

    MSEGuidByte1,

    MSEGuidByte3,

    MSEGuidByte0,

    MSEGuidByte4,

    MSEGuidByte2,

    MSEGuidByte6,

    MSEForces,

    MSEGuidByte7,

    MSEFallSinAngle,

    MSEFallHorizontalSpeed,

    MSEFallCosAngle,

    MSEFallVerticalSpeed,

    MSEFallTime,

    MSEPitch,

    MSETransportVehicleId,

    MSETransportGuidByte3,

    MSETransportTime2,

    MSETransportGuidByte7,

    MSETransportGuidByte1,

    MSETransportPositionX,

    MSETransportSeat,

    MSETransportGuidByte5,

    MSETransportGuidByte4,

    MSETransportGuidByte2,

    MSETransportGuidByte0,

    MSETransportPositionZ,

    MSETransportTime,

    MSETransportPositionY,

    MSETransportGuidByte6,

    MSETransportOrientation,

    MSEOrientation,

    MSETimestamp,

    MSESplineElevation,

    MSEOptionalCounter,

    MSEEnd,
};

static MovementStatusElements const MovementStop[] =
{

    MSEPositionX,

    MSEPositionY,

    MSEPositionZ,

    MSEHasGuidByte5,

    MSEHasGuidByte2,

    MSEHasFallData,

    MSEHasGuidByte0,

    MSEZeroBit,

    MSEZeroBit,

    MSEHasCounter,

    MSEHasGuidByte1,

    MSEForcesCount,

    MSEHasPitch,

    MSEHasGuidByte3,

    MSEHasGuidByte4,

    MSEHasTransportData,

    MSEZeroBit,

    MSEHasGuidByte6,

    MSEHasMovementFlags,

    MSEHasTimestamp,

    MSEHasMovementFlags2,

    MSEHasOrientation,

    MSEHasSplineElevation,

    MSEHasGuidByte7,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte3,

    MSEHasVehicleId,

    MSEHasTransportGuidByte6,

    MSEHasFallDirection,

    MSEMovementFlags2,

    MSEMovementFlags,

    MSEGuidByte0,

    MSEGuidByte3,

    MSEForces,

    MSEGuidByte6,

    MSEGuidByte1,

    MSEGuidByte4,

    MSEGuidByte2,

    MSEGuidByte5,

    MSEGuidByte7,

    MSEOrientation,

    MSEFallVerticalSpeed,

    MSEFallHorizontalSpeed,

    MSEFallSinAngle,

    MSEFallCosAngle,

    MSEFallTime,

    MSESplineElevation,

    MSETransportPositionX,

    MSETransportTime,

    MSETransportGuidByte3,

    MSETransportOrientation,

    MSETransportPositionY,

    MSETransportGuidByte2,

    MSETransportGuidByte6,

    MSETransportGuidByte7,

    MSETransportGuidByte1,

    MSETransportGuidByte4,

    MSETransportVehicleId,

    MSETransportGuidByte0,

    MSETransportSeat,

    MSETransportPositionZ,

    MSETransportGuidByte5,

    MSETransportTime2,

    MSEOptionalCounter,

    MSEPitch,

    MSETimestamp,

    MSEEnd,
};

static MovementStatusElements const MovementStopStrafe[] =
{

    MSEPositionZ,

    MSEPositionX,

    MSEPositionY,

    MSEHasFallData,

    MSEHasOrientation,

    MSEHasSplineElevation,

    MSEHasTimestamp,

    MSEHasMovementFlags,

    MSEHasCounter,

    MSEHasGuidByte6,

    MSEHasTransportData,

    MSEZeroBit,

    MSEHasMovementFlags2,

    MSEHasGuidByte4,

    MSEHasPitch,

    MSEHasGuidByte5,

    MSEHasGuidByte3,

    MSEHasGuidByte2,

    MSEForcesCount,

    MSEZeroBit,

    MSEHasGuidByte7,

    MSEHasGuidByte0,

    MSEZeroBit,

    MSEHasGuidByte1,

    MSEHasTransportGuidByte7,

    MSEHasVehicleId,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte6,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte0,

    MSEMovementFlags2,

    MSEHasFallDirection,

    MSEMovementFlags,

    MSEGuidByte5,

    MSEGuidByte3,

    MSEForces,

    MSEGuidByte2,

    MSEGuidByte0,

    MSEGuidByte1,

    MSEGuidByte6,

    MSEGuidByte4,

    MSEGuidByte7,

    MSETransportGuidByte0,

    MSETransportVehicleId,

    MSETransportGuidByte1,

    MSETransportGuidByte6,

    MSETransportTime,

    MSETransportPositionY,

    MSETransportPositionZ,

    MSETransportGuidByte4,

    MSETransportTime2,

    MSETransportGuidByte3,

    MSETransportSeat,

    MSETransportPositionX,

    MSETransportGuidByte2,

    MSETransportGuidByte7,

    MSETransportGuidByte5,

    MSETransportOrientation,

    MSEOrientation,

    MSESplineElevation,

    MSETimestamp,

    MSEFallCosAngle,

    MSEFallSinAngle,

    MSEFallHorizontalSpeed,

    MSEFallTime,

    MSEFallVerticalSpeed,

    MSEPitch,

    MSEOptionalCounter,

    MSEEnd,
};

static MovementStatusElements const MovementStopTurn[] =
{

    MSEPositionX,

    MSEPositionZ,

    MSEPositionY,

    MSEHasTransportData,

    MSEForcesCount,

    MSEZeroBit,

    MSEHasGuidByte4,

    MSEHasGuidByte5,

    MSEHasCounter,

    MSEHasGuidByte3,

    MSEZeroBit,

    MSEHasFallData,

    MSEHasGuidByte0,

    MSEHasGuidByte1,

    MSEHasPitch,

    MSEHasGuidByte6,

    MSEHasMovementFlags,

    MSEHasGuidByte2,

    MSEZeroBit,

    MSEHasMovementFlags2,

    MSEHasSplineElevation,

    MSEHasOrientation,

    MSEHasGuidByte7,

    MSEHasTimestamp,

    MSEMovementFlags2,

    MSEHasTransportGuidByte1,

    MSEHasVehicleId,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte4,

    MSEMovementFlags,

    MSEHasFallDirection,

    MSEGuidByte2,

    MSEGuidByte3,

    MSEGuidByte6,

    MSEForces,

    MSEGuidByte0,

    MSEGuidByte5,

    MSEGuidByte4,

    MSEGuidByte7,

    MSEGuidByte1,

    MSETransportTime,

    MSETransportVehicleId,

    MSETransportSeat,

    MSETransportPositionY,

    MSETransportPositionX,

    MSETransportTime2,

    MSETransportGuidByte4,

    MSETransportGuidByte3,

    MSETransportOrientation,

    MSETransportGuidByte0,

    MSETransportPositionZ,

    MSETransportGuidByte6,

    MSETransportGuidByte7,

    MSETransportGuidByte5,

    MSETransportGuidByte1,

    MSETransportGuidByte2,

    MSEOrientation,

    MSETimestamp,

    MSEFallSinAngle,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallVerticalSpeed,

    MSEFallTime,

    MSEOptionalCounter,

    MSESplineElevation,

    MSEPitch,

    MSEEnd,
};

static MovementStatusElements const MovementSetRunMode[] =
{

    MSEPositionZ,

    MSEPositionY,

    MSEPositionX,

    MSEHasPitch,

    MSEHasSplineElevation,

    MSEForcesCount,

    MSEHasOrientation,

    MSEHasGuidByte1,

    MSEHasGuidByte4,

    MSEHasGuidByte0,

    MSEHasGuidByte3,

    MSEHasCounter,

    MSEHasGuidByte6,

    MSEHasTimestamp,

    MSEHasGuidByte2,

    MSEHasGuidByte5,

    MSEHasMovementFlags2,

    MSEHasFallData,

    MSEHasGuidByte7,

    MSEZeroBit,

    MSEZeroBit,

    MSEZeroBit,

    MSEHasMovementFlags,

    MSEHasTransportData,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte5,

    MSEHasVehicleId,

    MSEHasTransportGuidByte3,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte0,

    MSEHasFallDirection,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEGuidByte5,

    MSEGuidByte6,

    MSEGuidByte3,

    MSEGuidByte7,

    MSEGuidByte1,

    MSEGuidByte0,

    MSEForces,

    MSEGuidByte4,

    MSEGuidByte2,

    MSETransportPositionZ,

    MSETransportGuidByte3,

    MSETransportVehicleId,

    MSETransportGuidByte0,

    MSETransportGuidByte1,

    MSETransportGuidByte7,

    MSETransportOrientation,

    MSETransportSeat,

    MSETransportPositionX,

    MSETransportTime,

    MSETransportTime2,

    MSETransportGuidByte4,

    MSETransportGuidByte6,

    MSETransportGuidByte2,

    MSETransportGuidByte5,

    MSETransportPositionY,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallSinAngle,

    MSEFallTime,

    MSEFallVerticalSpeed,

    MSEPitch,

    MSEOrientation,

    MSESplineElevation,

    MSETimestamp,

    MSEOptionalCounter,

    MSEEnd,
};

static MovementStatusElements const MovementSetWalkMode[] =
{

    MSEPositionY,

    MSEPositionX,

    MSEPositionZ,

    MSEHasSplineElevation,

    MSEZeroBit,

    MSEForcesCount,

    MSEHasMovementFlags,

    MSEHasGuidByte2,

    MSEHasGuidByte4,

    MSEHasGuidByte5,

    MSEHasGuidByte1,

    MSEHasGuidByte0,

    MSEHasFallData,

    MSEHasMovementFlags2,

    MSEHasPitch,

    MSEHasTimestamp,

    MSEZeroBit,

    MSEHasGuidByte7,

    MSEHasGuidByte3,

    MSEHasCounter,

    MSEHasGuidByte6,

    MSEZeroBit,

    MSEHasOrientation,

    MSEHasTransportData,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte0,

    MSEHasVehicleId,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte3,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte2,

    MSEMovementFlags2,

    MSEMovementFlags,

    MSEHasFallDirection,

    MSEGuidByte4,

    MSEGuidByte3,

    MSEGuidByte7,

    MSEGuidByte2,

    MSEGuidByte1,

    MSEGuidByte0,

    MSEGuidByte6,

    MSEForces,

    MSEGuidByte5,

    MSETransportPositionX,

    MSETransportGuidByte6,

    MSETransportPositionZ,

    MSETransportGuidByte0,

    MSETransportGuidByte1,

    MSETransportGuidByte4,

    MSETransportSeat,

    MSETransportGuidByte7,

    MSETransportPositionY,

    MSETransportGuidByte5,

    MSETransportTime,

    MSETransportGuidByte2,

    MSETransportOrientation,

    MSETransportGuidByte3,

    MSETransportTime2,

    MSETransportVehicleId,

    MSEFallHorizontalSpeed,

    MSEFallCosAngle,

    MSEFallSinAngle,

    MSEFallVerticalSpeed,

    MSEFallTime,

    MSEOrientation,

    MSEPitch,

    MSETimestamp,

    MSESplineElevation,

    MSEOptionalCounter,

    MSEEnd,
};
static MovementStatusElements const MovementSetFacing[] =
{
    MSEPositionY,
    MSEPositionX,
    MSEPositionZ,
    MSEHasGuidByte5,
    MSEHasMovementFlags2,
    MSEHasGuidByte3,
    MSEHasGuidByte2,
    MSEForcesCount,
    MSEZeroBit,
    MSEHasPitch,
    MSEHasGuidByte0,
    MSEHasOrientation,
    MSEHasTimestamp,
    MSEZeroBit,
    MSEHasCounter,
    MSEHasGuidByte4,
    MSEZeroBit,
    MSEHasGuidByte1,
    MSEHasGuidByte6,
    MSEHasFallData,
    MSEHasMovementFlags,
    MSEHasSplineElevation,
    MSEHasTransportData,
    MSEHasGuidByte7,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte0,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte6,
    MSEHasVehicleId,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte4,
    MSEHasFallDirection,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEForces,
    MSEGuidByte0,
    MSEGuidByte6,
    MSEGuidByte3,
    MSEGuidByte1,
    MSEGuidByte2,
    MSEGuidByte7,
    MSEGuidByte4,
    MSEGuidByte5,
    MSETransportGuidByte0,
    MSETransportGuidByte2,
    MSETransportOrientation,
    MSETransportGuidByte7,
    MSETransportVehicleId,
    MSETransportGuidByte5,
    MSETransportTime,
    MSETransportPositionX,
    MSETransportTime2,
    MSETransportPositionZ,
    MSETransportSeat,
    MSETransportPositionY,
    MSETransportGuidByte4,
    MSETransportGuidByte3,
    MSETransportGuidByte6,
    MSETransportGuidByte1,
    MSEFallTime,
    MSEFallVerticalSpeed,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallCosAngle,
    MSEOptionalCounter,
    MSETimestamp,
    MSESplineElevation,
    MSEOrientation,
    MSEPitch,
    MSEEnd,
};

static MovementStatusElements const MovementFallLand[] =
{

    MSEPositionY,

    MSEPositionZ,

    MSEPositionX,

    MSEHasFallData,

    MSEZeroBit,

    MSEZeroBit,

    MSEHasTimestamp,

    MSEHasGuidByte7,

    MSEZeroBit,

    MSEHasSplineElevation,

    MSEHasGuidByte5,

    MSEHasPitch,

    MSEHasMovementFlags2,

    MSEHasGuidByte2,

    MSEHasGuidByte3,

    MSEHasGuidByte0,

    MSEHasOrientation,

    MSEForcesCount,

    MSEHasMovementFlags,

    MSEHasCounter,

    MSEHasGuidByte1,

    MSEHasTransportData,

    MSEHasGuidByte6,

    MSEHasGuidByte4,

    MSEHasTransportGuidByte0,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte6,

    MSEHasVehicleId,

    MSEMovementFlags2,

    MSEHasFallDirection,

    MSEMovementFlags,

    MSEGuidByte4,

    MSEGuidByte3,

    MSEGuidByte7,

    MSEGuidByte0,

    MSEGuidByte2,

    MSEGuidByte5,

    MSEGuidByte1,

    MSEGuidByte6,

    MSEForces,

    MSEFallSinAngle,

    MSEFallHorizontalSpeed,

    MSEFallCosAngle,

    MSEFallTime,

    MSEFallVerticalSpeed,

    MSETransportGuidByte4,

    MSETransportPositionY,

    MSETransportOrientation,

    MSETransportPositionZ,

    MSETransportSeat,

    MSETransportGuidByte3,

    MSETransportGuidByte6,

    MSETransportTime2,

    MSETransportGuidByte2,

    MSETransportGuidByte1,

    MSETransportGuidByte5,

    MSETransportVehicleId,

    MSETransportTime,

    MSETransportPositionX,

    MSETransportGuidByte7,

    MSETransportGuidByte0,

    MSEOptionalCounter,

    MSETimestamp,

    MSESplineElevation,

    MSEPitch,

    MSEOrientation,

    MSEEnd,
};

static MovementStatusElements const MovementJump[] =
{

    MSEPositionY,

    MSEPositionX,

    MSEPositionZ,

    MSEHasGuidByte1,

    MSEHasGuidByte7,

    MSEHasMovementFlags2,

    MSEHasGuidByte5,

    MSEHasSplineElevation,

    MSEHasOrientation,

    MSEHasGuidByte6,

    MSEHasGuidByte4,

    MSEZeroBit,

    MSEHasTransportData,

    MSEZeroBit,

    MSEForcesCount,

    MSEHasPitch,

    MSEHasMovementFlags,

    MSEHasTimestamp,

    MSEHasCounter,

    MSEHasGuidByte3,

    MSEZeroBit,

    MSEHasFallData,

    MSEHasGuidByte2,

    MSEHasGuidByte0,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte4,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte7,

    MSEHasVehicleId,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEHasFallDirection,

    MSEGuidByte7,

    MSEGuidByte1,

    MSEGuidByte0,

    MSEForces,

    MSEGuidByte2,

    MSEGuidByte6,

    MSEGuidByte3,

    MSEGuidByte4,

    MSEGuidByte5,

    MSEFallVerticalSpeed,

    MSEFallSinAngle,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallTime,

    MSETransportGuidByte5,

    MSETransportGuidByte7,

    MSETransportSeat,

    MSETransportGuidByte4,

    MSETransportGuidByte0,

    MSETransportPositionZ,

    MSETransportGuidByte6,

    MSETransportGuidByte2,

    MSETransportPositionY,

    MSETransportTime,

    MSETransportPositionX,

    MSETransportTime2,

    MSETransportGuidByte1,

    MSETransportGuidByte3,

    MSETransportVehicleId,

    MSETransportOrientation,

    MSESplineElevation,

    MSEOrientation,

    MSEPitch,

    MSEOptionalCounter,

    MSETimestamp,

    MSEEnd,
};

static MovementStatusElements const MovementSetPitch[] =
{

    MSEPositionZ,

    MSEPositionX,

    MSEPositionY,

    MSEHasMovementFlags,

    MSEZeroBit,

    MSEHasGuidByte4,

    MSEHasTransportData,

    MSEHasGuidByte7,

    MSEHasTimestamp,

    MSEHasGuidByte0,

    MSEHasMovementFlags2,

    MSEHasGuidByte5,

    MSEHasCounter,

    MSEZeroBit,

    MSEHasGuidByte2,

    MSEForcesCount,

    MSEHasOrientation,

    MSEHasPitch,

    MSEHasSplineElevation,

    MSEHasGuidByte6,

    MSEHasGuidByte1,

    MSEZeroBit,

    MSEHasFallData,

    MSEHasGuidByte3,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte6,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte2,

    MSEHasVehicleId,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte0,

    MSEHasFallDirection,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEGuidByte2,

    MSEGuidByte4,

    MSEGuidByte5,

    MSEGuidByte6,

    MSEGuidByte0,

    MSEForces,

    MSEGuidByte3,

    MSEGuidByte7,

    MSEGuidByte1,

    MSESplineElevation,

    MSETransportPositionZ,

    MSETransportGuidByte3,

    MSETransportGuidByte2,

    MSETransportVehicleId,

    MSETransportGuidByte5,

    MSETransportTime2,

    MSETransportGuidByte6,

    MSETransportTime,

    MSETransportGuidByte4,

    MSETransportSeat,

    MSETransportOrientation,

    MSETransportPositionY,

    MSETransportGuidByte1,

    MSETransportGuidByte7,

    MSETransportPositionX,

    MSETransportGuidByte0,

    MSEFallHorizontalSpeed,

    MSEFallSinAngle,

    MSEFallCosAngle,

    MSEFallTime,

    MSEFallVerticalSpeed,

    MSEOptionalCounter,

    MSETimestamp,

    MSEOrientation,

    MSEPitch,

    MSEEnd,
};
static MovementStatusElements const MovementStartAscend[] =
{
    MSEPositionY,
    MSEPositionX,
    MSEPositionZ,
    MSEHasOrientation,
    MSEHasGuidByte3,
    MSEHasTransportData,
    MSEHasMovementFlags,
    MSEZeroBit,
    MSEHasGuidByte0,
    MSEHasGuidByte4,
    MSEHasTimestamp,
    MSEHasGuidByte7,
    MSEZeroBit,
    MSEHasPitch,
    MSEHasGuidByte5,
    MSEHasMovementFlags2,
    MSEZeroBit,
    MSEHasGuidByte6,
    MSEHasGuidByte2,
    MSEHasCounter,
    MSEForcesCount,
    MSEHasGuidByte1,
    MSEHasSplineElevation,
    MSEHasFallData,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte5,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte1,
    MSEHasVehicleId,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte7,
    MSEHasFallDirection,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEGuidByte2,
    MSEGuidByte5,
    MSEForces,
    MSEGuidByte1,
    MSEGuidByte0,
    MSEGuidByte4,
    MSEGuidByte7,
    MSEGuidByte6,
    MSEGuidByte3,
    MSEOrientation,
    MSETimestamp,
    MSETransportGuidByte3,
    MSETransportTime,
    MSETransportPositionY,
    MSETransportOrientation,
    MSETransportGuidByte6,
    MSETransportVehicleId,
    MSETransportPositionX,
    MSETransportGuidByte2,
    MSETransportTime2,
    MSETransportGuidByte1,
    MSETransportGuidByte7,
    MSETransportPositionZ,
    MSETransportSeat,
    MSETransportGuidByte0,
    MSETransportGuidByte4,
    MSETransportGuidByte5,
    MSESplineElevation,
    MSEFallVerticalSpeed,
    MSEFallSinAngle,
    MSEFallCosAngle,
    MSEFallHorizontalSpeed,
    MSEFallTime,
    MSEPitch,
    MSEOptionalCounter,
    MSEEnd,
};
static MovementStatusElements const MovementStartDescend[] =
{
    MSEPositionX,
    MSEPositionY,
    MSEPositionZ,
    MSEHasFallData,
    MSEHasMovementFlags,
    MSEHasGuidByte7,
    MSEHasGuidByte0,
    MSEHasGuidByte4,
    MSEHasMovementFlags2,
    MSEHasPitch,
    MSEHasGuidByte6,
    MSEHasGuidByte2,
    MSEZeroBit,
    MSEHasCounter,
    MSEForcesCount,
    MSEHasTransportData,
    MSEHasOrientation,
    MSEHasGuidByte1,
    MSEZeroBit,
    MSEZeroBit,
    MSEHasGuidByte3,
    MSEHasGuidByte5,
    MSEHasSplineElevation,
    MSEHasTimestamp,
    MSEHasTransportGuidByte0,
    MSEHasVehicleId,
    MSEHasTransportGuidByte7,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte2,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEHasFallDirection,
    MSEGuidByte4,
    MSEGuidByte7,
    MSEGuidByte1,
    MSEGuidByte3,
    MSEForces,
    MSEGuidByte2,
    MSEGuidByte6,
    MSEGuidByte0,
    MSEGuidByte5,
    MSETransportPositionX,
    MSETransportGuidByte0,
    MSETransportGuidByte3,
    MSETransportGuidByte7,
    MSETransportSeat,
    MSETransportGuidByte5,
    MSETransportGuidByte1,
    MSETransportPositionY,
    MSETransportVehicleId,
    MSETransportTime,
    MSETransportGuidByte4,
    MSETransportTime2,
    MSETransportOrientation,
    MSETransportPositionZ,
    MSETransportGuidByte2,
    MSETransportGuidByte6,
    MSEFallTime,
    MSEFallCosAngle,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallVerticalSpeed,
    MSEPitch,
    MSEOptionalCounter,
    MSESplineElevation,
    MSEOrientation,
    MSETimestamp,
    MSEEnd,
};
static MovementStatusElements const MovementStartSwim[] =
{
    MSEPositionX,
    MSEPositionY,
    MSEPositionZ,
    MSEHasSplineElevation,
    MSEHasTransportData,
    MSEHasGuidByte2,
    MSEZeroBit,
    MSEHasMovementFlags2,
    MSEForcesCount,
    MSEHasPitch,
    MSEHasTimestamp,
    MSEHasGuidByte3,
    MSEZeroBit,
    MSEHasGuidByte6,
    MSEHasGuidByte1,
    MSEHasFallData,
    MSEHasCounter,
    MSEZeroBit,
    MSEHasGuidByte7,
    MSEHasGuidByte0,
    MSEHasOrientation,
    MSEHasGuidByte5,
    MSEHasMovementFlags,
    MSEHasGuidByte4,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte4,
    MSEHasVehicleId,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte3,
    MSEMovementFlags,
    MSEHasFallDirection,
    MSEMovementFlags2,
    MSEGuidByte5,
    MSEGuidByte0,
    MSEForces,
    MSEGuidByte7,
    MSEGuidByte3,
    MSEGuidByte4,
    MSEGuidByte1,
    MSEGuidByte6,
    MSEGuidByte2,
    MSETransportGuidByte3,
    MSETransportPositionX,
    MSETransportGuidByte7,
    MSETransportGuidByte0,
    MSETransportPositionY,
    MSETransportSeat,
    MSETransportTime2,
    MSETransportGuidByte6,
    MSETransportGuidByte4,
    MSETransportGuidByte1,
    MSETransportGuidByte2,
    MSETransportPositionZ,
    MSETransportTime,
    MSETransportOrientation,
    MSETransportVehicleId,
    MSETransportGuidByte5,
    MSEFallCosAngle,
    MSEFallSinAngle,
    MSEFallHorizontalSpeed,
    MSEFallTime,
    MSEFallVerticalSpeed,
    MSETimestamp,
    MSESplineElevation,
    MSEOrientation,
    MSEOptionalCounter,
    MSEPitch,
    MSEEnd,
};
static MovementStatusElements const MovementStopSwim[] =
{
    MSEPositionX,
    MSEPositionZ,
    MSEPositionY,
    MSEHasPitch,
    MSEHasGuidByte6,
    MSEHasOrientation,
    MSEZeroBit,
    MSEZeroBit,
    MSEZeroBit,
    MSEHasCounter,
    MSEHasGuidByte4,
    MSEHasMovementFlags2,
    MSEHasTransportData,
    MSEHasFallData,
    MSEHasGuidByte1,
    MSEForcesCount,
    MSEHasMovementFlags,
    MSEHasGuidByte7,
    MSEHasGuidByte0,
    MSEHasTimestamp,
    MSEHasGuidByte3,
    MSEHasGuidByte5,
    MSEHasSplineElevation,
    MSEHasGuidByte2,
    MSEHasTransportGuidByte1,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte3,
    MSEHasVehicleId,
    MSEHasTransportGuidByte0,
    MSEHasFallDirection,
    MSEMovementFlags,
    MSEMovementFlags2,
    MSEGuidByte7,
    MSEGuidByte6,
    MSEGuidByte1,
    MSEGuidByte5,
    MSEGuidByte4,
    MSEGuidByte3,
    MSEForces,
    MSEGuidByte0,
    MSEGuidByte2,
    MSETransportTime,
    MSETransportGuidByte1,
    MSETransportGuidByte2,
    MSETransportPositionZ,
    MSETransportPositionX,
    MSETransportPositionY,
    MSETransportGuidByte0,
    MSETransportGuidByte6,
    MSETransportGuidByte3,
    MSETransportSeat,
    MSETransportOrientation,
    MSETransportGuidByte7,
    MSETransportGuidByte4,
    MSETransportGuidByte5,
    MSETransportTime2,
    MSETransportVehicleId,
    MSEFallCosAngle,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallVerticalSpeed,
    MSEFallTime,
    MSESplineElevation,
    MSEPitch,
    MSETimestamp,
    MSEOptionalCounter,
    MSEOrientation,
    MSEEnd,
};
static MovementStatusElements const MovementStopAscend[] =
{
    MSEPositionZ,
    MSEPositionX,
    MSEPositionY,
    MSEHasOrientation,
    MSEHasGuidByte0,
    MSEHasGuidByte3,
    MSEHasGuidByte7,
    MSEHasGuidByte2,
    MSEHasGuidByte6,
    MSEHasMovementFlags2,
    MSEHasTimestamp,
    MSEHasCounter,
    MSEHasTransportData,
    MSEHasPitch,
    MSEZeroBit,
    MSEZeroBit,
    MSEHasGuidByte4,
    MSEZeroBit,
    MSEHasGuidByte5,
    MSEForcesCount,
    MSEHasFallData,
    MSEHasMovementFlags,
    MSEHasGuidByte1,
    MSEHasSplineElevation,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte1,
    MSEHasVehicleId,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte7,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEHasFallDirection,
    MSEGuidByte0,
    MSEForces,
    MSEGuidByte4,
    MSEGuidByte5,
    MSEGuidByte1,
    MSEGuidByte7,
    MSEGuidByte6,
    MSEGuidByte3,
    MSEGuidByte2,
    MSETransportGuidByte5,
    MSETransportPositionY,
    MSETransportGuidByte4,
    MSETransportGuidByte7,
    MSETransportGuidByte1,
    MSETransportGuidByte3,
    MSETransportTime2,
    MSETransportPositionX,
    MSETransportOrientation,
    MSETransportGuidByte0,
    MSETransportGuidByte2,
    MSETransportPositionZ,
    MSETransportVehicleId,
    MSETransportTime,
    MSETransportSeat,
    MSETransportGuidByte6,
    MSEFallCosAngle,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallTime,
    MSEFallVerticalSpeed,
    MSETimestamp,
    MSESplineElevation,
    MSEPitch,
    MSEOptionalCounter,
    MSEOrientation,
    MSEEnd,
};
static MovementStatusElements const MovementStopPitch[] =
{
    MSEPositionY,
    MSEPositionX,
    MSEPositionZ,
    MSEHasFallData,
    MSEHasSplineElevation,
    MSEHasGuidByte0,
    MSEHasTimestamp,
    MSEZeroBit,
    MSEHasGuidByte2,
    MSEForcesCount,
    MSEHasOrientation,
    MSEHasMovementFlags,
    MSEHasPitch,
    MSEHasGuidByte3,
    MSEHasGuidByte7,
    MSEHasGuidByte5,
    MSEHasMovementFlags2,
    MSEZeroBit,
    MSEHasCounter,
    MSEHasTransportData,
    MSEHasGuidByte6,
    MSEHasGuidByte4,
    MSEZeroBit,
    MSEHasGuidByte1,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte5,
    MSEHasVehicleId,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte1,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte2,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEHasFallDirection,
    MSEGuidByte0,
    MSEGuidByte6,
    MSEGuidByte7,
    MSEGuidByte1,
    MSEForces,
    MSEGuidByte5,
    MSEGuidByte3,
    MSEGuidByte4,
    MSEGuidByte2,
    MSEFallSinAngle,
    MSEFallHorizontalSpeed,
    MSEFallCosAngle,
    MSEFallTime,
    MSEFallVerticalSpeed,
    MSETransportGuidByte1,
    MSETransportPositionX,
    MSETransportPositionZ,
    MSETransportPositionY,
    MSETransportGuidByte5,
    MSETransportGuidByte0,
    MSETransportTime2,
    MSETransportGuidByte6,
    MSETransportGuidByte2,
    MSETransportGuidByte3,
    MSETransportSeat,
    MSETransportVehicleId,
    MSETransportTime,
    MSETransportGuidByte4,
    MSETransportGuidByte7,
    MSETransportOrientation,
    MSEPitch,
    MSETimestamp,
    MSESplineElevation,
    MSEOrientation,
    MSEOptionalCounter,
    MSEEnd,
};
static MovementStatusElements const MovementStartPitchDown[] =
{
    MSEPositionZ,
    MSEPositionY,
    MSEPositionX,
    MSEHasGuidByte2,
    MSEHasGuidByte7,
    MSEHasGuidByte3,
    MSEHasOrientation,
    MSEHasGuidByte5,
    MSEZeroBit,
    MSEHasCounter,
    MSEZeroBit,
    MSEHasMovementFlags,
    MSEHasGuidByte4,
    MSEHasGuidByte1,
    MSEHasSplineElevation,
    MSEHasFallData,
    MSEZeroBit,
    MSEHasTransportData,
    MSEHasTimestamp,
    MSEHasMovementFlags2,
    MSEForcesCount,
    MSEHasGuidByte6,
    MSEHasGuidByte0,
    MSEHasPitch,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte4,
    MSEHasVehicleId,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte0,
    MSEMovementFlags,
    MSEMovementFlags2,
    MSEHasFallDirection,
    MSEGuidByte6,
    MSEGuidByte3,
    MSEGuidByte5,
    MSEGuidByte0,
    MSEGuidByte4,
    MSEForces,
    MSEGuidByte7,
    MSEGuidByte2,
    MSEGuidByte1,
    MSEFallHorizontalSpeed,
    MSEFallCosAngle,
    MSEFallSinAngle,
    MSEFallTime,
    MSEFallVerticalSpeed,
    MSETransportOrientation,
    MSETransportGuidByte4,
    MSETransportGuidByte7,
    MSETransportPositionZ,
    MSETransportGuidByte0,
    MSETransportTime,
    MSETransportGuidByte6,
    MSETransportGuidByte1,
    MSETransportSeat,
    MSETransportGuidByte2,
    MSETransportGuidByte5,
    MSETransportPositionX,
    MSETransportVehicleId,
    MSETransportPositionY,
    MSETransportGuidByte3,
    MSETransportTime2,
    MSEOrientation,
    MSETimestamp,
    MSESplineElevation,
    MSEOptionalCounter,
    MSEPitch,
    MSEEnd,
};
static MovementStatusElements const MovementStartPitchUp[] =
{
    MSEPositionY,
    MSEPositionZ,
    MSEPositionX,
    MSEHasGuidByte0,
    MSEHasPitch,
    MSEHasGuidByte3,
    MSEZeroBit,
    MSEHasOrientation,
    MSEHasGuidByte5,
    MSEZeroBit,
    MSEHasGuidByte2,
    MSEHasGuidByte7,
    MSEHasGuidByte1,
    MSEHasFallData,
    MSEHasMovementFlags2,
    MSEHasTransportData,
    MSEHasCounter,
    MSEHasMovementFlags,
    MSEHasGuidByte6,
    MSEForcesCount,
    MSEHasTimestamp,
    MSEHasGuidByte4,
    MSEZeroBit,
    MSEHasSplineElevation,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte1,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte0,
    MSEHasVehicleId,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte3,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEHasFallDirection,
    MSEGuidByte6,
    MSEForces,
    MSEGuidByte0,
    MSEGuidByte5,
    MSEGuidByte7,
    MSEGuidByte1,
    MSEGuidByte3,
    MSEGuidByte4,
    MSEGuidByte2,
    MSETransportSeat,
    MSETransportGuidByte3,
    MSETransportVehicleId,
    MSETransportGuidByte2,
    MSETransportGuidByte7,
    MSETransportGuidByte0,
    MSETransportPositionY,
    MSETransportGuidByte4,
    MSETransportGuidByte5,
    MSETransportTime2,
    MSETransportGuidByte1,
    MSETransportPositionX,
    MSETransportOrientation,
    MSETransportGuidByte6,
    MSETransportTime,
    MSETransportPositionZ,
    MSEPitch,
    MSEOrientation,
    MSESplineElevation,
    MSEFallVerticalSpeed,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallCosAngle,
    MSEFallTime,
    MSEOptionalCounter,
    MSETimestamp,
    MSEEnd,
};
static MovementStatusElements const MoveChngTransport[]=
{
    MSEPositionX,
    MSEPositionY,
    MSEPositionZ,
    MSEHasGuidByte1,
    MSEHasPitch,
    MSEHasSplineElevation,
    MSEHasGuidByte4,
    MSEHasCounter,
    MSEHasGuidByte5,
    MSEForcesCount,
    MSEZeroBit,
    MSEHasGuidByte6,
    MSEZeroBit,
    MSEHasGuidByte7,
    MSEHasGuidByte0,
    MSEHasTransportData,
    MSEHasMovementFlags,
    MSEZeroBit,
    MSEHasMovementFlags2,
    MSEHasGuidByte2,
    MSEHasGuidByte3,
    MSEHasTimestamp,
    MSEHasFallData,
    MSEHasOrientation,
    MSEHasVehicleId,
    MSEHasTransportGuidByte5,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte4,
    MSEHasTransportTime2,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte7,
    MSEHasTransportGuidByte0,
    MSEMovementFlags,
    MSEHasFallDirection,
    MSEMovementFlags2,
    MSEGuidByte5,
    MSEGuidByte2,
    MSEForces,
    MSEGuidByte6,
    MSEGuidByte3,
    MSEGuidByte0,
    MSEGuidByte4,
    MSEGuidByte7,
    MSEGuidByte1,
    MSEFallVerticalSpeed,
    MSEFallSinAngle,
    MSEFallHorizontalSpeed,
    MSEFallCosAngle,
    MSEFallTime,
    MSETransportGuidByte6,
    MSETransportPositionZ,
    MSETransportGuidByte5,
    MSETransportGuidByte4,
    MSETransportVehicleId,
    MSETransportGuidByte3,
    MSETransportTime,
    MSETransportGuidByte2,
    MSETransportTime2,
    MSETransportGuidByte0,
    MSETransportPositionY,
    MSETransportGuidByte1,
    MSETransportSeat,
    MSETransportOrientation,
    MSETransportGuidByte7,
    MSETransportPositionX,
    MSEOrientation,
    MSESplineElevation,
    MSEOptionalCounter,
    MSEPitch,
    MSETimestamp,
    MSEEnd,
};
static MovementStatusElements const MovementFallReset[] =
{
    MSEPositionZ,
    MSEPositionX,
    MSEPositionY,
    MSEHasGuidByte7,
    MSEHasTransportData,
    MSEHasGuidByte6,
    MSEHasPitch,
    MSEHasSplineElevation,
    MSEHasMovementFlags,
    MSEHasGuidByte4,
    MSEHasGuidByte2,
    MSEHasMovementFlags2,
    MSEHasGuidByte0,
    MSEForcesCount,
    MSEHasGuidByte1,
    MSEHasFallData,
    MSEHasTimestamp,
    MSEZeroBit,
    MSEZeroBit,
    MSEHasCounter,
    MSEHasGuidByte3,
    MSEHasGuidByte5,
    MSEZeroBit,
    MSEHasOrientation,
    MSEHasTransportGuidByte7,
    MSEHasTransportTime2,
    MSEHasVehicleId,
    MSEHasTransportGuidByte6,
    MSEHasTransportGuidByte4,
    MSEHasTransportGuidByte0,
    MSEHasTransportGuidByte3,
    MSEHasTransportGuidByte2,
    MSEHasTransportGuidByte1,
    MSEHasTransportGuidByte5,
    MSEHasFallDirection,
    MSEMovementFlags2,
    MSEMovementFlags,
    MSEForces,
    MSEGuidByte4,
    MSEGuidByte2,
    MSEGuidByte3,
    MSEGuidByte1,
    MSEGuidByte0,
    MSEGuidByte6,
    MSEGuidByte5,
    MSEGuidByte7,
    MSETransportGuidByte0,
    MSETransportTime2,
    MSETransportGuidByte4,
    MSETransportGuidByte7,
    MSETransportGuidByte1,
    MSETransportGuidByte2,
    MSETransportGuidByte6,
    MSETransportPositionZ,
    MSETransportOrientation,
    MSETransportPositionX,
    MSETransportPositionY,
    MSETransportGuidByte3,
    MSETransportVehicleId,
    MSETransportTime,
    MSETransportGuidByte5,
    MSETransportSeat,
    MSEOrientation,
    MSEFallTime,
    MSEFallCosAngle,
    MSEFallHorizontalSpeed,
    MSEFallSinAngle,
    MSEFallVerticalSpeed,
    MSESplineElevation,
    MSETimestamp,
    MSEOptionalCounter,
    MSEPitch,
    MSEEnd,
};

static MovementStatusElements const DismissControlledVehicle[] =
{

    MSEPositionZ,

    MSEPositionY,

    MSEPositionX,

    MSEHasGuidByte7,

    MSEHasFallData,

    MSEHasGuidByte2,

    MSEHasGuidByte5,

    MSEHasSplineElevation,

    MSEHasOrientation,

    MSEHasMovementFlags,

    MSEZeroBit,

    MSEHasGuidByte6,

    MSEHasCounter,

    MSEHasGuidByte1,

    MSEHasTimestamp,

    MSEHasGuidByte4,

    MSEHasTransportData,

    MSEZeroBit,

    MSEHasGuidByte3,

    MSEZeroBit,

    MSEHasMovementFlags2,

    MSEForcesCount,

    MSEHasPitch,

    MSEHasGuidByte0,

    MSEMovementFlags2,

    MSEHasTransportGuidByte3,

    MSEHasTransportTime2,

    MSEHasTransportGuidByte5,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte7,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte0,

    MSEHasVehicleId,

    MSEMovementFlags,

    MSEHasFallDirection,

    MSEGuidByte6,

    MSEGuidByte3,

    MSEForces,

    MSEGuidByte1,

    MSEGuidByte4,

    MSEGuidByte0,

    MSEGuidByte7,

    MSEGuidByte5,

    MSEGuidByte2,

    MSEFallSinAngle,

    MSEFallCosAngle,

    MSEFallHorizontalSpeed,

    MSEFallTime,

    MSEFallVerticalSpeed,

    MSETransportGuidByte0,

    MSETransportGuidByte4,

    MSETransportPositionX,

    MSETransportGuidByte2,

    MSETransportTime2,

    MSETransportPositionZ,

    MSETransportGuidByte1,

    MSETransportOrientation,

    MSETransportGuidByte3,

    MSETransportGuidByte7,

    MSETransportGuidByte6,

    MSETransportGuidByte5,

    MSETransportTime,

    MSETransportSeat,

    MSETransportVehicleId,

    MSETransportPositionY,

    MSEPitch,

    MSETimestamp,

    MSEOrientation,

    MSESplineElevation,

    MSEOptionalCounter,

    MSEEnd,
};

static MovementStatusElements const ChangeSeatsAccessoryGuid[] =
{

    MSEExtraInt8,

    MSEHasGuidByte2,

    MSEHasGuidByte1,

    MSEHasGuidByte7,

    MSEHasGuidByte5,

    MSEHasGuidByte3,

    MSEHasGuidByte6,

    MSEHasGuidByte4,

    MSEHasGuidByte0,

    MSEGuidByte5,

    MSEGuidByte4,

    MSEGuidByte7,

    MSEGuidByte1,

    MSEGuidByte3,

    MSEGuidByte2,

    MSEGuidByte6,

    MSEGuidByte0,

    MSEEnd,
};

static MovementStatusElements const ChangeSeatsOnControlledVehicle[] =
{

    MSEExtraElement,

    MSEPositionY,

    MSEPositionZ,

    MSEPositionX,

    MSEExtraElement,

    MSEExtraElement,

    MSEHasTimestamp,

    MSEHasGuidByte6,

    MSEHasGuidByte0,

    MSEHasGuidByte2,

    MSEExtraElement,

    MSEHasGuidByte7,

    MSEHasGuidByte1,

    MSEExtraElement,

    MSEExtraElement,

    MSEHasTransportData,

    MSEHasMovementFlags2,

    MSEExtraElement,

    MSEExtraElement,

    MSEHasGuidByte4,

    MSEForcesCount,

    MSEExtraElement,

    MSEHasOrientation,

    MSEHasGuidByte5,

    MSEZeroBit,

    MSEZeroBit,

    MSEHasSplineElevation,

    MSEHasFallData,

    MSEHasMovementFlags,

    MSEHasCounter,

    MSEZeroBit,

    MSEHasGuidByte3,

    MSEHasPitch,

    MSEHasTransportGuidByte6,

    MSEHasTransportGuidByte0,

    MSEHasTransportGuidByte3,

    MSEHasTransportGuidByte7,

    MSEHasTransportTime2,

    MSEHasVehicleId,

    MSEHasTransportGuidByte4,

    MSEHasTransportGuidByte1,

    MSEHasTransportGuidByte2,

    MSEHasTransportGuidByte5,

    MSEMovementFlags,

    MSEMovementFlags2,

    MSEHasFallDirection,

    MSEGuidByte6,

    MSEExtraElement,

    MSEGuidByte2,

    MSEForces,

    MSEGuidByte5,

    MSEExtraElement,

    MSEExtraElement,

    MSEGuidByte3,

    MSEGuidByte7,

    MSEExtraElement,

    MSEExtraElement,

    MSEExtraElement,

    MSEGuidByte4,

    MSEGuidByte1,

    MSEExtraElement,

    MSEExtraElement,

    MSEGuidByte0,

    MSEOptionalCounter,

    MSEFallVerticalSpeed,

    MSEFallCosAngle,

    MSEFallSinAngle,

    MSEFallHorizontalSpeed,

    MSEFallTime,

    MSETransportTime,

    MSETransportGuidByte6,

    MSETransportPositionZ,

    MSETransportGuidByte4,

    MSETransportGuidByte7,

    MSETransportOrientation,

    MSETransportGuidByte5,

    MSETransportSeat,

    MSETransportPositionX,

    MSETransportVehicleId,

    MSETransportGuidByte1,

    MSETransportGuidByte2,

    MSETransportGuidByte0,

    MSETransportTime2,

    MSETransportPositionY,

    MSETransportGuidByte3,

    MSESplineElevation,

    MSEOrientation,

    MSEPitch,

    MSETimestamp,

    MSEEnd,
};

// 5.4.8 sequences with no 4.3.4 counterpart, from Project SkyFire (MovementStructures.cpp).
namespace
{
    MovementStatusElements const PlayerMove[] =
    {
        MSEHasPitch,
        MSEHasGuidByte2,
        MSEZeroBit,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSEHasOrientation,
        MSEHasFallData,
        MSEHasCounter,
        MSEHasGuidByte3,
        MSEHasFallDirection,
        MSEHasTransportData,
        MSEHasGuidByte4,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte6,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte1,
        MSEHasVehicleId,
        MSEHasTransportGuidByte0,
        MSEHasSplineElevation,
        MSEHasMovementFlags,
        MSEZeroBit,
        MSEMovementFlags,
        MSEHasMovementFlags2,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasTimestamp,
        MSEMovementFlags2,
        MSEHasGuidByte5,
        MSEForcesCount,
        MSEHasGuidByte6,
        MSEPositionY,
        MSETransportGuidByte7,
        MSETransportTime2,
        MSETransportPositionX,
        MSETransportGuidByte5,
        MSETransportSeat,
        MSETransportGuidByte2,
        MSETransportGuidByte0,
        MSETransportGuidByte3,
        MSETransportTime,
        MSETransportGuidByte4,
        MSETransportPositionZ,
        MSETransportGuidByte1,
        MSETransportPositionY,
        MSETransportOrientation,
        MSETransportGuidByte6,
        MSETransportVehicleId,
        MSEGuidByte5,
        MSEGuidByte1,
        MSEPositionZ,
        MSEForces,
        MSETimestamp,
        MSEOrientation,
        MSEGuidByte3,
        MSEFallSinAngle,
        MSEFallHorizontalSpeed,
        MSEFallCosAngle,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSEGuidByte0,
        MSEPitch,
        MSEGuidByte2,
        MSEGuidByte6,
        MSESplineElevation,
        MSEOptionalCounter,
        MSEPositionX,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const MovementHeartBeat[] =
    {
        MSEPositionZ,
        MSEPositionX,
        MSEPositionY,
        MSEForcesCount,
        MSEHasMovementFlags,
        MSEZeroBit,
        MSEHasCounter,
        MSEHasGuidByte3,
        MSEHasGuidByte6,
        MSEHasPitch,
        MSEZeroBit,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte4,
        MSEHasMovementFlags2,
        MSEHasOrientation,
        MSEHasTimestamp,
        MSEHasTransportData,
        MSEHasFallData,
        MSEHasGuidByte5,
        MSEHasSplineElevation,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte7,
        MSEHasVehicleId,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte4,
        MSEHasTransportTime2,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEMovementFlags2,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte6,
        MSEGuidByte1,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEForces,
        MSEGuidByte5,
        MSEGuidByte0,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSETransportGuidByte1,
        MSETransportGuidByte3,
        MSETransportGuidByte2,
        MSETransportGuidByte0,
        MSETransportVehicleId,
        MSETransportSeat,
        MSETransportGuidByte7,
        MSETransportPositionX,
        MSETransportGuidByte4,
        MSETransportTime2,
        MSETransportPositionY,
        MSETransportGuidByte6,
        MSETransportGuidByte5,
        MSETransportPositionZ,
        MSETransportTime,
        MSETransportOrientation,
        MSEOptionalCounter,
        MSEOrientation,
        MSEPitch,
        MSETimestamp,
        MSESplineElevation,
        MSEEnd,
    };

    MovementStatusElements const MoveSplineDone[] =
    {
        MSEOptionalCounter,
        MSEPositionZ,
        MSEPositionY,
        MSEPositionX,
        MSEHasGuidByte7,
        MSEHasMovementFlags2,
        MSEHasSplineElevation,
        MSEHasMovementFlags,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSEHasPitch,
        MSEZeroBit,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSEHasGuidByte2,
        MSEHasCounter,
        MSEHasGuidByte4,
        MSEHasTransportData,
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEForcesCount,
        MSEHasOrientation,
        MSEHasTimestamp,
        MSEHasFallData,
        MSEHasTransportGuidByte2,
        MSEHasVehicleId,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte6,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEGuidByte6,
        MSEForces,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEGuidByte4,
        MSEGuidByte2,
        MSEGuidByte3,
        MSESplineElevation,
        MSETransportGuidByte3,
        MSETransportOrientation,
        MSETransportGuidByte2,
        MSETransportGuidByte5,
        MSETransportGuidByte1,
        MSETransportPositionX,
        MSETransportGuidByte6,
        MSETransportSeat,
        MSETransportVehicleId,
        MSETransportGuidByte4,
        MSETransportGuidByte7,
        MSETransportGuidByte0,
        MSETransportPositionY,
        MSETransportTime2,
        MSETransportPositionZ,
        MSETransportTime,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSEOptionalCounter,
        MSEOrientation,
        MSETimestamp,
        MSEPitch,
        MSEEnd,
    };

    MovementStatusElements const MoveNotActiveMover[] =
    {
        MSEPositionZ,
        MSEPositionX,
        MSEPositionY,
        MSEHasMovementFlags2,
        MSEHasTransportData,
        MSEHasGuidByte6,
        MSEHasSplineElevation,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasTimestamp,
        MSEHasGuidByte0,
        MSEHasOrientation,
        MSEHasGuidByte5,
        MSEHasPitch,
        MSEHasGuidByte1,
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEHasSpline,
        MSEHasGuidByte2,
        MSEHasFallData,
        MSEHasMovementFlags,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasVehicleId,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte7,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte3,
        MSEHasFallDirection,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte4,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEGuidByte5,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEFallVerticalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallTime,
        MSETransportVehicleId,
        MSETransportGuidByte1,
        MSETransportTime2,
        MSETransportOrientation,
        MSETransportGuidByte0,
        MSETransportSeat,
        MSETransportGuidByte4,
        MSETransportGuidByte6,
        MSETransportGuidByte3,
        MSETransportGuidByte5,
        MSETransportPositionY,
        MSETransportPositionX,
        MSETransportGuidByte2,
        MSETransportPositionZ,
        MSETransportGuidByte7,
        MSETransportTime,
        MSETimestamp,
        MSESplineElevation,
        MSEPitch,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MoveTeleport[] =
    {
        MSEHasGuidByte0,
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasTransportData,
        MSEHasGuidByte4,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte7,
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSETransportGuidByte4,
        MSETransportGuidByte3,
        MSETransportGuidByte7,
        MSETransportGuidByte1,
        MSETransportGuidByte6,
        MSETransportGuidByte0,
        MSETransportGuidByte2,
        MSETransportGuidByte5,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEPositionZ,
        MSEPositionY,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte5,
        MSEPositionX,
        MSECounter,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte1,
        MSEOrientationWithoutCheck,
        MSEEnd,
    };

    MovementStatusElements const MoveUpdateTeleport[] =
    {
        MSEPositionZ,
        MSEPositionY,
        MSEPositionX,
        MSEHasOrientation,
        MSEHasSpline,
        MSEHasMovementFlags,
        MSEHasGuidByte2,
        MSEHasGuidByte4,
        MSEHasGuidByte6,
        MSEHasFallData,
        MSEHasGuidByte0,
        MSEHasTransportData,
        MSEHasGuidByte5,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte0,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte6,
        MSEHasVehicleId,
        MSEHasTransportGuidByte2,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasGuidByte3,
        MSEHasPitch,
        MSEHasMovementFlags2,
        MSEHasTimestamp,
        MSEHasFallDirection,
        MSEMovementFlags2,
        MSEHasSplineElevation,
        MSEMovementFlags,
        MSEHasGuidByte1,
        MSEGuidByte7,
        MSETransportGuidByte3,
        MSETransportGuidByte4,
        MSETransportOrientation,
        MSETransportVehicleId,
        MSETransportGuidByte1,
        MSETransportTime2,
        MSETransportPositionZ,
        MSETransportGuidByte7,
        MSETransportGuidByte0,
        MSETransportGuidByte6,
        MSETransportGuidByte5,
        MSETransportGuidByte2,
        MSETransportSeat,
        MSETransportTime,
        MSETransportPositionY,
        MSETransportPositionX,
        MSEGuidByte6,
        MSEPitch,
        MSESplineElevation,
        MSEOrientation,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte1,
        MSEFallTime,
        MSEFallHorizontalSpeed,
        MSEFallCosAngle,
        MSEFallSinAngle,
        MSEFallVerticalSpeed,
        MSEGuidByte5,
        MSEGuidByte4,
        MSETimestamp,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const MovementSetCanFly[] =
    {
        MSEPositionY,
        MSEPositionX,
        MSEPositionZ,
        MSEHasFallData,
        MSEHasGuidByte0,
        MSEHasMovementFlags2,
        MSEHasGuidByte6,
        MSEHasMovementFlags,
        MSEZeroBit,
        MSEHasGuidByte2,
        MSEHasOrientation,
        MSEHasCounter,
        MSEHasSplineElevation,
        MSEHasTransportData,
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasPitch,
        MSEZeroBit,
        MSEHasGuidByte4,
        MSEHasTimestamp,
        MSEForcesCount,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasGuidByte3,
        MSEHasTransportGuidByte2,
        MSEHasVehicleId,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte6,
        MSEHasTransportTime2,
        MSEHasFallDirection,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEForces,
        MSEGuidByte4,
        MSEGuidByte2,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEOptionalCounter,
        MSETransportGuidByte7,
        MSETransportGuidByte4,
        MSETransportTime,
        MSETransportGuidByte3,
        MSETransportGuidByte6,
        MSETransportOrientation,
        MSETransportGuidByte0,
        MSETransportPositionZ,
        MSETransportPositionX,
        MSETransportSeat,
        MSETransportPositionY,
        MSETransportTime2,
        MSETransportGuidByte1,
        MSETransportGuidByte5,
        MSETransportGuidByte2,
        MSETransportVehicleId,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSESplineElevation,
        MSEOrientation,
        MSETimestamp,
        MSEPitch,
        MSEEnd,
    };

    MovementStatusElements const MovementApplyMovementForceAck[] =
    {
        MSECounter,
        MSEExtraElement,
        MSECounter,
        MSEPositionX,
        MSEPositionY,
        MSEPositionZ,
        MSEExtraElement,
        MSEExtraElement,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasOrientation,
        MSEHasTimestamp,
        MSEHasGuidByte2,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEHasMovementFlags2,
        MSEZeroBit,
        MSEHasCounter,
        MSEHasMovementFlags,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasSplineElevation,
        MSEForcesCount,
        MSEHasGuidByte6,
        MSEHasPitch,
        MSEZeroBit,
        MSEHasFallData,
        MSEHasTransportData,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte0,
        MSEHasTransportTime2,
        MSEHasVehicleId,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte4,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEGuidByte3,
        MSEGuidByte1,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEForces,
        MSEGuidByte2,
        MSETransportGuidByte0,
        MSETransportPositionX,
        MSETransportTime,
        MSETransportSeat,
        MSETransportGuidByte5,
        MSETransportTime2,
        MSETransportGuidByte1,
        MSETransportGuidByte3,
        MSETransportGuidByte2,
        MSETransportGuidByte4,
        MSETransportGuidByte7,
        MSETransportOrientation,
        MSETransportPositionZ,
        MSETransportVehicleId,
        MSETransportPositionY,
        MSETransportGuidByte6,
        MSEPitch,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSESplineElevation,
        MSETimestamp,
        MSEOptionalCounter,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MovementUpdateApplyMovementForce[] =
    {
        MSECounter,
        MSEExtraElement,
        MSECounter,
        MSEPositionX,
        MSEPositionY,
        MSEPositionZ,
        MSEExtraElement,
        MSEExtraElement,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasOrientation,
        MSEHasTimestamp,
        MSEHasGuidByte2,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEHasMovementFlags2,
        MSEZeroBit,
        MSEHasCounter,
        MSEHasMovementFlags,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasSplineElevation,
        MSEForcesCount,
        MSEHasGuidByte6,
        MSEHasPitch,
        MSEZeroBit,
        MSEHasFallData,
        MSEHasTransportData,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte0,
        MSEHasTransportTime2,
        MSEHasVehicleId,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte4,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEGuidByte3,
        MSEGuidByte1,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEForces,
        MSEGuidByte2,
        MSETransportGuidByte0,
        MSETransportPositionX,
        MSETransportTime,
        MSETransportSeat,
        MSETransportGuidByte5,
        MSETransportTime2,
        MSETransportGuidByte1,
        MSETransportGuidByte3,
        MSETransportGuidByte2,
        MSETransportGuidByte4,
        MSETransportGuidByte7,
        MSETransportOrientation,
        MSETransportPositionZ,
        MSETransportVehicleId,
        MSETransportPositionY,
        MSETransportGuidByte6,
        MSEPitch,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSESplineElevation,
        MSETimestamp,
        MSEOptionalCounter,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MovementRemoveMovementForceAck[] =
    {
        MSECounter,
        MSEPositionZ,
        MSECounter,
        MSEPositionX,
        MSEPositionY,
        MSEForcesCount,
        MSEHasMovementFlags,
        MSEHasSplineElevation,
        MSEHasGuidByte5,
        MSEZeroBit,
        MSEHasGuidByte6,
        MSEHasPitch,
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasTimestamp,
        MSEZeroBit,
        MSEHasGuidByte2,
        MSEHasFallData,
        MSEHasGuidByte7,
        MSEHasOrientation,
        MSEHasTransportData,
        MSEHasGuidByte4,
        MSEHasMovementFlags2,
        MSEHasCounter,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSETransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte4,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte0,
        MSEHasVehicleId,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte2,
        MSEHasFallDirection,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEGuidByte1,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEGuidByte6,
        MSEForces,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSETransportGuidByte0,
        MSETransportPositionY,
        MSETransportTime2,
        MSETransportOrientation,
        MSETransportGuidByte7,
        MSETransportVehicleId,
        MSETransportGuidByte1,
        MSETransportGuidByte2,
        MSETransportPositionZ,
        MSETransportTime,
        MSETransportGuidByte6,
        MSETransportPositionX,
        MSETransportGuidByte4,
        MSETransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSETransportSeat,
        MSEOrientation,
        MSEPitch,
        MSETimestamp,
        MSESplineElevation,
        MSEOptionalCounter,
        MSEEnd,
    };

    MovementStatusElements const MovementUpdateRemoveMovementForce[] =
    {
        MSECounter,
        MSEPositionZ,
        MSECounter,
        MSEPositionX,
        MSEPositionY,
        MSEForcesCount,
        MSEHasMovementFlags,
        MSEHasSplineElevation,
        MSEHasGuidByte5,
        MSEZeroBit,
        MSEHasGuidByte6,
        MSEHasPitch,
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasTimestamp,
        MSEZeroBit,
        MSEHasGuidByte2,
        MSEHasFallData,
        MSEHasGuidByte7,
        MSEHasOrientation,
        MSEHasTransportData,
        MSEHasGuidByte4,
        MSEHasMovementFlags2,
        MSEHasCounter,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSETransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte4,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte0,
        MSEHasVehicleId,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte2,
        MSEHasFallDirection,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEGuidByte1,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEGuidByte6,
        MSEForces,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSETransportGuidByte0,
        MSETransportPositionY,
        MSETransportTime2,
        MSETransportOrientation,
        MSETransportGuidByte7,
        MSETransportVehicleId,
        MSETransportGuidByte1,
        MSETransportGuidByte2,
        MSETransportPositionZ,
        MSETransportTime,
        MSETransportGuidByte6,
        MSETransportPositionX,
        MSETransportGuidByte4,
        MSETransportGuidByte5,
        MSEHasTransportGuidByte3,
        MSETransportSeat,
        MSEOrientation,
        MSEPitch,
        MSETimestamp,
        MSESplineElevation,
        MSEOptionalCounter,
        MSEEnd,
    };

    MovementStatusElements const MovementForcePitchRateChangeAck[] =
    {
        MSEPositionY,
        MSEExtraElement,
        MSEPositionZ,
        MSEPositionX,
        MSECounter,
        MSEForcesCount,
        MSEHasGuidByte5,
        MSEHasGuidByte3,
        MSEHasOrientation,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEHasGuidByte1,
        MSEHasFallData,
        MSEHasTimestamp,
        MSEHasCounter,
        MSEHasGuidByte2,
        MSEHasMovementFlags2,
        MSEHasGuidByte7,
        MSEHasPitch,
        MSEHasGuidByte6,
        MSEZeroBit,
        MSEHasMovementFlags,
        MSEZeroBit,
        MSEHasSplineElevation,
        MSEHasTransportData,
        MSEZeroBit,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte0,
        MSEHasVehicleId,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte3,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte7,
        MSEHasFallDirection,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEGuidByte3,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEGuidByte4,
        MSEForces,
        MSEGuidByte0,
        MSEGuidByte5,
        MSEGuidByte1,
        MSEGuidByte7,
        MSETransportVehicleId,
        MSETransportGuidByte5,
        MSETransportGuidByte6,
        MSETransportGuidByte1,
        MSETransportGuidByte4,
        MSETransportPositionX,
        MSETransportTime,
        MSETransportOrientation,
        MSETransportGuidByte3,
        MSETransportPositionY,
        MSETransportGuidByte0,
        MSETransportTime2,
        MSETransportSeat,
        MSETransportGuidByte2,
        MSETransportGuidByte7,
        MSETransportPositionZ,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSEOptionalCounter,
        MSEPitch,
        MSEOrientation,
        MSETimestamp,
        MSESplineElevation,
        MSEEnd,
    };

    MovementStatusElements const MovementSetCanFlyAck[] =
    {
        MSEPositionZ,
        MSECounter,
        MSEPositionX,
        MSEPositionY,
        MSEZeroBit,
        MSEHasMovementFlags2,
        MSEHasGuidByte3,
        MSEHasFallData,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEZeroBit,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSEHasCounter,
        MSEHasSplineElevation,
        MSEHasOrientation,
        MSEHasGuidByte6,
        MSEHasTransportData,
        MSEHasTimestamp,
        MSEHasPitch,
        MSEHasMovementFlags,
        MSEForcesCount,
        MSEHasGuidByte7,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEMovementFlags2,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte5,
        MSEHasVehicleId,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte0,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEForces,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte0,
        MSETransportTime2,
        MSETransportPositionX,
        MSETransportVehicleId,
        MSETransportOrientation,
        MSETransportGuidByte3,
        MSETransportSeat,
        MSETransportGuidByte5,
        MSETransportPositionZ,
        MSETransportGuidByte2,
        MSETransportGuidByte7,
        MSETransportGuidByte4,
        MSETransportTime,
        MSETransportGuidByte1,
        MSETransportGuidByte6,
        MSETransportGuidByte0,
        MSETransportPositionY,
        MSEOrientation,
        MSETimestamp,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSESplineElevation,
        MSEPitch,
        MSEOptionalCounter,
        MSEEnd,
    };

    MovementStatusElements const MovementSetFly[] =
    {
        MSEPositionY,
        MSEPositionZ,
        MSEPositionX,
        MSEHasGuidByte5,
        MSEHasTransportData,
        MSEHasGuidByte3,
        MSEForcesCount,
        MSEHasMovementFlags2,
        MSEZeroBit,
        MSEHasFallData,
        MSEHasGuidByte6,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasTimestamp,
        MSEHasGuidByte0,
        MSEHasGuidByte2,
        MSEHasPitch,
        MSEHasOrientation,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSEHasSplineElevation,
        MSEHasCounter,
        MSEHasGuidByte4,
        MSEHasMovementFlags,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte5,
        MSEHasVehicleId,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte4,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte0,
        MSEMovementFlags2,
        MSEHasFallDirection,
        MSEMovementFlags,
        MSEGuidByte1,
        MSEGuidByte6,
        MSEGuidByte5,
        MSEGuidByte2,
        MSEGuidByte4,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEGuidByte3,
        MSEForces,
        MSETransportGuidByte7,
        MSETransportGuidByte5,
        MSETransportGuidByte1,
        MSETransportTime,
        MSETransportSeat,
        MSETransportTime2,
        MSETransportGuidByte0,
        MSETransportGuidByte4,
        MSETransportPositionY,
        MSETransportGuidByte3,
        MSETransportPositionZ,
        MSETransportGuidByte2,
        MSETransportGuidByte6,
        MSETransportPositionX,
        MSETransportOrientation,
        MSETransportVehicleId,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSETimestamp,
        MSEOptionalCounter,
        MSESplineElevation,
        MSEPitch,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MovementForceTurnRateChangeAck[] =
    {
        MSECounter,
        MSEPositionZ,
        MSEExtraElement,
        MSEPositionX,
        MSEPositionY,
        MSEHasOrientation,
        MSEZeroBit,
        MSEHasPitch,
        MSEHasGuidByte1,
        MSEHasGuidByte7,
        MSEZeroBit,
        MSEHasTimestamp,
        MSEHasGuidByte0,
        MSEHasGuidByte2,
        MSEHasCounter,
        MSEHasGuidByte5,
        MSEHasMovementFlags2,
        MSEHasSplineElevation,
        MSEHasTransportData,
        MSEHasGuidByte6,
        MSEHasMovementFlags,
        MSEForcesCount,
        MSEHasFallData,
        MSEZeroBit,
        MSEHasGuidByte3,
        MSEHasGuidByte4,
        MSEMovementFlags2,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte6,
        MSEHasVehicleId,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte7,
        MSEHasTransportTime2,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEGuidByte2,
        MSEGuidByte5,
        MSEGuidByte7,
        MSEGuidByte4,
        MSEForces,
        MSEGuidByte6,
        MSEGuidByte1,
        MSEGuidByte3,
        MSEGuidByte0,
        MSEOptionalCounter,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSEFallCosAngle,
        MSEFallSinAngle,
        MSEFallHorizontalSpeed,
        MSETransportGuidByte0,
        MSETransportPositionX,
        MSETransportOrientation,
        MSETransportGuidByte6,
        MSETransportTime,
        MSETransportGuidByte5,
        MSETransportGuidByte1,
        MSETransportGuidByte3,
        MSETransportGuidByte4,
        MSETransportPositionY,
        MSETransportVehicleId,
        MSETransportGuidByte7,
        MSETransportPositionZ,
        MSETransportTime2,
        MSETransportGuidByte2,
        MSETransportSeat,
        MSEOrientation,
        MSEPitch,
        MSESplineElevation,
        MSETimestamp,
        MSEEnd,
    };

    MovementStatusElements const MovementFeatherFallAck[] =
    {
        MSEPositionY,
        MSEPositionX,
        MSEPositionZ,
        MSECounter,
        MSEHasSplineElevation,
        MSEHasGuidByte5,
        MSEHasGuidByte1,
        MSEHasGuidByte4,
        MSEZeroBit,
        MSEHasMovementFlags,
        MSEHasFallData,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasMovementFlags2,
        MSEHasGuidByte6,
        MSEHasTransportData,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasPitch,
        MSEHasTimestamp,
        MSEForcesCount,
        MSEHasCounter,
        MSEHasGuidByte0,
        MSEHasOrientation,
        MSEHasGuidByte2,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte5,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte3,
        MSEHasVehicleId,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEHasFallDirection,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte7,
        MSEGuidByte2,
        MSEForces,
        MSEGuidByte4,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte3,
        MSEFallHorizontalSpeed,
        MSEFallCosAngle,
        MSEFallSinAngle,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSETimestamp,
        MSEOptionalCounter,
        MSETransportPositionX,
        MSETransportPositionY,
        MSETransportVehicleId,
        MSETransportGuidByte7,
        MSETransportGuidByte1,
        MSETransportTime,
        MSETransportGuidByte4,
        MSETransportGuidByte2,
        MSETransportGuidByte3,
        MSETransportOrientation,
        MSETransportPositionZ,
        MSETransportSeat,
        MSETransportGuidByte0,
        MSETransportGuidByte5,
        MSETransportTime2,
        MSETransportGuidByte6,
        MSESplineElevation,
        MSEPitch,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MovementGravityDisableAck[] =
    {
        MSECounter,
        MSEPositionY,
        MSEPositionZ,
        MSEPositionX,
        MSEHasGuidByte0,
        MSEHasTransportData,
        MSEHasGuidByte5,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSEZeroBit,
        MSEHasGuidByte2,
        MSEHasOrientation,
        MSEHasFallData,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasGuidByte3,
        MSEHasMovementFlags2,
        MSEHasGuidByte6,
        MSEHasSplineElevation,
        MSEHasGuidByte4,
        MSEHasPitch,
        MSEForcesCount,
        MSEHasCounter,
        MSEHasMovementFlags,
        MSEHasTimestamp,
        MSEHasFallDirection,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte6,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte3,
        MSEHasVehicleId,
        MSEHasTransportGuidByte7,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEGuidByte7,
        MSEGuidByte2,
        MSEForces,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte0,
        MSEGuidByte4,
        MSEGuidByte1,
        MSEGuidByte5,
        MSETransportGuidByte1,
        MSETransportPositionX,
        MSETransportGuidByte5,
        MSETransportTime2,
        MSETransportSeat,
        MSETransportOrientation,
        MSETransportGuidByte4,
        MSETransportGuidByte7,
        MSETransportGuidByte0,
        MSETransportPositionY,
        MSETransportPositionZ,
        MSETransportGuidByte2,
        MSETransportGuidByte6,
        MSETransportTime,
        MSETransportVehicleId,
        MSETransportGuidByte3,
        MSEPitch,
        MSEFallTime,
        MSEFallHorizontalSpeed,
        MSEFallCosAngle,
        MSEFallSinAngle,
        MSEFallVerticalSpeed,
        MSEOrientation,
        MSETimestamp,
        MSESplineElevation,
        MSEOptionalCounter,
        MSEEnd,
    };

    MovementStatusElements const MovementGravityEnableAck[] =
    {
        MSEPositionY,
        MSEPositionX,
        MSEPositionZ,
        MSECounter,
        MSEHasOrientation,
        MSEHasGuidByte0,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasGuidByte3,
        MSEHasCounter,
        MSEHasMovementFlags2,
        MSEHasTransportData,
        MSEZeroBit,
        MSEHasMovementFlags,
        MSEHasTimestamp,
        MSEHasGuidByte6,
        MSEHasGuidByte4,
        MSEZeroBit,
        MSEHasSplineElevation,
        MSEHasPitch,
        MSEForcesCount,
        MSEHasFallData,
        MSEHasGuidByte2,
        MSEZeroBit,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte4,
        MSEHasVehicleId,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte5,
        MSEHasFallDirection,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte0,
        MSEForces,
        MSEGuidByte1,
        MSEGuidByte7,
        MSEGuidByte6,
        MSEGuidByte5,
        MSETransportTime2,
        MSETransportGuidByte5,
        MSETransportPositionX,
        MSETransportGuidByte2,
        MSETransportGuidByte6,
        MSETransportTime,
        MSETransportOrientation,
        MSETransportGuidByte0,
        MSETransportGuidByte4,
        MSETransportGuidByte1,
        MSETransportGuidByte7,
        MSETransportSeat,
        MSETransportPositionY,
        MSETransportGuidByte3,
        MSETransportVehicleId,
        MSETransportPositionZ,
        MSEPitch,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSEFallHorizontalSpeed,
        MSEFallCosAngle,
        MSEFallSinAngle,
        MSETimestamp,
        MSESplineElevation,
        MSEOptionalCounter,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MovementHoverAck[] =
    {
        MSECounter,
        MSEPositionY,
        MSEPositionX,
        MSEPositionZ,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasTransportData,
        MSEHasOrientation,
        MSEHasCounter,
        MSEHasTimestamp,
        MSEHasPitch,
        MSEForcesCount,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasMovementFlags2,
        MSEHasGuidByte4,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEZeroBit,
        MSEZeroBit,
        MSEHasMovementFlags,
        MSEHasSplineElevation,
        MSEHasFallData,
        MSEHasGuidByte1,
        MSEHasVehicleId,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte5,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte3,
        MSEHasFallDirection,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEForces,
        MSEGuidByte1,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEGuidByte5,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte0,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallTime,
        MSEFallVerticalSpeed,
        MSETransportGuidByte2,
        MSETransportGuidByte4,
        MSETransportPositionZ,
        MSETransportPositionX,
        MSETransportGuidByte0,
        MSETransportGuidByte5,
        MSETransportGuidByte1,
        MSETransportTime,
        MSETransportOrientation,
        MSETransportPositionY,
        MSETransportGuidByte7,
        MSETransportGuidByte6,
        MSETransportTime2,
        MSETransportSeat,
        MSETransportVehicleId,
        MSETransportGuidByte3,
        MSESplineElevation,
        MSEPitch,
        MSETimestamp,
        MSEOptionalCounter,
        MSEOrientation,
        MSEEnd,
    };

    MovementStatusElements const MovementUpdatePitchBack[] =
    {
        MSEHasGuidByte7,
        MSEHasMovementFlags,
        MSEHasSplineElevation,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSEForcesCount,
        MSEHasMovementFlags2,
        MSEHasGuidByte4,
        MSEMovementFlags2,
        MSEZeroBit,
        MSEHasGuidByte6,
        MSEHasGuidByte3,
        MSEHasOrientation,
        MSEHasGuidByte2,
        MSEZeroBit,
        MSEHasPitch,
        MSEHasFallData,
        MSEMovementFlags,
        MSEHasTransportData,
        MSEHasTransportGuidByte5,
        MSEHasVehicleId,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte0,
        MSEHasTransportTime2,
        MSEHasFallDirection,
        MSEHasCounter,
        MSEHasTimestamp,
        MSEHasGuidByte5,
        MSETransportGuidByte3,
        MSETransportPositionZ,
        MSETransportVehicleId,
        MSETransportGuidByte2,
        MSETransportGuidByte4,
        MSETransportTime2,
        MSETransportGuidByte1,
        MSETransportTime,
        MSETransportPositionY,
        MSETransportOrientation,
        MSETransportPositionX,
        MSETransportGuidByte0,
        MSETransportGuidByte5,
        MSETransportSeat,
        MSETransportGuidByte6,
        MSETransportGuidByte7,
        MSEGuidByte0,
        MSETimestamp,
        MSESplineElevation,
        MSEGuidByte7,
        MSEGuidByte3,
        MSEForces,
        MSEOrientation,
        MSEPositionX,
        MSEPositionY,
        MSEOptionalCounter,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte1,
        MSEGuidByte2,
        MSECounter,
        MSEPositionZ,
        MSEGuidByte5,
        MSEPitch,
        MSEEnd,
    };

    MovementStatusElements const MovementUpdateTurnRate[] =
    {
        MSEHasGuidByte4,
        MSEHasFallData,
        MSEHasMovementFlags,
        MSEHasSplineElevation,
        MSEHasFallDirection,
        MSEZeroBit,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEMovementFlags,
        MSEHasGuidByte3,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSEHasGuidByte1,
        MSEZeroBit,
        MSEHasGuidByte7,
        MSEHasTimestamp,
        MSEHasGuidByte2,
        MSEHasCounter,
        MSEHasOrientation,
        MSEForcesCount,
        MSEHasPitch,
        MSEHasMovementFlags2,
        MSEMovementFlags2,
        MSEHasTransportData,
        MSEHasTransportGuidByte1,
        MSEHasVehicleId,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte5,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte4,
        MSETransportVehicleId,
        MSETransportTime2,
        MSETransportGuidByte3,
        MSETransportOrientation,
        MSETransportGuidByte0,
        MSETransportGuidByte4,
        MSETransportGuidByte1,
        MSETransportGuidByte6,
        MSETransportTime,
        MSETransportGuidByte5,
        MSETransportGuidByte2,
        MSETransportPositionY,
        MSETransportPositionX,
        MSETransportGuidByte7,
        MSETransportPositionZ,
        MSETransportSeat,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallVerticalSpeed,
        MSEFallTime,
        MSEForces,
        MSEPositionY,
        MSEPitch,
        MSEGuidByte7,
        MSEOptionalCounter,
        MSEGuidByte5,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEPositionX,
        MSEGuidByte2,
        MSEGuidByte0,
        MSEOrientation,
        MSESplineElevation,
        MSEExtraElement,
        MSETimestamp,
        MSEGuidByte4,
        MSEGuidByte1,
        MSEPositionZ,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetWalkMode[] =
    {
        MSEHasGuidByte4,
        MSEHasGuidByte3,
        MSEHasGuidByte0,
        MSEHasGuidByte2,
        MSEHasGuidByte1,
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEGuidByte1,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetRunMode[] =
    {
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte2,
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasGuidByte3,
        MSEHasGuidByte0,
        MSEGuidByte5,
        MSEGuidByte1,
        MSEGuidByte4,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEGuidByte3,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveGravityDisable[] =
    {
        MSEHasGuidByte1,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEHasGuidByte2,
        MSEHasGuidByte3,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveGravityEnable[] =
    {
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte2,
        MSEHasGuidByte3,
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte4,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveCollisionDisable[] =
    {
        MSEHasGuidByte1,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEHasGuidByte2,
        MSEHasGuidByte3,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveCollisionEnable[] =
    {
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte2,
        MSEHasGuidByte3,
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte4,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetHover[] =
    {
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte1,
        MSEHasGuidByte3,
        MSEHasGuidByte0,
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte5,
        MSEGuidByte4,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveUnsetHover[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte6,
        MSEHasGuidByte2,
        MSEHasGuidByte0,
        MSEGuidByte3,
        MSEGuidByte5,
        MSEGuidByte2,
        MSEGuidByte1,
        MSEGuidByte6,
        MSEGuidByte7,
        MSEGuidByte4,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveStartSwim[] =
    {
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte2,
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEGuidByte4,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveStopSwim[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte2,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEGuidByte0,
        MSEGuidByte5,
        MSEGuidByte1,
        MSEGuidByte7,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetFlying[] =
    {
        MSEHasGuidByte4,
        MSEHasGuidByte1,
        MSEHasGuidByte2,
        MSEHasGuidByte0,
        MSEHasGuidByte7,
        MSEHasGuidByte5,
        MSEHasGuidByte3,
        MSEHasGuidByte6,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte5,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveUnsetFlying[] =
    {
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte6,
        MSEHasGuidByte3,
        MSEHasGuidByte0,
        MSEHasGuidByte4,
        MSEGuidByte2,
        MSEGuidByte5,
        MSEGuidByte4,
        MSEGuidByte6,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEGuidByte3,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetWaterWalk[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEGuidByte4,
        MSEGuidByte3,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetLandWalk[] =
    {
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte3,
        MSEHasGuidByte4,
        MSEGuidByte1,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte3,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEGuidByte2,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetFeatherFall[] =
    {
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte3,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetNormalFall[] =
    {
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte7,
        MSEHasGuidByte0,
        MSEHasGuidByte3,
        MSEGuidByte7,
        MSEGuidByte5,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveRoot[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEHasGuidByte1,
        MSEGuidByte2,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte6,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveUnroot[] =
    {
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte0,
        MSEHasGuidByte3,
        MSEHasGuidByte6,
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte3,
        MSEGuidByte5,
        MSEGuidByte0,
        MSEGuidByte4,
        MSEGuidByte6,
        MSEEnd,
    };

    MovementStatusElements const MoveSetCanFly[] =
    {
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEHasGuidByte3,
        MSEHasGuidByte7,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEGuidByte4,
        MSEGuidByte2,
        MSECounter,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const MoveUnsetCanFly[] =
    {
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte0,
        MSEHasGuidByte4,
        MSEHasGuidByte3,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte1,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte7,
        MSECounter,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEEnd,
    };

    MovementStatusElements const MoveSetCanTransitionBetweenSwimAndFly[] =
    {
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte0,
        MSEHasGuidByte3,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const MoveUnsetCanTransitionBetweenSwimAndFly[] =
    {
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte2,
        MSEHasGuidByte5,
        MSEHasGuidByte3,
        MSEGuidByte7,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const MoveSetCanTurnWhileFalling[] =
    {
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasGuidByte6,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte0,
        MSEHasGuidByte3,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const MoveUnsetCanTurnWhileFalling[] =
    {
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte0,
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte2,
        MSEHasGuidByte5,
        MSEHasGuidByte3,
        MSEGuidByte7,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const MoveSetHover[] =
    {
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEHasGuidByte4,
        MSEHasGuidByte2,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte3,
        MSEGuidByte1,
        MSEGuidByte6,
        MSECounter,
        MSEGuidByte2,
        MSEGuidByte4,
        MSEGuidByte3,
        MSEGuidByte5,
        MSEGuidByte0,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const MoveUnsetHover[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte5,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEHasGuidByte1,
        MSEHasGuidByte2,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEGuidByte6,
        MSEGuidByte4,
        MSEGuidByte5,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte1,
        MSEGuidByte0,
        MSECounter,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const MoveGravityDisable[] =
    {
        MSEHasGuidByte6,
        MSEHasGuidByte1,
        MSEHasGuidByte3,
        MSEHasGuidByte7,
        MSEHasGuidByte4,
        MSEHasGuidByte2,
        MSEHasGuidByte5,
        MSEHasGuidByte0,
        MSEGuidByte5,
        MSEGuidByte2,
        MSEGuidByte1,
        MSEGuidByte6,
        MSEGuidByte0,
        MSECounter,
        MSEGuidByte3,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const MoveGravityEnable[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte0,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte6,
        MSEHasGuidByte4,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte1,
        MSEGuidByte7,
        MSEGuidByte6,
        MSEGuidByte0,
        MSEGuidByte4,
        MSECounter,
        MSEGuidByte5,
        MSEEnd,
    };

    MovementStatusElements const MoveFeatherFall[] =
    {
        MSEHasGuidByte4,
        MSEHasGuidByte1,
        MSEHasGuidByte7,
        MSEHasGuidByte3,
        MSEHasGuidByte0,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte6,
        MSECounter,
        MSEGuidByte1,
        MSEGuidByte0,
        MSEGuidByte5,
        MSEGuidByte4,
        MSEGuidByte6,
        MSEGuidByte3,
        MSEGuidByte2,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const MoveNormalFall[] =
    {
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasGuidByte6,
        MSEHasGuidByte0,
        MSEHasGuidByte4,
        MSEHasGuidByte7,
        MSEHasGuidByte2,
        MSEHasGuidByte5,
        MSEGuidByte3,
        MSEGuidByte2,
        MSECounter,
        MSEGuidByte1,
        MSEGuidByte5,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEGuidByte0,
        MSEGuidByte6,
        MSEEnd,
    };

    MovementStatusElements const SetVehicleRecIdAck[] =
    {
        MSEPositionY,
        MSECounter,
        MSEPositionX,
        MSEPositionZ,
        MSEExtraElement,
        MSEHasGuidByte2,
        MSEHasMovementFlags2,
        MSEHasGuidByte3,
        MSEForcesCount,
        MSEHasMovementFlags,
        MSEHasTimestamp,
        MSEHasFallData,
        MSEHasGuidByte5,
        MSEHasTransportData,
        MSEZeroBit,
        MSEHasGuidByte0,
        MSEHasGuidByte7,
        MSEHasGuidByte1,
        MSEHasCounter,
        MSEHasGuidByte4,
        MSEZeroBit,
        MSEHasGuidByte6,
        MSEHasSplineElevation,
        MSEHasOrientation,
        MSEZeroBit,
        MSEHasPitch,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte3,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte4,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte1,
        MSEHasVehicleId,
        MSEHasTransportGuidByte5,
        MSEMovementFlags,
        MSEMovementFlags2,
        MSEHasFallDirection,
        MSEGuidByte6,
        MSEGuidByte2,
        MSEGuidByte0,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEForces,
        MSEGuidByte1,
        MSEGuidByte3,
        MSEGuidByte5,
        MSETransportSeat,
        MSETransportTime2,
        MSETransportVehicleId,
        MSETransportGuidByte6,
        MSETransportGuidByte3,
        MSETransportPositionX,
        MSETransportTime,
        MSETransportGuidByte4,
        MSETransportPositionZ,
        MSETransportGuidByte1,
        MSETransportGuidByte2,
        MSETransportGuidByte0,
        MSETransportGuidByte5,
        MSETransportGuidByte7,
        MSETransportOrientation,
        MSETransportPositionY,
        MSEFallTime,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallHorizontalSpeed,
        MSEFallVerticalSpeed,
        MSESplineElevation,
        MSEOptionalCounter,
        MSEOrientation,
        MSETimestamp,
        MSEPitch,
        MSEEnd,
    };
}

MovementStatusElements const* WorldPackets::Movement::GetChangeSeatsExtraSequence()
{
    return ChangeSeatsAccessoryGuid;
}

MovementStatusElements const* WorldPackets::Movement::GetMovementSequence(uint16 opcode)
{
    switch (opcode)
    {
        case CMSG_DISMISS_CONTROLLED_VEHICLE:            return DismissControlledVehicle;
        case CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE:    return ChangeSeatsOnControlledVehicle;
        case MSG_MOVE_START_BACKWARD:     return MovementStartBackward;
        case MSG_MOVE_START_FORWARD:      return MovementStartForward;
        case MSG_MOVE_START_STRAFE_LEFT:  return MovementStartStrafeLeft;
        case MSG_MOVE_START_STRAFE_RIGHT: return MovementStartStrafeRight;
        case MSG_MOVE_START_TURN_LEFT:    return MovementStartTurnLeft;
        case MSG_MOVE_START_TURN_RIGHT:   return MovementStartTurnRight;
        case MSG_MOVE_STOP:               return MovementStop;
        case MSG_MOVE_STOP_STRAFE:        return MovementStopStrafe;
        case MSG_MOVE_STOP_TURN:          return MovementStopTurn;
        case MSG_MOVE_SET_RUN_MODE:       return MovementSetRunMode;
        case MSG_MOVE_SET_WALK_MODE:      return MovementSetWalkMode;
        case MSG_MOVE_SET_FACING:         return MovementSetFacing;
        case MSG_MOVE_FALL_LAND:          return MovementFallLand;
        case MSG_MOVE_JUMP:               return MovementJump;
        case MSG_MOVE_SET_PITCH:          return MovementSetPitch;
        case MSG_MOVE_START_ASCEND:       return MovementStartAscend;
        case MSG_MOVE_START_DESCEND:      return MovementStartDescend;
        case MSG_MOVE_START_PITCH_DOWN:   return MovementStartPitchDown;
        case MSG_MOVE_START_PITCH_UP:     return MovementStartPitchUp;
        case MSG_MOVE_START_SWIM:         return MovementStartSwim;
        case MSG_MOVE_STOP_ASCEND:        return MovementStopAscend;
        case MSG_MOVE_STOP_PITCH:         return MovementStopPitch;
        case MSG_MOVE_STOP_SWIM:          return MovementStopSwim;
        case CMSG_MOVE_CHNG_TRANSPORT:    return MoveChngTransport;
        case CMSG_MOVE_FALL_RESET:        return MovementFallReset;
        case MSG_MOVE_HEARTBEAT:          return MovementHeartBeat;
        case CMSG_MOVE_SET_FLY:           return MovementSetFly;
        default:                         return nullptr;
    }
}

namespace
{
    MovementStatusElements const MoveRoot[] =
    {
        MSEHasGuidByte0,
        MSEHasGuidByte3,
        MSEHasGuidByte4,
        MSEHasGuidByte1,
        MSEHasGuidByte5,
        MSEHasGuidByte2,
        MSEHasGuidByte6,
        MSEHasGuidByte7,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEGuidByte1,
        MSEGuidByte2,
        MSEGuidByte6,
        MSEGuidByte5,
        MSECounter,
        MSEGuidByte0,
        MSEGuidByte3,
        MSEEnd,
    };

    MovementStatusElements const MoveUnroot[] =
    {

        MSEHasGuidByte3,

        MSEHasGuidByte5,

        MSEHasGuidByte7,

        MSEHasGuidByte1,

        MSEHasGuidByte0,

        MSEHasGuidByte2,

        MSEHasGuidByte4,

        MSEHasGuidByte6,

        MSEGuidByte0,

        MSEGuidByte7,

        MSECounter,

        MSEGuidByte5,

        MSEGuidByte4,

        MSEGuidByte2,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEGuidByte6,

        MSEEnd,
    };

    MovementStatusElements const MoveWaterWalk[] =
    {

        MSEHasGuidByte2,

        MSEHasGuidByte0,

        MSEHasGuidByte4,

        MSEHasGuidByte5,

        MSEHasGuidByte3,

        MSEHasGuidByte7,

        MSEHasGuidByte1,

        MSEHasGuidByte6,

        MSEGuidByte4,

        MSEGuidByte7,

        MSEGuidByte0,

        MSEGuidByte1,

        MSEGuidByte6,

        MSEGuidByte2,

        MSEGuidByte3,

        MSEGuidByte5,

        MSECounter,

        MSEEnd,
    };

    MovementStatusElements const MoveLandWalk[] =
    {

        MSEHasGuidByte0,

        MSEHasGuidByte7,

        MSEHasGuidByte3,

        MSEHasGuidByte1,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte2,

        MSEHasGuidByte4,

        MSEGuidByte7,

        MSEGuidByte6,

        MSEGuidByte4,

        MSEGuidByte3,

        MSEGuidByte2,

        MSEGuidByte0,

        MSEGuidByte1,

        MSECounter,

        MSEGuidByte5,

        MSEEnd,
    };

    MovementStatusElements const ForceMoveRootAck[] =
    {

        MSEPositionX,

        MSEOptionalCounter,

        MSEPositionY,

        MSEPositionZ,

        MSEZeroBit,

        MSEHasTimestamp,

        MSEHasSplineElevation,

        MSEZeroBit,

        MSEHasGuidByte3,

        MSEZeroBit,

        MSEHasGuidByte4,

        MSEHasMovementFlags2,

        MSEHasGuidByte6,

        MSEHasCounter,

        MSEHasPitch,

        MSEHasFallData,

        MSEHasGuidByte2,

        MSEHasGuidByte1,

        MSEHasGuidByte7,

        MSEHasTransportData,

        MSEForcesCount,

        MSEHasMovementFlags,

        MSEHasGuidByte0,

        MSEHasOrientation,

        MSEHasGuidByte5,

        MSEHasTransportGuidByte1,

        MSEHasVehicleId,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte5,

        MSEMovementFlags,

        MSEHasFallDirection,

        MSEMovementFlags2,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEGuidByte5,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEGuidByte4,

        MSEGuidByte2,

        MSEGuidByte6,

        MSEForces,

        MSEOptionalCounter,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSETransportGuidByte5,

        MSETransportGuidByte0,

        MSETransportGuidByte3,

        MSETransportGuidByte2,

        MSETransportPositionX,

        MSETransportGuidByte6,

        MSETransportPositionY,

        MSETransportTime2,

        MSETransportTime,

        MSETransportPositionZ,

        MSETransportGuidByte7,

        MSETransportOrientation,

        MSETransportGuidByte1,

        MSETransportSeat,

        MSETransportVehicleId,

        MSETransportGuidByte4,

        MSEPitch,

        MSETimestamp,

        MSEOrientation,

        MSESplineElevation,

        MSEEnd,
    };

    MovementStatusElements const ForceMoveUnrootAck[] =
    {

        MSEPositionX,

        MSEPositionY,

        MSEOptionalCounter,

        MSEPositionZ,

        MSEHasGuidByte0,

        MSEHasPitch,

        MSEHasFallData,

        MSEHasSplineElevation,

        MSEZeroBit,

        MSEHasGuidByte6,

        MSEHasGuidByte4,

        MSEHasGuidByte1,

        MSEHasGuidByte3,

        MSEZeroBit,

        MSEHasGuidByte7,

        MSEHasGuidByte5,

        MSEZeroBit,

        MSEHasTimestamp,

        MSEHasCounter,

        MSEHasMovementFlags,

        MSEHasTransportData,

        MSEHasGuidByte2,

        MSEForcesCount,

        MSEHasMovementFlags2,

        MSEHasOrientation,

        MSEHasVehicleId,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte2,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte3,

        MSEMovementFlags2,

        MSEHasFallDirection,

        MSEMovementFlags,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEForces,

        MSEGuidByte6,

        MSEGuidByte2,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEGuidByte4,

        MSEGuidByte7,

        MSEOptionalCounter,

        MSETransportSeat,

        MSETransportGuidByte7,

        MSETransportGuidByte4,

        MSETransportTime,

        MSETransportGuidByte1,

        MSETransportTime2,

        MSETransportPositionZ,

        MSETransportVehicleId,

        MSETransportGuidByte5,

        MSETransportPositionY,

        MSETransportGuidByte6,

        MSETransportGuidByte2,

        MSETransportPositionX,

        MSETransportGuidByte0,

        MSETransportGuidByte3,

        MSETransportOrientation,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSETimestamp,

        MSESplineElevation,

        MSEOrientation,

        MSEPitch,

        MSEEnd,
    };

    MovementStatusElements const MovementWaterWalkAck[] =
    {

        MSEPositionX,

        MSEPositionY,

        MSECounter,

        MSEPositionZ,

        MSEZeroBit,

        MSEHasGuidByte3,

        MSEHasFallData,

        MSEHasGuidByte2,

        MSEHasMovementFlags,

        MSEHasGuidByte0,

        MSEHasPitch,

        MSEZeroBit,

        MSEHasMovementFlags2,

        MSEHasGuidByte7,

        MSEHasGuidByte6,

        MSEHasTimestamp,

        MSEZeroBit,

        MSEHasSplineElevation,

        MSEHasTransportData,

        MSEHasGuidByte1,

        MSEHasGuidByte4,

        MSEHasCounter,

        MSEForcesCount,

        MSEHasGuidByte5,

        MSEHasOrientation,

        MSEHasTransportGuidByte1,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte7,

        MSEHasVehicleId,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte6,

        MSEMovementFlags,

        MSEHasFallDirection,

        MSEMovementFlags2,

        MSEGuidByte7,

        MSEForces,

        MSEGuidByte0,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEGuidByte6,

        MSEGuidByte2,

        MSETransportTime2,

        MSETransportGuidByte1,

        MSETransportTime,

        MSETransportPositionY,

        MSETransportGuidByte4,

        MSETransportPositionX,

        MSETransportGuidByte5,

        MSETransportGuidByte7,

        MSETransportGuidByte3,

        MSETransportOrientation,

        MSETransportGuidByte2,

        MSETransportSeat,

        MSETransportVehicleId,

        MSETransportGuidByte6,

        MSETransportGuidByte0,

        MSETransportPositionZ,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEOptionalCounter,

        MSEOrientation,

        MSEPitch,

        MSETimestamp,

        MSESplineElevation,

        MSEEnd,
    };

}

// Cataclysm 4.3.4 speed and collision height movement sequences, from TrinityCore 4.3.4
// (Movement/MovementStructures.cpp).
namespace
{
    MovementStatusElements const MoveSetWalkSpeed[] =
    {
        MSEHasGuidByte6,
        MSEHasGuidByte7,
        MSEHasGuidByte3,
        MSEHasGuidByte1,
        MSEHasGuidByte2,
        MSEHasGuidByte0,
        MSEHasGuidByte4,
        MSEHasGuidByte5,
        MSEGuidByte5,
        MSEGuidByte6,
        MSECounter,
        MSEGuidByte4,
        MSEExtraElement,
        MSEGuidByte2,
        MSEGuidByte3,
        MSEGuidByte0,
        MSEGuidByte1,
        MSEGuidByte7,
        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetWalkSpeed[] =
    {

        MSEHasGuidByte4,

        MSEHasGuidByte1,

        MSEHasGuidByte7,

        MSEHasGuidByte6,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEHasGuidByte5,

        MSEHasGuidByte0,

        MSEGuidByte2,

        MSEGuidByte3,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEGuidByte6,

        MSEGuidByte5,

        MSEGuidByte4,

        MSEGuidByte7,

        MSEExtraElement,

        MSEEnd,
    };

    MovementStatusElements const MoveSetRunSpeed[] =
    {

        MSEHasGuidByte1,

        MSEHasGuidByte7,

        MSEHasGuidByte4,

        MSEHasGuidByte2,

        MSEHasGuidByte5,

        MSEHasGuidByte3,

        MSEHasGuidByte6,

        MSEHasGuidByte0,

        MSEGuidByte1,

        MSECounter,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEGuidByte0,

        MSEExtraElement,

        MSEGuidByte2,

        MSEGuidByte4,

        MSEGuidByte6,

        MSEGuidByte5,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetRunSpeed[] =
    {

        MSEHasGuidByte3,

        MSEHasGuidByte0,

        MSEHasGuidByte1,

        MSEHasGuidByte4,

        MSEHasGuidByte7,

        MSEHasGuidByte5,

        MSEHasGuidByte6,

        MSEHasGuidByte2,

        MSEGuidByte4,

        MSEExtraElement,

        MSEGuidByte1,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEGuidByte7,

        MSEGuidByte6,

        MSEGuidByte2,

        MSEGuidByte0,

        MSEEnd,
    };

    MovementStatusElements const MoveSetRunBackSpeed[] =
    {

        MSEHasGuidByte7,

        MSEHasGuidByte1,

        MSEHasGuidByte0,

        MSEHasGuidByte2,

        MSEHasGuidByte4,

        MSEHasGuidByte3,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSECounter,

        MSEGuidByte0,

        MSEGuidByte3,

        MSEGuidByte7,

        MSEGuidByte5,

        MSEGuidByte2,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEExtraElement,

        MSEGuidByte6,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetRunBackSpeed[] =
    {

        MSEHasGuidByte7,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEHasGuidByte5,

        MSEHasGuidByte6,

        MSEHasGuidByte1,

        MSEGuidByte6,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEGuidByte5,

        MSEGuidByte2,

        MSEGuidByte3,

        MSEGuidByte7,

        MSEExtraElement,

        MSEGuidByte0,

        MSEEnd,
    };

    MovementStatusElements const MoveSetSwimSpeed[] =
    {

        MSEHasGuidByte5,

        MSEHasGuidByte0,

        MSEHasGuidByte6,

        MSEHasGuidByte3,

        MSEHasGuidByte7,

        MSEHasGuidByte2,

        MSEHasGuidByte4,

        MSEHasGuidByte1,

        MSECounter,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEExtraElement,

        MSEGuidByte6,

        MSEGuidByte7,

        MSEGuidByte0,

        MSEGuidByte5,

        MSEGuidByte2,

        MSEGuidByte4,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetSwimSpeed[] =
    {

        MSEHasGuidByte5,

        MSEHasGuidByte6,

        MSEHasGuidByte7,

        MSEHasGuidByte3,

        MSEHasGuidByte4,

        MSEHasGuidByte2,

        MSEHasGuidByte1,

        MSEHasGuidByte0,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEGuidByte6,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEExtraElement,

        MSEGuidByte5,

        MSEGuidByte0,

        MSEGuidByte2,

        MSEEnd,
    };

    MovementStatusElements const MoveSetSwimBackSpeed[] =
    {

        MSEHasGuidByte5,

        MSEHasGuidByte0,

        MSEHasGuidByte4,

        MSEHasGuidByte2,

        MSEHasGuidByte1,

        MSEHasGuidByte3,

        MSEHasGuidByte6,

        MSEHasGuidByte7,

        MSEGuidByte5,

        MSEGuidByte6,

        MSEGuidByte0,

        MSEGuidByte4,

        MSECounter,

        MSEExtraElement,

        MSEGuidByte1,

        MSEGuidByte7,

        MSEGuidByte2,

        MSEGuidByte3,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetSwimBackSpeed[] =
    {

        MSEHasGuidByte2,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte0,

        MSEHasGuidByte4,

        MSEHasGuidByte3,

        MSEHasGuidByte1,

        MSEHasGuidByte7,

        MSEGuidByte7,

        MSEGuidByte6,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEGuidByte2,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEExtraElement,

        MSEEnd,
    };

    MovementStatusElements const MoveSetTurnRate[] =
    {

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte1,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasGuidByte7,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEExtraElement,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEGuidByte5,

        MSEGuidByte0,

        MSEGuidByte4,

        MSEGuidByte6,

        MSEGuidByte2,

        MSECounter,

        MSEGuidByte1,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetTurnRate[] =
    {

        MSEHasGuidByte5,

        MSEHasGuidByte7,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasGuidByte1,

        MSEHasGuidByte6,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEGuidByte1,

        MSEGuidByte7,

        MSEExtraElement,

        MSEGuidByte6,

        MSEGuidByte0,

        MSEGuidByte4,

        MSEGuidByte2,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEEnd,
    };

    MovementStatusElements const MoveSetFlightSpeed[] =
    {

        MSEExtraElement,

        MSECounter,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte0,

        MSEHasGuidByte4,

        MSEHasGuidByte1,

        MSEHasGuidByte7,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEGuidByte0,

        MSEGuidByte7,

        MSEGuidByte4,

        MSEGuidByte5,

        MSEGuidByte6,

        MSEGuidByte2,

        MSEGuidByte3,

        MSEGuidByte1,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetFlightSpeed[] =
    {

        MSEExtraElement,

        MSEHasGuidByte1,

        MSEHasGuidByte4,

        MSEHasGuidByte7,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte0,

        MSEGuidByte5,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEGuidByte6,

        MSEGuidByte2,

        MSEGuidByte4,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEEnd,
    };

    MovementStatusElements const MoveSetFlightBackSpeed[] =
    {

        MSEHasGuidByte2,

        MSEHasGuidByte7,

        MSEHasGuidByte6,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasGuidByte1,

        MSEHasGuidByte5,

        MSEHasGuidByte3,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEGuidByte6,

        MSEGuidByte0,

        MSEGuidByte2,

        MSECounter,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEGuidByte5,

        MSEExtraElement,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetFlightBackSpeed[] =
    {

        MSEHasGuidByte6,

        MSEHasGuidByte0,

        MSEHasGuidByte2,

        MSEHasGuidByte7,

        MSEHasGuidByte5,

        MSEHasGuidByte4,

        MSEHasGuidByte3,

        MSEHasGuidByte1,

        MSEGuidByte7,

        MSEGuidByte6,

        MSEGuidByte4,

        MSEExtraElement,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEGuidByte2,

        MSEGuidByte0,

        MSEGuidByte5,

        MSEEnd,
    };

    MovementStatusElements const MoveSetPitchRate[] =
    {

        MSEHasGuidByte7,

        MSEHasGuidByte5,

        MSEHasGuidByte4,

        MSEHasGuidByte1,

        MSEHasGuidByte6,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEHasGuidByte0,

        MSEGuidByte4,

        MSECounter,

        MSEGuidByte2,

        MSEGuidByte5,

        MSEGuidByte6,

        MSEGuidByte1,

        MSEExtraElement,

        MSEGuidByte7,

        MSEGuidByte3,

        MSEGuidByte0,

        MSEEnd,
    };

    MovementStatusElements const SplineMoveSetPitchRate[] =
    {

        MSEHasGuidByte2,

        MSEHasGuidByte6,

        MSEHasGuidByte0,

        MSEHasGuidByte5,

        MSEHasGuidByte1,

        MSEHasGuidByte3,

        MSEHasGuidByte7,

        MSEHasGuidByte4,

        MSEGuidByte5,

        MSEExtraElement,

        MSEGuidByte4,

        MSEGuidByte0,

        MSEGuidByte3,

        MSEGuidByte6,

        MSEGuidByte1,

        MSEGuidByte2,

        MSEGuidByte7,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateWalkSpeed[] =
    {

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasSplineElevation,

        MSEHasCounter,

        MSEHasMovementFlags,

        MSEHasGuidByte7,

        MSEHasGuidByte5,

        MSEHasMovementFlags2,

        MSEHasGuidByte6,

        MSEZeroBit,

        MSEHasGuidByte3,

        MSEForcesCount,

        MSEHasPitch,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEMovementFlags,

        MSEHasGuidByte2,

        MSEMovementFlags2,

        MSEHasFallData,

        MSEHasTimestamp,

        MSEHasTransportData,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte0,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte3,

        MSEHasVehicleId,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte6,

        MSEHasFallDirection,

        MSEHasOrientation,

        MSEZeroBit,

        MSEPitch,

        MSEExtraElement,

        MSETransportGuidByte3,

        MSETransportTime2,

        MSETransportPositionY,

        MSETransportGuidByte1,

        MSETransportPositionX,

        MSETransportGuidByte6,

        MSETransportGuidByte4,

        MSETransportGuidByte5,

        MSETransportGuidByte0,

        MSETransportTime,

        MSETransportSeat,

        MSETransportGuidByte2,

        MSETransportVehicleId,

        MSETransportOrientation,

        MSETransportGuidByte7,

        MSETransportPositionZ,

        MSEGuidByte2,

        MSESplineElevation,

        MSEGuidByte1,

        MSETimestamp,

        MSEGuidByte4,

        MSEPositionY,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSEFallHorizontalSpeed,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEGuidByte5,

        MSEPositionZ,

        MSEGuidByte7,

        MSEPositionX,

        MSEOptionalCounter,

        MSEForces,

        MSEGuidByte3,

        MSEGuidByte6,

        MSEOrientation,

        MSEGuidByte0,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateRunSpeed[] =
    {

        MSEHasGuidByte0,

        MSEHasGuidByte3,

        MSEHasTimestamp,

        MSEHasGuidByte7,

        MSEHasGuidByte4,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEHasPitch,

        MSEHasGuidByte2,

        MSEHasGuidByte5,

        MSEHasCounter,

        MSEHasGuidByte6,

        MSEHasMovementFlags,

        MSEForcesCount,

        MSEMovementFlags,

        MSEHasMovementFlags2,

        MSEMovementFlags2,

        MSEHasTransportData,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte2,

        MSEHasVehicleId,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte4,

        MSEHasSplineElevation,

        MSEHasOrientation,

        MSEHasFallData,

        MSEZeroBit,

        MSEHasFallDirection,

        MSEZeroBit,

        MSEFallVerticalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallTime,

        MSETransportGuidByte4,

        MSETransportGuidByte2,

        MSETransportSeat,

        MSETransportPositionZ,

        MSETransportGuidByte5,

        MSETransportGuidByte7,

        MSETransportGuidByte3,

        MSETransportPositionY,

        MSETransportTime2,

        MSETransportGuidByte6,

        MSETransportTime,

        MSETransportOrientation,

        MSETransportVehicleId,

        MSETransportPositionX,

        MSETransportGuidByte0,

        MSETransportGuidByte1,

        MSEPositionX,

        MSESplineElevation,

        MSEGuidByte1,

        MSEGuidByte2,

        MSEGuidByte6,

        MSEOptionalCounter,

        MSEGuidByte0,

        MSETimestamp,

        MSEGuidByte3,

        MSEGuidByte5,

        MSEForces,

        MSEPitch,

        MSEGuidByte7,

        MSEOrientation,

        MSEPositionZ,

        MSEPositionY,

        MSEExtraElement,

        MSEGuidByte4,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateRunBackSpeed[] =
    {

        MSEPositionZ,

        MSEPositionY,

        MSEPositionX,

        MSEExtraElement,

        MSEHasMovementFlags2,

        MSEHasGuidByte2,

        MSEHasMovementFlags,

        MSEMovementFlags,

        MSEHasTransportData,

        MSEHasVehicleId,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte7,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte2,

        MSEHasTimestamp,

        MSEHasGuidByte3,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasOrientation,

        MSEHasPitch,

        MSEForcesCount,

        MSEMovementFlags2,

        MSEZeroBit,

        MSEHasGuidByte7,

        MSEZeroBit,

        MSEHasSplineElevation,

        MSEHasFallData,

        MSEHasFallDirection,

        MSEHasCounter,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEGuidByte5,

        MSETransportGuidByte1,

        MSETransportGuidByte6,

        MSETransportVehicleId,

        MSETransportGuidByte2,

        MSETransportPositionZ,

        MSETransportGuidByte5,

        MSETransportGuidByte7,

        MSETransportOrientation,

        MSETransportTime2,

        MSETransportTime,

        MSETransportPositionY,

        MSETransportGuidByte4,

        MSETransportGuidByte0,

        MSETransportSeat,

        MSETransportGuidByte3,

        MSETransportPositionX,

        MSESplineElevation,

        MSEGuidByte2,

        MSETimestamp,

        MSEForces,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallCosAngle,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEGuidByte6,

        MSEPitch,

        MSEGuidByte4,

        MSEGuidByte7,

        MSEOptionalCounter,

        MSEGuidByte0,

        MSEOrientation,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateSwimSpeed[] =
    {

        MSEHasOrientation,

        MSEHasGuidByte0,

        MSEZeroBit,

        MSEZeroBit,

        MSEHasPitch,

        MSEHasGuidByte6,

        MSEZeroBit,

        MSEHasCounter,

        MSEHasFallData,

        MSEHasTimestamp,

        MSEHasMovementFlags,

        MSEHasSplineElevation,

        MSEHasTransportData,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte2,

        MSEHasVehicleId,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte0,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte1,

        MSEMovementFlags,

        MSEHasFallDirection,

        MSEHasGuidByte4,

        MSEHasGuidByte2,

        MSEHasMovementFlags2,

        MSEForcesCount,

        MSEHasGuidByte3,

        MSEMovementFlags2,

        MSEHasGuidByte7,

        MSEHasGuidByte5,

        MSEHasGuidByte1,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSEExtraElement,

        MSETransportPositionX,

        MSETransportSeat,

        MSETransportGuidByte3,

        MSETransportPositionY,

        MSETransportGuidByte6,

        MSETransportGuidByte2,

        MSETransportGuidByte0,

        MSETransportGuidByte1,

        MSETransportPositionZ,

        MSETransportVehicleId,

        MSETransportTime,

        MSETransportGuidByte4,

        MSETransportOrientation,

        MSETransportGuidByte5,

        MSETransportGuidByte7,

        MSETransportTime2,

        MSEPositionX,

        MSEOptionalCounter,

        MSEGuidByte6,

        MSEGuidByte0,

        MSESplineElevation,

        MSEGuidByte5,

        MSEOrientation,

        MSEPitch,

        MSETimestamp,

        MSEPositionY,

        MSEGuidByte1,

        MSEForces,

        MSEGuidByte3,

        MSEPositionZ,

        MSEGuidByte4,

        MSEGuidByte7,

        MSEGuidByte2,

        MSEEnd,
    };

    MovementStatusElements const MoveUpdateSwimBackSpeed[] =
    {

        MSEHasGuidByte3,

        MSEHasGuidByte6,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEHasMovementFlags2,

        MSEHasOrientation,

        MSEForcesCount,

        MSEHasGuidByte7,

        MSEHasTimestamp,

        MSEZeroBit,

        MSEMovementFlags2,

        MSEHasFallData,

        MSEHasMovementFlags,

        MSEHasGuidByte5,

        MSEMovementFlags,

        MSEHasSplineElevation,

        MSEHasCounter,

        MSEZeroBit,

        MSEHasTransportData,

        MSEHasPitch,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte4,

        MSEHasVehicleId,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte2,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte1,

        MSEHasGuidByte2,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasFallDirection,

        MSEFallVerticalSpeed,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallTime,

        MSEGuidByte0,

        MSETransportTime2,

        MSETransportGuidByte7,

        MSETransportGuidByte2,

        MSETransportPositionY,

        MSETransportGuidByte4,

        MSETransportSeat,

        MSETransportGuidByte3,

        MSETransportGuidByte0,

        MSETransportVehicleId,

        MSETransportGuidByte1,

        MSETransportGuidByte5,

        MSETransportGuidByte6,

        MSETransportPositionX,

        MSETransportPositionZ,

        MSETransportOrientation,

        MSETransportTime,

        MSEExtraElement,

        MSEGuidByte1,

        MSEPositionX,

        MSEGuidByte4,

        MSEOrientation,

        MSESplineElevation,

        MSEGuidByte2,

        MSEGuidByte3,

        MSEPitch,

        MSEGuidByte7,

        MSEPositionZ,

        MSEForces,

        MSEPositionY,

        MSEOptionalCounter,

        MSEGuidByte5,

        MSETimestamp,

        MSEGuidByte6,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateFlightSpeed[] =
    {

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEHasTransportData,

        MSEHasOrientation,

        MSEHasSplineElevation,

        MSEHasGuidByte6,

        MSEHasGuidByte7,

        MSEZeroBit,

        MSEHasTransportGuidByte4,

        MSEHasVehicleId,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte0,

        MSEHasMovementFlags,

        MSEForcesCount,

        MSEZeroBit,

        MSEHasGuidByte0,

        MSEHasCounter,

        MSEHasFallData,

        MSEHasGuidByte5,

        MSEHasGuidByte4,

        MSEHasMovementFlags2,

        MSEHasPitch,

        MSEHasTimestamp,

        MSEHasFallDirection,

        MSEZeroBit,

        MSEMovementFlags,

        MSEMovementFlags2,

        MSEHasGuidByte1,

        MSEGuidByte5,

        MSEPositionZ,

        MSEGuidByte7,

        MSETransportPositionX,

        MSETransportGuidByte4,

        MSETransportGuidByte6,

        MSETransportGuidByte0,

        MSETransportGuidByte2,

        MSETransportGuidByte5,

        MSETransportTime2,

        MSETransportGuidByte1,

        MSETransportPositionZ,

        MSETransportVehicleId,

        MSETransportGuidByte7,

        MSETransportTime,

        MSETransportGuidByte3,

        MSETransportPositionY,

        MSETransportOrientation,

        MSETransportSeat,

        MSEGuidByte0,

        MSEGuidByte2,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEPositionX,

        MSESplineElevation,

        MSEGuidByte4,

        MSEForces,

        MSEPositionY,

        MSEGuidByte6,

        MSEOptionalCounter,

        MSETimestamp,

        MSEOrientation,

        MSEGuidByte1,

        MSEExtraElement,

        MSEPitch,

        MSEGuidByte3,

        MSEEnd,
    };

    MovementStatusElements const MoveUpdateFlightBackSpeed[] =
    {

        MSEHasGuidByte0,

        MSEZeroBit,

        MSEHasGuidByte2,

        MSEHasGuidByte1,

        MSEHasMovementFlags2,

        MSEZeroBit,

        MSEHasFallData,

        MSEHasCounter,

        MSEForcesCount,

        MSEHasSplineElevation,

        MSEHasTimestamp,

        MSEHasTransportData,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte1,

        MSEHasVehicleId,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte6,

        MSEHasMovementFlags,

        MSEZeroBit,

        MSEHasOrientation,

        MSEHasFallDirection,

        MSEHasGuidByte5,

        MSEHasGuidByte7,

        MSEHasGuidByte6,

        MSEHasPitch,

        MSEMovementFlags2,

        MSEHasGuidByte3,

        MSEMovementFlags,

        MSEHasGuidByte4,

        MSETransportPositionZ,

        MSETransportGuidByte4,

        MSETransportGuidByte1,

        MSETransportGuidByte0,

        MSETransportGuidByte5,

        MSETransportGuidByte3,

        MSETransportTime2,

        MSETransportGuidByte7,

        MSETransportOrientation,

        MSETransportVehicleId,

        MSETransportGuidByte6,

        MSETransportGuidByte2,

        MSETransportTime,

        MSETransportPositionX,

        MSETransportSeat,

        MSETransportPositionY,

        MSEFallHorizontalSpeed,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSESplineElevation,

        MSEGuidByte5,

        MSEPositionX,

        MSEGuidByte3,

        MSEGuidByte0,

        MSEForces,

        MSETimestamp,

        MSEPitch,

        MSEGuidByte2,

        MSEGuidByte1,

        MSEGuidByte6,

        MSEExtraElement,

        MSEOrientation,

        MSEOptionalCounter,

        MSEPositionY,

        MSEGuidByte7,

        MSEGuidByte4,

        MSEPositionZ,

        MSEEnd,
    };

    MovementStatusElements const MovementForceWalkSpeedChangeAck[] =
    {

        MSECounter,

        MSEExtraElement,

        MSEPositionX,

        MSEPositionY,

        MSEPositionZ,

        MSEZeroBit,

        MSEHasGuidByte0,

        MSEHasCounter,

        MSEHasOrientation,

        MSEHasSplineElevation,

        MSEHasPitch,

        MSEHasMovementFlags,

        MSEHasGuidByte3,

        MSEForcesCount,

        MSEHasFallData,

        MSEHasMovementFlags2,

        MSEHasTimestamp,

        MSEHasGuidByte2,

        MSEHasGuidByte1,

        MSEHasGuidByte7,

        MSEHasGuidByte4,

        MSEZeroBit,

        MSEZeroBit,

        MSEHasTransportData,

        MSEHasGuidByte5,

        MSEHasGuidByte6,

        MSEMovementFlags2,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte3,

        MSEHasVehicleId,

        MSEHasTransportGuidByte5,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte6,

        MSEMovementFlags,

        MSEHasFallDirection,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEGuidByte7,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEGuidByte4,

        MSEForces,

        MSEGuidByte2,

        MSEGuidByte6,

        MSESplineElevation,

        MSETransportGuidByte5,

        MSETransportPositionX,

        MSETransportGuidByte3,

        MSETransportOrientation,

        MSETransportSeat,

        MSETransportPositionZ,

        MSETransportGuidByte1,

        MSETransportTime,

        MSETransportGuidByte7,

        MSETransportGuidByte2,

        MSETransportGuidByte6,

        MSETransportPositionY,

        MSETransportGuidByte0,

        MSETransportGuidByte4,

        MSETransportTime2,

        MSETransportVehicleId,

        MSETimestamp,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSEOrientation,

        MSEOptionalCounter,

        MSEPitch,

        MSEEnd,
    };

    MovementStatusElements const MovementForceRunSpeedChangeAck[] =
    {

        MSECounter,

        MSEPositionY,

        MSEPositionZ,

        MSEPositionX,

        MSEExtraElement,

        MSEHasMovementFlags2,

        MSEHasCounter,

        MSEHasGuidByte6,

        MSEHasTimestamp,

        MSEHasFallData,

        MSEHasGuidByte5,

        MSEHasOrientation,

        MSEZeroBit,

        MSEHasGuidByte3,

        MSEHasGuidByte2,

        MSEHasGuidByte1,

        MSEZeroBit,

        MSEHasGuidByte4,

        MSEHasTransportData,

        MSEHasGuidByte7,

        MSEHasGuidByte0,

        MSEForcesCount,

        MSEHasMovementFlags,

        MSEHasPitch,

        MSEHasSplineElevation,

        MSEZeroBit,

        MSEHasVehicleId,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte7,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte5,

        MSEHasFallDirection,

        MSEMovementFlags,

        MSEMovementFlags2,

        MSEGuidByte0,

        MSEGuidByte4,

        MSEGuidByte2,

        MSEGuidByte6,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEGuidByte7,

        MSEGuidByte5,

        MSEForces,

        MSEOptionalCounter,

        MSESplineElevation,

        MSETransportGuidByte2,

        MSETransportSeat,

        MSETransportVehicleId,

        MSETransportGuidByte1,

        MSETransportGuidByte6,

        MSETransportGuidByte3,

        MSETransportPositionZ,

        MSETransportPositionX,

        MSETransportGuidByte7,

        MSETransportOrientation,

        MSETransportGuidByte0,

        MSETransportPositionY,

        MSETransportTime2,

        MSETransportGuidByte5,

        MSETransportTime,

        MSETransportGuidByte4,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEOrientation,

        MSETimestamp,

        MSEPitch,

        MSEEnd,
    };

    MovementStatusElements const MovementForceRunBackSpeedChangeAck[] =
    {

        MSEExtraElement,

        MSECounter,

        MSEPositionY,

        MSEPositionX,

        MSEPositionZ,

        MSEZeroBit,

        MSEHasGuidByte7,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEForcesCount,

        MSEHasSplineElevation,

        MSEHasMovementFlags,

        MSEHasMovementFlags2,

        MSEHasGuidByte3,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte4,

        MSEZeroBit,

        MSEHasOrientation,

        MSEHasFallData,

        MSEHasPitch,

        MSEHasCounter,

        MSEHasTimestamp,

        MSEHasTransportData,

        MSEHasGuidByte2,

        MSEHasGuidByte0,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte7,

        MSEHasVehicleId,

        MSEMovementFlags,

        MSEMovementFlags2,

        MSEHasFallDirection,

        MSEGuidByte4,

        MSEForces,

        MSEGuidByte2,

        MSEGuidByte0,

        MSEGuidByte7,

        MSEGuidByte5,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEGuidByte6,

        MSETransportTime2,

        MSETransportPositionZ,

        MSETransportGuidByte3,

        MSETransportGuidByte6,

        MSETransportGuidByte2,

        MSETransportSeat,

        MSETransportGuidByte7,

        MSETransportTime,

        MSETransportOrientation,

        MSETransportGuidByte0,

        MSETransportPositionX,

        MSETransportGuidByte5,

        MSETransportGuidByte4,

        MSETransportPositionY,

        MSETransportGuidByte1,

        MSETransportVehicleId,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEPitch,

        MSESplineElevation,

        MSEOptionalCounter,

        MSEOrientation,

        MSETimestamp,

        MSEEnd,
    };

    MovementStatusElements const MovementForceSwimSpeedChangeAck[] =
    {

        MSEExtraElement,

        MSEPositionY,

        MSECounter,

        MSEPositionZ,

        MSEPositionX,

        MSEHasGuidByte4,

        MSEZeroBit,

        MSEHasSplineElevation,

        MSEHasGuidByte2,

        MSEHasMovementFlags2,

        MSEHasGuidByte5,

        MSEHasGuidByte3,

        MSEHasMovementFlags,

        MSEHasGuidByte0,

        MSEHasPitch,

        MSEHasCounter,

        MSEHasOrientation,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEHasFallData,

        MSEForcesCount,

        MSEHasTimestamp,

        MSEHasGuidByte7,

        MSEHasGuidByte6,

        MSEHasTransportData,

        MSEZeroBit,

        MSEMovementFlags2,

        MSEHasFallDirection,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte7,

        MSEHasVehicleId,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte0,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte5,

        MSEMovementFlags,

        MSEGuidByte0,

        MSEGuidByte4,

        MSEGuidByte5,

        MSEGuidByte6,

        MSEForces,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEGuidByte7,

        MSEGuidByte2,

        MSETransportGuidByte7,

        MSETransportTime2,

        MSETransportSeat,

        MSETransportVehicleId,

        MSETransportGuidByte4,

        MSETransportPositionY,

        MSETransportPositionZ,

        MSETransportGuidByte0,

        MSETransportGuidByte6,

        MSETransportGuidByte3,

        MSETransportGuidByte2,

        MSETransportOrientation,

        MSETransportTime,

        MSETransportGuidByte5,

        MSETransportGuidByte1,

        MSETransportPositionX,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSETimestamp,

        MSESplineElevation,

        MSEOptionalCounter,

        MSEPitch,

        MSEOrientation,

        MSEEnd,
    };

    MovementStatusElements const MoveForceSwimBackSpeedChangeAck[] =
    {

        MSEExtraElement,

        MSEPositionY,

        MSECounter,

        MSEPositionZ,

        MSEPositionX,

        MSEHasGuidByte5,

        MSEForcesCount,

        MSEHasTransportData,

        MSEHasCounter,

        MSEHasMovementFlags,

        MSEHasGuidByte7,

        MSEHasOrientation,

        MSEHasGuidByte4,

        MSEHasSplineElevation,

        MSEZeroBit,

        MSEHasGuidByte6,

        MSEHasGuidByte2,

        MSEHasMovementFlags2,

        MSEZeroBit,

        MSEHasGuidByte0,

        MSEHasTimestamp,

        MSEHasGuidByte3,

        MSEHasFallData,

        MSEZeroBit,

        MSEHasPitch,

        MSEHasGuidByte1,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte5,

        MSEHasVehicleId,

        MSEHasTransportGuidByte6,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte4,

        MSEHasFallDirection,

        MSEMovementFlags,

        MSEMovementFlags2,

        MSEForces,

        MSEGuidByte5,

        MSEGuidByte0,

        MSEGuidByte6,

        MSEGuidByte4,

        MSEGuidByte3,

        MSEGuidByte2,

        MSEGuidByte1,

        MSEGuidByte7,

        MSETransportGuidByte7,

        MSETransportPositionY,

        MSETransportGuidByte2,

        MSETransportGuidByte3,

        MSETransportGuidByte4,

        MSETransportGuidByte0,

        MSETransportOrientation,

        MSETransportVehicleId,

        MSETransportPositionX,

        MSETransportTime2,

        MSETransportPositionZ,

        MSETransportSeat,

        MSETransportGuidByte6,

        MSETransportTime,

        MSETransportGuidByte5,

        MSETransportGuidByte1,

        MSEOptionalCounter,

        MSESplineElevation,

        MSEFallVerticalSpeed,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallTime,

        MSETimestamp,

        MSEOrientation,

        MSEPitch,

        MSEEnd,
    };

    MovementStatusElements const MovementForceFlightSpeedChangeAck[] =
    {

        MSEPositionY,

        MSECounter,

        MSEPositionX,

        MSEPositionZ,

        MSEExtraElement,

        MSEHasTimestamp,

        MSEHasCounter,

        MSEHasGuidByte6,

        MSEHasGuidByte4,

        MSEHasTransportData,

        MSEHasOrientation,

        MSEHasMovementFlags2,

        MSEHasGuidByte3,

        MSEForcesCount,

        MSEHasGuidByte7,

        MSEHasGuidByte2,

        MSEHasSplineElevation,

        MSEZeroBit,

        MSEHasGuidByte0,

        MSEHasGuidByte5,

        MSEHasGuidByte1,

        MSEZeroBit,

        MSEHasFallData,

        MSEHasMovementFlags,

        MSEHasPitch,

        MSEZeroBit,

        MSEHasTransportGuidByte7,

        MSEHasVehicleId,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte2,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte6,

        MSEMovementFlags,

        MSEMovementFlags2,

        MSEHasFallDirection,

        MSEGuidByte1,

        MSEGuidByte3,

        MSEGuidByte7,

        MSEGuidByte4,

        MSEGuidByte2,

        MSEForces,

        MSEGuidByte6,

        MSEGuidByte5,

        MSEGuidByte0,

        MSETransportOrientation,

        MSETransportTime,

        MSETransportGuidByte0,

        MSETransportGuidByte7,

        MSETransportPositionY,

        MSETransportPositionZ,

        MSETransportVehicleId,

        MSETransportGuidByte1,

        MSETransportGuidByte5,

        MSETransportGuidByte4,

        MSETransportGuidByte3,

        MSETransportGuidByte6,

        MSETransportSeat,

        MSETransportPositionX,

        MSETransportTime2,

        MSETransportGuidByte2,

        MSESplineElevation,

        MSEPitch,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallTime,

        MSEFallVerticalSpeed,

        MSETimestamp,

        MSEOrientation,

        MSEOptionalCounter,

        MSEEnd,
    };

    MovementStatusElements const MoveForceFlightBackSpeedChangeAck[] =
    {

        MSEExtraElement,

        MSEPositionZ,

        MSECounter,

        MSEPositionX,

        MSEPositionY,

        MSEHasTransportData,

        MSEHasMovementFlags,

        MSEHasOrientation,

        MSEHasCounter,

        MSEHasSplineElevation,

        MSEZeroBit,

        MSEHasPitch,

        MSEHasGuidByte6,

        MSEHasFallData,

        MSEForcesCount,

        MSEHasMovementFlags2,

        MSEHasGuidByte2,

        MSEZeroBit,

        MSEHasGuidByte5,

        MSEHasGuidByte4,

        MSEHasGuidByte3,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEHasTimestamp,

        MSEHasGuidByte0,

        MSEHasGuidByte7,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte4,

        MSEHasVehicleId,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte3,

        MSEMovementFlags2,

        MSEMovementFlags,

        MSEHasFallDirection,

        MSEForces,

        MSEGuidByte7,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEGuidByte6,

        MSEGuidByte4,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEGuidByte2,

        MSETransportGuidByte7,

        MSETransportPositionX,

        MSETransportPositionY,

        MSETransportOrientation,

        MSETransportGuidByte0,

        MSETransportGuidByte1,

        MSETransportSeat,

        MSETransportGuidByte4,

        MSETransportTime2,

        MSETransportPositionZ,

        MSETransportTime,

        MSETransportGuidByte6,

        MSETransportGuidByte3,

        MSETransportGuidByte5,

        MSETransportVehicleId,

        MSETransportGuidByte2,

        MSEFallCosAngle,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEPitch,

        MSEOptionalCounter,

        MSETimestamp,

        MSESplineElevation,

        MSEOrientation,

        MSEEnd,
    };

    MovementStatusElements const MovementSetCollisionHeight[] =
    {

        MSEHasGuidByte7,

        MSEHasGuidByte0,

        MSEHasGuidByte1,

        MSEHasMountDisplayId,

        MSEHasGuidByte3,

        MSEZeroBit,

        MSEZeroBit,

        MSEHasGuidByte2,

        MSEHasGuidByte6,

        MSEHasGuidByte5,

        MSEHasGuidByte4,

        MSEExtraElement,

        MSEMountDisplayIdWithCheck,

        MSEGuidByte3,

        MSEGuidByte2,

        MSEGuidByte5,

        MSEGuidByte6,

        MSECounter,

        MSEExtraElement,

        MSEGuidByte7,

        MSEGuidByte1,

        MSEGuidByte4,

        MSEGuidByte0,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateCollisionHeight[] =
    {

        MSEHasGuidByte7,

        MSEHasGuidByte3,

        MSEHasMovementFlags2,

        MSEZeroBit,

        MSEHasGuidByte4,

        MSEHasGuidByte2,

        MSEHasOrientation,

        MSEHasGuidByte5,

        MSEZeroBit,

        MSEHasSplineElevation,

        MSEHasMovementFlags,

        MSEHasTimestamp,

        MSEHasPitch,

        MSEHasGuidByte6,

        MSEHasGuidByte1,

        MSEHasCounter,

        MSEMovementFlags,

        MSEZeroBit,

        MSEForcesCount,

        MSEHasFallData,

        MSEHasFallDirection,

        MSEHasTransportData,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte3,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte4,

        MSEHasVehicleId,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte0,

        MSEHasGuidByte0,

        MSEMovementFlags2,

        MSETransportOrientation,

        MSETransportGuidByte7,

        MSETransportPositionZ,

        MSETransportSeat,

        MSETransportGuidByte3,

        MSETransportTime,

        MSETransportVehicleId,

        MSETransportGuidByte6,

        MSETransportGuidByte1,

        MSETransportGuidByte2,

        MSETransportPositionY,

        MSETransportGuidByte0,

        MSETransportTime2,

        MSETransportGuidByte4,

        MSETransportGuidByte5,

        MSETransportPositionX,

        MSESplineElevation,

        MSEPositionZ,

        MSEPitch,

        MSEExtraElement,

        MSEGuidByte5,

        MSEGuidByte7,

        MSEGuidByte4,

        MSEGuidByte3,

        MSEOptionalCounter,

        MSEPositionX,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEOrientation,

        MSEGuidByte1,

        MSEPositionY,

        MSEForces,

        MSEGuidByte0,

        MSEExtraElement,

        MSEGuidByte6,

        MSETimestamp,

        MSEGuidByte2,

        MSEEnd,
    };

    // Movement update embedded in CMSG_CAST_SPELL, CMSG_USE_ITEM and the pet cast packet
    // when the cast carries a missile trajectory
    MovementStatusElements const SpellCastMovement[] =
    {
        MSEPositionZ,
        MSEPositionY,
        MSEPositionX,
        MSEHasFallData,
        MSEHasTimestamp,
        MSEHasOrientation,
        MSEZeroBit,
        MSEHasSpline,
        MSEHasGuidByte6,
        MSEHasGuidByte4,
        MSEHasMovementFlags2,
        MSEHasGuidByte3,
        MSEHasGuidByte5,
        MSEHasSplineElevation,
        MSEHasPitch,
        MSEHasGuidByte7,
        MSEHasTransportData,
        MSEHasGuidByte2,
        MSEHasMovementFlags,
        MSEHasGuidByte1,
        MSEHasGuidByte0,
        MSEHasTransportGuidByte6,
        MSEHasTransportGuidByte2,
        MSEHasTransportGuidByte5,
        MSEHasTransportTime2,
        MSEHasTransportGuidByte7,
        MSEHasTransportGuidByte4,
        MSEHasVehicleId,
        MSEHasTransportGuidByte0,
        MSEHasTransportGuidByte1,
        MSEHasTransportGuidByte3,
        MSEMovementFlags2,
        MSEMovementFlags,
        MSEHasFallDirection,
        MSEGuidByte1,
        MSEGuidByte4,
        MSEGuidByte7,
        MSEGuidByte3,
        MSEGuidByte0,
        MSEGuidByte2,
        MSEGuidByte5,
        MSEGuidByte6,
        MSETransportSeat,
        MSETransportOrientation,
        MSETransportTime,
        MSETransportGuidByte6,
        MSETransportGuidByte5,
        MSETransportVehicleId,
        MSETransportPositionX,
        MSETransportGuidByte4,
        MSETransportPositionZ,
        MSETransportGuidByte2,
        MSETransportGuidByte0,
        MSETransportTime2,
        MSETransportGuidByte1,
        MSETransportGuidByte3,
        MSETransportPositionY,
        MSETransportGuidByte7,
        MSEOrientation,
        MSESplineElevation,
        MSEFallTime,
        MSEFallHorizontalSpeed,
        MSEFallSinAngle,
        MSEFallCosAngle,
        MSEFallVerticalSpeed,
        MSETimestamp,
        MSEPitch,
        MSEEnd,
    };

    MovementStatusElements const MovementKnockBackAck[] =
    {

        MSECounter,

        MSEPositionX,

        MSEPositionZ,

        MSEPositionY,

        MSEHasGuidByte5,

        MSEHasCounter,

        MSEHasTimestamp,

        MSEHasMovementFlags2,

        MSEZeroBit,

        MSEHasTransportData,

        MSEHasSplineElevation,

        MSEHasGuidByte3,

        MSEHasPitch,

        MSEForcesCount,

        MSEHasGuidByte1,

        MSEHasMovementFlags,

        MSEHasGuidByte7,

        MSEHasGuidByte6,

        MSEHasGuidByte4,

        MSEHasGuidByte0,

        MSEHasOrientation,

        MSEHasGuidByte2,

        MSEZeroBit,

        MSEHasFallData,

        MSEZeroBit,

        MSEMovementFlags,

        MSEHasTransportGuidByte2,

        MSEHasVehicleId,

        MSEHasTransportGuidByte3,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte0,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte7,

        MSEHasFallDirection,

        MSEMovementFlags2,

        MSEGuidByte4,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEGuidByte2,

        MSEGuidByte5,

        MSEGuidByte3,

        MSEForces,

        MSEGuidByte7,

        MSEGuidByte6,

        MSETransportVehicleId,

        MSETransportTime2,

        MSETransportTime,

        MSETransportPositionX,

        MSETransportPositionY,

        MSETransportGuidByte7,

        MSETransportSeat,

        MSETransportGuidByte1,

        MSETransportPositionZ,

        MSETransportOrientation,

        MSETransportGuidByte6,

        MSETransportGuidByte2,

        MSETransportGuidByte3,

        MSETransportGuidByte0,

        MSETransportGuidByte4,

        MSETransportGuidByte5,

        MSEFallHorizontalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSESplineElevation,

        MSEPitch,

        MSETimestamp,

        MSEOrientation,

        MSEOptionalCounter,

        MSEEnd,
    };

    MovementStatusElements const MovementUpdateKnockBack[] =
    {

        MSEHasGuidByte5,

        MSEHasSplineElevation,

        MSEHasTimestamp,

        MSEForcesCount,

        MSEHasMovementFlags2,

        MSEHasGuidByte2,

        MSEHasGuidByte4,

        MSEHasGuidByte6,

        MSEHasGuidByte1,

        MSEHasGuidByte0,

        MSEZeroBit,

        MSEHasOrientation,

        MSEZeroBit,

        MSEHasTransportData,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte7,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte4,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte3,

        MSEHasVehicleId,

        MSEHasGuidByte3,

        MSEHasFallData,

        MSEHasCounter,

        MSEHasFallDirection,

        MSEHasGuidByte7,

        MSEZeroBit,

        MSEHasPitch,

        MSEMovementFlags2,

        MSEHasMovementFlags,

        MSEMovementFlags,

        MSEGuidByte1,

        MSETransportGuidByte5,

        MSETransportVehicleId,

        MSETransportGuidByte3,

        MSETransportGuidByte1,

        MSETransportGuidByte4,

        MSETransportPositionZ,

        MSETransportGuidByte7,

        MSETransportGuidByte6,

        MSETransportGuidByte2,

        MSETransportPositionY,

        MSETransportGuidByte0,

        MSETransportSeat,

        MSETransportOrientation,

        MSETransportPositionX,

        MSETransportTime2,

        MSETransportTime,

        MSEGuidByte2,

        MSESplineElevation,

        MSEFallSinAngle,

        MSEFallHorizontalSpeed,

        MSEFallCosAngle,

        MSEFallVerticalSpeed,

        MSEFallTime,

        MSEPositionY,

        MSEOrientation,

        MSEForces,

        MSEGuidByte7,

        MSEGuidByte6,

        MSEGuidByte4,

        MSEPositionZ,

        MSEOptionalCounter,

        MSEGuidByte3,

        MSEGuidByte0,

        MSEPositionX,

        MSEPitch,

        MSEGuidByte5,

        MSETimestamp,

        MSEEnd,
    };

    MovementStatusElements const MovementSetCollisionHeightAck[] =
    {

        MSEMountDisplayIdWithoutCheck,

        MSEPositionZ,

        MSEExtraElement,

        MSEPositionY,

        MSECounter,

        MSEPositionX,

        MSEZeroBit,

        MSEForcesCount,

        MSEHasCounter,

        MSEExtraElement,

        MSEHasGuidByte5,

        MSEHasTimestamp,

        MSEHasSplineElevation,

        MSEHasGuidByte4,

        MSEHasGuidByte6,

        MSEHasGuidByte3,

        MSEHasTransportData,

        MSEHasFallData,

        MSEHasPitch,

        MSEHasOrientation,

        MSEHasMovementFlags,

        MSEHasMovementFlags2,

        MSEHasGuidByte0,

        MSEZeroBit,

        MSEHasGuidByte1,

        MSEZeroBit,

        MSEHasGuidByte7,

        MSEHasGuidByte2,

        MSEHasTransportGuidByte0,

        MSEHasTransportGuidByte6,

        MSEHasTransportGuidByte1,

        MSEHasTransportGuidByte4,

        MSEHasTransportGuidByte5,

        MSEHasTransportGuidByte2,

        MSEHasTransportGuidByte3,

        MSEHasVehicleId,

        MSEHasTransportTime2,

        MSEHasTransportGuidByte7,

        MSEMovementFlags,

        MSEHasFallDirection,

        MSEMovementFlags2,

        MSEGuidByte1,

        MSEGuidByte0,

        MSEForces,

        MSEGuidByte5,

        MSEGuidByte4,

        MSEGuidByte3,

        MSEGuidByte6,

        MSEGuidByte7,

        MSEGuidByte2,

        MSETransportTime,

        MSETransportPositionY,

        MSETransportPositionX,

        MSETransportGuidByte3,

        MSETransportSeat,

        MSETransportGuidByte5,

        MSETransportOrientation,

        MSETransportGuidByte1,

        MSETransportGuidByte4,

        MSETransportGuidByte0,

        MSETransportTime2,

        MSETransportVehicleId,

        MSETransportGuidByte6,

        MSETransportGuidByte7,

        MSETransportGuidByte2,

        MSETransportPositionZ,

        MSEFallVerticalSpeed,

        MSEFallSinAngle,

        MSEFallCosAngle,

        MSEFallHorizontalSpeed,

        MSEFallTime,

        MSEOrientation,

        MSEPitch,

        MSESplineElevation,

        MSETimestamp,

        MSEOptionalCounter,

        MSEEnd,
    };
}

void WorldPackets::Movement::ReadGroundMovement(WorldPacket& packet, MovementInfo& info,
    MovementStatusElements const* sequence, uint32* counter, ExtraMovementStatusElement* extras)
{
    info = MovementInfo();
    bool hasTransportData = false;
    bool hasVehicleId = false;
    bool hasTransportTime2 = false;
    bool hasMovementFlags = false;
    bool hasMovementFlags2 = false;
    bool hasTimestamp = false;
    bool hasOrientation = false;
    bool hasPitch = false;
    bool hasSplineElevation = false;
    bool hasSpline = false;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool hasCounter = false;
    bool counterRead = false;
    bool hasMountDisplayId = false;
    uint32 forcesCount = 0;

    for (; *sequence != MSEEnd; ++sequence)
    {
        switch (*sequence)
        {
            case MSEHasGuidByte0: info.guid[0] = packet.ReadBit(); break;
            case MSEHasGuidByte1: info.guid[1] = packet.ReadBit(); break;
            case MSEHasGuidByte2: info.guid[2] = packet.ReadBit(); break;
            case MSEHasGuidByte3: info.guid[3] = packet.ReadBit(); break;
            case MSEHasGuidByte4: info.guid[4] = packet.ReadBit(); break;
            case MSEHasGuidByte5: info.guid[5] = packet.ReadBit(); break;
            case MSEHasGuidByte6: info.guid[6] = packet.ReadBit(); break;
            case MSEHasGuidByte7: info.guid[7] = packet.ReadBit(); break;
            case MSEGuidByte0: packet.ReadByteSeq(info.guid[0]); break;
            case MSEGuidByte1: packet.ReadByteSeq(info.guid[1]); break;
            case MSEGuidByte2: packet.ReadByteSeq(info.guid[2]); break;
            case MSEGuidByte3: packet.ReadByteSeq(info.guid[3]); break;
            case MSEGuidByte4: packet.ReadByteSeq(info.guid[4]); break;
            case MSEGuidByte5: packet.ReadByteSeq(info.guid[5]); break;
            case MSEGuidByte6: packet.ReadByteSeq(info.guid[6]); break;
            case MSEGuidByte7: packet.ReadByteSeq(info.guid[7]); break;
            case MSEHasTransportGuidByte0: if (hasTransportData) info.transport.guid[0] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte1: if (hasTransportData) info.transport.guid[1] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte2: if (hasTransportData) info.transport.guid[2] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte3: if (hasTransportData) info.transport.guid[3] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte4: if (hasTransportData) info.transport.guid[4] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte5: if (hasTransportData) info.transport.guid[5] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte6: if (hasTransportData) info.transport.guid[6] = packet.ReadBit(); break;
            case MSEHasTransportGuidByte7: if (hasTransportData) info.transport.guid[7] = packet.ReadBit(); break;
            case MSETransportGuidByte0: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[0]); break;
            case MSETransportGuidByte1: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[1]); break;
            case MSETransportGuidByte2: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[2]); break;
            case MSETransportGuidByte3: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[3]); break;
            case MSETransportGuidByte4: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[4]); break;
            case MSETransportGuidByte5: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[5]); break;
            case MSETransportGuidByte6: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[6]); break;
            case MSETransportGuidByte7: if (hasTransportData) packet.ReadByteSeq(info.transport.guid[7]); break;
            case MSEHasTransportData: hasTransportData = packet.ReadBit(); break;
            case MSEHasVehicleId: if (hasTransportData) hasVehicleId = packet.ReadBit(); break;
            case MSEHasTransportTime2: if (hasTransportData) hasTransportTime2 = packet.ReadBit(); break;
            case MSEHasMovementFlags: hasMovementFlags = !packet.ReadBit(); break;
            case MSEHasMovementFlags2: hasMovementFlags2 = !packet.ReadBit(); break;
            case MSEHasTimestamp: hasTimestamp = !packet.ReadBit(); break;
            case MSEHasOrientation: hasOrientation = !packet.ReadBit(); break;
            case MSEHasPitch: hasPitch = !packet.ReadBit(); break;
            case MSEHasSplineElevation: hasSplineElevation = !packet.ReadBit(); break;
            case MSEHasSpline: hasSpline = packet.ReadBit(); break;
            case MSEHasFallData: hasFallData = packet.ReadBit(); break;
            case MSEHasFallDirection: if (hasFallData) hasFallDirection = packet.ReadBit(); break;
            case MSEZeroBit: packet.ReadBit(); break;
            case MSEMovementFlags:
                if (hasMovementFlags)
                {
                    uint32 flags = packet.ReadBits(30);
                    info.flags = (flags & 0x000001FF) | ((flags & 0x03FFFE00) << 1) | ((flags & 0x3C000000) << 2);
                }
                break;
            case MSEMovementFlags2:
                if (hasMovementFlags2)
                {
                    uint16 flags = packet.ReadBits(13);
                    info.flags2 = (flags & 0x03C3) | ((flags & 0x001C) << 1) | ((flags & 0x0020) >> 3) |
                        ((flags & 0xE000) >> 3) | ((flags & 0x1C00) << 3);
                }
                break;
            case MSEPositionX: packet >> info.pos.m_positionX; break;
            case MSEPositionY: packet >> info.pos.m_positionY; break;
            case MSEPositionZ: packet >> info.pos.m_positionZ; break;
            case MSEOrientation: if (hasOrientation) info.pos.SetOrientation(packet.read<float>()); break;
            case MSETransportPositionX: if (hasTransportData) packet >> info.transport.pos.m_positionX; break;
            case MSETransportPositionY: if (hasTransportData) packet >> info.transport.pos.m_positionY; break;
            case MSETransportPositionZ: if (hasTransportData) packet >> info.transport.pos.m_positionZ; break;
            case MSETransportOrientation: if (hasTransportData) info.transport.pos.SetOrientation(packet.read<float>()); break;
            case MSETransportSeat: if (hasTransportData) packet >> info.transport.seat; break;
            case MSETransportTime: if (hasTransportData) packet >> info.transport.time; break;
            case MSETransportTime2: if (hasTransportData && hasTransportTime2) packet >> info.transport.time2; break;
            case MSETransportVehicleId: if (hasTransportData && hasVehicleId) packet >> info.transport.vehicleId; break;
            case MSEPitch: if (hasPitch) info.pitch = G3D::wrap(packet.read<float>(), float(-M_PI), float(M_PI)); break;
            case MSEFallTime: if (hasFallData) packet >> info.fallTime; break;
            case MSEFallVerticalSpeed: if (hasFallData) packet >> info.jump.zspeed; break;
            case MSEFallCosAngle: if (hasFallData && hasFallDirection) packet >> info.jump.cosAngle; break;
            case MSEFallSinAngle: if (hasFallData && hasFallDirection) packet >> info.jump.sinAngle; break;
            case MSEFallHorizontalSpeed: if (hasFallData && hasFallDirection) packet >> info.jump.xyspeed; break;
            case MSESplineElevation: if (hasSplineElevation) packet >> info.splineElevation; break;
            case MSETimestamp: if (hasTimestamp) packet >> info.time; break;
            case MSECounter:
                if (counter)
                    packet >> *counter;
                else
                    packet.read_skip<uint32>();
                counterRead = true;
                break;
            case MSEExtraElement:
                extras->ReadNextElement(packet);
                break;
            case MSEForcesCount: forcesCount = packet.ReadBits(22); break;
            case MSEForces:
                for (uint32 i = 0; i < forcesCount; ++i)
                    packet.read_skip<uint32>();
                break;
            case MSEHasCounter: hasCounter = !packet.ReadBit(); break;
            case MSEOptionalCounter:
                // The movement counter. Acks without an order counter field (root, unroot) send it here.
                if (hasCounter)
                {
                    uint32 value = packet.read<uint32>();
                    if (counter && !counterRead)
                        *counter = value;
                }
                break;
            case MSEHasMountDisplayId: hasMountDisplayId = !packet.ReadBit(); break;
            case MSEMountDisplayIdWithCheck: if (hasMountDisplayId) packet.read_skip<uint32>(); break;
            case MSEMountDisplayIdWithoutCheck: packet.read_skip<uint32>(); break;
            case MSEOrientationWithoutCheck: info.pos.SetOrientation(packet.read<float>()); break;
            case MSEExtraFloat:
            case MSEExtraFloat2:
            case MSEExtraInt8:
            case MSEExtraInt32:
            case MSEExtraTwoBits:
                break; // only valid inside an ExtraMovementStatusElement sequence
            case MSEOneBit:
            case MSEHasHeightChangeFailed:
                packet.ReadBit();
                break;
            case MSEFlushBits:
                packet.ResetBitPos();
                break;
            case MSEEnd: break;
        }
    }

    if (hasTransportData)
        info.AddMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
    if (hasSpline)
        info.AddMovementFlag(MOVEMENTFLAG_SPLINE_ENABLED);
    if (hasTransportTime2)
        info.AddExtraMovementFlag(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT);

    if (packet.rpos() != packet.size())
        throw ByteBufferInvalidValueException("ground movement payload", "trailing bytes");
}

void WorldPackets::Movement::ExtraMovementStatusElement::ReadNextElement(WorldPacket& packet)
{
    MovementStatusElements const element = _elements[_index++];

    switch (element)
    {
        case MSEHasGuidByte0:
        case MSEHasGuidByte1:
        case MSEHasGuidByte2:
        case MSEHasGuidByte3:
        case MSEHasGuidByte4:
        case MSEHasGuidByte5:
        case MSEHasGuidByte6:
        case MSEHasGuidByte7:
            Data.guid[element - MSEHasGuidByte0] = packet.ReadBit();
            break;
        case MSEGuidByte0:
        case MSEGuidByte1:
        case MSEGuidByte2:
        case MSEGuidByte3:
        case MSEGuidByte4:
        case MSEGuidByte5:
        case MSEGuidByte6:
        case MSEGuidByte7:
            packet.ReadByteSeq(Data.guid[element - MSEGuidByte0]);
            break;
        case MSEExtraFloat:
            packet >> Data.floatData;
            break;
        case MSEExtraFloat2:
            packet >> Data.floatData2;
            break;
        case MSEExtraInt8:
            packet >> Data.byteData;
            break;
        case MSEExtraInt32:
            packet >> Data.extraInt32Data;
            break;
        case MSEExtraTwoBits:
            Data.byteData = packet.ReadBits(2);
            break;
        default:
            break;
    }
}

void WorldPackets::Movement::ExtraMovementStatusElement::WriteNextElement(WorldPacket& packet)
{
    MovementStatusElements const element = _elements[_index++];

    switch (element)
    {
        case MSEHasGuidByte0:
        case MSEHasGuidByte1:
        case MSEHasGuidByte2:
        case MSEHasGuidByte3:
        case MSEHasGuidByte4:
        case MSEHasGuidByte5:
        case MSEHasGuidByte6:
        case MSEHasGuidByte7:
            packet.WriteBit(Data.guid[element - MSEHasGuidByte0]);
            break;
        case MSEGuidByte0:
        case MSEGuidByte1:
        case MSEGuidByte2:
        case MSEGuidByte3:
        case MSEGuidByte4:
        case MSEGuidByte5:
        case MSEGuidByte6:
        case MSEGuidByte7:
            packet.FlushBits();
            packet.WriteByteSeq(Data.guid[element - MSEGuidByte0]);
            break;
        case MSEExtraFloat:
            packet.FlushBits();
            packet << Data.floatData;
            break;
        case MSEExtraFloat2:
            packet.FlushBits();
            packet << Data.floatData2;
            break;
        case MSEExtraInt8:
            packet.FlushBits();
            packet << Data.byteData;
            break;
        case MSEExtraInt32:
            packet.FlushBits();
            packet << Data.extraInt32Data;
            break;
        case MSEExtraTwoBits:
            packet.WriteBits(Data.byteData, 2);
            break;
        default:
            break;
    }
}

WorldPackets::Movement::SpeedChangeSequences const& WorldPackets::Movement::GetSpeedChangeSequences(uint8 moveType)
{
    // Indexed by UnitMoveType.
    static SpeedChangeSequences const sequences[MAX_MOVE_TYPE] =
    {
        { MoveSetWalkSpeed,       SplineMoveSetWalkSpeed,       MovementUpdateWalkSpeed,    MovementForceWalkSpeedChangeAck    },
        { MoveSetRunSpeed,        SplineMoveSetRunSpeed,        MovementUpdateRunSpeed,     MovementForceRunSpeedChangeAck     },
        { MoveSetRunBackSpeed,    SplineMoveSetRunBackSpeed,    MovementUpdateRunBackSpeed, MovementForceRunBackSpeedChangeAck },
        { MoveSetSwimSpeed,       SplineMoveSetSwimSpeed,       MovementUpdateSwimSpeed,    MovementForceSwimSpeedChangeAck    },
        { MoveSetSwimBackSpeed,   SplineMoveSetSwimBackSpeed,   MoveUpdateSwimBackSpeed,    MoveForceSwimBackSpeedChangeAck    },
        { MoveSetTurnRate,        SplineMoveSetTurnRate,        MovementUpdateTurnRate,     MovementForceTurnRateChangeAck     },
        { MoveSetFlightSpeed,     SplineMoveSetFlightSpeed,     MovementUpdateFlightSpeed,  MovementForceFlightSpeedChangeAck  },
        { MoveSetFlightBackSpeed, SplineMoveSetFlightBackSpeed, MoveUpdateFlightBackSpeed,  MoveForceFlightBackSpeedChangeAck  },
        { MoveSetPitchRate,       SplineMoveSetPitchRate,       MovementUpdatePitchBack,    MovementForcePitchRateChangeAck    },
    };

    return sequences[moveType < MAX_MOVE_TYPE ? moveType : uint8(MOVE_RUN)];
}

namespace
{
    // { SMSG_MOVE_SET_*, SMSG_SPLINE_MOVE_SET_*, SMSG_MOVE_UPDATE_*, CMSG_MOVE_FORCE_*_CHANGE_ACK } per UnitMoveType
    struct SpeedOpcodes
    {
        OpcodeServer MoveSet;
        OpcodeServer SplineSet;
        OpcodeServer Update;
        OpcodeClient Ack;
    };

    SpeedOpcodes const SpeedChangeOpcodes[MAX_MOVE_TYPE] =
    {
        { SMSG_MOVE_SET_WALK_SPEED,        SMSG_SPLINE_MOVE_SET_WALK_SPEED,        SMSG_MOVE_UPDATE_WALK_SPEED,        CMSG_MOVE_FORCE_WALK_SPEED_CHANGE_ACK        },
        { SMSG_MOVE_SET_RUN_SPEED,         SMSG_SPLINE_MOVE_SET_RUN_SPEED,         SMSG_MOVE_UPDATE_RUN_SPEED,         CMSG_MOVE_FORCE_RUN_SPEED_CHANGE_ACK         },
        { SMSG_MOVE_SET_RUN_BACK_SPEED,    SMSG_SPLINE_MOVE_SET_RUN_BACK_SPEED,    SMSG_MOVE_UPDATE_RUN_BACK_SPEED,    CMSG_MOVE_FORCE_RUN_BACK_SPEED_CHANGE_ACK    },
        { SMSG_MOVE_SET_SWIM_SPEED,        SMSG_SPLINE_MOVE_SET_SWIM_SPEED,        SMSG_MOVE_UPDATE_SWIM_SPEED,        CMSG_MOVE_FORCE_SWIM_SPEED_CHANGE_ACK        },
        { SMSG_MOVE_SET_SWIM_BACK_SPEED,   SMSG_SPLINE_MOVE_SET_SWIM_BACK_SPEED,   SMSG_MOVE_UPDATE_SWIM_BACK_SPEED,   CMSG_MOVE_FORCE_SWIM_BACK_SPEED_CHANGE_ACK   },
        { SMSG_MOVE_SET_TURN_RATE,         SMSG_SPLINE_MOVE_SET_TURN_RATE,         SMSG_MOVE_UPDATE_TURN_RATE,         CMSG_MOVE_FORCE_TURN_RATE_CHANGE_ACK         },
        { SMSG_MOVE_SET_FLIGHT_SPEED,      SMSG_SPLINE_MOVE_SET_FLIGHT_SPEED,      SMSG_MOVE_UPDATE_FLIGHT_SPEED,      CMSG_MOVE_FORCE_FLIGHT_SPEED_CHANGE_ACK      },
        { SMSG_MOVE_SET_FLIGHT_BACK_SPEED, SMSG_SPLINE_MOVE_SET_FLIGHT_BACK_SPEED, SMSG_MOVE_UPDATE_FLIGHT_BACK_SPEED, CMSG_MOVE_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK },
        { SMSG_MOVE_SET_PITCH_RATE,        SMSG_SPLINE_MOVE_SET_PITCH_RATE,        SMSG_MOVE_UPDATE_PITCH_RATE,        CMSG_MOVE_FORCE_PITCH_RATE_CHANGE_ACK        },
    };
}

OpcodeServer WorldPackets::Movement::MoveSetSpeedOpcode(uint8 moveType)
{
    return SpeedChangeOpcodes[moveType < MAX_MOVE_TYPE ? moveType : uint8(MOVE_RUN)].MoveSet;
}

OpcodeServer WorldPackets::Movement::SplineSetSpeedOpcode(uint8 moveType)
{
    return SpeedChangeOpcodes[moveType < MAX_MOVE_TYPE ? moveType : uint8(MOVE_RUN)].SplineSet;
}

OpcodeServer WorldPackets::Movement::MoveUpdateSpeedOpcode(uint8 moveType)
{
    return SpeedChangeOpcodes[moveType < MAX_MOVE_TYPE ? moveType : uint8(MOVE_RUN)].Update;
}

uint8 WorldPackets::Movement::GetSpeedChangeAckMoveType(uint16 opcode)
{
    for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
        if (SpeedChangeOpcodes[i].Ack == opcode)
            return i;

    return MAX_MOVE_TYPE;
}

WorldPackets::Movement::MovementStatusElements const*
WorldPackets::Movement::GetCollisionHeightSequence(uint16 opcode)
{
    switch (opcode)
    {
        case SMSG_MOVE_SET_COLLISION_HEIGHT: return MovementSetCollisionHeight;
        case SMSG_MOVE_UPDATE_COLLISION_HEIGHT: return MovementUpdateCollisionHeight;
        case CMSG_MOVE_SET_COLLISION_HEIGHT_ACK: return MovementSetCollisionHeightAck;
        default: return nullptr;
    }
}

WorldPackets::Movement::MovementStatusElements const*
WorldPackets::Movement::GetKnockBackSequence(uint16 opcode)
{
    switch (opcode)
    {
        case CMSG_MOVE_KNOCK_BACK_ACK: return MovementKnockBackAck;
        case SMSG_MOVE_UPDATE_KNOCK_BACK: return MovementUpdateKnockBack;
        default: return nullptr;
    }
}

WorldPackets::Movement::MovementStatusElements const*
WorldPackets::Movement::GetSpellCastMovementSequence()
{
    return SpellCastMovement;
}

WorldPacket const* WorldPackets::Movement::MoveKnockBack::Write()
{
    _worldPacket.WriteBit(MoverGUID[0]);
    _worldPacket.WriteBit(MoverGUID[3]);
    _worldPacket.WriteBit(MoverGUID[6]);
    _worldPacket.WriteBit(MoverGUID[7]);
    _worldPacket.WriteBit(MoverGUID[2]);
    _worldPacket.WriteBit(MoverGUID[5]);
    _worldPacket.WriteBit(MoverGUID[1]);
    _worldPacket.WriteBit(MoverGUID[4]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(MoverGUID[1]);

    _worldPacket << float(DirectionY);
    _worldPacket << uint32(SequenceIndex);

    _worldPacket.WriteByteSeq(MoverGUID[6]);
    _worldPacket.WriteByteSeq(MoverGUID[7]);

    _worldPacket << float(HorzSpeed);

    _worldPacket.WriteByteSeq(MoverGUID[4]);
    _worldPacket.WriteByteSeq(MoverGUID[5]);
    _worldPacket.WriteByteSeq(MoverGUID[3]);

    _worldPacket << float(VertSpeed);
    _worldPacket << float(DirectionX);

    _worldPacket.WriteByteSeq(MoverGUID[2]);
    _worldPacket.WriteByteSeq(MoverGUID[0]);

    return &_worldPacket;
}

void WorldPackets::Movement::WriteGroundMovement(WorldPacket& packet, MovementInfo const& info,
    MovementStatusElements const* sequence, uint32 counter, ExtraMovementStatusElement* extras)
{
    uint32 flags = MovementFlagsToClient(info.flags);
    uint16 flags2 = ExtraMovementFlagsToClient(info.flags2) & 0x1FFF;
    bool hasFallDirection = info.HasMovementFlag(MOVEMENTFLAG_FALLING);
    bool hasFallData = hasFallDirection || info.fallTime != 0;
    bool hasOrientation = !G3D::fuzzyEq(info.pos.GetOrientation(), 0.0f);
    bool hasTransport = !info.transport.guid.IsEmpty();
    bool hasVehicleId = hasTransport && info.transport.vehicleId != 0;
    bool hasTransportTime2 = hasTransport && info.transport.time2 != 0;
    bool hasPitch = info.HasMovementFlag(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING) ||
        info.HasExtraMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING);
    bool hasSplineElevation = info.HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION);
    bool hasSpline = info.HasMovementFlag(MOVEMENTFLAG_SPLINE_ENABLED);
    ObjectGuid const& guid = info.guid;
    ObjectGuid const& transport = info.transport.guid;

    // Sequences without an order counter field (root and unroot acks) carry it in the optional
    // movement counter instead.
    bool sequenceHasCounter = false;
    for (MovementStatusElements const* element = sequence; *element != MSEEnd; ++element)
        if (*element == MSECounter)
            sequenceHasCounter = true;
    bool const hasOptionalCounter = !sequenceHasCounter && counter;

    for (; *sequence != MSEEnd; ++sequence)
    {
        switch (*sequence)
        {
            case MSEHasGuidByte0: packet.WriteBit(guid[0]); break;
            case MSEHasGuidByte1: packet.WriteBit(guid[1]); break;
            case MSEHasGuidByte2: packet.WriteBit(guid[2]); break;
            case MSEHasGuidByte3: packet.WriteBit(guid[3]); break;
            case MSEHasGuidByte4: packet.WriteBit(guid[4]); break;
            case MSEHasGuidByte5: packet.WriteBit(guid[5]); break;
            case MSEHasGuidByte6: packet.WriteBit(guid[6]); break;
            case MSEHasGuidByte7: packet.WriteBit(guid[7]); break;
            case MSEHasTransportGuidByte0: if (hasTransport) packet.WriteBit(transport[0]); break;
            case MSEHasTransportGuidByte1: if (hasTransport) packet.WriteBit(transport[1]); break;
            case MSEHasTransportGuidByte2: if (hasTransport) packet.WriteBit(transport[2]); break;
            case MSEHasTransportGuidByte3: if (hasTransport) packet.WriteBit(transport[3]); break;
            case MSEHasTransportGuidByte4: if (hasTransport) packet.WriteBit(transport[4]); break;
            case MSEHasTransportGuidByte5: if (hasTransport) packet.WriteBit(transport[5]); break;
            case MSEHasTransportGuidByte6: if (hasTransport) packet.WriteBit(transport[6]); break;
            case MSEHasTransportGuidByte7: if (hasTransport) packet.WriteBit(transport[7]); break;
            case MSEHasTransportData: packet.WriteBit(hasTransport); break;
            case MSEHasVehicleId: if (hasTransport) packet.WriteBit(hasVehicleId); break;
            case MSEHasTransportTime2: if (hasTransport) packet.WriteBit(hasTransportTime2); break;
            case MSEHasMovementFlags: packet.WriteBit(!flags); break;
            case MSEHasMovementFlags2: packet.WriteBit(!flags2); break;
            case MSEHasTimestamp: packet.WriteBit(false); break; // Timestamp is always present in server movement updates.
            case MSEHasOrientation: packet.WriteBit(!hasOrientation); break;
            case MSEHasPitch: packet.WriteBit(!hasPitch); break;
            case MSEHasSplineElevation: packet.WriteBit(!hasSplineElevation); break;
            case MSEHasSpline: packet.WriteBit(hasSpline); break;
            case MSEHasFallData: packet.WriteBit(hasFallData); break;
            case MSEHasFallDirection: if (hasFallData) packet.WriteBit(hasFallDirection); break;
            case MSEZeroBit: packet.WriteBit(false); break;
            case MSEMovementFlags: if (flags) packet.WriteBits(flags, 30); break;
            case MSEMovementFlags2: if (flags2) packet.WriteBits(flags2, 13); break;
            case MSEPositionX: packet << info.pos.GetPositionX(); break;
            case MSEPositionY: packet << info.pos.GetPositionY(); break;
            case MSEPositionZ: packet << info.pos.GetPositionZ(); break;
            case MSEOrientation: packet.FlushBits(); if (hasOrientation) packet << info.pos.GetOrientation(); break;
            case MSEGuidByte0: packet.FlushBits(); packet.WriteByteSeq(guid[0]); break;
            case MSEGuidByte1: packet.FlushBits(); packet.WriteByteSeq(guid[1]); break;
            case MSEGuidByte2: packet.FlushBits(); packet.WriteByteSeq(guid[2]); break;
            case MSEGuidByte3: packet.FlushBits(); packet.WriteByteSeq(guid[3]); break;
            case MSEGuidByte4: packet.FlushBits(); packet.WriteByteSeq(guid[4]); break;
            case MSEGuidByte5: packet.FlushBits(); packet.WriteByteSeq(guid[5]); break;
            case MSEGuidByte6: packet.FlushBits(); packet.WriteByteSeq(guid[6]); break;
            case MSEGuidByte7: packet.FlushBits(); packet.WriteByteSeq(guid[7]); break;
            case MSETransportGuidByte0: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[0]); break;
            case MSETransportGuidByte1: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[1]); break;
            case MSETransportGuidByte2: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[2]); break;
            case MSETransportGuidByte3: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[3]); break;
            case MSETransportGuidByte4: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[4]); break;
            case MSETransportGuidByte5: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[5]); break;
            case MSETransportGuidByte6: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[6]); break;
            case MSETransportGuidByte7: packet.FlushBits(); if (hasTransport) packet.WriteByteSeq(transport[7]); break;
            case MSETransportPositionX: packet.FlushBits(); if (hasTransport) packet << info.transport.pos.GetPositionX(); break;
            case MSETransportPositionY: packet.FlushBits(); if (hasTransport) packet << info.transport.pos.GetPositionY(); break;
            case MSETransportPositionZ: packet.FlushBits(); if (hasTransport) packet << info.transport.pos.GetPositionZ(); break;
            case MSETransportOrientation: packet.FlushBits(); if (hasTransport) packet << info.transport.pos.GetOrientation(); break;
            case MSETransportSeat: packet.FlushBits(); if (hasTransport) packet << info.transport.seat; break;
            case MSETransportTime: packet.FlushBits(); if (hasTransport) packet << info.transport.time; break;
            case MSETransportTime2: packet.FlushBits(); if (hasTransport && hasTransportTime2) packet << info.transport.time2; break;
            case MSETransportVehicleId: packet.FlushBits(); if (hasTransport && hasVehicleId) packet << info.transport.vehicleId; break;
            case MSEPitch: packet.FlushBits(); if (hasPitch) packet << info.pitch; break;
            case MSEFallTime: packet.FlushBits(); if (hasFallData) packet << info.fallTime; break;
            case MSEFallVerticalSpeed: packet.FlushBits(); if (hasFallData) packet << info.jump.zspeed; break;
            case MSEFallCosAngle: packet.FlushBits(); if (hasFallData && hasFallDirection) packet << info.jump.cosAngle; break;
            case MSEFallSinAngle: packet.FlushBits(); if (hasFallData && hasFallDirection) packet << info.jump.sinAngle; break;
            case MSEFallHorizontalSpeed: packet.FlushBits(); if (hasFallData && hasFallDirection) packet << info.jump.xyspeed; break;
            case MSESplineElevation: packet.FlushBits(); if (hasSplineElevation) packet << info.splineElevation; break;
            case MSETimestamp: packet.FlushBits(); packet << info.time; break;
            case MSECounter: packet.FlushBits(); packet << counter; break;
            case MSEExtraElement:
                if (extras)
                    extras->WriteNextElement(packet);
                break;
            case MSEForcesCount: packet.WriteBits(0, 22); break;
            case MSEForces: break;
            case MSEHasCounter: packet.WriteBit(!hasOptionalCounter); break;
            case MSEOptionalCounter: packet.FlushBits(); if (hasOptionalCounter) packet << counter; break;
            case MSEHasMountDisplayId: packet.WriteBit(true); break;
            case MSEMountDisplayIdWithCheck: break;
            case MSEMountDisplayIdWithoutCheck: packet.FlushBits(); packet << uint32(0); break;
            case MSEOrientationWithoutCheck: packet.FlushBits(); packet << info.pos.GetOrientation(); break;
            case MSEExtraFloat:
            case MSEExtraFloat2:
            case MSEExtraInt8:
            case MSEExtraInt32:
            case MSEExtraTwoBits:
                break; // only valid inside an ExtraMovementStatusElement sequence
            case MSEOneBit: packet.WriteBit(true); break;
            case MSEHasHeightChangeFailed: packet.WriteBit(false); break;
            case MSEFlushBits: packet.FlushBits(); break;
            case MSEEnd: break;
        }
    }
}

WorldPackets::Movement::MovementStatusElements const*
WorldPackets::Movement::GetMovementFlagSequence(uint16 opcode)
{
    switch (opcode)
    {
        case SMSG_MOVE_ROOT: return MoveRoot;
        case SMSG_MOVE_UNROOT: return MoveUnroot;
        case SMSG_MOVE_WATER_WALK: return MoveWaterWalk;
        case SMSG_MOVE_LAND_WALK: return MoveLandWalk;
        case SMSG_MOVE_FEATHER_FALL: return MoveFeatherFall;
        case SMSG_MOVE_NORMAL_FALL: return MoveNormalFall;
        case SMSG_MOVE_SET_HOVER: return MoveSetHover;
        case SMSG_MOVE_UNSET_HOVER: return MoveUnsetHover;
        case SMSG_MOVE_GRAVITY_DISABLE: return MoveGravityDisable;
        case SMSG_MOVE_GRAVITY_ENABLE: return MoveGravityEnable;
        case SMSG_MOVE_SET_CAN_FLY: return MoveSetCanFly;
        case SMSG_MOVE_UNSET_CAN_FLY: return MoveUnsetCanFly;
        case SMSG_MOVE_SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY: return MoveSetCanTransitionBetweenSwimAndFly;
        case SMSG_MOVE_UNSET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY: return MoveUnsetCanTransitionBetweenSwimAndFly;
        case SMSG_MOVE_SET_CAN_TURN_WHILE_FALLING: return MoveSetCanTurnWhileFalling;
        case SMSG_MOVE_UNSET_CAN_TURN_WHILE_FALLING: return MoveUnsetCanTurnWhileFalling;
        case SMSG_SPLINE_MOVE_ROOT: return SplineMoveRoot;
        case SMSG_SPLINE_MOVE_UNROOT: return SplineMoveUnroot;
        case SMSG_SPLINE_MOVE_GRAVITY_DISABLE: return SplineMoveGravityDisable;
        case SMSG_SPLINE_MOVE_GRAVITY_ENABLE: return SplineMoveGravityEnable;
        case SMSG_SPLINE_MOVE_COLLISION_DISABLE: return SplineMoveCollisionDisable;
        case SMSG_SPLINE_MOVE_COLLISION_ENABLE: return SplineMoveCollisionEnable;
        case SMSG_SPLINE_MOVE_SET_FLYING: return SplineMoveSetFlying;
        case SMSG_SPLINE_MOVE_UNSET_FLYING: return SplineMoveUnsetFlying;
        case SMSG_SPLINE_MOVE_FEATHER_FALL: return SplineMoveSetFeatherFall;
        case SMSG_SPLINE_MOVE_NORMAL_FALL: return SplineMoveSetNormalFall;
        case SMSG_SPLINE_MOVE_SET_HOVER: return SplineMoveSetHover;
        case SMSG_SPLINE_MOVE_UNSET_HOVER: return SplineMoveUnsetHover;
        case SMSG_SPLINE_MOVE_WATER_WALK: return SplineMoveSetWaterWalk;
        case SMSG_SPLINE_MOVE_LAND_WALK: return SplineMoveSetLandWalk;
        case SMSG_SPLINE_MOVE_SET_WALK_MODE: return SplineMoveSetWalkMode;
        case SMSG_SPLINE_MOVE_SET_RUN_MODE: return SplineMoveSetRunMode;
        case SMSG_SPLINE_MOVE_START_SWIM: return SplineMoveStartSwim;
        case SMSG_SPLINE_MOVE_STOP_SWIM: return SplineMoveStopSwim;
        case CMSG_FORCE_MOVE_ROOT_ACK: return ForceMoveRootAck;
        case CMSG_FORCE_MOVE_UNROOT_ACK: return ForceMoveUnrootAck;
        case CMSG_MOVE_WATER_WALK_ACK: return MovementWaterWalkAck;
        case CMSG_MOVE_FEATHER_FALL_ACK: return MovementFeatherFallAck;
        case CMSG_MOVE_HOVER_ACK: return MovementHoverAck;
        case CMSG_MOVE_GRAVITY_DISABLE_ACK: return MovementGravityDisableAck;
        case CMSG_MOVE_GRAVITY_ENABLE_ACK: return MovementGravityEnableAck;
        case CMSG_MOVE_SET_CAN_FLY_ACK: return MovementSetCanFlyAck;
        case CMSG_MOVE_SPLINE_DONE: return MoveSplineDone;
        case CMSG_MOVE_NOT_ACTIVE_MOVER: return MoveNotActiveMover;
        case CMSG_SET_VEHICLE_REC_ID_ACK: return SetVehicleRecIdAck;
        default: return nullptr;
    }
}

void WorldPackets::Movement::WriteTimeSkipped(WorldPacket& packet, ObjectGuid const& guid, uint32 time)
{
    packet << time;
    packet << guid.WriteAsPacked();
}

// The 5.4.8 packets below use the sequences from Project SkyFire (MovementStructures.cpp).
void WorldPackets::Movement::ReadHeartbeat(WorldPacket& packet, MovementInfo& info)
{
    ReadGroundMovement(packet, info, MovementHeartBeat);
}

void WorldPackets::Movement::WriteMovementUpdate(WorldPacket& packet, MovementInfo const& info)
{
    WriteGroundMovement(packet, info, PlayerMove);
}

void WorldPackets::Movement::WriteRunSpeedChange(WorldPacket& packet, ObjectGuid const& guid, uint32 counter, float speed)
{
    static MovementStatusElements const speedElement[] = { MSEExtraFloat };
    ExtraMovementStatusElement extra(speedElement);
    extra.Data.floatData = speed;

    MovementInfo info;
    info.guid = guid;
    WriteGroundMovement(packet, info, MoveSetRunSpeed, counter, &extra);
}

void WorldPackets::Movement::ReadRunSpeedChangeAck(WorldPacket& packet, MovementInfo& info, uint32& counter, float& speed)
{
    static MovementStatusElements const speedElement[] = { MSEExtraFloat };
    ExtraMovementStatusElement extra(speedElement);
    ReadGroundMovement(packet, info, MovementForceRunSpeedChangeAck, &counter, &extra);
    speed = extra.Data.floatData;
}

void WorldPackets::Movement::WriteRunSpeedUpdate(WorldPacket& packet, MovementInfo const& info, float speed)
{
    static MovementStatusElements const speedElement[] = { MSEExtraFloat };
    ExtraMovementStatusElement extra(speedElement);
    extra.Data.floatData = speed;
    WriteGroundMovement(packet, info, MovementUpdateRunSpeed, 0, &extra);
}

void WorldPackets::Movement::WriteTeleport(WorldPacket& packet, ObjectGuid guid, uint32 counter,
    Position const& pos, ObjectGuid transport)
{
    MovementInfo info;
    info.guid = guid;
    info.pos.Relocate(pos);
    info.transport.guid = transport;
    WriteGroundMovement(packet, info, MoveTeleport, counter);
}

void WorldPackets::Movement::ReadTeleportAck(WorldPacket& packet, ObjectGuid& guid, uint32& counter, uint32& time)
{
    packet >> time >> counter;
    guid.Clear();
    for (uint8 index : { 0, 7, 3, 5, 4, 6, 1, 2 })
        guid[index] = packet.ReadBit();
    for (uint8 index : { 4, 1, 6, 7, 0, 2, 5, 3 })
        packet.ReadByteSeq(guid[index]);
}

void WorldPackets::Movement::ReadTimeSkipped(WorldPacket& packet, ObjectGuid& guid, uint32& time)
{
    packet >> time;
    guid.Clear();
    for (uint8 index : { 5, 0, 7, 4, 1, 2, 6, 3 })
        guid[index] = packet.ReadBit();
    for (uint8 index : { 7, 2, 0, 6, 1, 5, 3, 4 })
        packet.ReadByteSeq(guid[index]);
}

void WorldPackets::Movement::WriteTeleportUpdate(WorldPacket& packet, MovementInfo const& info)
{
    WriteGroundMovement(packet, info, MoveUpdateTeleport);
}

void WorldPackets::Movement::WriteMovementFlagChange(WorldPacket& packet, ObjectGuid const& guid, uint32 counter)
{
    MovementInfo info;
    info.guid = guid;
    if (MovementStatusElements const* sequence = GetMovementFlagSequence(packet.GetOpcode()))
        WriteGroundMovement(packet, info, sequence, counter);
}
