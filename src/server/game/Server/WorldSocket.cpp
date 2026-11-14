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

#include "WorldSocket.h"
#include "TC9Sidecar.h"
#include "AccountMgr.h"
#include "AuthenticationPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "CharacterPackets.h"
#include "Config.h"
#include "CryptoHash.h"
#include "CryptoRandom.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "HMAC.h"
#include "IPLocation.h"
#include "Opcodes.h"
#include "PacketLog.h"
#include "RBAC.h"
#include "Realm.h"
#include "ScriptMgr.h"
#include "SessionKeyGenerator.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include <zlib.h>
#include <memory>

// 6.2.4 21742 world connection, from TrinityCore 6.x src/server/game/Server/WorldSocket.cpp

#pragma pack(push, 1)

struct CompressedWorldPacket
{
    uint32 UncompressedSize;
    uint32 UncompressedAdler;
    uint32 CompressedAdler;
};

#pragma pack(pop)

uint32 const WorldSocket::ConnectionInitializeMagic = 0xF5EB1CE;
std::string const WorldSocket::ServerConnectionInitialize("WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT");
std::string const WorldSocket::ClientConnectionInitialize("WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER", 48);
uint32 const WorldSocket::MinSizeForCompression = 0x400;

std::array<uint8, 16> const WorldSocket::AuthCheckSeed = { 0xC5, 0xC6, 0x98, 0x95, 0x76, 0x3F, 0x1D, 0xCD, 0xB6, 0xA1, 0x37, 0x28, 0xB3, 0x12, 0xFF, 0x8A };
std::array<uint8, 16> const WorldSocket::SessionKeySeed = { 0x58, 0xCB, 0xCF, 0x40, 0xFE, 0x2E, 0xCE, 0xA6, 0x5A, 0x90, 0xB8, 0x01, 0x68, 0x6C, 0x28, 0x0B };
std::array<uint8, 16> const WorldSocket::ContinuedSessionSeed = { 0x16, 0xAD, 0x0C, 0xD4, 0x46, 0xF9, 0x4F, 0xB2, 0xEF, 0x7D, 0xEA, 0x2A, 0x17, 0x66, 0x4D, 0x2F };

WorldSocket::WorldSocket(IoContextTcpSocket&& socket)
    : Socket(std::move(socket)), _type(CONNECTION_TYPE_REALM), _OverSpeedPings(0), _worldSession(nullptr), _authed(false),
      _compressionStream(nullptr), _sendBufferSize(4096),
      // sessions created while PacketLogFile is configured log their packets (the packet log caches the setting)
      _loggingPackets(sPacketLog->CanLogPacket())
{
    Acore::Crypto::GetRandomBytes(_serverChallenge);
    _headerBuffer.Resize(sizeof(ClientPktHeader::Setup));
}

WorldSocket::~WorldSocket()
{
    if (_compressionStream)
    {
        deflateEnd(_compressionStream);
        delete _compressionStream;
    }
}

void WorldSocket::Start()
{
    std::string ip_address = GetRemoteIpAddress().to_string();

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_IP_INFO);
    stmt->SetData(0, ip_address);

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSocket::CheckIpCallback, this, std::placeholders::_1)));
}

void WorldSocket::CheckIpCallback(PreparedQueryResult result)
{
    if (result)
    {
        bool banned = false;
        do
        {
            Field* fields = result->Fetch();
            if (fields[0].Get<uint64>() != 0)
                banned = true;

        } while (result->NextRow());

        if (banned)
        {
            LOG_ERROR("network", "WorldSocket::CheckIpCallback: Sent Auth Response (IP {} banned).", GetRemoteIpAddress().to_string());
            DelayedCloseSocket();
            return;
        }
    }

    // magic, uint16 length, then the connection string (no terminator) from both sides
    _packetBuffer.Resize(4 + 2 + ClientConnectionInitialize.length());

    AsyncReadWithCallback(&WorldSocket::InitializeHandler);

    MessageBuffer initializer;
    uint16 const initializerSize = uint16(ServerConnectionInitialize.size());
    initializer.Write(&ConnectionInitializeMagic, sizeof(ConnectionInitializeMagic));
    initializer.Write(&initializerSize, sizeof(initializerSize));
    initializer.Write(ServerConnectionInitialize.c_str(), ServerConnectionInitialize.length());

    QueuePacket(std::move(initializer));
}

