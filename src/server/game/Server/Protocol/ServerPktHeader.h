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

#ifndef __SERVERPKTHDR_H__
#define __SERVERPKTHDR_H__

#include "Log.h"

#pragma pack(push, 1)

// 5.4.8 server packet header (SkyFire_548 WorldSocket)
// - encrypted: one uint32, (payload size << 13) | (opcode & 0x1FFF), encrypted as 4 bytes
// - before encryption: uint16 size (payload + opcode) then uint16 opcode, little endian
struct ServerPktHeader
{
    static constexpr uint32 OPCODE_MASK = 0x1FFF;

    ServerPktHeader(uint32 payloadSize, uint16 cmd, bool encrypted) : size(payloadSize)
    {
        if (encrypted)
        {
            uint32 data = (payloadSize << 13) | (cmd & OPCODE_MASK);
            header[0] = uint8(data);
            header[1] = uint8(data >> 8);
            header[2] = uint8(data >> 16);
            header[3] = uint8(data >> 24);
        }
        else
        {
            uint32 const sizeWithOpcode = payloadSize + 2;
            header[0] = uint8(sizeWithOpcode);
            header[1] = uint8(sizeWithOpcode >> 8);
            header[2] = uint8(cmd);
            header[3] = uint8(cmd >> 8);
        }
    }

    uint8 getHeaderLength() const { return 4; }

    const uint32 size;
    uint8 header[4];
};

#pragma pack(pop)

#endif
