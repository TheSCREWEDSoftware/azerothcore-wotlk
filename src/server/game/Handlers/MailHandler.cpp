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

#include "AccountMgr.h"
#include "Chat.h"
#include "RBAC.h"
#include "CharacterCache.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "Mail.h"
#include "MailMgr.h"
#include "MailPackets.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#define MAX_INBOX_CLIENT_CAPACITY 50

bool WorldSession::CanOpenMailBox(ObjectGuid guid)
{
    if (guid == _player->GetGUID())
    {
        if (!HasPermission(rbac::RBAC_PERM_COMMAND_MAILBOX))
        {
            LOG_WARN("cheat", "{} attempted to open mailbox by using a cheat.", _player->GetName());
            return false;
        }
    }
    else if (guid.IsGameObject())
    {
        if (!_player->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_MAILBOX))
            return false;
    }
    else if (guid.IsAnyTypeCreature())
    {
        if (!_player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_MAILBOX))
            return false;
    }
    else
        return false;

    return true;
}

void WorldSession::HandleSendMail(WorldPackets::Mail::SendMail& packet)
{
    if (packet.Info.Attachments.size() > MAX_MAIL_ITEMS)                      // client limit
    {
        GetPlayer()->SendMailResult(0, MAIL_SEND, MAIL_ERR_TOO_MANY_ATTACHMENTS);
        return;
    }

    ObjectGuid mailbox = packet.Info.Mailbox;
    std::string receiver = packet.Info.Target;
    std::string subject = packet.Info.Subject;
    std::string body = packet.Info.Body;
    uint32 stationery = uint32(packet.Info.StationeryID);
    uint32 package = 0;
    int64 money64 = packet.Info.SendMoney;
    int64 COD64 = packet.Info.Cod;
    uint8 items_count = uint8(packet.Info.Attachments.size());

    ObjectGuid itemGUIDs[MAX_MAIL_ITEMS];
    for (uint8 i = 0; i < items_count; ++i)
        itemGUIDs[i] = packet.Info.Attachments[i].ItemGUID;

    // prevent client crash
    if (subject.find("| |") != std::string::npos || body.find("| |") != std::string::npos)
        return;

    // the server keeps money as 32 bits
    if (money64 < 0 || COD64 < 0 || money64 > std::numeric_limits<uint32>::max() || COD64 > std::numeric_limits<uint32>::max())
    {
        GetPlayer()->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    uint32 money = uint32(money64);
    uint32 COD = uint32(COD64);

    // packet read complete, now do check

    if (!CanOpenMailBox(mailbox))
        return;

    if (receiver.empty())
        return;

    Player* player = _player;

    if (sWorld->getBoolConfig(CONFIG_TRIAL_RESTRICTION_MAIL) && IsTrialAccount())
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_DISABLED_FOR_TRIAL_ACC);
        return;
    }

    if (player->GetLevel() < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        ChatHandler(this).SendNotification(LANG_MAIL_SENDER_REQ, sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    ObjectGuid receiverGuid;
    if (normalizePlayerName(receiver))
    {
        receiverGuid = sCharacterCache->GetCharacterGuidByName(receiver);
    }

    if (!receiverGuid)
    {
        LOG_DEBUG("network.opcode", "Player {} is sending mail to {} (GUID: not existed!) with subject {} and body {} includes {} items, {} copper and {} COD copper with stationery = {}, package = {}",
            player->GetGUID().ToString(), receiver, subject, body, items_count, money, COD, stationery, package);
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_NOT_FOUND);
        return;
    }

    LOG_DEBUG("network.opcode", "Player {} is sending mail to {} ({}) with subject {} and body {} includes {} items, {} copper and {} COD copper with stationery = {}, package = {}",
        player->GetGUID().ToString(), receiver, receiverGuid.ToString(), subject, body, items_count, money, COD, stationery, package);

    if (player->GetGUID() == receiverGuid)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANNOT_SEND_TO_SELF);
        return;
    }

    if (money && COD) // cannot send money in a COD mail
    {
        LOG_ERROR("network.opcode", "{} attempt to dupe money!!!.", receiver);
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    uint32 cost = items_count ? 30 * items_count : 30; // price hardcoded in client

    uint32 reqmoney = cost + money;

    // Check for overflow
    if (reqmoney < money)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    if (!player->HasEnoughMoney(reqmoney))
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Player* receive = ObjectAccessor::FindConnectedPlayer(receiverGuid);

    uint32 rc_teamId = TEAM_NEUTRAL;
    uint16 mails_count = 0;                                  //do not allow to send to one player more than 100 mails

    if (receive)
    {
        rc_teamId = receive->GetTeamId();
        mails_count = receive->GetMailSize();
    }
    else
    {
        // xinef: get data from global storage
        if (CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(receiverGuid))
        {
            rc_teamId = Player::TeamIdForRace(playerData->Race);
            mails_count = playerData->MailCount;
        }
    }
    //do not allow to have more than 100 mails in mailbox.. mails count is in opcode uint8!!! - so max can be 255..
    if (mails_count > 100)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_CAP_REACHED);
        return;
    }
    // test the receiver's Faction... or all items are account bound
    // Xinef: check for boa items, not used currently
    /*bool accountBound = items_count && !money && !COD ? true : false;
    for (uint8 i = 0; i < items_count; ++i)
    {
        Item* item = player->GetItemByGuid(itemGUIDs[i]);
        if (item)
        {
            ItemTemplate const* itemProto = item->GetTemplate();
            if (!itemProto || !itemProto->HasFlag(ITEM_FLAG_IS_BOUND_TO_ACCOUNT))
            {
                accountBound = false;
                break;
            }
        }
    }*/

    uint32 rc_account = receive ? receive->GetSession()->GetAccountId() : sCharacterCache->GetCharacterAccountIdByGuid(receiverGuid);

    if (/*!accountBound*/ GetAccountId() != rc_account && player->GetTeamId() != rc_teamId && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_MAIL))
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_YOUR_TEAM);
        return;
    }

    Item* items[MAX_MAIL_ITEMS];

    for (uint8 i = 0; i < items_count; ++i)
    {
        if (!itemGUIDs[i])
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        Item* item = player->GetItemByGuid(itemGUIDs[i]);

        // prevent sending bag with items (cheat: can be placed in bag after adding equipped empty bag to mail)
        if (!item)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        // handle empty bag before CanBeTraded, since that func already has that check
        if (item->IsNotEmptyBag())
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS);
            return;
        }

        if (!item->CanBeTraded(true))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (item->IsBoundAccountWide() && item->IsSoulBound() && GetAccountId() != rc_account)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_ARTEFACTS_ONLY_FOR_OWN_CHARACTERS);
            return;
        }

        if (item->GetTemplate()->HasFlag(ITEM_FLAG_CONJURED) || item->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (COD && item->IsWrapped())
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANT_SEND_WRAPPED_COD);
            return;
        }

        if (!sScriptMgr->OnPlayerCanSendMail(player, receiverGuid, mailbox, subject, body, money, COD, item))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        items[i] = item;
    }

    if (!items_count && !sScriptMgr->OnPlayerCanSendMail(player, receiverGuid, mailbox, subject, body, money, COD, nullptr))
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    player->SendMailResult(0, MAIL_SEND, MAIL_OK);

    if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
    {
        if (items_count > 0)
        {
            for (uint8 i = 0; i < items_count; ++i)
            {
                Item* item = items[i];
                LOG_GM(GetAccountId(), "GM {} (Account: {}) sent mail to {} (Account: {}) with item: {} (Entry: {} Count: {})",
                    player->GetName(), GetAccountId(), receiver, rc_account,
                    item->GetTemplate()->Name1, item->GetEntry(), item->GetCount());
            }
        }
        if (money > 0)
        {
            LOG_GM(GetAccountId(), "GM {} (Account: {}) sent mail to {} (Account: {}) with money: {}",
                player->GetName(), GetAccountId(), receiver, rc_account, money);
        }
    }

    player->ModifyMoney(-int32(reqmoney));
    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL, cost);

    bool needItemDelay = false;

    MailDraft draft(subject, body);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    std::string itemLogStr = "";
    if (items_count > 0 || money > 0)
    {
        if (items_count > 0)
        {
            for (uint8 i = 0; i < items_count; ++i)
            {
                // log item
                itemLogStr += Acore::StringFormat("{} (Entry: {}) x{}, ", items[i]->GetTemplate()->Name1, items[i]->GetEntry(), items[i]->GetCount());

                Item* item = items[i];

                item->SetNotRefundable(GetPlayer()); // makes the item no longer refundable
                player->MoveItemFromInventory(items[i]->GetBagSlot(), item->GetSlot(), true);

                item->DeleteFromInventoryDB(trans);     // deletes item from character's inventory
                if (item->GetState() == ITEM_UNCHANGED)
                    item->FSetState(ITEM_CHANGED);      // pussywizard: so the item will be saved and owner will be updated in database
                item->SetOwnerGUID(receiverGuid);
                item->SaveToDB(trans);                  // recursive and not have transaction guard into self, item not in inventory and can be save standalone

                draft.AddItem(item);
            }

            // if item send to character at another account, then apply item delivery delay
            needItemDelay = GetAccountId() != rc_account;
        }

        if (money >= 10 * GOLD)
        {
            CleanStringForMysqlQuery(subject);
            CharacterDatabase.Execute("INSERT INTO log_money VALUES({}, {}, \"{}\", \"{}\", {}, \"{}\", {}, \"{}\", NOW(), {})",
                GetAccountId(), player->GetGUID().GetCounter(), player->GetName(), player->GetSession()->GetRemoteAddress(), rc_account, receiver, money, subject, 5);
        }
    }

    // If theres is an item, there is a one hour delivery delay if sent to another account's character.
    uint32 deliver_delay = needItemDelay ? sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY) : 0;

    // don't ask for COD if there are no items
    if (items_count == 0)
        COD = 0;

    // will delete item or place to receiver mail list
    draft
    .AddMoney(money)
    .AddCOD(COD)
    .SendMailTo(trans, MailReceiver(receive, receiverGuid.GetCounter()), MailSender(player), body.empty() ? MAIL_CHECK_MASK_COPIED : MAIL_CHECK_MASK_HAS_BODY, deliver_delay);

    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    LOG_INFO("entities.player.mail", "Mail: Account: {} (IP: {}), Player [{}] ({}) sent mail to Player [{}] ({}): subject='{}', body='{}', {} copper, {} COD copper, sent item(s) [{}]",
        GetAccountId(), GetRemoteAddress(), player->GetName(), player->GetGUID().GetCounter(),
        receiver, receiverGuid.GetCounter(), subject, body, money, COD, itemLogStr);
}