void WorldSocket::InitializeHandler(boost::system::error_code error, std::size_t transferedBytes)
{
    if (error)
    {
        CloseSocket();
        return;
    }

    GetReadBuffer().WriteCompleted(transferedBytes);

    if (!IsOpen())
        return;

    MessageBuffer& packet = GetReadBuffer();
    if (packet.GetActiveSize() > 0)
    {
        if (_packetBuffer.GetRemainingSpace() > 0)
        {
            // need to receive the initializer
            std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
            _packetBuffer.Write(packet.GetReadPointer(), readHeaderSize);
            packet.ReadCompleted(readHeaderSize);

            if (_packetBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole initializer this time.
                ASSERT(packet.GetActiveSize() == 0);
                AsyncReadWithCallback(&WorldSocket::InitializeHandler);
                return;
            }

            ByteBuffer buffer(std::move(_packetBuffer));

            uint32 magic = buffer.read<uint32>();
            if (magic != ConnectionInitializeMagic)
            {
                LOG_DEBUG("network", "WorldSocket::InitializeHandler: client {} sent a wrong connection magic {:X}", GetRemoteIpAddress().to_string(), magic);
                CloseSocket();
                return;
            }

            uint16 length = buffer.read<uint16>();
            if (length > ClientConnectionInitialize.length())
            {
                CloseSocket();
                return;
            }

            std::string initializer(length, '\0');
            buffer.read(reinterpret_cast<uint8*>(initializer.data()), length);
            if (initializer != ClientConnectionInitialize)
            {
                LOG_DEBUG("network", "WorldSocket::InitializeHandler: client {} sent a wrong connection string", GetRemoteIpAddress().to_string());
                CloseSocket();
                return;
            }

            auto compressionStream = std::make_unique<z_stream>();
            compressionStream->zalloc = (alloc_func)nullptr;
            compressionStream->zfree = (free_func)nullptr;
            compressionStream->opaque = (voidpf)nullptr;
            compressionStream->avail_in = 0;
            compressionStream->next_in = nullptr;
            // raw deflate stream (negative window bits), one stream for the whole connection
            int32 z_res = deflateInit2(compressionStream.get(), sWorld->getIntConfig(CONFIG_COMPRESSION), Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
            if (z_res != Z_OK)
            {
                LOG_ERROR("network", "Can't initialize packet compression (zlib: deflateInit2) Error code: {} ({})", z_res, zError(z_res));
                CloseSocket();
                return;
            }

            _compressionStream = compressionStream.release();

            _packetBuffer.Reset();
            HandleSendAuthSession();
            AsyncRead();
            return;
        }
    }

    AsyncReadWithCallback(&WorldSocket::InitializeHandler);
}

std::size_t WorldSocket::GetClientHeaderSize() const
{
    return _authCrypt.IsInitialized() ? sizeof(ClientPktHeader::Normal) : sizeof(ClientPktHeader::Setup);
}

std::size_t WorldSocket::GetServerHeaderSize(bool encrypted) const
{
    return encrypted ? sizeof(ServerPktHeader::Normal) : sizeof(ServerPktHeader::Setup);
}

bool WorldSocket::Update()
{
    EncryptablePacket* queued;
    if (_bufferQueue.Dequeue(queued))
    {
        // Allocate buffer only when it's needed but not on every Update() call.
        MessageBuffer buffer(_sendBufferSize);
        do
        {
            std::size_t sizeOfHeader = GetServerHeaderSize(queued->NeedsEncryption());
            std::size_t packetSize = queued->size();
            if (packetSize > MinSizeForCompression && queued->NeedsEncryption())
                packetSize = deflateBound(_compressionStream, packetSize + sizeof(uint16)) + sizeof(CompressedWorldPacket);

            if (buffer.GetRemainingSpace() < packetSize + sizeOfHeader)
            {
                QueuePacket(std::move(buffer));
                buffer.Resize(_sendBufferSize);
            }

            if (buffer.GetRemainingSpace() >= packetSize + sizeOfHeader)
                WritePacketToBuffer(*queued, buffer);
            else    // single packet larger than the buffer
            {
                MessageBuffer packetBuffer(packetSize + sizeOfHeader);
                WritePacketToBuffer(*queued, packetBuffer);
                QueuePacket(std::move(packetBuffer));
            }

            delete queued;
        } while (_bufferQueue.Dequeue(queued));

        if (buffer.GetActiveSize() > 0)
            QueuePacket(std::move(buffer));
    }

    if (!BaseSocket::Update())
        return false;

    _queryProcessor.ProcessReadyCallbacks();

    return true;
}

void WorldSocket::HandleSendAuthSession()
{
    Acore::Crypto::GetRandomBytes(_encryptSeed);
    Acore::Crypto::GetRandomBytes(_decryptSeed);

    WorldPackets::Auth::AuthChallenge challenge;
    challenge.Challenge = _serverChallenge;
    std::copy(_encryptSeed.begin(), _encryptSeed.end(), challenge.DosChallenge.begin());
    std::copy(_decryptSeed.begin(), _decryptSeed.end(), challenge.DosChallenge.begin() + 16);
    challenge.DosZeroBits = 1;

    SendPacketAndLogOpcode(*challenge.Write());
}

void WorldSocket::OnClose()
{
    {
        std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
        _worldSession = nullptr;
    }
}

SocketReadCallbackResult WorldSocket::ReadHandler()
{
    if (!IsOpen())
        return SocketReadCallbackResult::Stop;

    MessageBuffer& packet = GetReadBuffer();
    while (packet.GetActiveSize() > 0)
    {
        if (_headerBuffer.GetRemainingSpace() > 0)
        {
            // need to receive the header
            std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _headerBuffer.GetRemainingSpace());
            _headerBuffer.Write(packet.GetReadPointer(), readHeaderSize);
            packet.ReadCompleted(readHeaderSize);

            if (_headerBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole header this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }

            // We just received nice new header
            if (!ReadHeaderHandler())
            {
                CloseSocket();
                return SocketReadCallbackResult::Stop;
            }
        }

        // We have full read header, now check the data payload
        if (_packetBuffer.GetRemainingSpace() > 0)
        {
            // need more data in the payload
            std::size_t readDataSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
            _packetBuffer.Write(packet.GetReadPointer(), readDataSize);
            packet.ReadCompleted(readDataSize);

            if (_packetBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole data this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }
        }

        // just received fresh new payload
        ReadDataHandlerResult result = ReadDataHandler();
        // the client switches to the 6 byte header after CMSG_AUTH_SESSION / CMSG_AUTH_CONTINUED_SESSION
        _headerBuffer.Resize(GetClientHeaderSize());
        _headerBuffer.Reset();
        if (result != ReadDataHandlerResult::Ok)
        {
            if (result != ReadDataHandlerResult::WaitingForQuery)
                CloseSocket();

            return SocketReadCallbackResult::Stop;
        }
    }

    return SocketReadCallbackResult::KeepReading;
}

