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

#include "AuthenticationPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "HMAC.h"
#include "SessionKeyGenerator.h"
#include "CryptoHash.h"
#include "Util.h"
#include <gtest/gtest.h>
#include <numeric>
#include <span>
#include <string>
#include <vector>

// 6.2.4 21742 authentication packet layouts (TrinityCore 6.x AuthenticationPackets.cpp)

namespace
{
    std::string PayloadHex(ByteBuffer const* packet)
    {
        return ByteArrayToHexStr(std::span<uint8 const>(packet->contents(), packet->size()));
    }
}

TEST(AuthenticationPacketsTest, WritesAuthChallenge)
{
    WorldPackets::Auth::AuthChallenge challenge;
    std::iota(challenge.DosChallenge.begin(), challenge.DosChallenge.end(), uint8(0));
    std::iota(challenge.Challenge.begin(), challenge.Challenge.end(), uint8(0x40));
    challenge.DosZeroBits = 1;

    WorldPacket const* packet = challenge.Write();
    ASSERT_EQ(packet->size(), 32u + 16u + 1u);
    EXPECT_EQ(packet->contents()[0], 0x00);      // encryption seeds first
    EXPECT_EQ(packet->contents()[31], 0x1F);
    EXPECT_EQ(packet->contents()[32], 0x40);     // then the server challenge
    EXPECT_EQ(packet->contents()[48], 0x01);     // DosZeroBits
}

TEST(AuthenticationPacketsTest, ReadsAuthSession)
{
    WorldPacket packet(CMSG_AUTH_SESSION, 100);
    packet << uint16(21742);                     // build
    packet << int8(0);                           // build type
    packet << uint32(1) << uint32(1) << uint32(7); // region, battlegroup, realm
    for (uint8 i = 0; i < 16; ++i)
        packet << uint8(i);                      // local challenge
    for (uint8 i = 0; i < 24; ++i)
        packet << uint8(0x80 + i);               // digest
    packet << uint64(0x1122334455667788);        // dos response
    packet << uint32(3);
    packet << uint8(1) << uint8(2) << uint8(3);  // addon info
    packet << uint32(3);
    packet.append(reinterpret_cast<uint8 const*>("1#1"), 3);
    packet.WriteBit(true);                       // use IPv6
    packet.FlushBits();

    WorldPackets::Auth::AuthSession authSession(std::move(packet));
    ASSERT_TRUE(authSession.ReadNoThrow());
    EXPECT_EQ(authSession.Build, 21742);
    EXPECT_EQ(authSession.RealmID, 7u);
    EXPECT_EQ(authSession.LocalChallenge[15], 15);
    EXPECT_EQ(authSession.Digest[0], 0x80);
    EXPECT_EQ(authSession.Digest[23], 0x80 + 23);
    EXPECT_EQ(authSession.DosResponse, 0x1122334455667788u);
    EXPECT_EQ(authSession.AddonInfo.size(), 3u);
    EXPECT_EQ(authSession.RealmJoinTicket, "1#1");
    EXPECT_TRUE(authSession.UseIPv6);
}

TEST(AuthenticationPacketsTest, RejectsTruncatedAuthSession)
{
    WorldPacket packet(CMSG_AUTH_SESSION, 4);
    packet << uint16(21742);
    WorldPackets::Auth::AuthSession authSession(std::move(packet));
    EXPECT_FALSE(authSession.ReadNoThrow());
}

TEST(AuthenticationPacketsTest, WritesErrorAuthResponse)
{
    WorldPackets::Auth::AuthResponse response;
    response.Result = ERROR_DENIED;

    // uint32 result, then the two presence bits flushed into one byte
    EXPECT_EQ(PayloadHex(response.Write()), "0300000000");
}

TEST(AuthenticationPacketsTest, WritesQueuedAuthResponse)
{
    WorldPackets::Auth::AuthResponse response;
    response.Result = ERROR_OK;
    response.WaitInfo.emplace();
    response.WaitInfo->WaitCount = 5;

    // result, bits (no success info, wait info), wait count, wait time, HasFCM bit
    EXPECT_EQ(PayloadHex(response.Write()), "00000000" "40" "05000000" "00000000" "00");
}