//called when mail is read
void WorldSession::HandleMailMarkAsRead(WorldPackets::Mail::MailMarkAsRead& packet)
{
    ObjectGuid mailbox = packet.Mailbox;
    uint32 mailId = uint32(packet.MailID);

    if (!CanOpenMailBox(mailbox))
        return;

    Player* player = _player;
    Mail* m = player->GetMail(mailId);
    if (m && m->state != MAIL_STATE_DELETED)
    {
        if (player->unReadMails)
            --player->unReadMails;
        m->checked = m->checked | MAIL_CHECK_MASK_READ;
        player->m_mailsUpdated = true;
        m->state = MAIL_STATE_CHANGED;
    }
}

//called when client deletes mail
void WorldSession::HandleMailDelete(WorldPackets::Mail::MailDelete& packet)
{
    // the client sends no mailbox, the mail must be the player's
    uint32 mailId = uint32(packet.MailID);

    Mail* m = _player->GetMail(mailId);
    Player* player = _player;
    player->m_mailsUpdated = true;
    if (m && m->state != MAIL_STATE_DELETED)
    {
        // delete shouldn't show up for COD mails
        if (m->COD)
        {
            player->SendMailResult(mailId, MAIL_DELETED, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        m->state = MAIL_STATE_DELETED;

        sMailMgr->OnMailDeleted(player->GetGUID().GetCounter());
    }
    player->SendMailResult(mailId, MAIL_DELETED, MAIL_OK);
}

void WorldSession::HandleMailReturnToSender(WorldPackets::Mail::MailReturnToSender& packet)
{
    // 6.x: the client sends the mail sender instead of the mailbox
    ObjectGuid mailbox;
    uint32 mailId = uint32(packet.MailID);

    Player* player = _player;
    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > GameTime::GetGameTime().count() || (packet.SenderGUID && m->sender != packet.SenderGUID.GetCounter()))
    {
        player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    if (m->HasItems())
    {
        for (MailItemInfoVec::iterator itr = m->items.begin(); itr != m->items.end(); ++itr)
        {
            Item* item = player->GetMItem(itr->item_guid);
            if (item && !sScriptMgr->OnPlayerCanSendMail(player, ObjectGuid::Create<HighGuid::Player>(m->sender), mailbox, m->subject, m->body, m->money, m->COD, item))
            {
                player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_ERR_INTERNAL_ERROR);
                return;
            }
        }
    }
    else if (!sScriptMgr->OnPlayerCanSendMail(player, ObjectGuid::Create<HighGuid::Player>(m->sender), mailbox, m->subject, m->body, m->money, m->COD, nullptr))
    {
        player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    //we can return mail now
    //so firstly delete the old one
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
    stmt->SetData(0, mailId);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
    stmt->SetData(0, mailId);
    trans->Append(stmt);

    player->RemoveMail(mailId);

    // only return mail if the player exists (and delete if not existing)
    if (m->messageType == MAIL_NORMAL && m->sender)
    {
        MailDraft draft(m->subject, m->body);
        if (m->mailTemplateId)
            draft = MailDraft(m->mailTemplateId, false);     // items already included

        if (m->HasItems())
        {
            for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
            {
                Item* item = player->GetMItem(itr2->item_guid);
                if (item)
                    draft.AddItem(item);

                player->RemoveMItem(itr2->item_guid);
            }
        }
        draft.AddMoney(m->money).SendReturnToSender(GetAccountId(), m->receiver, m->sender, trans);
    }

    CharacterDatabase.CommitTransaction(trans);

    delete m;                                               //we can deallocate old mail
    player->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, MAIL_OK);

    sMailMgr->OnMailDeleted(player->GetGUID().GetCounter());
}

//called when player takes item attached in mail
void WorldSession::HandleMailTakeItem(WorldPackets::Mail::MailTakeItem& packet)
{
    ObjectGuid mailbox = packet.Mailbox;
    uint32 mailId = uint32(packet.MailID);
    ObjectGuid::LowType itemLowGuid = ObjectGuid::LowType(packet.AttachID);

    if (!CanOpenMailBox(mailbox))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > GameTime::GetGameTime().count())
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    // verify that the mail has the item to avoid cheaters taking COD items without paying
    bool foundItem = false;
    for (std::vector<MailItemInfo>::const_iterator itr = m->items.begin(); itr != m->items.end(); ++itr)
        if (itr->item_guid == itemLowGuid)
        {
            foundItem = true;
            break;
        }
    if (!foundItem)
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    // prevent cheating with skip client money check
    if (!player->HasEnoughMoney(m->COD))
    {
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Item* it = player->GetMItem(itemLowGuid);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, it, false);
    if (msg == EQUIP_ERR_OK)
    {
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        m->RemoveItem(itemLowGuid);
        m->removedItems.push_back(itemLowGuid);

        if (m->COD > 0) // if there is COD, take COD money from player and send them to sender by mail
        {
            uint32 sender_accId = 0;
            Player* sender = ObjectAccessor::FindPlayerByLowGUID(m->sender);
            if (sender)
            {
                sender_accId = sender->GetSession()->GetAccountId();
            }
            else
            {
                sender_accId = sCharacterCache->GetCharacterAccountIdByGuid(ObjectGuid::Create<HighGuid::Player>(m->sender));
            }

            // check player existence
            if (sender || sender_accId)
            {
                MailDraft(m->subject, "")
                .AddMoney(m->COD)
                .SendMailTo(trans, MailReceiver(sender, m->sender), MailSender(MAIL_NORMAL, m->receiver), MAIL_CHECK_MASK_COD_PAYMENT);

                if (m->COD >= 10 * GOLD)
                {
                    std::string senderName;
                    if (!sCharacterCache->GetCharacterNameByGuid(ObjectGuid::Create<HighGuid::Player>(m->sender), senderName))
                    {
                        senderName = sObjectMgr->GetAcoreStringForDBCLocale(LANG_UNKNOWN);
                    }
                    std::string subj = m->subject;
                    CleanStringForMysqlQuery(subj);
                    CharacterDatabase.Execute("INSERT INTO log_money VALUES({}, {}, \"{}\", \"{}\", {}, \"{}\", {}, \"{}\", NOW(), {})",
                        GetAccountId(), player->GetGUID().GetCounter(), player->GetName(), player->GetSession()->GetRemoteAddress(), sender_accId, senderName, m->COD, subj, 1);
                }
            }

            player->ModifyMoney(-int32(m->COD));
        }

        m->COD = 0;
        m->state = MAIL_STATE_CHANGED;
        player->m_mailsUpdated = true;
        player->RemoveMItem(it->GetGUID().GetCounter());

        uint32 count = it->GetCount();                      // save counts before store and possible merge with deleting
        it->SetState(ITEM_UNCHANGED);                       // need to set this state, otherwise item cannot be removed later, if neccessary

        // `stored` is the stack the character ends up holding. On a full stack merge, `it` is marked for removal
        // and `SaveInventoryAndGoldToDB` below deletes it, so we pass `stored` instead of `it` to downstream hooks.
        Item* stored = player->MoveItemToInventory(dest, it, true);

        if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
        {
            LOG_GM(GetAccountId(), "GM {} (Account: {}) took mail item: {} (Entry: {} Count: {}) from mailbox",
                player->GetName(), GetAccountId(), it->GetTemplate()->Name1, it->GetEntry(), count);
        }

        player->SaveInventoryAndGoldToDB(trans);
        player->_SaveMail(trans);
        CharacterDatabase.CommitTransaction(trans);

        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_OK, 0, itemLowGuid, count);

        sScriptMgr->OnPlayerAfterTakeItemFromMail(player, stored, count);
    }
    else
        player->SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_EQUIP_ERROR, msg);
}