void WorldSocket::ExtractOpcodeAndSize(ClientPktHeader const* header, uint32& opcode, uint32& size) const
{
    if (_authCrypt.IsInitialized())
    {
        opcode = header->Normal.Command;
        size = header->Normal.Size - 2;
    }
    else
    {
        opcode = header->Setup.Command;
        size = header->Setup.Size - 2;
    }
}

void WorldSocket::SetWorldSession(WorldSession* session)
{
    std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);
    _worldSession = session;
    _authed = true;
}

bool WorldSocket::ReadHeaderHandler()
{
    ASSERT(_headerBuffer.GetActiveSize() == GetClientHeaderSize(), "Header size {} different than expected {}", _headerBuffer.GetActiveSize(), GetClientHeaderSize());

    if (_authCrypt.IsInitialized())
        _authCrypt.DecryptRecv(_headerBuffer.GetReadPointer(), 4);

    ClientPktHeader* header = reinterpret_cast<ClientPktHeader*>(_headerBuffer.GetReadPointer());
    uint32 headerSize = _authCrypt.IsInitialized() ? header->Normal.Size : header->Setup.Size;
    uint32 opcode;
    uint32 size;

    ExtractOpcodeAndSize(header, opcode, size);

    if (headerSize < 2 || !ClientPktHeader::IsValidSize(size) || opcode >= NUM_OPCODE_HANDLERS)
    {
        LOG_ERROR("network", "WorldSocket::ReadHeaderHandler(): client {} sent malformed packet (size: {}, cmd: {})",
            GetRemoteIpAddress().to_string(), size, opcode);
        return false;
    }

    _packetBuffer.Resize(size);
    return true;
}