TEST(AuthenticationPacketsTest, WritesSuccessAuthResponse)
{
    WorldPackets::Auth::ExpansionRequirementContainer const races = { { 1, 0 }, { 24, 4 } };
    WorldPackets::Auth::ExpansionRequirementContainer const classes = { { 10, 4 } };

    WorldPackets::Auth::AuthResponse response;
    response.Result = ERROR_OK;
    WorldPackets::Auth::AuthResponse::AuthSuccessInfo& success = response.SuccessInfo.emplace();
    success.VirtualRealmAddress = 0x01010001;
    success.ActiveExpansionLevel = 5;
    success.AccountExpansionLevel = 5;
    success.VirtualRealms.emplace_back(0x01010001, true, false, "My Realm", "MyRealm");
    success.AvailableRaces = &races;
    success.AvailableClasses = &classes;

    WorldPacket const* packet = response.Write();
    // result (4) + presence bits (1) + 10 header fields (4+4+4+1+1+4+4+4+4+4) + billing (4+4+1)
    // + realm (4 + 3 bytes of bits + 8 + 7) + races (2*2) + classes (2) + trailing bits (1)
    ASSERT_EQ(packet->size(), 4u + 1u + 34u + 9u + 22u + 4u + 2u + 1u);
    EXPECT_EQ(packet->contents()[4], 0x80);      // success info present, no wait info
    EXPECT_EQ(packet->contents()[5], 0x01);      // virtual realm address, little endian
}

TEST(AuthenticationPacketsTest, WritesConnectTo)
{
    WorldPackets::Auth::ConnectTo connectTo;
    connectTo.Key = 0x0102030405060708;
    connectTo.Serial = WorldPackets::Auth::ConnectToSerial::WorldAttempt1;
    connectTo.Payload.Where = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 8086);
    connectTo.Con = 1;

    WorldPacket const* packet = connectTo.Write();
    ASSERT_EQ(packet->size(), 8u + 4u + 256u + 1u);
    EXPECT_EQ(packet->contents()[0], 0x08);
    EXPECT_EQ(packet->contents()[8], 17);        // WorldAttempt1
    EXPECT_EQ(packet->contents()[8 + 4 + 256], 1);
}

TEST(AuthenticationPacketsTest, ReadsAuthContinuedSession)
{
    WorldPacket packet(CMSG_AUTH_CONTINUED_SESSION, 56);
    packet << uint64(1) << uint64(0x8000000100000002);
    for (uint8 i = 0; i < 16 + 24; ++i)
        packet << uint8(i);

    WorldPackets::Auth::AuthContinuedSession continued(std::move(packet));
    ASSERT_TRUE(continued.ReadNoThrow());
    EXPECT_EQ(continued.Key, 0x8000000100000002u);
    EXPECT_EQ(continued.LocalChallenge[0], 0);
    EXPECT_EQ(continued.Digest[0], 16);
}

TEST(AuthenticationPacketsTest, PingPong)
{
    WorldPacket packet(CMSG_PING, 8);
    packet << uint32(42) << uint32(30);
    WorldPackets::Auth::Ping ping(std::move(packet));
    ASSERT_TRUE(ping.ReadNoThrow());
    EXPECT_EQ(ping.Serial, 42u);
    EXPECT_EQ(ping.Latency, 30u);

    EXPECT_EQ(PayloadHex(WorldPackets::Auth::Pong(ping.Serial).Write()), "2A000000");
}

TEST(AuthenticationPacketsTest, WorldSessionKeyIsFortyBytesFromTheJoinKey)
{
    // HMAC-SHA256(join key, server challenge, client challenge, seed) expanded with the SHA256 session key generator
    std::array<uint8, 32> joinKey{};
    std::iota(joinKey.begin(), joinKey.end(), uint8(1));
    Acore::Crypto::HMAC_SHA256 hmac(joinKey);
    hmac.UpdateData(std::string("x"));
    hmac.Finalize();

    SessionKeyGenerator<Acore::Crypto::SHA256> generator(hmac.GetDigest());
    std::array<uint8, 40> first{}, second{};
    generator.Generate(first.data(), 40);
    SessionKeyGenerator<Acore::Crypto::SHA256> generator2(hmac.GetDigest());
    generator2.Generate(second.data(), 40);
    EXPECT_EQ(first, second);
}
