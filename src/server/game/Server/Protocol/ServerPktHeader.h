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

#include "Define.h"

#pragma pack(push, 1)

/// 6.2.4 world packet headers (TrinityCore 6.x WorldSocket)
/// - before encryption (connection setup, SMSG_AUTH_CHALLENGE): uint16 size (payload + opcode), uint16 opcode
/// - encrypted: uint32 size (payload + opcode), uint16 opcode; the first 4 bytes are encrypted
union ServerPktHeader
{
    struct
    {
        uint16 Size;
        uint16 Command;
    } Setup;

    struct
    {
        uint32 Size;
        uint16 Command;
    } Normal;
};

union ClientPktHeader
{
    struct
    {
        uint16 Size;
        uint16 Command;
    } Setup;

    struct
    {
        uint32 Size;
        uint16 Command;
    } Normal;

    static bool IsValidSize(uint32 size) { return size < 10240; }
};

#pragma pack(pop)

#endif
