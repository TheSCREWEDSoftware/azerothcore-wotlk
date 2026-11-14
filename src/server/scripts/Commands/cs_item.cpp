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

#include "Chat.h"
#include "CommandScript.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "ItemRecovery.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include <unordered_map>

using namespace Acore::ChatCommands;

class item_commandscript : public CommandScript
{
public:
    item_commandscript() : CommandScript("item_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable HandleItemRestoreCommandTable =
        {
            { "list",      HandleItemRestoreListCommand,        rbac::RBAC_PERM_COMMAND_ITEM_RESTORE_LIST, Console::Yes },
            { "",          HandleItemRestoreCommand,            rbac::RBAC_PERM_COMMAND_ITEM_RESTORE,      Console::Yes },
        };
        static ChatCommandTable itemCommandTable =
        {
            { "restore",   HandleItemRestoreCommandTable },
            { "move",      HandleItemMoveCommand,               rbac::RBAC_PERM_COMMAND_ITEMMOVE,          Console::Yes },
            { "refund",    HandleItemRefundCommand,             rbac::RBAC_PERM_COMMAND_ITEM_REFUND,       Console::Yes },
        };
        static ChatCommandTable commandTable =
        {
            { "item",      itemCommandTable }
        };
        return commandTable;
    }

    static bool HandleItemRestoreCommand(ChatHandler* handler, uint32 restoreId, PlayerIdentifier player)
    {
        if (!restoreId)
        {
            return false;
        }

        if (!HasItemDeletionConfig())
        {
            handler->SendErrorMessage(LANG_COMMAND_DISABLED);
            return false;
        }

        std::optional<RecoveryItem> recovery = ItemRecovery::Load(restoreId);
        if (!recovery || !recovery->ItemEntry || recovery->OwnerGuid != player.GetGUID().GetCounter())
        {
            handler->SendErrorMessage(LANG_ITEM_RESTORE_MISSING, restoreId);
            return false;
        }

        Player* onlinePlayer = player.GetConnectedPlayer();
        if (onlinePlayer && recovery->Source == RECOVERY_ITEM_SOURCE_VENDORED && ItemRecovery::IsInBuyback(onlinePlayer, *recovery))
        {
            handler->SendErrorMessage(LANG_ITEM_RESTORE_IN_BUYBACK, recovery->ItemEntry);
            return false;
        }

        if (!ItemRecovery::Restore(*recovery, onlinePlayer))
        {
            handler->SendErrorMessage(LANG_ITEM_RESTORE_MISSING, restoreId);
            return false;
        }

        std::string nameLink = handler->playerLink(player.GetName());
        handler->PSendSysMessage(LANG_MAIL_SENT, nameLink);
        return true;
    }

    static bool HandleItemRestoreListCommand(ChatHandler* handler, PlayerIdentifier player)
    {
        if (!HasItemDeletionConfig())
        {
            handler->SendErrorMessage(LANG_COMMAND_DISABLED);
            return false;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_RECOVERY_ITEM_LIST);
        stmt->SetData(0, player.GetGUID().GetCounter());
        PreparedQueryResult disposedItems = CharacterDatabase.Query(stmt);

        if (!disposedItems)
        {
            handler->SendErrorMessage(LANG_ITEM_RESTORE_LIST_EMPTY);
            return false;
        }

        std::unordered_map<uint32, RecoveryItemLoot> lootByRecoveryId;
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_RECOVERY_ITEM_LOOT_BY_GUID);
        stmt->SetData(0, player.GetGUID().GetCounter());
        if (PreparedQueryResult loot = CharacterDatabase.Query(stmt))
        {
            do
            {
                Field* lootFields = loot->Fetch();
                lootByRecoveryId[lootFields[0].Get<uint32>()].emplace_back(lootFields[1].Get<uint32>(), lootFields[2].Get<uint32>());
            } while (loot->NextRow());
        }

        do
        {
            Field* fields    = disposedItems->Fetch();
            uint32 id        = fields[0].Get<uint32>();
            uint32 itemId    = fields[1].Get<uint32>();
            uint32 count     = fields[2].Get<uint32>();
            uint8 source     = fields[3].Get<uint8>();
            uint32 money     = fields[4].Get<uint32>();

            std::string itemName = GetItemName(handler, itemId);

            switch (source)
            {
                case RECOVERY_ITEM_SOURCE_VENDORED:
                    handler->PSendSysMessage(LANG_ITEM_RESTORE_LIST_VENDORED, id, itemName, itemId, count,
                        Acore::StringFormat("{}g {}s {}c", money / GOLD, (money % GOLD) / SILVER, money % SILVER));
                    break;
                case RECOVERY_ITEM_SOURCE_DISENCHANTED:
                {
                    std::string materials;
                    for (auto const& [lootEntry, lootCount] : lootByRecoveryId[id])
                    {
                        if (!materials.empty())
                            materials += ", ";

                        materials += Acore::StringFormat("{}x {} ({})", lootCount, GetItemName(handler, lootEntry), lootEntry);
                    }

                    handler->PSendSysMessage(LANG_ITEM_RESTORE_LIST_DISENCHANTED, id, itemName, itemId, count, materials);
                    break;
                }
                default:
                    handler->PSendSysMessage(LANG_ITEM_RESTORE_LIST, id, itemName, itemId, count);
                    break;
            }
        } while (disposedItems->NextRow());

