/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * You may redistribute it and/or modify it under the terms of the GNU General Public License
 * version 2 or, at your option, any later version.
 */

#include "MovementPackets.h"
#include "IntegrationTestFixture.h"
#include "Object.h"
#include "Util.h"
#include <gtest/gtest.h>
#include <cstring>
#include <limits>
#include <span>

namespace
{
    WorldPacket ClientPacket(OpcodeClient opcode, std::string const& hex)
    {
        std::vector<uint8> bytes(hex.size() / 2);
        Acore::Impl::HexStrToByteArray(hex, bytes.data(), bytes.size());
        WorldPacket packet(opcode, bytes.size());
        if (!bytes.empty())
            packet.append(bytes.data(), bytes.size());
        return packet;
    }

    MovementInfo SampleMovement()
    {
        MovementInfo info;
        info.guid = ObjectGuid(uint64(0x0807060504030201));
        info.pos.Relocate(1.0f, 2.0f, 3.0f, 4.0f);
        info.time = 0x01020304;
        info.flags = MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_SWIMMING;
        info.pitch = 0.5f;
        return info;
    }

    // Client packets are built with the same sequence the server reads them with.
    WorldPacket WriteClientMovement(OpcodeClient opcode, MovementInfo const& info, uint32 counter = 0,
        WorldPackets::Movement::ExtraMovementStatusElement* extras = nullptr)
    {
        WorldPackets::Movement::MovementStatusElements const* sequence = WorldPackets::Movement::GetMovementSequence(opcode);
        if (!sequence)
            sequence = WorldPackets::Movement::GetMovementFlagSequence(opcode);
        WorldPacket packet(opcode);
        WorldPackets::Movement::WriteGroundMovement(packet, info, sequence, counter, extras);
        return packet;
    }

    void ExpectSameMovement(MovementInfo const& received, MovementInfo const& sent)
    {
        EXPECT_EQ(received.guid, sent.guid);
        EXPECT_FLOAT_EQ(received.pos.GetPositionX(), sent.pos.GetPositionX());
        EXPECT_FLOAT_EQ(received.pos.GetPositionY(), sent.pos.GetPositionY());
        EXPECT_FLOAT_EQ(received.pos.GetPositionZ(), sent.pos.GetPositionZ());
        EXPECT_FLOAT_EQ(received.pos.GetOrientation(), sent.pos.GetOrientation());
        EXPECT_EQ(received.time, sent.time);
        EXPECT_EQ(received.flags, sent.flags);
        EXPECT_FLOAT_EQ(received.pitch, sent.pitch);
    }
}

TEST(MovementPacketsTest, RoundTripsTheHeartbeat)
{
    MovementInfo const sent = SampleMovement();
    WorldPacket packet = WriteClientMovement(MSG_MOVE_HEARTBEAT, sent);
    MovementInfo received;
    WorldPackets::Movement::ReadHeartbeat(packet, received);
    ExpectSameMovement(received, sent);
    EXPECT_EQ(packet.rpos(), packet.size());
}

TEST(MovementPacketsTest, RoundTripsFallAndTransportData)
{
    MovementInfo sent = SampleMovement();
    sent.flags = MOVEMENTFLAG_FALLING;
    sent.fallTime = 250;
    sent.jump.zspeed = -3.0f;
    sent.jump.sinAngle = 0.25f;
    sent.jump.cosAngle = 0.75f;
    sent.jump.xyspeed = 7.0f;
    sent.transport.guid = ObjectGuid(uint64(0x1F00000000000042));
    sent.transport.pos.Relocate(0.5f, 1.5f, 2.5f, 3.5f);
    sent.transport.seat = 2;
    sent.transport.time = 99;

    WorldPacket packet = WriteClientMovement(MSG_MOVE_HEARTBEAT, sent);
    MovementInfo received;
    WorldPackets::Movement::ReadHeartbeat(packet, received);
    EXPECT_EQ(received.guid, sent.guid);
    EXPECT_TRUE(received.HasMovementFlag(MOVEMENTFLAG_FALLING));
    EXPECT_TRUE(received.HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT));
    EXPECT_EQ(received.fallTime, 250u);
    EXPECT_FLOAT_EQ(received.jump.zspeed, -3.0f);
    EXPECT_FLOAT_EQ(received.jump.sinAngle, 0.25f);
    EXPECT_FLOAT_EQ(received.jump.cosAngle, 0.75f);
    EXPECT_FLOAT_EQ(received.jump.xyspeed, 7.0f);
    EXPECT_EQ(received.transport.guid, sent.transport.guid);
    EXPECT_FLOAT_EQ(received.transport.pos.GetPositionY(), 1.5f);
    EXPECT_EQ(received.transport.seat, 2);
    EXPECT_EQ(received.transport.time, 99u);
    EXPECT_EQ(packet.rpos(), packet.size());
}

