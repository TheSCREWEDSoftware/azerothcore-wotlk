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
#include "Language.h"
#include "RBAC.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Realm.h"
#include "SocialMgr.h"
#include "SocialPackets.h"
#include "World.h"
#include "WorldSession.h"

void WorldSession::HandleContactListOpcode(WorldPackets::Social::SendContactList& packet)
{
    _player->GetSocial()->SendSocialList(_player, packet.Flags);
}

void WorldSession::HandleAddFriendOpcode(WorldPackets::Social::AddFriend& packet)
{
    std::string friendName = packet.Name;
    std::string const& friendNote = packet.Notes;

    // the client may send "Name-Realm"
    friendName = friendName.substr(0, friendName.find('-'));

    if (!normalizePlayerName(friendName))
        return;

    ObjectGuid friendGuid = sCharacterCache->GetCharacterGuidByName(friendName);
    if (!friendGuid)
    {
        sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_NOT_FOUND, ObjectGuid::Empty, false);
        return;
    }

    CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(friendGuid);
    if (!playerData)
        return;

    uint32 friendAccountId = playerData->AccountId;
    TeamId teamId = Player::TeamIdForRace(playerData->Race);
    FriendsResult friendResult = FRIEND_NOT_FOUND;

    if (HasPermission(rbac::RBAC_PERM_ALLOW_GM_FRIEND) || sWorld->getBoolConfig(CONFIG_ALLOW_GM_FRIEND) || AccountMgr::IsPlayerAccount(AccountMgr::GetSecurity(friendAccountId, realm.Id.Realm)))
    {
        if (friendGuid)
        {
            if (friendGuid == GetPlayer()->GetGUID())
                friendResult = FRIEND_SELF;
            else if (GetPlayer()->GetTeamId() != teamId && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_ADD_FRIEND))
                friendResult = FRIEND_ENEMY;
            else if (GetPlayer()->GetSocial()->HasFriend(friendGuid))
                friendResult = FRIEND_ALREADY;
            else
            {
                Player* pFriend = ObjectAccessor::FindConnectedPlayer(friendGuid);
                if (pFriend && pFriend->IsVisibleGloballyFor(GetPlayer()) && !pFriend->GetSession()->IsGMAccount())
                    friendResult = FRIEND_ADDED_ONLINE;
                else
                    friendResult = FRIEND_ADDED_OFFLINE;
                if (GetPlayer()->GetSocial()->AddToSocialList(friendGuid, ObjectGuid::Create<HighGuid::WowAccount>(friendAccountId), SOCIAL_FLAG_FRIEND))
                    GetPlayer()->GetSocial()->SetFriendNote(friendGuid, friendNote);
                else
                    friendResult = FRIEND_LIST_FULL;
            }
        }
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), friendResult, friendGuid, false);

    LOG_DEBUG("network", "WORLD: Sent (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleDelFriendOpcode(WorldPackets::Social::DelFriend& packet)
{
    /// @todo: handle VirtualRealmAddress
    ObjectGuid FriendGUID = packet.Player.Guid;

    _player->GetSocial()->RemoveFromSocialList(FriendGUID, SOCIAL_FLAG_FRIEND);

    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_REMOVED, FriendGUID, false);

    LOG_DEBUG("network", "WORLD: Sent motd (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleAddIgnoreOpcode(WorldPackets::Social::AddIgnore& packet)
{
    std::string ignoreName = packet.Name.substr(0, packet.Name.find('-'));

    if (!normalizePlayerName(ignoreName))
        return;

    LOG_DEBUG("network", "WORLD: {} asked to Ignore: '{}'", GetPlayer()->GetName(), ignoreName);

    ObjectGuid ignoreGuid = sCharacterCache->GetCharacterGuidByName(ignoreName);
    if (!ignoreGuid)
    {
        sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_IGNORE_NOT_FOUND, ObjectGuid::Empty, false);
        return;
    }

    FriendsResult ignoreResult;

    if (ignoreGuid == GetPlayer()->GetGUID())              //not add yourself
        ignoreResult = FRIEND_IGNORE_SELF;
    else if (GetPlayer()->GetSocial()->HasIgnore(ignoreGuid))
        ignoreResult = FRIEND_IGNORE_ALREADY;
    else
    {
        ignoreResult = FRIEND_IGNORE_ADDED;

        // ignore list full
        if (!GetPlayer()->GetSocial()->AddToSocialList(ignoreGuid, ObjectGuid::Create<HighGuid::WowAccount>(sCharacterCache->GetCharacterAccountIdByGuid(ignoreGuid)), SOCIAL_FLAG_IGNORED))
            ignoreResult = FRIEND_IGNORE_FULL;
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), ignoreResult, ignoreGuid, false);

    LOG_DEBUG("network", "WORLD: Sent (SMSG_FRIEND_STATUS)");
}

void WorldSession::HandleDelIgnoreOpcode(WorldPackets::Social::DelIgnore& packet)
{
    /// @todo: handle VirtualRealmAddress
    ObjectGuid IgnoreGUID = packet.Player.Guid;

    _player->GetSocial()->RemoveFromSocialList(IgnoreGUID, SOCIAL_FLAG_IGNORED);
    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_IGNORE_REMOVED, IgnoreGUID, false);
}

void WorldSession::HandleSetContactNotesOpcode(WorldPackets::Social::SetContactNotes& packet)
{
    /// @todo: handle VirtualRealmAddress
    _player->GetSocial()->SetFriendNote(packet.Player.Guid, packet.Notes);
}