void WorldSession::HandleMailTakeMoney(WorldPackets::Mail::MailTakeMoney& packet)
{
    ObjectGuid mailbox = packet.Mailbox;
    uint32 mailId = uint32(packet.MailID);
    int64 money = packet.Money;

    if (!CanOpenMailBox(mailbox))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || m->state == MAIL_STATE_DELETED || m->deliver_time > GameTime::GetGameTime().count() ||
        (money > 0 && int64(m->money) != money))
    {
        player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    if (!player->ModifyMoney(m->money, false))
    {
        player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_TOO_MUCH_GOLD);
        return;
    }

    if (HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
    {
        LOG_GM(GetAccountId(), "GM {} (Account: {}) took mail money: {} from mailbox",
            player->GetName(), GetAccountId(), m->money);
    }

    m->money = 0;
    m->state = MAIL_STATE_CHANGED;
    player->m_mailsUpdated = true;

    player->SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_OK);

    // save money and mail to prevent cheating
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    player->SaveGoldToDB(trans);
    player->_SaveMail(trans);
    CharacterDatabase.CommitTransaction(trans);
}

//called when player lists his received mails
void WorldSession::HandleGetMailList(WorldPackets::Mail::MailGetList& packet)
{
    ObjectGuid mailbox = packet.Mailbox;

    if (!CanOpenMailBox(mailbox))
        return;

    SendMailList();

    // recalculate m_nextMailDelivereTime and unReadMails
    _player->UpdateNextMailTimeAndUnreads();
}