TEST(MovementPacketsTest, RejectsTruncatedAndTrailingHeartbeatData)
{
    WorldPacket full = WriteClientMovement(MSG_MOVE_HEARTBEAT, SampleMovement());
    std::vector<uint8> bytes(full.contents(), full.contents() + full.size());

    WorldPacket truncated(MSG_MOVE_HEARTBEAT);
    truncated.append(bytes.data(), bytes.size() - 1);
    MovementInfo info;
    EXPECT_THROW(WorldPackets::Movement::ReadHeartbeat(truncated, info), ByteBufferException);

    WorldPacket trailing(MSG_MOVE_HEARTBEAT);
    trailing.append(bytes.data(), bytes.size());
    trailing << uint8(0);
    EXPECT_THROW(WorldPackets::Movement::ReadHeartbeat(trailing, info), ByteBufferInvalidValueException);
}

TEST(MovementPacketsTest, TranslatesInternalFlagsAtTheWireBoundary)
{
    using namespace WorldPackets::Movement;
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_WALKING), 0x100u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_DISABLE_GRAVITY), 0x200u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_ROOT), 0x400u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_FALLING), 0x800u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_SWIMMING), 0x100000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_FLYING), 0x1000000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_SPLINE_ELEVATION), 0x2000000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_WATERWALKING), 0x4000000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_FALLING_SLOW), 0x8000000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_HOVER), 0x10000000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_DISABLE_COLLISION), 0x20000000u);
    EXPECT_EQ(MovementFlagsToClient(MOVEMENTFLAG_ONTRANSPORT | MOVEMENTFLAG_SPLINE_ENABLED), 0u);
    EXPECT_EQ(ExtraMovementFlagsToClient(MOVEMENTFLAG2_FULL_SPEED_TURNING), 0x4u);
    EXPECT_EQ(ExtraMovementFlagsToClient(MOVEMENTFLAG2_FULL_SPEED_PITCHING), 0x8u);
    EXPECT_EQ(ExtraMovementFlagsToClient(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING), 0x10u);
    EXPECT_EQ(ExtraMovementFlagsToClient(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT), 0x2000u);
}

TEST(MovementPacketsTest, RoundTripsTheRunSpeedAcknowledgement)
{
    using namespace WorldPackets::Movement;
    static MovementStatusElements const speedElement[] = { MSEExtraFloat };
    ExtraMovementStatusElement extra(speedElement);
    extra.Data.floatData = 10.5f;

    MovementInfo const sent = SampleMovement();
    MovementInfo received;
    uint32 counter = 0;
    float speed = 0.0f;
    WorldPacket written(CMSG_MOVE_FORCE_RUN_SPEED_CHANGE_ACK);
    WriteGroundMovement(written, sent, GetSpeedChangeSequences(MOVE_RUN).Ack, 0x10203040, &extra);
    ReadRunSpeedChangeAck(written, received, counter, speed);
    ExpectSameMovement(received, sent);
    EXPECT_EQ(counter, 0x10203040u);
    EXPECT_FLOAT_EQ(speed, 10.5f);
    EXPECT_EQ(written.rpos(), written.size());

    WorldPacket request(SMSG_MOVE_SET_RUN_SPEED);
    WriteRunSpeedChange(request, sent.guid, 7, 10.5f);
    EXPECT_FALSE(request.empty());
    WorldPacket update(SMSG_MOVE_UPDATE_RUN_SPEED);
    WriteRunSpeedUpdate(update, sent, 10.5f);
    EXPECT_FALSE(update.empty());
}

