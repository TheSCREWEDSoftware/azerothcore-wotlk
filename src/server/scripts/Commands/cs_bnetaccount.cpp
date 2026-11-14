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

/* ScriptData
Name: bnetaccount_commandscript
%Complete: 100
Comment: Battle.net account commands of the 6.x login, from TrinityCore 6.x src/server/scripts/Commands/cs_battlenet_account.cpp
Category: commandscripts
EndScriptData */

#include "AccountMgr.h"
#include "BattlenetAccountMgr.h"
#include "BigNumber.h"
#include "Chat.h"
#include "CommandScript.h"
#include "DatabaseEnv.h"
#include "IPLocation.h"
#include "Language.h"
#include "Player.h"
#include "RBAC.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "Util.h"
#include "WorldSession.h"

using namespace Acore::ChatCommands;

class bnetaccount_commandscript : public CommandScript
{
public:
    bnetaccount_commandscript() : CommandScript("bnetaccount_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable accountSetCommandTable =
        {
            { "password", HandleAccountSetPasswordCommand, rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_SET_PASSWORD, Console::Yes },
        };

        static ChatCommandTable accountLockCommandTable =
        {
            { "country",  HandleAccountLockCountryCommand, rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_LOCK_COUNTRY, Console::No },
            { "ip",       HandleAccountLockIpCommand,      rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_LOCK_IP,      Console::No },
        };

        static ChatCommandTable accountCommandTable =
        {
            { "create",            HandleAccountCreateCommand,     rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_CREATE,      Console::Yes },
            { "gameaccountcreate", HandleGameAccountCreateCommand, rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_CREATE_GAME, Console::Yes },
            { "lock",              accountLockCommandTable },
            { "set",               accountSetCommandTable  },
            { "password",          HandleAccountPasswordCommand,   rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_PASSWORD,    Console::No  },
            { "link",              HandleAccountLinkCommand,       rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_LINK,        Console::Yes },
            { "unlink",            HandleAccountUnlinkCommand,     rbac::RBAC_PERM_COMMAND_BNET_ACCOUNT_UNLINK,      Console::Yes },
        };

        static ChatCommandTable commandTable =
        {
            { "bnetaccount", accountCommandTable },
        };

        return commandTable;
    }

    /// .bnetaccount create <email> <password> [create game account (default yes)]
    static bool HandleAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* accountName = strtok((char*)args, " ");
        char* password = strtok(nullptr, " ");
        if (!accountName || !password)
            return false;

        if (!strchr(accountName, '@'))
        {
            handler->SendErrorMessage(LANG_ACCOUNT_INVALID_BNET_NAME);
            return false;
        }

        bool createGameAccount = true;
        if (char* createGameAccountParam = strtok(nullptr, " "))
            createGameAccount = Acore::StringTo<bool>(createGameAccountParam).value_or(true);