// Builds and sends the inbox. Normally a reply to CMSG_GET_MAIL_LIST, but it is also pushed
// unsolicited on delivery when Mail.PushInboxOnDelivery is on, because the client refuses to
// re-query its inbox more than once a minute and would otherwise show a stale mailbox
void WorldSession::SendMailList()
{
    Player* player = _player;

    WorldPackets::Mail::MailListResult response;
    time_t curTime = GameTime::GetGameTime().count();
    std::size_t packetSize = 8;

    for (Mail const* mail : player->GetMails())
    {
        // skip deleted or not delivered (deliver delay not expired) mails
        if (mail->state == MAIL_STATE_DELETED || curTime < mail->deliver_time || curTime > mail->expire_time)
            continue;

        ++response.TotalNumRecords;

        // prevent client storage overflow
        if (response.Mails.size() >= MAX_INBOX_CLIENT_CAPACITY)
            continue;

        WorldPackets::Mail::MailListEntry entry(mail, player);

        // prevent client crash
        if (entry.Subject.find("| |") != std::string::npos)
            entry.Subject.clear();

        if (entry.Body.find("| |") != std::string::npos)
            entry.Body.clear();

        std::size_t const entrySize = 4 + 1 + 16 + 4 + 8 * 2 + 4 * 4 + 4 + entry.Subject.size() + entry.Body.size() + entry.Attachments.size() * 150;
        if (packetSize + entrySize > MAX_NETCLIENT_PACKET_SIZE)
            continue;

        packetSize += entrySize;
        response.Mails.push_back(std::move(entry));
    }

    SendPacket(response.Write());
}