TEST(MovementPacketsTest, RoundTripsEveryGroundMovementOpcode)
{
    for (OpcodeClient opcode : { MSG_MOVE_START_FORWARD, MSG_MOVE_START_BACKWARD, MSG_MOVE_STOP,
        MSG_MOVE_START_STRAFE_LEFT, MSG_MOVE_START_STRAFE_RIGHT, MSG_MOVE_STOP_STRAFE, MSG_MOVE_JUMP,
        MSG_MOVE_START_TURN_LEFT, MSG_MOVE_START_TURN_RIGHT, MSG_MOVE_STOP_TURN, MSG_MOVE_START_PITCH_UP,
        MSG_MOVE_START_PITCH_DOWN, MSG_MOVE_STOP_PITCH, MSG_MOVE_SET_RUN_MODE, MSG_MOVE_SET_WALK_MODE,
        MSG_MOVE_FALL_LAND, MSG_MOVE_START_SWIM, MSG_MOVE_STOP_SWIM, MSG_MOVE_SET_FACING,
        MSG_MOVE_SET_PITCH, MSG_MOVE_START_ASCEND, MSG_MOVE_STOP_ASCEND, MSG_MOVE_START_DESCEND,
        MSG_MOVE_HEARTBEAT, CMSG_MOVE_CHNG_TRANSPORT, CMSG_MOVE_FALL_RESET, CMSG_DISMISS_CONTROLLED_VEHICLE })
    {
        MovementInfo const sent = SampleMovement();
        WorldPacket packet = WriteClientMovement(opcode, sent);
        MovementInfo received;
        WorldPackets::Movement::ReadGroundMovement(packet, received, WorldPackets::Movement::GetMovementSequence(opcode));
        ExpectSameMovement(received, sent);
        EXPECT_EQ(packet.rpos(), packet.size()) << opcode;
    }
}

TEST(MovementPacketsTest, ReadsVehicleSeatChangeExtraElements)
{
    using namespace WorldPackets::Movement;
    ExtraMovementStatusElement written(GetChangeSeatsExtraSequence());
    written.Data.guid = ObjectGuid(uint64(0x0807060504030201));
    written.Data.byteData = 2;

    MovementInfo const sent = SampleMovement();
    WorldPacket packet = WriteClientMovement(CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE, sent, 0, &written);

    ExtraMovementStatusElement extra(GetChangeSeatsExtraSequence());
    MovementInfo received;
    ReadGroundMovement(packet, received, GetMovementSequence(CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE), nullptr, &extra);
    ExpectSameMovement(received, sent);
    EXPECT_EQ(extra.Data.guid, written.Data.guid);
    EXPECT_EQ(extra.Data.byteData, 2);
    EXPECT_EQ(packet.rpos(), packet.size());
}

using RunSpeedAcknowledgementTest = IntegrationTestFixture;

