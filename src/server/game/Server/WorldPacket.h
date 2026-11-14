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

#ifndef _WORLDPACKET_H_
#define _WORLDPACKET_H_

#include "ByteBuffer.h"
#include "Duration.h"
#include "Opcodes.h"

class WorldPacket : public ByteBuffer
{
public:
    // just container for later use
    WorldPacket() : ByteBuffer(0) { }

    explicit WorldPacket(uint16 opcode, std::size_t res = 200, ConnectionType connection = CONNECTION_TYPE_DEFAULT) :
        ByteBuffer(res), m_opcode(opcode), _connection(connection) { }

    WorldPacket(WorldPacket&& packet) noexcept :
        ByteBuffer(std::move(packet)), m_opcode(packet.m_opcode), _connection(packet._connection) { }

    WorldPacket(WorldPacket&& packet, TimePoint receivedTime) :
        ByteBuffer(std::move(packet)), m_opcode(packet.m_opcode), _connection(packet._connection), m_receivedTime(receivedTime) { }

    WorldPacket(WorldPacket const& right) :
        ByteBuffer(right), m_opcode(right.m_opcode), _connection(right._connection) { }

    WorldPacket& operator=(WorldPacket const& right)
    {
        if (this != &right)
        {
            m_opcode = right.m_opcode;
            _connection = right._connection;
            ByteBuffer::operator=(right);
        }

        return *this;
    }

    WorldPacket& operator=(WorldPacket&& right) noexcept
    {
        if (this != &right)
        {
            m_opcode = right.m_opcode;
            _connection = right._connection;
            ByteBuffer::operator=(std::move(right));
        }

        return *this;
    }

    WorldPacket(uint16 opcode, MessageBuffer&& buffer, ConnectionType connection = CONNECTION_TYPE_DEFAULT) :
        ByteBuffer(std::move(buffer)), m_opcode(opcode), _connection(connection) { }

    void Initialize(uint16 opcode, std::size_t newres = 200)
    {
        clear();
        _storage.reserve(newres);
        m_opcode = opcode;
    }

    [[nodiscard]] uint16 GetOpcode() const { return m_opcode; }
    void SetOpcode(uint16 opcode) { m_opcode = opcode; }

    [[nodiscard]] TimePoint GetReceivedTime() const { return m_receivedTime; }

    /// socket the packet came from, or for a server packet the one it must go to (CONNECTION_TYPE_DEFAULT: opcode table)
    [[nodiscard]] ConnectionType GetConnection() const { return _connection; }
    void SetConnection(ConnectionType connection) { _connection = connection; }

protected:
    uint16 m_opcode{NULL_OPCODE};
    ConnectionType _connection{CONNECTION_TYPE_DEFAULT};
    TimePoint m_receivedTime; // only set for a specific set of opcodes, for performance reasons.
};

#endif