WorldSocket::ReadDataHandlerResult WorldSocket::ReadDataHandler()
{
    ClientPktHeader* header = reinterpret_cast<ClientPktHeader*>(_headerBuffer.GetReadPointer());
    uint32 cmd;
    uint32 size;

    ExtractOpcodeAndSize(header, cmd, size);

    OpcodeClient opcode = static_cast<OpcodeClient>(cmd);

    WorldPacket packet(opcode, std::move(_packetBuffer), GetConnectionType());
    WorldPacket* packetToQueue;

    if (sPacketLog->CanLogPacket() && IsLoggingPackets())
        sPacketLog->LogPacket(packet, CLIENT_TO_SERVER, GetRemoteIpAddress(), GetRemotePort());

    std::unique_lock<std::mutex> sessionGuard(_worldSessionLock, std::defer_lock);

    switch (opcode)
    {
        case CMSG_PING:
        {
            LogOpcodeText(opcode, sessionGuard);
            WorldPackets::Auth::Ping ping(std::move(packet));
            if (!ping.ReadNoThrow())
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_PING", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }

            return HandlePing(ping) ? ReadDataHandlerResult::Ok : ReadDataHandlerResult::Error;
        }
        case CMSG_AUTH_SESSION:
        {
            LogOpcodeText(opcode, sessionGuard);
            if (_authed)
            {
                // locking just to safely log offending user is probably overkill but we are disconnecting him anyway
                if (sessionGuard.try_lock())
                    LOG_ERROR("network", "WorldSocket::ProcessIncoming: received duplicate CMSG_AUTH_SESSION from {}", _worldSession->GetPlayerInfo());
                return ReadDataHandlerResult::Error;
            }

            std::shared_ptr<WorldPackets::Auth::AuthSession> authSession = std::make_shared<WorldPackets::Auth::AuthSession>(std::move(packet));
            if (!authSession->ReadNoThrow())
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_SESSION", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }

            HandleAuthSession(authSession);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        case CMSG_AUTH_CONTINUED_SESSION:
        {
            LogOpcodeText(opcode, sessionGuard);
            if (_authed)
            {
                if (sessionGuard.try_lock())
                    LOG_ERROR("network", "WorldSocket::ProcessIncoming: received duplicate CMSG_AUTH_CONTINUED_SESSION from {}", _worldSession->GetPlayerInfo());
                return ReadDataHandlerResult::Error;
            }

            std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession = std::make_shared<WorldPackets::Auth::AuthContinuedSession>(std::move(packet));
            if (!authSession->ReadNoThrow())
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_AUTH_CONTINUED_SESSION", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }

            HandleAuthContinuedSession(authSession);
            return ReadDataHandlerResult::WaitingForQuery;
        }
        case CMSG_KEEP_ALIVE:
            sessionGuard.lock();
            LogOpcodeText(opcode, sessionGuard);
            if (_worldSession)
                _worldSession->ResetTimeOutTime(true);
            return ReadDataHandlerResult::Ok;
        case CMSG_LOG_DISCONNECT:
            LogOpcodeText(opcode, sessionGuard);
            packet.rfinish();   // contains uint32 disconnectReason;
            return ReadDataHandlerResult::Ok;
        case CMSG_ENABLE_NAGLE:
            LogOpcodeText(opcode, sessionGuard);
            SetNoDelay(false);
            return ReadDataHandlerResult::Ok;
        case CMSG_CONNECT_TO_FAILED:
        {
            sessionGuard.lock();

            LogOpcodeText(opcode, sessionGuard);
            WorldPackets::Auth::ConnectToFailed connectToFailed(std::move(packet));
            if (!connectToFailed.ReadNoThrow())
            {
                LOG_ERROR("network", "WorldSocket::ReadDataHandler(): client {} sent malformed CMSG_CONNECT_TO_FAILED", GetRemoteIpAddress().to_string());
                return ReadDataHandlerResult::Error;
            }

            HandleConnectToFailed(connectToFailed);
            return ReadDataHandlerResult::Ok;
        }
        case CMSG_TIME_SYNC_RESPONSE:
            packetToQueue = new WorldPacket(std::move(packet), GameTime::Now());
            break;
        default:
            packetToQueue = new WorldPacket(std::move(packet));
            break;
    }

    sessionGuard.lock();

    LogOpcodeText(opcode, sessionGuard);

    if (!_worldSession)
    {
        LOG_ERROR("network.opcode", "ProcessIncoming: Client not authed opcode = {}", uint32(opcode));
        delete packetToQueue;
        return ReadDataHandlerResult::Error;
    }

    OpcodeHandler const* handler = opcodeTable.GetIncomingOpcode(packetToQueue->GetOpcode());
    if (!handler)
    {
        LOG_ERROR("network.opcode", "No defined handler for opcode {} sent by {}", GetOpcodeNameForLogging(static_cast<OpcodeClient>(packetToQueue->GetOpcode())), _worldSession->GetPlayerInfo());
        delete packetToQueue;
        return ReadDataHandlerResult::Ok;
    }

    // Our Idle timer will reset on any non PING opcodes on login screen, allowing us to catch people idling.
    if (packetToQueue->GetOpcode() != CMSG_WARDEN_DATA)
        _worldSession->ResetTimeOutTime(false);

    // Copy the packet to the heap before enqueuing
    _worldSession->QueuePacket(packetToQueue);

    return ReadDataHandlerResult::Ok;
}

void WorldSocket::LogOpcodeText(OpcodeClient opcode, std::unique_lock<std::mutex> const& guard) const
{
    if (!guard)
    {
        LOG_TRACE("network.opcode", "C->S: {} {}", GetRemoteIpAddress().to_string(), GetOpcodeNameForLogging(opcode));
    }
    else
    {
        LOG_TRACE("network.opcode", "C->S: {} {}", (_worldSession ? _worldSession->GetPlayerInfo() : GetRemoteIpAddress().to_string()),
            GetOpcodeNameForLogging(opcode));
    }
}

void WorldSocket::SendPacketAndLogOpcode(WorldPacket const& packet)
{
    LOG_TRACE("network.opcode", "S->C: {} {}", GetRemoteIpAddress().to_string(), GetOpcodeNameForLogging(static_cast<OpcodeServer>(packet.GetOpcode())));
    SendPacket(packet);
}

void WorldSocket::SendPacket(WorldPacket const& packet)
{
    if (!IsOpen())
        return;

    if (sPacketLog->CanLogPacket() && IsLoggingPackets())
        sPacketLog->LogPacket(packet, SERVER_TO_CLIENT, GetRemoteIpAddress(), GetRemotePort());

    _bufferQueue.Enqueue(new EncryptablePacket(packet, _authCrypt.IsInitialized()));
}

void WorldSocket::WritePacketToBuffer(EncryptablePacket const& packet, MessageBuffer& buffer)
{
    ServerPktHeader header;
    std::size_t sizeOfHeader = GetServerHeaderSize(packet.NeedsEncryption());
    uint32 opcode = packet.GetOpcode();
    uint32 packetSize = packet.size();

    // Reserve space for the header
    uint8* headerPos = buffer.GetWritePointer();
    buffer.WriteCompleted(sizeOfHeader);

    if (packetSize > MinSizeForCompression && packet.NeedsEncryption())
    {
        CompressedWorldPacket cmp;
        cmp.UncompressedSize = packetSize + 2;
        cmp.UncompressedAdler = adler32(adler32(0x9827D8F1, reinterpret_cast<Bytef const*>(&opcode), 2), packet.contents(), packetSize);

        // Reserve space for compression info - uncompressed size and checksums
        uint8* compressionInfo = buffer.GetWritePointer();
        buffer.WriteCompleted(sizeof(CompressedWorldPacket));

        uint32 compressedSize = CompressPacket(buffer.GetWritePointer(), packet);

        cmp.CompressedAdler = adler32(0x9827D8F1, buffer.GetWritePointer(), compressedSize);

        memcpy(compressionInfo, &cmp, sizeof(CompressedWorldPacket));
        buffer.WriteCompleted(compressedSize);
        packetSize = compressedSize + sizeof(CompressedWorldPacket);

        opcode = SMSG_COMPRESSED_PACKET;
    }
    else if (!packet.empty())
        buffer.Write(packet.contents(), packet.size());

    packetSize += 2 /*opcode*/;

    if (packet.NeedsEncryption())
    {
        header.Normal.Size = packetSize;
        header.Normal.Command = uint16(opcode);
        _authCrypt.EncryptSend(reinterpret_cast<uint8*>(&header), 4);
    }
    else
    {
        header.Setup.Size = uint16(packetSize);
        header.Setup.Command = uint16(opcode);
    }

    memcpy(headerPos, &header, sizeOfHeader);
}

uint32 WorldSocket::CompressPacket(uint8* buffer, WorldPacket const& packet)
{
    uint32 opcode = packet.GetOpcode();
    uint32 bufferSize = deflateBound(_compressionStream, packet.size() + sizeof(uint16));

    _compressionStream->next_out = buffer;
    _compressionStream->avail_out = bufferSize;
    _compressionStream->next_in = reinterpret_cast<Bytef*>(&opcode);
    _compressionStream->avail_in = sizeof(uint16);

    int32 z_res = deflate(_compressionStream, Z_NO_FLUSH);
    if (z_res != Z_OK)
    {
        LOG_ERROR("network", "Can't compress packet opcode (zlib: deflate) Error code: {} ({}, msg: {})", z_res, zError(z_res), _compressionStream->msg ? _compressionStream->msg : "");
        return 0;
    }

    _compressionStream->next_in = const_cast<Bytef*>(packet.contents());
    _compressionStream->avail_in = packet.size();

    z_res = deflate(_compressionStream, Z_SYNC_FLUSH);
    if (z_res != Z_OK)
    {
        LOG_ERROR("network", "Can't compress packet data (zlib: deflate) Error code: {} ({}, msg: {})", z_res, zError(z_res), _compressionStream->msg ? _compressionStream->msg : "");
        return 0;
    }

    return bufferSize - _compressionStream->avail_out;
}

struct AccountInfo
{
    struct
    {
        uint32 Id;
        bool IsLockedToIP;
        std::string LastIP;
        std::string LockCountry;
        bool IsBanned;
    } BattleNet;

    struct
    {
        uint32 Id;
        std::array<uint8, 32> SessionKey;
        uint8 Expansion;
        uint32 Flags;
        int64 MuteTime;
        LocaleConstant Locale;
        uint32 Recruiter;
        std::string OS;
        uint32 TotalTime;
        bool IsRectuiter;
        AccountTypes Security;
        bool IsBanned;
    } Game;

    bool IsBanned() const { return BattleNet.IsBanned || Game.IsBanned; }

    explicit AccountInfo(Field* fields)
    {
        //           0                   1           2          3                4            5        6           7         8            9     10           11     12           13
        // SELECT a.id, a.session_key_bnet, ba.last_ip, ba.locked, ba.lock_country, a.expansion, a.Flags, a.mutetime, a.locale, a.recruiter, a.os, a.totaltime, ba.id, aa.gmlevel,
        //                                                              14                                                            15    16
        // bab.unbandate > UNIX_TIMESTAMP() OR bab.unbandate = bab.bandate, ab.unbandate > UNIX_TIMESTAMP() OR ab.unbandate = ab.bandate, r.id
        Game.Id = fields[0].Get<uint32>();
        Game.SessionKey = fields[1].Get<Binary, 32>();
        BattleNet.LastIP = fields[2].Get<std::string>();
        BattleNet.IsLockedToIP = fields[3].Get<bool>();
        BattleNet.LockCountry = fields[4].Get<std::string>();
        Game.Expansion = fields[5].Get<uint8>();
        Game.Flags = fields[6].Get<uint32>();
        Game.MuteTime = fields[7].Get<int64>();
        Game.Locale = GetLocaleByName(fields[8].Get<std::string>());
        Game.Recruiter = fields[9].Get<uint32>();
        Game.OS = fields[10].Get<std::string>();
        Game.TotalTime = fields[11].Get<uint32>();
        BattleNet.Id = fields[12].Get<uint32>();
        Game.Security = AccountTypes(fields[13].Get<uint8>());
        BattleNet.IsBanned = fields[14].Get<uint64>() != 0;
        Game.IsBanned = fields[15].Get<uint64>() != 0;
        Game.IsRectuiter = fields[16].Get<uint32>() != 0;

        uint32 world_expansion = sWorld->getIntConfig(CONFIG_EXPANSION);
        if (Game.Expansion > world_expansion)
            Game.Expansion = world_expansion;
    }
};

void WorldSocket::HandleAuthSession(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession)
{
    // Get the account information from the auth database
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_BY_NAME);
    stmt->SetData(0, int32(realm.Id.Realm));
    stmt->SetData(1, authSession->RealmJoinTicket);

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSocket::HandleAuthSessionCallback, this, authSession, std::placeholders::_1)));
}

