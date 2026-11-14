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

#include "BattlenetAccountMgr.h"
#include "CryptoHash.h"
#include "DatabaseEnv.h"
#include "QueryResult.h"
#include "Util.h"

using GameAccountMgr = AccountMgr;

AccountOpResult Battlenet::AccountMgr::CreateBattlenetAccount(std::string email, std::string password, bool withGameAccount, std::string* gameAccountName)
{
    if (utf8length(email) > MAX_BNET_EMAIL_STR)
        return AOR_NAME_TOO_LONG;

    if (utf8length(password) > MAX_PASS_STR)
        return AOR_PASS_TOO_LONG;

    Utf8ToUpperOnlyLatin(email);
    Utf8ToUpperOnlyLatin(password);

    if (GetId(email))
        return AOR_NAME_ALREADY_EXIST;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BNET_ACCOUNT);
    stmt->SetData(0, email);
    stmt->SetData(1, CalculateShaPassHash(email, password));
    LoginDatabase.DirectExecute(stmt);

    uint32 newAccountId = GetId(email);
    if (!newAccountId)
        return AOR_DB_INTERNAL_ERROR;

    if (withGameAccount)
    {
        // game accounts of a Battle.net account are named <id>#<index>
        std::string gameAccount = std::to_string(newAccountId) + "#1";
        AccountOpResult result = GameAccountMgr::instance()->CreateAccount(gameAccount, password, email, true);
        if (result != AOR_OK)
            return result;

        uint32 gameAccountId = GameAccountMgr::GetId(gameAccount);
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LINK);
        stmt->SetData(0, newAccountId);
        stmt->SetData(1, uint8(1));
        stmt->SetData(2, gameAccountId);
        LoginDatabase.DirectExecute(stmt);

        if (gameAccountName)
            *gameAccountName = gameAccount;
    }

    return AOR_OK;
}

AccountOpResult Battlenet::AccountMgr::ChangePassword(uint32 accountId, std::string newPassword)
{
    std::string username;
    if (!GetName(accountId, username))
        return AOR_NAME_NOT_EXIST;

    Utf8ToUpperOnlyLatin(username);
    Utf8ToUpperOnlyLatin(newPassword);
    if (utf8length(newPassword) > MAX_PASS_STR)
        return AOR_PASS_TOO_LONG;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_PASSWORD);
    stmt->SetData(0, CalculateShaPassHash(username, newPassword));
    stmt->SetData(1, accountId);
    LoginDatabase.Execute(stmt);

    return AOR_OK;
}

bool Battlenet::AccountMgr::CheckPassword(uint32 accountId, std::string password)
{
    std::string username;
    if (!GetName(accountId, username))
        return false;

    Utf8ToUpperOnlyLatin(username);
    Utf8ToUpperOnlyLatin(password);

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_CHECK_PASSWORD);
    stmt->SetData(0, accountId);
    stmt->SetData(1, CalculateShaPassHash(username, password));

    return LoginDatabase.Query(stmt) != nullptr;
}

AccountOpResult Battlenet::AccountMgr::LinkWithGameAccount(std::string const& email, std::string const& gameAccountName)
{
    uint32 bnetAccountId = GetId(email);
    if (!bnetAccountId)
        return AOR_NAME_NOT_EXIST;

    uint32 gameAccountId = GameAccountMgr::GetId(gameAccountName);
    if (!gameAccountId)
        return AOR_NAME_NOT_EXIST;

    if (GetIdByGameAccount(gameAccountId))
        return AOR_ACCOUNT_BAD_LINK;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LINK);
    stmt->SetData(0, bnetAccountId);
    stmt->SetData(1, uint8(GetMaxIndex(bnetAccountId) + 1));
    stmt->SetData(2, gameAccountId);
    LoginDatabase.Execute(stmt);
    return AOR_OK;
}

AccountOpResult Battlenet::AccountMgr::UnlinkGameAccount(std::string const& gameAccountName)
{
    uint32 gameAccountId = GameAccountMgr::GetId(gameAccountName);
    if (!gameAccountId)
        return AOR_NAME_NOT_EXIST;

    if (!GetIdByGameAccount(gameAccountId))
        return AOR_ACCOUNT_BAD_LINK;

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LINK);
    stmt->SetData(0, nullptr);
    stmt->SetData(1, nullptr);
    stmt->SetData(2, gameAccountId);
    LoginDatabase.Execute(stmt);
    return AOR_OK;
}

uint32 Battlenet::AccountMgr::GetId(std::string const& username)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_ID_BY_EMAIL);
    stmt->SetData(0, username);
    if (PreparedQueryResult result = LoginDatabase.Query(stmt))
        return (*result)[0].Get<uint32>();

    return 0;
}

bool Battlenet::AccountMgr::GetName(uint32 accountId, std::string& name)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_EMAIL_BY_ID);
    stmt->SetData(0, accountId);
    if (PreparedQueryResult result = LoginDatabase.Query(stmt))
    {
        name = (*result)[0].Get<std::string>();
        return true;
    }

    return false;
}

uint32 Battlenet::AccountMgr::GetIdByGameAccount(uint32 gameAccountId)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_ID_BY_GAME_ACCOUNT);
    stmt->SetData(0, gameAccountId);
    if (PreparedQueryResult result = LoginDatabase.Query(stmt))
        return (*result)[0].IsNull() ? 0 : (*result)[0].Get<uint32>();

    return 0;
}

uint8 Battlenet::AccountMgr::GetMaxIndex(uint32 accountId)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_MAX_ACCOUNT_INDEX);
    stmt->SetData(0, accountId);
    if (PreparedQueryResult result = LoginDatabase.Query(stmt))
        return (*result)[0].IsNull() ? 0 : (*result)[0].Get<uint8>();

    return 0;
}

std::string Battlenet::AccountMgr::CalculateShaPassHash(std::string const& name, std::string const& password)
{
    Acore::Crypto::SHA256::Digest email = Acore::Crypto::SHA256::GetDigestOf(name);

    Acore::Crypto::SHA256 sha;
    sha.UpdateData(ByteArrayToHexStr(email));
    sha.UpdateData(":");
    sha.UpdateData(password);
    sha.Finalize();

    return ByteArrayToHexStr(sha.GetDigest(), true);
}
