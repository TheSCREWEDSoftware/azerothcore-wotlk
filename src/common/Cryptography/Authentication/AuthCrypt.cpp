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

#include "AuthCrypt.h"
#include "Errors.h"
#include "HMAC.h"

void AuthCrypt::Init(SessionKey const& K)
{
    // 5.4.8 / 6.x world header keys (3.3.5a and 4.3.4 use CC98AE04.../C2B3723C...)
    std::array<uint8, 16> const ServerEncryptionKey = { 0x08, 0xF1, 0x95, 0x9F, 0x47, 0xE5, 0xD2, 0xDB, 0xA1, 0x3D, 0x77, 0x8F, 0x3F, 0x3E, 0xE7, 0x00 };
    std::array<uint8, 16> const ServerDecryptionKey = { 0x40, 0xAA, 0xD3, 0x92, 0x26, 0x71, 0x43, 0x47, 0x3A, 0x31, 0x08, 0xA6, 0xE7, 0xDC, 0x98, 0x2A };
    Init(K, ServerEncryptionKey, ServerDecryptionKey);
}

void AuthCrypt::Init(SessionKey const& K, std::array<uint8, 16> const& serverEncryptSeed, std::array<uint8, 16> const& clientDecryptSeed)
{
    _serverEncrypt.Init(Acore::Crypto::HMAC_SHA1::GetDigestOf(serverEncryptSeed, K));
    _clientDecrypt.Init(Acore::Crypto::HMAC_SHA1::GetDigestOf(clientDecryptSeed, K));

    // Drop first 1024 bytes, as WoW uses ARC4-drop1024.
    std::array<uint8, 1024> syncBuf{};
    _serverEncrypt.UpdateData(syncBuf);
    _clientDecrypt.UpdateData(syncBuf);

    _initialized = true;
}

void AuthCrypt::DecryptRecv(uint8* data, std::size_t len)
{
    ASSERT(_initialized);
    _clientDecrypt.UpdateData(data, len);
}

void AuthCrypt::EncryptSend(uint8* data, std::size_t len)
{
    ASSERT(_initialized);
    _serverEncrypt.UpdateData(data, len);
}