void WorldSocket::HandleAuthSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthSession> authSession, PreparedQueryResult result)
{
    // Stop if the account is not found
    if (!result)
    {
        // We can not log here, as we do not know the account. Thus, no accountId.
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Sent Auth Response (unknown account '{}').", authSession->RealmJoinTicket);
        DelayedCloseSocket();
        return;
    }

    AccountInfo account(result->Fetch());

    // For hook purposes, we get Remoteaddress at this point.
    std::string address = sConfigMgr->GetOption<bool>("AllowLoggingIPAddressesInDatabase", true, true) ? GetRemoteIpAddress().to_string() : "0.0.0.0";

    // As we don't know if attempted login process by ip works, we update last_attempt_ip right away
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_ATTEMPT_IP);
    stmt->SetData(0, address);
    stmt->SetData(1, authSession->RealmJoinTicket);
    LoginDatabase.Execute(stmt);

    bool const clusterMode = sToCloud9Sidecar->ClusterModeEnabled();
    bool const wardenActive = sWorld->getBoolConfig(CONFIG_WARDEN_ENABLED);

    if (!clusterMode)
    {
        // Check that Key and account name are the same on client and server
        Acore::Crypto::HMAC_SHA256 hmac(account.Game.SessionKey);
        hmac.UpdateData(authSession->LocalChallenge);
        hmac.UpdateData(_serverChallenge);
        hmac.UpdateData(AuthCheckSeed);
        hmac.Finalize();

        if (memcmp(hmac.GetDigest().data(), authSession->Digest.data(), authSession->Digest.size()) != 0)
        {
            LOG_ERROR("network", "WorldSocket::HandleAuthSession: Authentication failed for account: {} ('{}') address: {}", account.Game.Id, authSession->RealmJoinTicket, address);
            DelayedCloseSocket();
            return;
        }

        // world session key: 40 bytes generated from HMAC-SHA256(join session key, server challenge, client challenge, seed)
        Acore::Crypto::HMAC_SHA256 sessionKeyHmac(account.Game.SessionKey);
        sessionKeyHmac.UpdateData(_serverChallenge);
        sessionKeyHmac.UpdateData(authSession->LocalChallenge);
        sessionKeyHmac.UpdateData(SessionKeySeed);
        sessionKeyHmac.Finalize();

        SessionKey sessionKey;
        SessionKeyGenerator<Acore::Crypto::SHA256> sessionKeyGenerator(sessionKeyHmac.GetDigest());
        sessionKeyGenerator.Generate(sessionKey.data(), uint32(sessionKey.size()));

        _authCrypt.Init(sessionKey);

        // the instance connection authenticates against this key
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_INFO_CONTINUED_SESSION);
        stmt->SetData(0, sessionKey);
        stmt->SetData(1, account.Game.Id);
        LoginDatabase.Execute(stmt);
    }

    // First reject the connection if packet contains invalid data or realm state doesn't allow logging in
    if (sWorld->IsClosed())
    {
        SendAuthResponseError(ERROR_DENIED);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: World closed, denying client ({}).", GetRemoteIpAddress().to_string());
        DelayedCloseSocket();
        return;
    }

    if (!clusterMode && authSession->RealmID != realm.Id.Realm)
    {
        SendAuthResponseError(ERROR_DENIED);
        LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} requested connecting with realm id {} but this realm has id {} set in config.",
            GetRemoteIpAddress().to_string(), authSession->RealmID, realm.Id.Realm);
        DelayedCloseSocket();
        return;
    }

    if (!clusterMode)
    {
        // Must be done before WorldSession is created
        if (wardenActive && account.Game.OS != "Win" && account.Game.OS != "Wn64" && account.Game.OS != "Mc64")
        {
            SendAuthResponseError(ERROR_DENIED);
            LOG_ERROR("network", "WorldSocket::HandleAuthSession: Client {} attempted to log in using invalid client OS ({}).", address, account.Game.OS);
            DelayedCloseSocket();
            return;
        }

        if (IpLocationRecord const* location = sIPLocation->GetLocationRecord(address))
            _ipCountry = location->CountryCode;

        ///- Re-check ip locking (same check as in auth).
        if (account.BattleNet.IsLockedToIP)
        {
            if (account.BattleNet.LastIP != address)
            {
                SendAuthResponseError(ERROR_RISK_ACCOUNT_LOCKED);
                LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account IP differs. Original IP: {}, new IP: {}).", account.BattleNet.LastIP, address);
                // We could log on hook only instead of an additional db log, however action logger is config based. Better keep DB logging as well
                sScriptMgr->OnFailedAccountLogin(account.Game.Id);
                DelayedCloseSocket();
                return;
            }
        }
        else if (!account.BattleNet.LockCountry.empty() && account.BattleNet.LockCountry != "00" && !_ipCountry.empty())
        {
            if (account.BattleNet.LockCountry != _ipCountry)
            {
                SendAuthResponseError(ERROR_RISK_ACCOUNT_LOCKED);
                LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account country differs. Original country: {}, new country: {}).", account.BattleNet.LockCountry, _ipCountry);
                // We could log on hook only instead of an additional db log, however action logger is config based. Better keep DB logging as well
                sScriptMgr->OnFailedAccountLogin(account.Game.Id);
                DelayedCloseSocket();
                return;
            }
        }

        //! Negative mutetime indicates amount of seconds to be muted effective on next login - which is now.
        if (account.Game.MuteTime < 0)
        {
            account.Game.MuteTime = GameTime::GetGameTime().count() + std::llabs(account.Game.MuteTime);

            stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME_LOGIN);
            stmt->SetData(0, account.Game.MuteTime);
            stmt->SetData(1, account.Game.Id);
            LoginDatabase.Execute(stmt);
        }

        if (account.IsBanned())
        {
            SendAuthResponseError(ERROR_GAME_ACCOUNT_BANNED);
            LOG_ERROR("network", "WorldSocket::HandleAuthSession: Sent Auth Response (Account banned).");
            sScriptMgr->OnFailedAccountLogin(account.Game.Id);
            DelayedCloseSocket();
            return;
        }

        // Check locked state for server
        AccountTypes allowedAccountType = sWorld->GetPlayerSecurityLimit();
        LOG_DEBUG("network", "Allowed Level: {} Player Level {}", allowedAccountType, account.Game.Security);
        if (allowedAccountType > SEC_PLAYER && account.Game.Security < allowedAccountType)
        {
            SendAuthResponseError(ERROR_SERVER_IS_PRIVATE);
            LOG_DEBUG("network", "WorldSocket::HandleAuthSession: User tries to login but his security level is not enough");
            sScriptMgr->OnFailedAccountLogin(account.Game.Id);
            DelayedCloseSocket();
            return;
        }

        LOG_DEBUG("network", "WorldSocket::HandleAuthSession: Client '{}' authenticated successfully from {}.", authSession->RealmJoinTicket, address);

        // Update the last_ip in the database as it was successful for login
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_IP);
        stmt->SetData(0, address);
        stmt->SetData(1, authSession->RealmJoinTicket);
        LoginDatabase.Execute(stmt);
    }

    // At this point, we can safely hook a successful login
    sScriptMgr->OnAccountLogin(account.Game.Id);

    _authed = true;

    sScriptMgr->OnLastIpUpdate(account.Game.Id, address);

    _worldSession = new WorldSession(account.Game.Id, std::move(authSession->RealmJoinTicket), account.Game.Flags, shared_from_this(), account.Game.Security,
        account.Game.Expansion, account.Game.MuteTime, account.Game.Locale, account.Game.Recruiter, account.Game.IsRectuiter,
        account.Game.Security ? true : false, account.Game.TotalTime);

    _worldSession->SetBattlenetAccountId(account.BattleNet.Id);
    _worldSession->SetOS(account.Game.OS);
    _worldSession->ReadAddonsInfo(authSession->AddonInfo);

    // Initialize Warden system only if it is enabled by config
    if (!clusterMode && wardenActive)
    {
        SessionKey wardenKey{};
        std::copy(account.Game.SessionKey.begin(), account.Game.SessionKey.end(), wardenKey.begin());
        _worldSession->InitWarden(wardenKey, account.Game.OS);
    }

    _worldSession->ValidateAccountFlags();

    sWorldSessionMgr->AddSession(_worldSession);

    AsyncRead();
}