//used when player copies mail body to his inventory
void WorldSession::HandleMailCreateTextItem(WorldPackets::Mail::MailCreateTextItem& packet)
{
    ObjectGuid mailbox = packet.Mailbox;
    uint32 mailId = packet.MailID;

    if (!CanOpenMailBox(mailbox))
        return;

    Player* player = _player;

    Mail* m = player->GetMail(mailId);
    if (!m || (m->body.empty() && !m->mailTemplateId) || m->state == MAIL_STATE_DELETED || m->deliver_time > GameTime::GetGameTime().count() || (m->checked & MAIL_CHECK_MASK_COPIED))
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    Item* bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if (!bodyItem->Create(sObjectMgr->GetGenerator<HighGuid::Item>().Generate(), MAIL_BODY_ITEM_TEMPLATE, player))
    {
        delete bodyItem;
        return;
    }

    // in mail template case we need create new item text
    if (m->mailTemplateId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if (!mailTemplateEntry)
        {
            player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
            delete bodyItem;
            return;
        }

        bodyItem->SetText(mailTemplateEntry->Body->Str[GetSessionDbcLocale()]);
    }
    else
        bodyItem->SetText(m->body);

    bodyItem->SetUInt32Value(ITEM_FIELD_CREATOR, m->sender);
    bodyItem->SetFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_MAIL_TEXT_MASK);

    LOG_DEBUG("network.opcode", "HandleMailCreateTextItem mailid={}", mailId);

    ItemPosCountVec dest;
    uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, bodyItem, false);
    if (msg == EQUIP_ERR_OK)
    {
        m->checked = m->checked | MAIL_CHECK_MASK_COPIED;
        m->state = MAIL_STATE_CHANGED;
        player->m_mailsUpdated = true;

        player->StoreItem(dest, bodyItem, true);
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_OK);
    }
    else
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_EQUIP_ERROR, msg);
        delete bodyItem;
    }
}

void WorldSession::HandleQueryNextMailTime(WorldPackets::Mail::MailQueryNextMailTime& /*packet*/)
{
    WorldPackets::Mail::MailQueryNextTimeResult result;

    if (_player->unReadMails > 0)
    {
        result.NextMailTime = 0.0f;

        time_t now = GameTime::GetGameTime().count();
        std::set<ObjectGuid::LowType> sentSenders;

        for (Mail const* mail : _player->GetMails())
        {
            // must be not checked yet
            if (mail->checked & MAIL_CHECK_MASK_READ)
                continue;

            // and already delivered or expired
            if (now < mail->deliver_time || now > mail->expire_time)
                continue;

            // only send each mail sender once
            if (!sentSenders.insert(mail->sender).second)
                continue;

            result.Next.emplace_back(mail);

            if (result.Next.size() == 3)                    // do not display more than 3 mails
                break;
        }
    }
    else
        result.NextMailTime = -DAY;

    SendPacket(result.Write());
}