TEST_F(RunSpeedAcknowledgementTest, TracksLatestRequestAndRejectsInvalidAcknowledgements)
{
    using namespace WorldPackets::Movement;
    TestPlayer* player = CreateTestPlayer(0x01020304);
    WorldSession* session = player->GetSession();
    player->SetSpeedRate(MOVE_RUN, 1.5f);
    player->SendSpeedToController(MOVE_RUN, player);
    ASSERT_TRUE(player->m_pendingRunSpeedChange);
    EXPECT_GT(player->m_pendingRunSpeedChange->Counter, player->GetMapChangeOrderCounter());
    EXPECT_FLOAT_EQ(player->m_pendingRunSpeedChange->Speed, 10.5f);
    uint32 firstCounter = player->m_pendingRunSpeedChange->Counter;
    player->SendSpeedToController(MOVE_RUN, player);
    EXPECT_GT(player->m_pendingRunSpeedChange->Counter, firstCounter);

    auto ack = [](uint32 counter, float speed, ObjectGuid guid)
    {
        static MovementStatusElements const speedElement[] = { MSEExtraFloat };
        ExtraMovementStatusElement extra(speedElement);
        extra.Data.floatData = speed;
        MovementInfo info;
        info.guid = guid;
        info.pos.Relocate(3.0f, 0.0f, 0.0f, 0.0f);
        WorldPacket packet(CMSG_MOVE_FORCE_RUN_SPEED_CHANGE_ACK);
        WriteGroundMovement(packet, info, GetSpeedChangeSequences(MOVE_RUN).Ack, counter, &extra);
        return packet;
    };

    // No pending request: an unsolicited or duplicate acknowledgement cannot be accepted.
    player->m_pendingRunSpeedChange.reset();
    WorldPacket packet = ack(0x10203040, 10.5f, player->GetGUID());
    session->HandleForceRunSpeedChangeAck(packet);
    EXPECT_FALSE(player->m_pendingRunSpeedChange);
    EXPECT_FALSE(session->IsKicked());

    // An older counter must be ignored before comparing the requested speed.
    player->m_pendingRunSpeedChange = Unit::PendingRunSpeedChange{ 0x10203041, 7.0f };
    packet = ack(0x10203040, 10.5f, player->GetGUID());
    session->HandleForceRunSpeedChangeAck(packet);
    EXPECT_EQ(player->m_pendingRunSpeedChange->Counter, 0x10203041u);
    EXPECT_FALSE(session->IsKicked());

    // The map-change boundary is also stale, even if it matches the pending counter.
    player->m_pendingRunSpeedChange = Unit::PendingRunSpeedChange{ 0, 7.0f };
    packet = ack(0, 10.5f, player->GetGUID());
    session->HandleForceRunSpeedChangeAck(packet);
    EXPECT_TRUE(player->m_pendingRunSpeedChange);
    EXPECT_FALSE(session->IsKicked());

    player->m_pendingRunSpeedChange = Unit::PendingRunSpeedChange{ 0x10203040, 7.0f };

    // A matching counter with the wrong speed is rejected without consuming the pending request.
    packet = ack(0x10203040, 10.5f, player->GetGUID());
    session->HandleForceRunSpeedChangeAck(packet);
    EXPECT_TRUE(player->m_pendingRunSpeedChange);
    EXPECT_TRUE(session->IsKicked());
    EXPECT_FLOAT_EQ(player->GetPositionX(), 0.0f);
}

TEST(MovementPacketsTest, ReadsTheTeleportAcknowledgment)
{
    // 5.4.8: move time, ack index, then the bit-packed mover GUID.
    WorldPacket packet = ClientPacket(MSG_MOVE_TELEPORT_ACK, "e3dcc80001000000a302050300");
    ObjectGuid guid;
    uint32 counter = 0;
    uint32 time = 0;
    WorldPackets::Movement::ReadTeleportAck(packet, guid, counter, time);
    EXPECT_EQ(guid, ObjectGuid(uint64(0x01020304)));
    EXPECT_EQ(counter, 1u);
    EXPECT_EQ(time, 0x00C8DCE3u);
    EXPECT_EQ(packet.rpos(), packet.size());
}

TEST(MovementPacketsTest, ReadsAndWritesMoveTimeSkipped)
{
    WorldPacket packet = ClientPacket(CMSG_MOVE_TIME_SKIPPED, "680100004d03050200");
    ObjectGuid guid;
    uint32 time = 0;
    WorldPackets::Movement::ReadTimeSkipped(packet, guid, time);
    EXPECT_EQ(guid, ObjectGuid(uint64(0x01020304)));
    EXPECT_EQ(time, 360u);
    EXPECT_EQ(packet.rpos(), packet.size());
}

using GroundMovementRoutingTest = IntegrationTestFixture;

TEST_F(GroundMovementRoutingTest, RejectsGroundMovementFromASpoofedMoverGuid)
{
    // Full relocation acceptance (grid-crossing, anti-cheat) needs a live map and is covered by
    // the real-client acceptance pass; this exercises HandleMovementOpcodes' own guid gate, which
    // must reject a packet before it reaches ProcessMovementInfo/relocation at all.
    TestPlayer* player = CreateTestPlayer(0x01020304);
    WorldSession* session = player->GetSession();

    MovementInfo spoofed = SampleMovement();
    spoofed.guid = ObjectGuid(uint64(0x0010000000000001));
    WorldPacket packet = WriteClientMovement(MSG_MOVE_START_BACKWARD, spoofed);
    session->HandleMovementOpcodes(packet);
    EXPECT_FLOAT_EQ(player->GetPositionX(), 0.0f);
    EXPECT_FLOAT_EQ(player->GetPositionY(), 0.0f);
    EXPECT_FLOAT_EQ(player->GetPositionZ(), 0.0f);
    EXPECT_FALSE(session->IsKicked());
}