void WorldSocket::HandleAuthContinuedSession(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession)
{
    WorldSession::ConnectToKey key;
    key.Raw = authSession->Key;

    _type = ConnectionType(key.Fields.ConnectionType);
    if (_type != CONNECTION_TYPE_INSTANCE)
    {
        SendAuthResponseError(ERROR_DENIED);
        DelayedCloseSocket();
        return;
    }

    uint32 accountId = uint32(key.Fields.AccountId);
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO_CONTINUED_SESSION);
    stmt->SetData(0, accountId);

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSocket::HandleAuthContinuedSessionCallback, this, authSession, std::placeholders::_1)));
}

void WorldSocket::HandleAuthContinuedSessionCallback(std::shared_ptr<WorldPackets::Auth::AuthContinuedSession> authSession, PreparedQueryResult result)
{
    if (!result)
    {
        SendAuthResponseError(ERROR_DENIED);
        DelayedCloseSocket();
        return;
    }

    WorldSession::ConnectToKey key;
    key.Raw = authSession->Key;

    uint32 accountId = uint32(key.Fields.AccountId);
    Field* fields = result->Fetch();
    std::string login = fields[0].Get<std::string>();
    SessionKey sessionKey = fields[1].Get<Binary, SESSION_KEY_LENGTH>();

    Acore::Crypto::HMAC_SHA256 hmac(sessionKey);
    hmac.UpdateData(reinterpret_cast<uint8 const*>(&authSession->Key), sizeof(authSession->Key));
    hmac.UpdateData(authSession->LocalChallenge);
    hmac.UpdateData(_serverChallenge);
    hmac.UpdateData(ContinuedSessionSeed);
    hmac.Finalize();

    if (memcmp(hmac.GetDigest().data(), authSession->Digest.data(), authSession->Digest.size()) != 0)
    {
        LOG_ERROR("network", "WorldSocket::HandleAuthContinuedSession: Authentication failed for account: {} ('{}') address: {}", accountId, login, GetRemoteIpAddress().to_string());
        DelayedCloseSocket();
        return;
    }

    _authCrypt.Init(sessionKey, _encryptSeed, _decryptSeed);

    sWorldSessionMgr->AddInstanceSocket(std::static_pointer_cast<WorldSocket>(shared_from_this()), authSession->Key);
    AsyncRead();
}

