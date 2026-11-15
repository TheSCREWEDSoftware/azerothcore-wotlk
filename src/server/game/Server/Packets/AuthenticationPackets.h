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

#ifndef AuthenticationPackets_h__
#define AuthenticationPackets_h__

#include "BigNumber.h"
#include "Packet.h"
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <optional>

namespace WorldPackets
{
    namespace Auth
    {
        class AuthChallenge final : public ServerPacket
        {
        public:
            AuthChallenge() : ServerPacket(SMSG_AUTH_CHALLENGE, 37) { }

            WorldPacket const* Write() override;

            std::array<uint8, 32> DosChallenge = {};
            std::array<uint8, 4> Challenge = {};
            uint8 DosZeroBits = 1;
        };

        class AuthSession final : public ClientPacket
        {
        public:
            explicit AuthSession(WorldPacket&& packet) : ClientPacket(CMSG_AUTH_SESSION, std::move(packet)) { }

            void Read() override;

            int32 LoginServerID = 0;
            uint32 BattlegroupID = 0;
            int8 LoginServerType = 0;
            uint64 DosResponse = 0;
            uint16 Build = 0;
            uint32 RealmID = 0;
            int8 BuildType = 0;
            std::array<uint8, 4> LocalChallenge = {};
            uint32 RegionID = 0;
            std::array<uint8, 20> Digest = {};
            ByteBuffer AddonInfo;
            bool UseIPv6 = false;
            std::string Account;
        };

        struct AuthSuccessInfo
        {
            uint32 TimeRemain = 0;
            uint8 ActiveExpansionLevel = 0;
            uint32 TimeSecondsUntilPCKick = 0;
            uint8 AccountExpansionLevel = 0;
            uint32 TimeRested = 0;
            uint8 TimeOptions = 0;
        };

        struct AuthWaitInfo
        {
            uint32 WaitCount = 0;
            bool HasFCM = false;
        };

        class AuthResponse final : public ServerPacket
        {
        public:
            explicit AuthResponse(uint8 result) : ServerPacket(SMSG_AUTH_RESPONSE, 21), Result(result) { }

            WorldPacket const* Write() override;

            std::optional<AuthSuccessInfo> SuccessInfo;
            std::optional<AuthWaitInfo> WaitInfo;
            uint8 Result;
        };

        /// CMSG_AUTH_CONTINUED_SESSION: authenticates the instance connection opened after SMSG_CONNECT_TO
        class AuthContinuedSession final : public ClientPacket
        {
        public:
            explicit AuthContinuedSession(WorldPacket&& packet) : ClientPacket(CMSG_AUTH_CONTINUED_SESSION, std::move(packet)) { }

            void Read() override;

            uint64 DosResponse = 0;
            uint64 Key = 0;
            std::array<uint8, 20> Digest = {};
        };

        /// SMSG_RESUME_COMMS: must be sent on the instance connection once it is linked to the session
        class ResumeComms final : public ServerPacket
        {
        public:
            ResumeComms() : ServerPacket(SMSG_RESUME_COMMS, 0) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        enum class ConnectToSerial : uint32
        {
            None            = 0,
            Realm           = 14,
            WorldAttempt1   = 17,
            WorldAttempt2   = 35,
            WorldAttempt3   = 53,
            WorldAttempt4   = 71,
            WorldAttempt5   = 89
        };

        /// SMSG_CONNECT_TO: tells the client where to open the instance connection. The address block is RSA signed
        /// with the key TCPP's client_launcher patches into the 15595 client.
        class ConnectTo final : public ServerPacket
        {
            static std::string const Haiku;
            static uint8 const PiDigits[142];

            struct ConnectPayload
            {
                boost::asio::ip::tcp::endpoint Where;
                uint32 Adler32 = 0;
                uint8 XorMagic = 0x2A;
            };

        public:
            ConnectTo();

            WorldPacket const* Write() override;

            uint64 Key = 0;
            ConnectToSerial Serial = ConnectToSerial::None;
            ConnectPayload Payload;
            uint8 Con = 0;

        private:
            BigNumber p;
            BigNumber q;
            BigNumber dmp1;
            BigNumber dmq1;
            BigNumber iqmp;
        };

        class ConnectToFailed final : public ClientPacket
        {
        public:
            explicit ConnectToFailed(WorldPacket&& packet) : ClientPacket(CMSG_CONNECT_TO_FAILED, std::move(packet)) { }

            void Read() override;

            ConnectToSerial Serial = ConnectToSerial::None;
            uint8 Con = 0;
        };
    }
}

#endif // AuthenticationPackets_h__
