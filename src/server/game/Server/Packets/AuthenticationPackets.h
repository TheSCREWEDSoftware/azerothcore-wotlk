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

#include "Packet.h"
#include "Optional.h"
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <optional>
#include <vector>

/// 6.2.4 21742 authentication packets, from TrinityCore 6.x src/server/game/Server/Packets/AuthenticationPackets.h
namespace WorldPackets
{
    namespace Auth
    {
        /// packets the world socket handles itself, before or outside the world session
        class EarlyProcessClientPacket : public ClientPacket
        {
        public:
            EarlyProcessClientPacket(OpcodeClient opcode, WorldPacket&& packet) : ClientPacket(opcode, std::move(packet)) { }

            bool ReadNoThrow();
        };

        class Ping final : public EarlyProcessClientPacket
        {
        public:
            explicit Ping(WorldPacket&& packet) : EarlyProcessClientPacket(CMSG_PING, std::move(packet)) { }

            uint32 Serial = 0;
            uint32 Latency = 0;

        private:
            void Read() override;
        };

        class Pong final : public ServerPacket
        {
        public:
            explicit Pong(uint32 serial) : ServerPacket(SMSG_PONG, 4), Serial(serial) { }

            WorldPacket const* Write() override;

            uint32 Serial = 0;
        };

        class AuthChallenge final : public ServerPacket
        {
        public:
            AuthChallenge() : ServerPacket(SMSG_AUTH_CHALLENGE, 4 * 8 + 16 + 1) { }

            WorldPacket const* Write() override;

            std::array<uint8, 16> Challenge = {};
            std::array<uint8, 32> DosChallenge = {};    ///< encryption seeds of the instance connection (server, client)
            uint8 DosZeroBits = 0;
        };

        class AuthSession final : public EarlyProcessClientPacket
        {
        public:
            static uint32 const DigestLength = 24;

            explicit AuthSession(WorldPacket&& packet) : EarlyProcessClientPacket(CMSG_AUTH_SESSION, std::move(packet)) { }

            uint16 Build = 0;
            int8 BuildType = 0;
            uint32 RegionID = 0;
            uint32 BattlegroupID = 0;
            uint32 RealmID = 0;
            std::array<uint8, 16> LocalChallenge = {};
            std::array<uint8, DigestLength> Digest = {};
            uint64 DosResponse = 0;
            ByteBuffer AddonInfo;
            std::string RealmJoinTicket;                ///< the game account name (bnetserver JoinRealm)
            bool UseIPv6 = false;

        private:
            void Read() override;
        };

        /// race or class id and the expansion it requires
        typedef std::vector<std::pair<uint8, uint8>> ExpansionRequirementContainer;

        struct CharacterTemplateClass
        {
            uint8 FactionGroup = 0;
            uint8 ClassID = 0;
        };

        struct CharacterTemplate
        {
            uint32 TemplateSetId = 0;
            std::vector<CharacterTemplateClass> Classes;
            std::string Name;
            std::string Description;
        };

        class AuthResponse final : public ServerPacket
        {
        public:
            struct RealmInfo
            {
                RealmInfo(uint32 realmAddress, bool isHomeRealm, bool isInternalRealm, std::string const& realmNameActual, std::string const& realmNameNormalized) :
                    RealmAddress(realmAddress), IsLocal(isHomeRealm), IsInternalRealm(isInternalRealm), RealmNameActual(realmNameActual), RealmNameNormalized(realmNameNormalized) { }

                uint32 RealmAddress;             ///< RealmHandle::GetAddress()
                bool IsLocal;                    ///< true for the account's home realm
                bool IsInternalRealm;
                std::string RealmNameActual;
                std::string RealmNameNormalized; ///< the realm name without spaces
            };

            struct AuthSuccessInfo
            {
                struct BillingInfo
                {
                    uint32 BillingPlan = 0;
                    uint32 TimeRemain = 0;
                    bool InGameRoom = false;
                };

                uint8 AccountExpansionLevel = 0;
                uint8 ActiveExpansionLevel = 0;
                uint32 TimeRested = 0;

                uint32 VirtualRealmAddress = 0;
                uint32 TimeSecondsUntilPCKick = 0;
                uint32 CurrencyID = 0;

                BillingInfo Billing;

                std::vector<RealmInfo> VirtualRealms;
                std::vector<CharacterTemplate> Templates;

                ExpansionRequirementContainer const* AvailableClasses = nullptr;
                ExpansionRequirementContainer const* AvailableRaces = nullptr;

                bool IsExpansionTrial = false;
                bool ForceCharacterTemplate = false;
                Optional<uint16> NumPlayersHorde;
                Optional<uint16> NumPlayersAlliance;
            };

            struct AuthWaitInfo
            {
                uint32 WaitCount = 0;   ///< position in the login queue
                uint32 WaitTime = 0;    ///< minutes; 0 shows "unknown time"
                bool HasFCM = false;
            };

            AuthResponse() : ServerPacket(SMSG_AUTH_RESPONSE, 132) { }

            WorldPacket const* Write() override;

            Optional<AuthSuccessInfo> SuccessInfo;
            Optional<AuthWaitInfo> WaitInfo;
            uint32 Result = 0;  ///< BattlenetRpcErrorCode
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

        /// tells the client to open the instance connection; the address is signed with the client's RSA key
        class ConnectTo final : public ServerPacket
        {
        public:
            struct ConnectPayload
            {
                boost::asio::ip::tcp::endpoint Where;
                uint32 Adler32 = 0;
                uint8 XorMagic = 0x2A;
                std::array<uint8, 32> PanamaKey = {};
            };

            ConnectTo();

            WorldPacket const* Write() override;

            uint64 Key = 0;
            ConnectToSerial Serial = ConnectToSerial::None;
            ConnectPayload Payload;
            uint8 Con = 0;
        };

        class AuthContinuedSession final : public EarlyProcessClientPacket
        {
        public:
            static uint32 const DigestLength = 24;

            explicit AuthContinuedSession(WorldPacket&& packet) : EarlyProcessClientPacket(CMSG_AUTH_CONTINUED_SESSION, std::move(packet)) { }

            uint64 DosResponse = 0;
            uint64 Key = 0;
            std::array<uint8, 16> LocalChallenge = {};
            std::array<uint8, DigestLength> Digest = {};

        private:
            void Read() override;
        };

        class ResumeComms final : public ServerPacket
        {
        public:
            explicit ResumeComms(ConnectionType connection) : ServerPacket(SMSG_RESUME_COMMS, 0, connection) { }

            WorldPacket const* Write() override { return &_worldPacket; }
        };

        class ConnectToFailed final : public EarlyProcessClientPacket
        {
        public:
            explicit ConnectToFailed(WorldPacket&& packet) : EarlyProcessClientPacket(CMSG_CONNECT_TO_FAILED, std::move(packet)) { }

            ConnectToSerial Serial = ConnectToSerial::None;
            uint8 Con = 0;

        private:
            void Read() override;
        };
    }
}

#endif // AuthenticationPackets_h__