void WorldSocket::HandleConnectToFailed(WorldPackets::Auth::ConnectToFailed& connectToFailed)
{
    if (!_worldSession || !_worldSession->PlayerLoading())
        return;

    switch (connectToFailed.Serial)
    {
        case WorldPackets::Auth::ConnectToSerial::WorldAttempt1:
            _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt2);
            break;
        case WorldPackets::Auth::ConnectToSerial::WorldAttempt2:
            _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt3);
            break;
        case WorldPackets::Auth::ConnectToSerial::WorldAttempt3:
            _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt4);
            break;
        case WorldPackets::Auth::ConnectToSerial::WorldAttempt4:
            _worldSession->SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt5);
            break;
        case WorldPackets::Auth::ConnectToSerial::WorldAttempt5:
            LOG_ERROR("network", "{} failed to connect 5 times to world socket, aborting login", _worldSession->GetPlayerInfo());
            _worldSession->AbortLogin(LoginFailureReason::NoWorld);
            break;
        default:
            break;
    }
}

void WorldSocket::SendAuthResponseError(uint32 code)
{
    WorldPackets::Auth::AuthResponse response;
    response.Result = code;
    SendPacketAndLogOpcode(*response.Write());
}

bool WorldSocket::HandlePing(WorldPackets::Auth::Ping& ping)
{
    using namespace std::chrono;

    if (_LastPingTime == TimePoint())
    {
        _LastPingTime = steady_clock::now();
    }
    else
    {
        TimePoint now = steady_clock::now();
        steady_clock::duration diff = now - _LastPingTime;

        _LastPingTime = now;

        if (diff < seconds(27))
        {
            ++_OverSpeedPings;

            uint32 maxAllowed = sWorld->getIntConfig(CONFIG_MAX_OVERSPEED_PINGS);

            if (maxAllowed && _OverSpeedPings > maxAllowed)
            {
                std::unique_lock<std::mutex> sessionGuard(_worldSessionLock);

                if (_worldSession && !_worldSession->HasPermission(rbac::RBAC_PERM_SKIP_CHECK_OVERSPEED_PING))
                {
                    LOG_ERROR("network", "WorldSocket::HandlePing: {} kicked for over-speed pings (address: {})",
                        _worldSession->GetPlayerInfo(), GetRemoteIpAddress().to_string());

                    return false;
                }
            }
        }
        else
            _OverSpeedPings = 0;
    }

    {
        std::lock_guard<std::mutex> sessionGuard(_worldSessionLock);

        if (_worldSession)
            _worldSession->SetLatency(ping.Latency);
        else
        {
            LOG_ERROR("network", "WorldSocket::HandlePing: peer sent CMSG_PING, but is not authenticated or got recently kicked, address = {}", GetRemoteIpAddress().to_string());
            return false;
        }
    }

    SendPacketAndLogOpcode(*WorldPackets::Auth::Pong(ping.Serial).Write());
    return true;
}
