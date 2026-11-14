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

// Battle.net (6.x client) side of the realm list, from TrinityCore 6.x src/server/shared/Realm/RealmList.cpp

#include "RealmList.h"
#include "BattlenetRpcErrorCodes.h"
#include "BigNumber.h"
#include "CryptoHash.h"
#include "DatabaseEnv.h"
#include "ProtobufJSON.h"
#include "RealmList.pb.h"
#include "Util.h"
#include "game_utilities_service.pb.h"
#include <boost/asio/ip/tcp.hpp>
#include <zlib.h>

namespace
{
    // the client expects the uncompressed size (including the terminating zero) followed by zlib data
    std::vector<uint8> Compress(std::string const& json)
    {
        uLong compressedLength = compressBound(json.length() + 1);
        std::vector<uint8> compressed(compressedLength + 4);
        uint32 uncompressedLength = uint32(json.length() + 1);
        std::memcpy(compressed.data(), &uncompressedLength, sizeof(uncompressedLength));

        if (compress(compressed.data() + 4, &compressedLength, reinterpret_cast<uint8 const*>(json.c_str()), json.length() + 1) != Z_OK)
            return {};

        compressed.resize(compressedLength + 4);
        return compressed;
    }

    void FillVersion(JSON::RealmList::ClientVersion* version, RealmBuildInfo const* buildInfo, uint32 build)
    {
        if (buildInfo)
        {
            version->set_versionmajor(buildInfo->MajorVersion);
            version->set_versionminor(buildInfo->MinorVersion);
            version->set_versionrevision(buildInfo->BugfixVersion);
            version->set_versionbuild(buildInfo->Build);
        }
        else
        {
            version->set_versionmajor(6);
            version->set_versionminor(2);
            version->set_versionrevision(4);
            version->set_versionbuild(build);
        }
    }
}

void RealmList::WriteSubRegions(bgs::protocol::game_utilities::v1::GetAllValuesForAttributeResponse* response) const
{
    for (std::string const& subRegion : GetSubRegions())
        response->add_attribute_value()->set_string_value(subRegion);
}

std::vector<uint8> RealmList::GetRealmEntryJSON(RealmHandle const& id, uint32 build) const
{
    Realm const* realm = GetRealm(id);
    if (!realm || (realm->Flags & REALM_FLAG_OFFLINE) || realm->Build != build)
        return {};

    JSON::RealmList::RealmEntry realmEntry;
    realmEntry.set_wowrealmaddress(realm->Id.GetAddress());
    realmEntry.set_cfgtimezonesid(1);
    realmEntry.set_populationstate(std::max(uint32(realm->PopulationLevel), 1u));
    realmEntry.set_cfgcategoriesid(realm->Timezone);
    FillVersion(realmEntry.mutable_version(), GetBuildInfo(realm->Build), realm->Build);
    realmEntry.set_cfgrealmsid(realm->Id.Realm);
    realmEntry.set_flags(realm->Flags);
    realmEntry.set_name(realm->Name);
    realmEntry.set_cfgconfigsid(realm->GetConfigId());
    realmEntry.set_cfglanguagesid(1);

    return Compress("JamJSONRealmEntry:" + JSON::Serialize(realmEntry));
}

std::vector<uint8> RealmList::GetRealmList(uint32 build, std::string const& subRegion) const
{
    JSON::RealmList::RealmListUpdates realmList;
    for (auto const& [handle, realm] : _realms)
    {
        if (realm.Id.GetSubRegionAddress() != subRegion)
            continue;

        uint32 flag = realm.Flags;
        if (realm.Build != build)
            flag |= REALM_FLAG_VERSION_MISMATCH;

        JSON::RealmList::RealmState* state = realmList.add_updates();
        JSON::RealmList::RealmEntry* update = state->mutable_update();
        update->set_wowrealmaddress(realm.Id.GetAddress());
        update->set_cfgtimezonesid(1);
        update->set_populationstate((realm.Flags & REALM_FLAG_OFFLINE) ? 0u : std::max(uint32(realm.PopulationLevel), 1u));
        update->set_cfgcategoriesid(realm.Timezone);
        FillVersion(update->mutable_version(), GetBuildInfo(realm.Build), realm.Build);
        update->set_cfgrealmsid(realm.Id.Realm);
        update->set_flags(flag);
        update->set_name(realm.Name);
        update->set_cfgconfigsid(realm.GetConfigId());
        update->set_cfglanguagesid(1);

        state->set_deleting(false);
    }

    return Compress("JSONRealmListUpdates:" + JSON::Serialize(realmList));
}

uint32 RealmList::JoinRealm(uint32 realmAddress, uint32 build, boost::asio::ip::address const& clientAddress, std::array<uint8, 32> const& clientSecret,
    LocaleConstant locale, std::string const& os, std::string const& accountName, bgs::protocol::game_utilities::v1::ClientResponse* response) const
{
    Realm const* realm = GetRealm(RealmHandle::FromAddress(realmAddress));
    if (!realm)
        return ERROR_UTIL_SERVER_UNKNOWN_REALM;

    if ((realm->Flags & REALM_FLAG_OFFLINE) || realm->Build != build)
        return ERROR_USER_SERVER_NOT_PERMITTED_ON_REALM;

    JSON::RealmList::RealmListServerIPAddresses serverAddresses;
    JSON::RealmList::RealmIPAddressFamily* addressFamily = serverAddresses.add_families();
    addressFamily->set_family(1);

    JSON::RealmList::IPAddress* address = addressFamily->add_addresses();
    address->set_ip(realm->GetAddressForClient(clientAddress).address().to_string());
    address->set_port(realm->Port);

    std::vector<uint8> compressed = Compress("JSONRealmListServerIPAddresses:" + JSON::Serialize(serverAddresses));
    if (compressed.empty())
        return ERROR_UTIL_SERVER_FAILED_TO_SERIALIZE_RESPONSE;

    BigNumber serverSecret;
    serverSecret.SetRand(8 * 32);
    std::array<uint8, 32> serverSecretBytes = serverSecret.ToByteArray<32>();

    // the world server checks the 6.x AuthSession digest against this key (account.session_key_bnet)
    Acore::Crypto::SHA256::Digest sessionKey = Acore::Crypto::SHA256::GetDigestOf(clientSecret, serverSecretBytes);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LOGIN_INFO);
    stmt->SetData(0, sessionKey);
    stmt->SetData(1, clientAddress.to_string());
    stmt->SetData(2, localeNames[locale < TOTAL_LOCALES ? locale : LOCALE_enUS]); // account.locale is the locale name
    stmt->SetData(3, os);
    stmt->SetData(4, accountName);
    LoginDatabase.DirectExecute(stmt);

    bgs::protocol::Attribute* attribute = response->add_attribute();
    attribute->set_name("Param_RealmJoinTicket");
    attribute->mutable_value()->set_blob_value(accountName);

    attribute = response->add_attribute();
    attribute->set_name("Param_ServerAddresses");
    attribute->mutable_value()->set_blob_value(compressed.data(), compressed.size());

    attribute = response->add_attribute();
    attribute->set_name("Param_JoinSecret");
    attribute->mutable_value()->set_blob_value(serverSecretBytes.data(), serverSecretBytes.size());
    return ERROR_OK;
}