        return true;
    }

    static std::string GetItemName(ChatHandler* handler, uint32 itemId)
    {
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);
        if (!item)
            return "";

        std::string name = item->Name1;
        int localeIndex = handler->GetSessionDbLocaleIndex();
        if (localeIndex >= 0)
            if (ItemLocale const* locale = sObjectMgr->GetItemLocale(itemId))
                ObjectMgr::GetLocaleString(locale->Name, localeIndex, name);

        return name;
    }

    /// @todo - move item to other slot
    static bool HandleItemMoveCommand(ChatHandler* handler, uint8 srcSlot, uint8 dstSlot)
    {
        if (srcSlot == dstSlot)
            return true;

        if (!handler->GetSession()->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, srcSlot, true))
            return false;

        if (!handler->GetSession()->GetPlayer()->IsValidPos(INVENTORY_SLOT_BAG_0, dstSlot, false))
            return false;

        uint16 src = ((INVENTORY_SLOT_BAG_0 << 8) | srcSlot);
        uint16 dst = ((INVENTORY_SLOT_BAG_0 << 8) | dstSlot);

        handler->GetSession()->GetPlayer()->SwapItem(src, dst);

        return true;
    }

    static bool HasItemDeletionConfig()
    {
        return sWorld->getBoolConfig(CONFIG_ITEMDELETE_METHOD) || sWorld->getBoolConfig(CONFIG_ITEMDELETE_VENDOR);
    }

    static bool HandleItemRefundCommand(ChatHandler* handler, PlayerIdentifier player, uint32 itemId, uint32 extendedCost)
    {
        ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(extendedCost);
        if (!iece)
        {
            handler->SendErrorMessage(LANG_CMD_ITEM_REFUND_BAD_EXTENDED_COST);
            return false;
        }

        ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);

        if (!item)
        {
            handler->SendErrorMessage(LANG_COMMAND_ITEMIDINVALID, itemId);
            return false;
        }

        if (Player* target = player.GetConnectedPlayer())
        {
            if (!target->HasItemCount(itemId, 1, true))
            {
                handler->SendErrorMessage(LANG_CMD_ITEM_REFUND_NOT_FOUND, itemId);
                return false;
            }

            // 6.x: honor and conquest are currencies
            for (uint8 i = 0; i < MAX_ITEM_EXT_COST_CURRENCIES; ++i)
                if (iece->RequiredCurrency[i] && iece->RequiredCurrencyCount[i])
                    target->ModifyCurrency(iece->RequiredCurrency[i], int32(iece->RequiredCurrencyCount[i]));

            uint8 count = 0;
            for (uint32 const& reqItem : iece->RequiredItem)
            {
                if (reqItem)
                {
                    target->AddItem(reqItem, iece->RequiredItemCount[count]);
                }

                ++count;
            }

            target->DestroyItemCount(itemId, 1, true);
        }
        else
        {
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            CharacterDatabasePreparedStatement* stmt;

            ObjectGuid::LowType guid = player.GetGUID().GetCounter();

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_INVENTORY_ITEM_BY_ENTRY_AND_OWNER);
            stmt->SetData(0, itemId);
            stmt->SetData(1, guid);

            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (result)
            {
                // 6.x: honor and conquest are currencies
                for (uint8 i = 0; i < MAX_ITEM_EXT_COST_CURRENCIES; ++i)
                {
                    if (!iece->RequiredCurrency[i] || !iece->RequiredCurrencyCount[i])
                        continue;

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_CURRENCY_ADD);
                    stmt->SetData(0, guid);
                    stmt->SetData(1, uint16(iece->RequiredCurrency[i]));
                    stmt->SetData(2, iece->RequiredCurrencyCount[i]);
                    trans->Append(stmt);
                }

                MailSender sender(MAIL_NORMAL, guid, MAIL_STATIONERY_GM);
                // fill mail
                std::string msg = "Your item " + item->Name1 + " has been removed and the used currency restored. This mail contains any items used as currency.";
                MailDraft   draft("Item Refund", msg);

                uint8 count      = 0;
                bool  foundItems = false;
                for (uint32 const& reqItem : iece->RequiredItem)
                {
                    if (reqItem)
                    {
                        // Skip invalid items.
                        if (!sObjectMgr->GetItemTemplate(reqItem))
                        {
                            continue;
                        }

                        if (Item* item = Item::CreateItem(reqItem, iece->RequiredItemCount[count]))
                        {
                            item->SaveToDB(trans);
                            draft.AddItem(item);
                            foundItems = true;
                        }
                    }

                    ++count;
                }

                if (foundItems)
                {
                    draft.SendMailTo(trans, MailReceiver(nullptr, guid), sender);
                }

                Field* fields = result->Fetch();

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INVENTORY_BY_ITEM);
                stmt->SetData(0, fields[0].Get<uint32>());
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                stmt->SetData(0, fields[0].Get<uint32>());
                trans->Append(stmt);

                CharacterDatabase.CommitTransaction(trans);
            }
            else
            {
                handler->SendErrorMessage(LANG_CMD_ITEM_REFUND_NOT_FOUND, itemId);
                return false;
            }
        }

        return true;
    }
};

void AddSC_item_commandscript()
{
    new item_commandscript();
}
