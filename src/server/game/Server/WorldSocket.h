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

#ifndef __WORLDSOCKET_H__
#define __WORLDSOCKET_H__

#include "AuthCrypt.h"
#include "AsyncCallbackProcessor.h"
#include "Common.h"
#include "MPSCQueue.h"
#include "ServerPktHeader.h"
#include "Socket.h"
#include "Util.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <boost/asio/ip/tcp.hpp>

using boost::asio::ip::tcp;

struct z_stream_s;
class WorldAuthenticationHandoffTest;

/// a packet queued for the network thread; encrypted (and maybe compressed) once the session is authenticated
class EncryptablePacket : public WorldPacket
{
public:
    EncryptablePacket(WorldPacket const& packet, bool encrypt) : WorldPacket(packet), _encrypt(encrypt)
    {
        SocketQueueLink.store(nullptr, std::memory_order_relaxed);
    }

    bool NeedsEncryption() const { return _encrypt; }

    std::atomic<EncryptablePacket*> SocketQueueLink;

private:
    bool _encrypt;
};

namespace WorldPackets
{
    namespace Auth
    {
        class AuthSession;
        class AuthContinuedSession;
        class ConnectToFailed;
        class Ping;
    }
}

/// 6.2.4 world socket (TrinityCore 6.x src/server/game/Server/WorldSocket.h): one realm connection per session and,
/// once a character logs in, one instance connection opened through SMSG_CONNECT_TO.
class AC_GAME_API WorldSocket final : public Socket<WorldSocket>
{
    friend class WorldAuthenticationHandoffTest;

    static uint32 const ConnectionInitializeMagic;
    static std::string const ServerConnectionInitialize;
    static std::string const ClientConnectionInitialize;
    static uint32 const MinSizeForCompression;

    static std::array<uint8, 16> const AuthCheckSeed;
    static std::array<uint8, 16> const SessionKeySeed;
    static std::array<uint8, 16> const ContinuedSessionSeed;

    typedef Socket<WorldSocket> BaseSocket;

public:
    WorldSocket(IoContextTcpSocket&& socket);
    ~WorldSocket();

    WorldSocket(WorldSocket const& right) = delete;
    WorldSocket& operator=(WorldSocket const& right) = delete;

    void Start() override;
    bool Update() final;

    void SendPacket(WorldPacket const& packet);
    void SendPacketAndLogOpcode(WorldPacket const& packet);

    ConnectionType GetConnectionType() const { return _type; }

    void SendAuthResponseError(uint32 code);
    void SetWorldSession(WorldSession* session);

    void SetSendBufferSize(std::size_t sendBufferSize) { _sendBufferSize = sendBufferSize; }

    bool IsLoggingPackets() const { return _loggingPackets; }
    void SetPacketLogging(bool state) { _loggingPackets = state; }

protected:
    void OnClose() override;
    SocketReadCallbackResult ReadHandler() final;
    bool ReadHeaderHandler();

    enum class ReadDataHandlerResult
    {
        Ok = 0,
        Error = 1,
        WaitingForQuery = 2
    };

    ReadDataHandlerResult ReadDataHandler();

private:
    void CheckIpCallback(PreparedQueryResult result);
    void InitializeHandler(boost::system::error_code error, std::size_t transferedBytes);

    /// writes network.opcode log
    /// accessing WorldSession is not threadsafe, only do it when holding _worldSessionLock
    void LogOpcodeText(OpcodeClient opcode, std::unique_lock<std::mutex> const& guard) const;

    void WritePacketToBuffer(EncryptablePacket const& packet, MessageBuffer& buffer);
    uint32 CompressPacket(uint8* buffer, WorldPacket const& packet);

    std::size_t GetClientHeaderSize() const;
    std::size_t GetServerHeaderSize(bool encrypted) const;
    void ExtractOpcodeAndSize(ClientPktHeader const* header, uint32& opcode, uint32& size) const;

    void HandleSendAuthSession();
    void HandleAuthSession(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession);
    void HandleAuthSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession, PreparedQueryResult result);
    void HandleAuthContinuedSession(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession);
    void HandleAuthContinuedSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession, PreparedQueryResult result);
    void HandleConnectToFailed(WorldPackets::Auth::ConnectToFailed& connectToFailed);
    bool HandlePing(WorldPackets::Auth::Ping& ping);

    ConnectionType _type;

    std::array<uint8, 16> _serverChallenge;
    AuthCrypt _authCrypt;
    std::array<uint8, 16> _encryptSeed;
    std::array<uint8, 16> _decryptSeed;

    TimePoint _LastPingTime;
    uint32 _OverSpeedPings;

    std::mutex _worldSessionLock;
    WorldSession* _worldSession;
    bool _authed;

    MessageBuffer _headerBuffer;
    MessageBuffer _packetBuffer;

    z_stream_s* _compressionStream;

    MPSCQueue<EncryptablePacket, &EncryptablePacket::SocketQueueLink> _bufferQueue;
    std::size_t _sendBufferSize;

    QueryCallbackProcessor _queryProcessor;
    std::string _ipCountry;

    bool _loggingPackets;
};

#endif
