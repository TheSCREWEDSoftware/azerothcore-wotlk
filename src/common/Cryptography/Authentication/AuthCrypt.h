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

#ifndef _AUTHCRYPT_H
#define _AUTHCRYPT_H

#include "ARC4.h"
#include "AuthDefines.h"

class AC_COMMON_API AuthCrypt
{
public:
    AuthCrypt() = default;

    void Init(SessionKey const& K);
    /// 4.3.4 instance connection: the ARC4 keys are HMAC-SHA1(seed, K) with the seeds sent in SMSG_AUTH_CHALLENGE
    void Init(SessionKey const& K, std::array<uint8, 16> const& serverEncryptSeed, std::array<uint8, 16> const& clientDecryptSeed);
    void DecryptRecv(uint8* data, std::size_t len);
    void EncryptSend(uint8* data, std::size_t len);

    bool IsInitialized() const { return _initialized; }

private:
    Acore::Crypto::ARC4 _clientDecrypt;
    Acore::Crypto::ARC4 _serverEncrypt;
    bool _initialized{ false };
};
#endif