        std::string gameAccountName;
        switch (Battlenet::AccountMgr::CreateBattlenetAccount(std::string(accountName), std::string(password), createGameAccount, &gameAccountName))
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName);
                if (!gameAccountName.empty())
                    handler->PSendSysMessage(LANG_ACCOUNT_CREATED, gameAccountName);
                if (handler->GetSession())
                {
                    LOG_INFO("entities.player.character", "Battle.net account: {} (IP: {}) Character:[{}] ({}) created Battle.net account {}",
                        handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress(),
                        handler->GetSession()->GetPlayerName(), handler->GetSession()->GetPlayer() ? handler->GetSession()->GetPlayer()->GetGUID().ToString() : "",
                        accountName);
                }
                break;
            case AOR_NAME_TOO_LONG:
                handler->SendErrorMessage(LANG_ACCOUNT_TOO_LONG);
                return false;
            case AOR_NAME_ALREADY_EXIST:
                handler->SendErrorMessage(LANG_ACCOUNT_ALREADY_EXIST);
                return false;
            case AOR_PASS_TOO_LONG:
                handler->SendErrorMessage(LANG_PASSWORD_TOO_LONG);
                return false;
            default:
                handler->SendErrorMessage(LANG_ACCOUNT_NOT_CREATED, accountName);
                return false;
        }

        return true;
    }

    /// Sets the country lock of the own Battle.net account
    static bool HandleAccountLockCountryCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendErrorMessage(LANG_USE_BOL);
            return false;
        }

        std::string param = args;
        if (param == "on")
        {
            IpLocationRecord const* location = sIPLocation->GetLocationRecord(handler->GetSession()->GetRemoteAddress());
            if (!location)
            {
                handler->SendErrorMessage("No IP2Location information - account not locked");
                return false;
            }

            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_ACCOUNT_LOCK_COUNTRY);
            stmt->SetData(0, location->CountryCode);
            stmt->SetData(1, handler->GetSession()->GetBattlenetAccountId());
            LoginDatabase.Execute(stmt);
            handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
            return true;
        }

        if (param == "off")
        {
            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_ACCOUNT_LOCK_COUNTRY);
            stmt->SetData(0, "00");
            stmt->SetData(1, handler->GetSession()->GetBattlenetAccountId());
            LoginDatabase.Execute(stmt);
            handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            return true;
        }

        handler->SendErrorMessage(LANG_USE_BOL);
        return false;
    }

    /// Sets the ip lock of the own Battle.net account
    static bool HandleAccountLockIpCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendErrorMessage(LANG_USE_BOL);
            return false;
        }

        std::string param = args;
        if (param != "on" && param != "off")
        {
            handler->SendErrorMessage(LANG_USE_BOL);
            return false;
        }

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_ACCOUNT_LOCK);
        stmt->SetData(0, param == "on");
        stmt->SetData(1, handler->GetSession()->GetBattlenetAccountId());
        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(param == "on" ? LANG_COMMAND_ACCLOCKLOCKED : LANG_COMMAND_ACCLOCKUNLOCKED);
        return true;
    }

    /// .bnetaccount password <old password> <new password> <new password again>
    static bool HandleAccountPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        char* oldPassword = strtok((char*)args, " ");
        char* newPassword = strtok(nullptr, " ");
        char* passwordConfirmation = strtok(nullptr, " ");
        if (!oldPassword || !newPassword || !passwordConfirmation)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        uint32 battlenetAccountId = handler->GetSession()->GetBattlenetAccountId();
        if (!Battlenet::AccountMgr::CheckPassword(battlenetAccountId, std::string(oldPassword)))
        {
            handler->SendErrorMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            LOG_INFO("entities.player.character", "Battle.net account: {} (IP: {}) Character:[{}] tried to change password, but the provided old password is wrong.",
                battlenetAccountId, handler->GetSession()->GetRemoteAddress(), handler->GetSession()->GetPlayerName());
            return false;
        }

        if (strcmp(newPassword, passwordConfirmation) != 0)
        {
            handler->SendErrorMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            return false;
        }

        switch (Battlenet::AccountMgr::ChangePassword(battlenetAccountId, std::string(newPassword)))
        {
            case AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                LOG_INFO("entities.player.character", "Battle.net account: {} (IP: {}) Character:[{}] changed password.",
                    battlenetAccountId, handler->GetSession()->GetRemoteAddress(), handler->GetSession()->GetPlayerName());
                break;
            case AOR_PASS_TOO_LONG:
                handler->SendErrorMessage(LANG_PASSWORD_TOO_LONG);
                return false;
            default:
                handler->SendErrorMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                return false;
        }

        return true;
    }

    /// .bnetaccount set password <email> <password> <password again>
    static bool HandleAccountSetPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        char* account = strtok((char*)args, " ");
        char* password = strtok(nullptr, " ");
        char* passwordConfirmation = strtok(nullptr, " ");
        if (!account || !password || !passwordConfirmation)
            return false;

        std::string accountName = account;
        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->SendErrorMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            return false;
        }

        uint32 targetAccountId = Battlenet::AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->SendErrorMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
            return false;
        }

        if (strcmp(password, passwordConfirmation) != 0)
        {
            handler->SendErrorMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            return false;
        }

        switch (Battlenet::AccountMgr::ChangePassword(targetAccountId, password))
        {
            case AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                break;
            case AOR_NAME_NOT_EXIST:
                handler->SendErrorMessage(LANG_ACCOUNT_NOT_EXIST, accountName);
                return false;
            case AOR_PASS_TOO_LONG:
                handler->SendErrorMessage(LANG_PASSWORD_TOO_LONG);
                return false;
            default:
                handler->SendErrorMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                return false;
        }

        return true;
    }

    /// .bnetaccount link <email> <game account>
    static bool HandleAccountLinkCommand(ChatHandler* handler, char const* args)
    {
        std::vector<std::string_view> tokens = Acore::Tokenize(args, ' ', false);
        if (tokens.size() != 2)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        std::string bnetAccountName(tokens[0]);
        std::string gameAccountName(tokens[1]);
        Utf8ToUpperOnlyLatin(bnetAccountName);
        Utf8ToUpperOnlyLatin(gameAccountName);

        switch (Battlenet::AccountMgr::LinkWithGameAccount(bnetAccountName, gameAccountName))
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_BNET_LINKED, bnetAccountName, gameAccountName);
                return true;
            case AOR_NAME_NOT_EXIST:
                handler->SendErrorMessage(LANG_ACCOUNT_OR_BNET_DOES_NOT_EXIST, bnetAccountName, gameAccountName);
                return false;
            case AOR_ACCOUNT_BAD_LINK:
                handler->SendErrorMessage(LANG_ACCOUNT_ALREADY_LINKED, gameAccountName);
                return false;
            default:
                return false;
        }
    }

    /// .bnetaccount unlink <game account>
    static bool HandleAccountUnlinkCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        std::string gameAccountName = args;
        Utf8ToUpperOnlyLatin(gameAccountName);

        switch (Battlenet::AccountMgr::UnlinkGameAccount(gameAccountName))
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_BNET_UNLINKED, gameAccountName);
                return true;
            case AOR_NAME_NOT_EXIST:
                handler->SendErrorMessage(LANG_ACCOUNT_NOT_EXIST, gameAccountName);
                return false;
            case AOR_ACCOUNT_BAD_LINK:
                handler->SendErrorMessage(LANG_ACCOUNT_BNET_NOT_LINKED, gameAccountName);
                return false;
            default:
                return false;
        }
    }

    /// .bnetaccount gameaccountcreate <email>: adds a game account <id>#<next index> to a Battle.net account
    static bool HandleGameAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        std::string bnetAccountName = args;
        Utf8ToUpperOnlyLatin(bnetAccountName);
        uint32 accountId = Battlenet::AccountMgr::GetId(bnetAccountName);
        if (!accountId)
        {
            handler->SendErrorMessage(LANG_ACCOUNT_NOT_EXIST, bnetAccountName);
            return false;
        }

        uint8 index = Battlenet::AccountMgr::GetMaxIndex(accountId) + 1;
        std::string accountName = std::to_string(accountId) + '#' + std::to_string(uint32(index));

        // random password: these accounts log in through bnetserver only, never through the old authserver
        BigNumber randPassword;
        randPassword.SetRand(8 * 8);

        switch (sAccountMgr->CreateAccount(accountName, ByteArrayToHexStr(randPassword.ToByteArray<8>()), bnetAccountName, true))
        {
            case AOR_OK:
            {
                LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BNET_GAME_ACCOUNT_LINK);
                stmt->SetData(0, accountId);
                stmt->SetData(1, index);
                stmt->SetData(2, AccountMgr::GetId(accountName));
                LoginDatabase.Execute(stmt);

                handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName);
                break;
            }
            case AOR_NAME_TOO_LONG:
                handler->SendErrorMessage(LANG_ACCOUNT_TOO_LONG);
                return false;
            case AOR_NAME_ALREADY_EXIST:
                handler->SendErrorMessage(LANG_ACCOUNT_ALREADY_EXIST);
                return false;
            case AOR_DB_INTERNAL_ERROR:
                handler->SendErrorMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName);
                return false;
            default:
                handler->SendErrorMessage(LANG_ACCOUNT_NOT_CREATED, accountName);
                return false;
        }

        return true;
    }
};

void AddSC_bnetaccount_commandscript()
{
    new bnetaccount_commandscript();
}
