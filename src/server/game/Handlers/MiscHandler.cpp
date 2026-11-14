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
#include "BattlefieldMgr.h"
#include "RBAC.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "ArchaeologyPackets.h"
#include "CharacterPackets.h"
#include "Chat.h"
#include "Common.h"
#include "DBCEnums.h"
#include "DatabaseEnv.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "Group.h"
#include "GuildMgr.h"
#include "InspectPackets.h"
#include "InstancePackets.h"
#include "InstanceScript.h"
#include "Language.h"
#include "Log.h"
#include "LootMgr.h"
#include "MapMgr.h"
#include "MovementPackets.h"
#include "MiscPackets.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Realm.h"
#include "Opcodes.h"
#include "OutdoorPvP.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "Spell.h"
#include "Vehicle.h"
#include "WhoListCacheMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <zlib.h>

#include "Corpse.h"

void WorldSession::HandleRepopRequestOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "WORLD: Recvd CMSG_REPOP_REQUEST Message");

    recv_data.read_skip<uint8>();

    if (GetPlayer()->IsAlive() || GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_GHOST))
        return;

    if (GetPlayer()->HasPreventResurectionAura())
        return; // silently return, client should display the error by itself

    // the world update order is sessions, players, creatures
    // the netcode runs in parallel with all of these
    // creatures can kill players
    // so if the server is lagging enough the player can
    // release spirit after he's killed but before he is updated
    if (GetPlayer()->getDeathState() == DeathState::JustDied)
    {
        LOG_DEBUG("network", "HandleRepopRequestOpcode: got request after player {} ({}) was killed and before he was updated",
            GetPlayer()->GetName(), GetPlayer()->GetGUID().ToString());
        GetPlayer()->KillPlayer();
    }

    //this is spirit release confirm?
    GetPlayer()->RemovePet(nullptr, PET_SAVE_NOT_IN_SLOT, true);
    GetPlayer()->BuildPlayerRepop();
    GetPlayer()->RepopAtGraveyard();
}

void WorldSession::HandleViolenceLevel(WorldPacket& recvPacket)
{
    uint8 violenceLevel;
    recvPacket >> violenceLevel;

    // do something?
}

void WorldSession::HandleGossipSelectOptionOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 gossipListId;
    uint32 menuId;
    ObjectGuid guid;
    std::string code = "";

    // 5.4.8 18414, from Project SkyFire
    recv_data >> gossipListId >> menuId;
    recv_data.ReadGuidMask(guid, { 3, 0, 1, 4, 7, 5, 6 });
    uint32 codeLength = recv_data.ReadBits(8);
    recv_data.ReadGuidMask(guid, { 2 });
    recv_data.ReadGuidBytes(guid, { 7, 3, 4, 6, 0, 5 });
    code = recv_data.ReadString(codeLength);
    recv_data.ReadGuidBytes(guid, { 2, 1 });

    if (!_player->PlayerTalkClass->GetGossipMenu().GetItem(gossipListId))
        return;

    if (!_player->PlayerTalkClass->IsGossipOptionCoded(gossipListId))
        code.clear();

    // Prevent cheating on C++ scripted menus
    if (_player->PlayerTalkClass->GetGossipMenu().GetSenderGUID() != guid)
    {
        return;
    }

    Creature* unit = nullptr;
    GameObject* go = nullptr;
    Item* item = nullptr;
    if (guid.IsCreatureOrVehicle())
    {
        unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
        if (!unit)
        {
            LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - Unit ({}) not found or you can't interact with him.", guid.ToString());
            return;
        }
    }
    else if (guid.IsGameObject())
    {
        go = _player->GetMap()->GetGameObject(guid);
        if (!go)
        {
            LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - GameObject ({}) not found.", guid.ToString());
            return;
        }
    }
    else if (guid.IsItem())
    {
        item = _player->GetItemByGuid(guid);
        if (!item || _player->IsBankPos(item->GetPos()))
        {
            //LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - {} not found.", guid.ToString());
            return;
        }
    }
    else if (guid.IsPlayer())
    {
        if (guid != _player->GetGUID() || menuId != _player->PlayerTalkClass->GetGossipMenu().GetMenuId())
        {
            //LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - {} not found.", guid.ToString());
            return;
        }
    }
    else
    {
        LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - unsupported GUID type for {}.", guid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    if ((unit && unit->GetScriptId() != unit->LastUsedScriptID) || (go && go->GetScriptId() != go->LastUsedScriptID))
    {
        LOG_DEBUG("network", "WORLD: HandleGossipSelectOptionOpcode - Script reloaded while in use, ignoring and set new scipt id");
        if (unit)
            unit->LastUsedScriptID = unit->GetScriptId();
        if (go)
            go->LastUsedScriptID = go->GetScriptId();
        _player->PlayerTalkClass->SendCloseGossip();
        return;
    }
    if (!code.empty())
    {
        if (unit)
        {
            unit->AI()->sGossipSelectCode(_player, menuId, gossipListId, code.c_str());
            if (!sScriptMgr->OnGossipSelectCode(_player, unit, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId), code.c_str()))
                _player->OnGossipSelect(unit, gossipListId, menuId);
        }
        else if (go)
        {
            go->AI()->GossipSelectCode(_player, menuId, gossipListId, code.c_str());
            sScriptMgr->OnGossipSelectCode(_player, go, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId), code.c_str());
        }
        else if (item)
        {
            sScriptMgr->OnGossipSelectCode(_player, item, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId), code.c_str());
        }
        else
        {
            sScriptMgr->OnPlayerGossipSelectCode(_player, menuId, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId), code.c_str());
        }
    }
    else
    {
        if (unit)
        {
            unit->AI()->sGossipSelect(_player, menuId, gossipListId);
            if (!sScriptMgr->OnGossipSelect(_player, unit, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId)))
                _player->OnGossipSelect(unit, gossipListId, menuId);
        }
        else if (go)
        {
            go->AI()->GossipSelect(_player, menuId, gossipListId);
            if (!sScriptMgr->OnGossipSelect(_player, go, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId)))
                _player->OnGossipSelect(go, gossipListId, menuId);
        }
        else if (item)
        {
            sScriptMgr->OnGossipSelect(_player, item, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId));
        }
        else
        {
            sScriptMgr->OnPlayerGossipSelect(_player, menuId, _player->PlayerTalkClass->GetGossipOptionSender(gossipListId), _player->PlayerTalkClass->GetGossipOptionAction(gossipListId));
        }
    }
}

void WorldSession::HandleWhoOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Recvd CMSG_WHO Message");

    uint32 matchCount = 0;

    uint32 levelMin, levelMax, racemask, classmask, zonesCount, strCount;
    std::array<uint32, 10> zoneids = {};                    // 10 is client limit
    std::string packetPlayerName, packetGuildName;

    // 5.4.8 18414, from Project SkyFire (which read the words as C strings; they are sized by the bits)
    recvData >> classmask;
    recvData >> racemask;
    recvData >> levelMax;                                   // minimal player level, default 100 (MAX_LEVEL)
    recvData >> levelMin;                                   // maximal player level, default 0

    recvData.ReadBit();                                     // show enemies
    recvData.ReadBit();                                     // exact name
    bool requestServerInfo = recvData.ReadBit();
    uint32 guildRealmNameLength = recvData.ReadBits(9);
    recvData.ReadBit();                                     // show arena players
    uint32 playerNameLength = recvData.ReadBits(6);
    zonesCount = recvData.ReadBits(4);                      // zones count, client limit = 10 (2.0.10)

    if (zonesCount > 10)
        return;                                             // can't be received from real client or broken packet

    uint32 realmNameLength = recvData.ReadBits(9);
    uint32 guildNameLength = recvData.ReadBits(7);
    strCount = recvData.ReadBits(3);                        // user entered strings count, client limit=4 (checked on 2.0.10)

    if (strCount > 4)
        return;                                             // can't be received from real client or broken packet

    std::array<uint32, 4> wordLengths = {};
    for (uint32 i = 0; i < strCount; ++i)
        wordLengths[i] = recvData.ReadBits(7);

    std::wstring str[4];                                    // 4 is client limit
    for (uint32 i = 0; i < strCount; ++i)
    {
        std::string temp = recvData.ReadString(wordLengths[i]); // user entered string, it used as universal search pattern(guild+player name)?

        if (!Utf8toWStr(temp, str[i]))
            continue;

        wstrToLower(str[i]);

        LOG_DEBUG("network.who", "String {}: {}", i, temp);
    }

    recvData.ReadString(guildRealmNameLength);

    for (uint32 i = 0; i < zonesCount; ++i)
    {
        zoneids[i] = recvData.read<uint32>();               // zone id, 0 if zone is unknown...
        LOG_DEBUG("network.who", "Zone {}: {}", i, zoneids[i]);
    }

    packetPlayerName = recvData.ReadString(playerNameLength);
    recvData.ReadString(realmNameLength);
    packetGuildName = recvData.ReadString(guildNameLength);

    if (requestServerInfo)
    {
        recvData.read_skip<int32>();                        // locale
        recvData.read_skip<uint32>();                       // virtual realm address
        recvData.read_skip<int32>();                        // faction
    }

    LOG_DEBUG("network.who", "Minlvl {}, maxlvl {}, name {}, guild {}, racemask {}, classmask {}, zones {}, strings {}",
        levelMin, levelMax, packetPlayerName, packetGuildName, racemask, classmask, zonesCount, strCount);

    std::wstring wpacketPlayerName;
    std::wstring wpacketGuildName;
    if (!(Utf8toWStr(packetPlayerName, wpacketPlayerName) && Utf8toWStr(packetGuildName, wpacketGuildName)))
        return;

    wstrToLower(wpacketPlayerName);
    wstrToLower(wpacketGuildName);

    // client send in case not set max level value 100 but Acore supports 255 max level,
    // update it to show GMs with characters after 100 level
    if (levelMax >= MAX_LEVEL)
        levelMax = STRONG_MAX_LEVEL;

    uint32 team = _player->GetTeamId();
    uint32 gmLevelInWhoList = sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_WHO_LIST);
    uint32 displaycount = 0;

    // 5.4.8 18414, from Project SkyFire
    WorldPacket data(SMSG_WHO, 50);     // guess size
    std::size_t countPos = data.bitwpos();
    data.WriteBits(0, 6);               // count of players displayed
    ByteBuffer bytesData;

    for (auto const& target : sWhoListCacheMgr->GetWhoList())
    {
        if (target.GetTeamId() != team && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_WHO_LIST))
            continue;

        // player can see MODERATOR, GAME MASTER, ADMINISTRATOR only if CONFIG_GM_IN_WHO_LIST
        if (!HasPermission(rbac::RBAC_PERM_WHO_SEE_ALL_SEC_LEVELS) && target.GetSecurity() > AccountTypes(gmLevelInWhoList))
            continue;

        // check if target is globally visible for player
        if ((_player->GetGUID() != target.GetGuid() && !target.IsVisible()) &&
            (AccountMgr::IsPlayerAccount(_player->GetSession()->GetSecurity()) || target.GetSecurity() > _player->GetSession()->GetSecurity()))
        {
            continue;
        }

        // check if target's level is in level range
        uint8 lvl = target.GetLevel();
        if (lvl < levelMin || lvl > levelMax)
        {
            continue;
        }

        // check if class matches classmask
        uint8 class_ = target.GetClass();
        if (!(classmask & (1 << class_)))
        {
            continue;
        }

        // check if race matches racemask
        uint32 race = target.GetRace();
        if (!(racemask & (1 << race)))
        {
            continue;
        }

        uint32 playerZoneId = target.GetZoneId();
        uint8 gender = target.GetGender();

        bool showZones = true;
        for (uint32 i = 0; i < zonesCount; ++i)
        {
            if (zoneids[i] == playerZoneId)
            {
                showZones = true;
                break;
            }

            showZones = false;
        }

        if (!showZones)
        {
            continue;
        }

        std::wstring const& wideplayername = target.GetWidePlayerName();
        if (!(wpacketPlayerName.empty() || wideplayername.find(wpacketPlayerName) != std::wstring::npos))
        {
            continue;
        }

        std::wstring const& wideguildname = target.GetWideGuildName();
        if (!(wpacketGuildName.empty() || wideguildname.find(wpacketGuildName) != std::wstring::npos))
        {
            continue;
        }

        std::string aname;
        if (AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(playerZoneId))
        {
            aname = areaEntry->area_name[GetSessionDbcLocale()];
        }

        bool s_show = true;
        for (uint32 i = 0; i < strCount; ++i)
        {
            if (!str[i].empty())
            {
                if (wideguildname.find(str[i]) != std::wstring::npos ||
                    wideplayername.find(str[i]) != std::wstring::npos ||
                    Utf8FitTo(aname, str[i]))
                {
                    s_show = true;
                    break;
                }

                s_show = false;
            }
        }

        if (!s_show)
        {
            continue;
        }

        // 49 is maximum player count sent to client - can be overridden
        // through config, but is unstable
        if ((matchCount++) >= sWorld->getIntConfig(CONFIG_MAX_WHO_LIST_RETURN))
        {
            continue;
        }

        ObjectGuid playerGuid = target.GetGuid();
        ObjectGuid accountId = ObjectGuid(uint64(target.GetAccountId()));
        ObjectGuid guildGuid = target.GetGuildId() ? ObjectGuid::Create<HighGuid::Guild>(target.GetGuildId()) : ObjectGuid::Empty;
        std::string const& playerName = target.GetPlayerName();
        std::string const& guildName = target.GetGuildName();

        data.WriteGuidMask(accountId, { 2 });
        data.WriteGuidMask(playerGuid, { 2 });
        data.WriteGuidMask(accountId, { 7 });
        data.WriteGuidMask(guildGuid, { 5 });
        data.WriteBits(guildName.size(), 7);
        data.WriteGuidMask(accountId, { 1, 5 });
        data.WriteGuidMask(guildGuid, { 7 });
        data.WriteGuidMask(playerGuid, { 5 });
        data.WriteBit(false);
        data.WriteGuidMask(guildGuid, { 1 });
        data.WriteGuidMask(playerGuid, { 6 });
        data.WriteGuidMask(guildGuid, { 2 });
        data.WriteGuidMask(playerGuid, { 4 });
        data.WriteGuidMask(guildGuid, { 0, 3 });
        data.WriteGuidMask(accountId, { 6 });
        data.WriteBit(false);
        data.WriteGuidMask(playerGuid, { 1 });
        data.WriteGuidMask(guildGuid, { 4 });
        data.WriteGuidMask(accountId, { 0 });
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            data.WriteBits(0, 7);                           // declined names
        data.WriteGuidMask(playerGuid, { 3 });
        data.WriteGuidMask(guildGuid, { 6 });
        data.WriteGuidMask(playerGuid, { 0 });
        data.WriteGuidMask(accountId, { 4, 3 });
        data.WriteGuidMask(playerGuid, { 7 });
        data.WriteBits(playerName.size(), 6);

        bytesData.WriteGuidBytes(playerGuid, { 1 });
        bytesData << uint32(realm.Id.Realm);                // virtual realm id
        bytesData.WriteGuidBytes(playerGuid, { 7 });
        bytesData << uint32(realm.Id.Realm);                // guild virtual realm id
        bytesData.WriteGuidBytes(playerGuid, { 4 });
        bytesData.WriteString(playerName);
        bytesData.WriteGuidBytes(guildGuid, { 1 });
        bytesData.WriteGuidBytes(playerGuid, { 0 });
        bytesData.WriteGuidBytes(guildGuid, { 2, 0, 4 });
        bytesData.WriteGuidBytes(playerGuid, { 3 });
        bytesData.WriteGuidBytes(guildGuid, { 6 });
        bytesData << uint32(target.GetAccountId());
        bytesData.WriteString(guildName);
        bytesData.WriteGuidBytes(guildGuid, { 3 });
        bytesData.WriteGuidBytes(accountId, { 4 });
        bytesData << uint8(class_);
        bytesData.WriteGuidBytes(accountId, { 7 });
        bytesData.WriteGuidBytes(playerGuid, { 6, 2 });
        bytesData.WriteGuidBytes(accountId, { 2, 3 });
        bytesData << uint8(race);
        bytesData.WriteGuidBytes(guildGuid, { 7 });
        bytesData.WriteGuidBytes(accountId, { 1, 5, 6 });
        bytesData.WriteGuidBytes(playerGuid, { 5 });
        bytesData.WriteGuidBytes(accountId, { 0 });
        bytesData << uint8(gender);
        bytesData.WriteGuidBytes(guildGuid, { 5 });
        bytesData << uint8(lvl);
        bytesData << int32(playerZoneId);

        if (++displaycount >= 63)                         // the count has 6 bits
            break;
    }

    data.FlushBits();
    data.PutBits(countPos, displaycount, 6);
    data.append(bytesData);

    SendPacket(&data);
    LOG_DEBUG("network", "WORLD: Send SMSG_WHO Message");
}

void WorldSession::HandleLogoutRequestOpcode(WorldPackets::Character::LogoutRequest& /*logoutRequest*/)
{
    LOG_DEBUG("network", "WORLD: Recvd CMSG_LOGOUT_REQUEST Message, security - {}", GetSecurity());

    if (ObjectGuid lguid = GetPlayer()->GetLootGUID())
        DoLootRelease(lguid);

    bool instantLogout = (HasPermission(rbac::RBAC_PERM_INSTANT_LOGOUT)
                          || (GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_RESTING) && !GetPlayer()->IsInCombat())) || GetPlayer()->IsInFlight();

    bool preventAfkSanctuaryLogout = sWorld->getIntConfig(CONFIG_AFK_PREVENT_LOGOUT) == 1
                                     && GetPlayer()->isAFK() && sAreaTableStore.LookupEntry(GetPlayer()->GetAreaId())->IsSanctuary();

    bool preventAfkLogout = ((sWorld->getIntConfig(CONFIG_AFK_PREVENT_LOGOUT) == 2) || HasAccountFlag(ACCOUNT_FLAG_NOKICK))
                            && GetPlayer()->isAFK();

    /// @todo: Possibly add RBAC permission to log out in combat
    bool canLogoutInCombat = GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_RESTING);

    uint32 reason = 0;
    if (GetPlayer()->IsInCombat() && !canLogoutInCombat)
        reason = 1;
    else if (GetPlayer()->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR))
        reason = 3;                                         // is jumping or falling
    else if (preventAfkSanctuaryLogout || preventAfkLogout || GetPlayer()->duel || GetPlayer()->HasAura(9454)) // is dueling or frozen by GM via freeze command
        reason = 2;                                         // FIXME - Need the correct value

    WorldPackets::Character::LogoutResponse logoutResponse;
    logoutResponse.LogoutResult = reason;
    logoutResponse.Instant = instantLogout;
    SendPacket(logoutResponse.Write());

    if (reason)
    {
        SetLogoutStartTime(0);
        return;
    }

    //instant logout in taverns/cities or on taxi or for admins, gm's, mod's if its enabled in worldserver.conf
    if (instantLogout)
    {
        LogoutPlayer(true);
        return;
    }

    // not set flags if player can't free move to prevent lost state at logout cancel
    if (GetPlayer()->CanFreeMove())
    {
        if (GetPlayer()->getStandState() == UNIT_STAND_STATE_STAND)
        {
            GetPlayer()->SetStandState(UNIT_STAND_STATE_SIT);
        }

        GetPlayer()->SetRooted(true, true, true);
        GetPlayer()->SetUnitFlag(UNIT_FLAG_STUNNED);
    }

    SetLogoutStartTime(GameTime::GetGameTime().count());
}

void WorldSession::HandlePlayerLogoutOpcode(WorldPackets::Character::PlayerLogout& /*playerLogout*/)
{
}

void WorldSession::HandleLogoutCancelOpcode(WorldPackets::Character::LogoutCancel& /*logoutCancel*/)
{
    // Player have already logged out serverside, too late to cancel
    if (!GetPlayer())
        return;

    SetLogoutStartTime(0);

    SendPacket(WorldPackets::Character::LogoutCancelAck().Write());

    // not remove flags if can't free move - its not set in Logout request code.
    if (GetPlayer()->CanFreeMove())
    {
        GetPlayer()->SetRooted(false, true, true);

        GetPlayer()->SetStandState(UNIT_STAND_STATE_STAND);
        GetPlayer()->RemoveUnitFlag(UNIT_FLAG_STUNNED);
    }
}

void WorldSession::HandleTogglePvP(WorldPacket& recv_data)
{
    // this opcode can be used in two ways: Either set explicit new status or toggle old status
    if (recv_data.size() == 1)
    {
        bool newPvPStatus;
        recv_data >> newPvPStatus;
        GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP, newPvPStatus);
    }
    else
        GetPlayer()->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);

    if (GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(true, true);
    else if (!GetPlayer()->pvpInfo.IsHostile && GetPlayer()->IsPvP()) // pussywizard: in pvp mode, but doesn't want to be and not in hostile territory, so start timer
        GetPlayer()->UpdatePvP(true, false);

    //if (OutdoorPvP* pvp = _player->GetOutdoorPvP())
    //    pvp->HandlePlayerActivityChanged(_player);
}

// 5.4.8 18414, from Project SkyFire: CMSG_SET_PVP carries the new state, CMSG_TOGGLE_PVP has none
void WorldSession::HandleSetPvP(WorldPacket& recvData)
{
    bool newPvPStatus = recvData.ReadBit();
    GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP, newPvPStatus);

    if (GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(true, true);
    else if (!GetPlayer()->pvpInfo.IsHostile && GetPlayer()->IsPvP())
        GetPlayer()->UpdatePvP(true, false);
}

void WorldSession::HandleZoneUpdateOpcode(WorldPacket& recv_data)
{
    uint32 newZone;
    recv_data >> newZone;

    LOG_DEBUG("network", "WORLD: Recvd ZONE_UPDATE: {}", newZone);

    // use server side data, but only after update the player position. See Player::UpdatePosition().
    GetPlayer()->SetNeedZoneUpdate(true);

    //GetPlayer()->SendInitWorldStates(true, newZone);
}

void WorldSession::HandleSetSelectionOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recv_data.ReadGuidMask(guid, { 7, 6, 5, 4, 3, 2, 1, 0 });
    recv_data.ReadGuidBytes(guid, { 0, 7, 3, 5, 1, 4, 6, 2 });

    _player->SetSelection(guid);

    // Change target of current autoshoot spell
    if (guid)
    {
        if (Spell* autoReapeatSpell = _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL))
        {
            if (autoReapeatSpell->m_targets.GetUnitTargetGUID() != guid)
            {
                if (Unit* unit = ObjectAccessor::GetUnit(*_player, guid))
                {
                    if (unit->IsAlive() && !_player->IsFriendlyTo(unit) && unit->isTargetableForAttack(true, _player))
                    {
                        autoReapeatSpell->m_targets.SetUnitTarget(unit);
                    }
                }
            }
        }
    }
}

void WorldSession::HandleStandStateChangeOpcode(WorldPacket& recv_data)
{
    uint32 animstate;
    recv_data >> animstate;

    switch (animstate)
    {
        case UNIT_STAND_STATE_STAND:
        case UNIT_STAND_STATE_SIT:
        case UNIT_STAND_STATE_SLEEP:
        case UNIT_STAND_STATE_KNEEL:
            break;
        default:
            return;
    }

    _player->SetStandState(animstate);
}

void WorldSession::HandleLoadActionsSwitchSpec(PreparedQueryResult result)
{
    if (!GetPlayer())
        return;

    if (result)
        GetPlayer()->_LoadActions(result);

    GetPlayer()->SendActionButtons(1);
}

void WorldSession::HandleCharacterAuraFrozen(PreparedQueryResult result)
{
    if (!GetPlayer())
        return;

    ChatHandler handler = ChatHandler(this);

    // Select
    if (!result)
    {
        handler.SendSysMessage(LANG_COMMAND_NO_FROZEN_PLAYERS);
        return;
    }

    // Header of the names
    handler.PSendSysMessage(LANG_COMMAND_LIST_FREEZE);

    // Output of the results
    do
    {
        Field* fields = result->Fetch();
        std::string player = fields[0].Get<std::string>();
        handler.PSendSysMessage(LANG_COMMAND_FROZEN_PLAYERS, player);
    } while (result->NextRow());
}

void WorldSession::HandleBugOpcode(WorldPacket& recv_data)
{
    uint32 suggestion, contentlen, typelen;
    std::string content, type;

    // 5.4.8 18414, from Project SkyFire: the strings are sized, not terminated
    recv_data >> suggestion >> contentlen;
    content = recv_data.ReadString(contentlen);

    recv_data >> typelen;
    type = recv_data.ReadString(typelen);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BUG_REPORT);

    stmt->SetData(0, type);
    stmt->SetData(1, content);

    CharacterDatabase.Execute(stmt);
}

// Bug and suggestion reports go to the bug report table; complaints are read and dropped.
// 5.4.8 18414 layout, from Project SkyFire: position, facing and map, then the note.
static std::string ReadSupportTicketNote(WorldPacket& recvData)
{
    recvData.read_skip<float>();                            // x
    recvData.read_skip<float>();                            // y
    recvData.read_skip<float>();                            // z
    recvData.read_skip<float>();                            // facing
    recvData.read_skip<uint32>();                           // map
    return recvData.ReadString(recvData.ReadBits(10));
}

void WorldSession::HandleSupportTicketSubmitBugOpcode(WorldPacket& recvData)
{
    std::string note = ReadSupportTicketNote(recvData);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BUG_REPORT);
    stmt->SetData(0, std::string("Bug"));
    stmt->SetData(1, note);
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleSupportTicketSubmitSuggestionOpcode(WorldPacket& recvData)
{
    std::string note = ReadSupportTicketNote(recvData);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BUG_REPORT);
    stmt->SetData(0, std::string("Suggestion"));
    stmt->SetData(1, note);
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleSupportTicketSubmitComplaintOpcode(WorldPacket& recvData)
{
    recvData.rfinish();
}

void WorldSession::HandleReclaimCorpseOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recv_data.ReadGuidMask(guid, { 1, 5, 7, 2, 6, 3, 0, 4 });
    recv_data.ReadGuidBytes(guid, { 2, 5, 4, 6, 1, 0, 7, 3 });

    if (_player->IsAlive())
        return;

    // do not allow corpse reclaim in arena
    if (_player->InArena())
        return;

    // body not released yet
    if (!_player->HasPlayerFlag(PLAYER_FLAGS_GHOST))
        return;

    Corpse* corpse = _player->GetCorpse();
    if (!corpse)
        return;

    // prevent resurrect before 30-sec delay after body release not finished
    if (time_t(corpse->GetGhostTime() + _player->GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP)) > time_t(GameTime::GetGameTime().count()))
        return;

    // skip phase check
    if (!corpse->IsInMap(_player) || !corpse->IsWithinDist(_player, CORPSE_RECLAIM_RADIUS, true))
        return;

    // resurrect
    _player->ResurrectPlayer(_player->InBattleground() ? 1.0f : 0.5f);

    // spawn bones
    _player->SpawnCorpseBones();
}

void WorldSession::HandleResurrectResponseOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    uint32 status;
    // 5.4.8 18414, from Project SkyFire
    recv_data >> status;
    recv_data.ReadGuidMask(guid, { 3, 0, 6, 4, 5, 2, 1, 7 });
    recv_data.ReadGuidBytes(guid, { 7, 0, 1, 3, 4, 6, 2, 5 });

    // Xinef: Prevent resurrect with prevent resurrection aura
    if (GetPlayer()->IsAlive() || GetPlayer()->HasPreventResurectionAura())
        return;

    if (status == 0)
    {
        GetPlayer()->clearResurrectRequestData();           // reject
        return;
    }

    if (!GetPlayer()->isResurrectRequestedBy(guid))
        return;

    GetPlayer()->ResurectUsingRequestData();
}

void WorldSession::HandleAreaTriggerOpcode(WorldPacket& recv_data)
{
    uint32 triggerId;
    // 5.4.8 18414, from Project SkyFire
    recv_data >> triggerId;
    recv_data.ReadBit();                                    // entered
    recv_data.ReadBit();                                    // from client

    LOG_DEBUG("network", "CMSG_AREATRIGGER. Trigger ID: {}", triggerId);

    Player* player = GetPlayer();
    if (player->IsInFlight())
    {
        LOG_DEBUG("network", "HandleAreaTriggerOpcode: Player '{}' ({}) in flight, ignore Area Trigger ID:{}",
                       player->GetName(), player->GetGUID().ToString(), triggerId);
        return;
    }

    AreaTrigger const* atEntry = sObjectMgr->GetAreaTrigger(triggerId);
    if (!atEntry)
    {
        LOG_DEBUG("network", "HandleAreaTriggerOpcode: Player '{}' ({}) send unknown (by DBC) Area Trigger ID:{}",
                       player->GetName(), player->GetGUID().ToString(), triggerId);
        return;
    }

    uint32 teamFaction = player->GetTeamId(true) == TEAM_ALLIANCE ? FACTION_MASK_ALLIANCE : FACTION_MASK_HORDE;
    bool isTavernAreatrigger = sObjectMgr->IsTavernAreaTrigger(triggerId, teamFaction);
    if (!player->IsInAreaTriggerRadius(atEntry, isTavernAreatrigger ? 5.f : 0.f))
    {
        LOG_DEBUG("network", "HandleAreaTriggerOpcode: Player {} ({}) too far (trigger map: {} player map: {}), ignore Area Trigger ID: {}",
                       player->GetName(), player->GetGUID().ToString(), atEntry->map, player->GetMapId(), triggerId);
        return;
    }

    if (player->isDebugAreaTriggers)
        ChatHandler(this).PSendSysMessage(LANG_DEBUG_AREATRIGGER_REACHED, triggerId);

    // Skip areatrigger scripts for GMs unless debug areatriggers is enabled
    if ((!player->IsGameMaster() || player->isDebugAreaTriggers) && sScriptMgr->OnAreaTrigger(player, atEntry))
        return;

    if (player->IsAlive())
    {
        if (std::unordered_set<uint32> const* quests = sObjectMgr->GetQuestsForAreaTrigger(triggerId))
        {
            for (uint32 questId : *quests)
            {
                Quest const* qInfo = sObjectMgr->GetQuestTemplate(questId);
                if (!qInfo || player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
                    continue;

                // 6.x QUEST_OBJECTIVE_AREATRIGGER objective, otherwise areatrigger_involvedrelation exploration
                bool hasObjective = false;
                for (QuestObjective const& obj : qInfo->GetObjectives())
                {
                    if (obj.Type == QUEST_OBJECTIVE_AREATRIGGER && obj.StorageIndex >= 0)
                    {
                        player->SetQuestObjectiveData(qInfo, obj.StorageIndex, int32(true));
                        hasObjective = true;
                        break;
                    }
                }

                if (hasObjective)
                {
                    if (player->CanCompleteQuest(questId))
                        player->CompleteQuest(questId);
                }
                else
                    player->AreaExploredOrEventHappens(questId);
            }
        }
    }

    if (isTavernAreatrigger)
    {
        // set resting flag we are in the inn
        player->SetRestFlag(REST_FLAG_IN_TAVERN, atEntry->entry);

        if (sWorld->IsFFAPvPRealm())
        {
            if (player->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP))
            {
                player->RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
                sScriptMgr->OnPlayerFfaPvpStateUpdate(player, false);

            }
        }
        return;
    }

    if (Battleground* bg = player->GetBattleground())
        if (bg->GetStatus() == STATUS_IN_PROGRESS)
        {
            bg->HandleAreaTrigger(player, triggerId);
            return;
        }

    if (OutdoorPvP* pvp = player->GetOutdoorPvP())
        if (pvp->HandleAreaTrigger(_player, triggerId))
            return;

    AreaTriggerTeleport const* at = sObjectMgr->GetAreaTriggerTeleport(triggerId);
    if (!at)
        return;

    bool teleported = false;
    if (player->GetMapId() != at->target_mapId)
    {
       if (Map::EnterState denyReason = sMapMgr->PlayerCannotEnter(at->target_mapId, player, false))
        {
            bool reviveAtTrigger = false; // should we revive the player if he is trying to enter the correct instance?
            switch (denyReason)
            {
                case Map::CANNOT_ENTER_NOT_IN_RAID:
                case Map::CANNOT_ENTER_INSTANCE_BIND_MISMATCH:
                case Map::CANNOT_ENTER_TOO_MANY_INSTANCES:
                case Map::CANNOT_ENTER_MAX_PLAYERS:
                case Map::CANNOT_ENTER_ZONE_IN_COMBAT:
                    reviveAtTrigger = true;
                    break;
                default:
                    break;
            }

            if (reviveAtTrigger) // check if the player is touching the areatrigger leading to the map his corpse is on
            {
                if (!player->IsAlive() && player->HasCorpse())
                {
                    if (player->GetCorpseLocation().GetMapId() == at->target_mapId)
                    {
                        player->ResurrectPlayer(0.5f);
                        player->SpawnCorpseBones();
                    }
                }
            }

            return;
        }

        if (Group* group = player->GetGroup())
            if (group->isLFGGroup() && player->GetMap()->IsDungeon())
                teleported = player->TeleportToEntryPoint();
    }

    if (!teleported)
        player->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, at->target_Orientation, TELE_TO_NOT_LEAVE_TRANSPORT);
}

void WorldSession::HandleUpdateAccountData(WorldPacket& recv_data)
{
    // 5.4.8 18414, from Project SkyFire: the type follows the compressed data
    uint32 timestamp, decompressedSize, compressedSize;
    recv_data >> decompressedSize >> timestamp >> compressedSize;

    LOG_DEBUG("network", "UAD: time {}, decompressedSize {}", timestamp, decompressedSize);

    if (decompressedSize > 0xFFFF || compressedSize > recv_data.size() - recv_data.rpos())
    {
        recv_data.rfinish();                   // unnneded warning spam in this case
        LOG_ERROR("network.opcode", "UAD: Account data packet too big, size {}", decompressedSize);
        return;
    }

    std::string adata;
    if (decompressedSize)
    {
        ByteBuffer dest;
        dest.resize(decompressedSize);

        uLongf realSize = decompressedSize;
        if (uncompress(dest.contents(), &realSize, recv_data.contents() + recv_data.rpos(), compressedSize) != Z_OK)
        {
            recv_data.rfinish();                   // unnneded warning spam in this case
            LOG_ERROR("network.opcode", "UAD: Failed to decompress account data");
            return;
        }

        adata = dest.ReadString(realSize);
    }

    recv_data.read_skip(compressedSize);
    uint32 type = recv_data.ReadBits(3);

    if (type >= NUM_ACCOUNT_DATA_TYPES)
        return;

    SetAccountData(AccountDataType(type), decompressedSize ? timestamp : 0, adata);

}

void WorldSession::HandleRequestAccountData(WorldPacket& recv_data)
{
    uint32 type = recv_data.ReadBits(3);                    // 5.4.8, from Project SkyFire

    LOG_DEBUG("network", "RAD: type {}", type);

    if (type >= NUM_ACCOUNT_DATA_TYPES)
        return;

    AccountData* adata = GetAccountData(AccountDataType(type));

    uint32 size = adata->Data.size();

    uLongf destSize = compressBound(size);

    ByteBuffer dest;
    dest.resize(destSize);

    if (size && compress(dest.contents(), &destSize, (uint8 const*)adata->Data.c_str(), size) != Z_OK)
    {
        LOG_DEBUG("network", "RAD: Failed to compress account data");
        return;
    }

    dest.resize(destSize);

    // 5.4.8 18414, from Project SkyFire
    ObjectGuid guid = _player ? _player->GetGUID() : ObjectGuid::Empty;
    WorldPacket data(SMSG_UPDATE_ACCOUNT_DATA, 1 + 8 + 4 + 4 + 4 + destSize);
    data.WriteBits(type, 3);                                    // type (0-7)
    data.WriteGuidMask(guid, { 5, 1, 3, 7, 0, 4, 2, 6 });
    data.FlushBits();
    data.WriteGuidBytes(guid, { 3, 1, 5 });
    data << uint32(size);                                       // decompressed length
    data << uint32(destSize);
    data.append(dest);                                          // compressed data
    data.WriteGuidBytes(guid, { 7, 4, 0, 6, 2 });
    data << uint32(adata->Time);                                // unix time
    SendPacket(&data);
}

void WorldSession::HandleSetActionButtonOpcode(WorldPacket& recv_data)
{
    // 5.4.8 18414, from Project SkyFire: the action in the low four bytes, the type in the high byte
    uint8 button;
    ObjectGuid buttonStream;
    recv_data >> button;
    recv_data.ReadGuidMask(buttonStream, { 7, 0, 5, 2, 1, 6, 3, 4 });
    recv_data.ReadGuidBytes(buttonStream, { 6, 7, 3, 5, 2, 1, 4, 0 });

    uint64 packetData = buttonStream.GetRawValue();
    uint32 action = uint32(packetData);
    uint8  type   = uint8(packetData >> 56);

    LOG_DEBUG("network.opcode", "BUTTON: {} ACTION: {} TYPE: {}", button, action, type);
    if (!packetData)
    {
        LOG_DEBUG("network.opcode", "MISC: Remove action from button {}", button);
        GetPlayer()->removeActionButton(button);
    }
    else
    {
        switch (type)
        {
            case ACTION_BUTTON_MACRO:
            case ACTION_BUTTON_CMACRO:
                LOG_DEBUG("network.opcode", "MISC: Added Macro {} into button {}", action, button);
                break;
            case ACTION_BUTTON_EQSET:
                LOG_DEBUG("network.opcode", "MISC: Added EquipmentSet {} into button {}", action, button);
                break;
            case ACTION_BUTTON_SPELL:
                LOG_DEBUG("network.opcode", "MISC: Added Spell {} into button {}", action, button);
                break;
            case ACTION_BUTTON_ITEM:
                LOG_DEBUG("network.opcode", "MISC: Added Item {} into button {}", action, button);
                break;
            default:
                LOG_ERROR("network.opcode", "MISC: Unknown action button type {} for action {} into button {} for player {} ({})",
                    type, action, button, _player->GetName(), _player->GetGUID().ToString());
                return;
        }
        GetPlayer()->addActionButton(button, action, type);
    }
}

void WorldSession::HandleCompleteCinematic(WorldPacket& /*recv_data*/)
{
    // End the current cinematic and restore the normal player view
    GetPlayer()->GetCinematicMgr().EndCinematic();
}

void WorldSession::HandleCompleteMovie(WorldPacket& /*recv_data*/)
{
    if (!GetPlayer()->GetMovie())
        return;

    GetPlayer()->SetMovie(0);
}

void WorldSession::HandleNextCinematicCamera(WorldPacket& /*recv_data*/)
{
    // Sent by client when cinematic actually begun. So we begin the server side process
    GetPlayer()->GetCinematicMgr().StartCinematicCamera();
}

void WorldSession::HandleSetActionBarToggles(WorldPacket& recv_data)
{
    uint8 ActionBar;

    recv_data >> ActionBar;

    if (!GetPlayer())                                        // ignore until not logged (check needed because STATUS_AUTHED)
    {
        if (ActionBar != 0)
            LOG_ERROR("network.opcode", "WorldSession::HandleSetActionBarToggles in not logged state with value: {}, ignored", uint32(ActionBar));
        return;
    }

    GetPlayer()->SetByteValue(PLAYER_FIELD_BYTES, 2, ActionBar);
}

void WorldSession::HandlePlayedTime(WorldPackets::Character::PlayedTimeClient& packet)
{
    WorldPackets::Character::PlayedTime playedTime;
    playedTime.TotalTime = _player->GetTotalPlayedTime();
    playedTime.LevelTime = _player->GetLevelPlayedTime();
    playedTime.TriggerScriptEvent = packet.TriggerScriptEvent;  // 0-1 - will not show in chat frame
    SendPacket(playedTime.Write());
}

void WorldSession::HandleInspectOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recv_data.ReadGuidMask(guid, { 0, 3, 7, 2, 5, 1, 4, 6 });
    recv_data.ReadGuidBytes(guid, { 3, 5, 2, 4, 1, 6, 0, 7 });

    Player* player = ObjectAccessor::GetPlayer(*_player, guid);
    if (!player)
    {
        LOG_DEBUG("network", "CMSG_INSPECT: No player found from {}", guid.ToString());
        return;
    }

    if (!GetPlayer()->IsWithinDistInMap(player, INSPECT_DISTANCE, false))
    {
        return;
    }

    if (GetPlayer()->IsValidAttackTarget(player))
    {
        return;
    }

    // 5.4.8 18414, from Project SkyFire (SMSG_INSPECT_RESULTS). SkyFire sends the inspector's
    // talent group; the inspected player's active group is used here.
    bool const showTalents = sWorld->getBoolConfig(CONFIG_TALENTS_INSPECTING) || _player->CanBeGameMaster();
    uint8 const activeSpec = player->GetActiveSpec();
    Guild* guild = sGuildMgr->GetGuildById(player->GetGuildId());
    ObjectGuid const guildGuid = guild ? guild->GetGUID() : ObjectGuid::Empty;

    ByteBuffer itemData;
    WorldPacket data(SMSG_INSPECT_RESULTS, 256);
    data.WriteBit(guild != nullptr);
    data.WriteBit(guid[2]);
    if (guild)
        data.WriteGuidMask(guildGuid, { 7, 0, 5, 3, 2, 4, 6, 1 });
    data.WriteBit(guid[4]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[7]);
    std::size_t const slotCountPos = data.bitwpos();
    data.WriteBits(0, 20);                                  // [PH] item count
    data.WriteBit(guid[0]);

    uint32 slotCount = 0;
    for (uint8 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;

        ++slotCount;

        ObjectGuid const creatorGuid = item->GetGuidValue(ITEM_FIELD_CREATOR);
        int32 const randomProperty = item->GetItemRandomPropertyId();
        uint32 const suffixFactor = item->GetItemSuffixFactor();

        data.WriteBit(creatorGuid[1]);
        data.WriteBit(suffixFactor != 0);
        data.WriteBit(true);                                // item modifiers present
        data.WriteBit(creatorGuid[3]);
        std::size_t const enchantCountPos = data.bitwpos();
        data.WriteBits(0, 21);                              // [PH] enchant count
        data.WriteBit(creatorGuid[2]);
        data.WriteBit(creatorGuid[6]);
        data.WriteBit(creatorGuid[4]);
        data.WriteBit(randomProperty != 0);
        data.WriteBit(creatorGuid[0]);
        data.WriteBit(creatorGuid[5]);
        data.WriteBit(creatorGuid[7]);

        if (randomProperty)
            itemData << int16(randomProperty);
        itemData.WriteByteSeq(creatorGuid[3]);

        // item modifiers block: byte size, then the modifier mask (no modifier values are kept on items yet)
        itemData << uint32(4);
        itemData << uint32(0);

        uint32 enchantCount = 0;
        for (uint8 enchantSlot = 0; enchantSlot < MAX_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            uint32 const enchantId = item->GetEnchantmentId(EnchantmentSlot(enchantSlot));
            if (!enchantId)
                continue;

            ++enchantCount;
            itemData << uint32(enchantId);
            itemData << uint8(enchantSlot);
        }
        data.PutBits(enchantCountPos, enchantCount, 21);

        itemData << uint32(item->GetEntry());
        itemData.WriteByteSeq(creatorGuid[6]);
        itemData.WriteByteSeq(creatorGuid[4]);
        itemData.WriteByteSeq(creatorGuid[7]);
        itemData.WriteByteSeq(creatorGuid[2]);
        if (suffixFactor)
            itemData << uint32(suffixFactor);
        itemData.WriteByteSeq(creatorGuid[5]);
        itemData << uint8(slot);
        itemData.WriteByteSeq(creatorGuid[0]);
        itemData.WriteByteSeq(creatorGuid[1]);
    }
    data.PutBits(slotCountPos, slotCount, 20);

    std::vector<uint16> glyphs;
    std::vector<uint16> talents;
    if (showTalents)
    {
        for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
            if (uint32 glyphId = player->GetGlyph(i))
                glyphs.push_back(uint16(glyphId));

        for (auto const& [spellId, talent] : player->GetTalentMap())
            if (talent->State != PLAYERSPELL_REMOVED && talent->IsInSpec(activeSpec))
                talents.push_back(uint16(talent->talentID));
    }

    data.WriteBits(glyphs.size(), 23);
    data.WriteBits(talents.size(), 23);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[1]);
    data.FlushBits();

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[2]);
    data.append(itemData);

    if (guild)
    {
        data.WriteGuidBytes(guildGuid, { 6, 2, 5, 0 });
        data << uint32(guild->GetMemberCount());
        data.WriteGuidBytes(guildGuid, { 4, 7 });
        data << uint64(guild->GetExperience());
        data.WriteByteSeq(guildGuid[1]);
        data << uint32(guild->GetLevel());
        data.WriteByteSeq(guildGuid[3]);
    }

    data.WriteByteSeq(guid[5]);
    for (uint16 glyphId : glyphs)
        data << uint16(glyphId);                            // GlyphProperties.dbc
    data.WriteByteSeq(guid[0]);
    data << uint32(showTalents ? player->GetSpecializationId(activeSpec) : 0);
    for (uint16 talentId : talents)
        data << uint16(talentId);                           // Talent.dbc
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    SendPacket(&data);
}

void WorldSession::HandleSelectFactionOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire: 0 = Horde, 1 = Alliance
    uint32 faction;
    recvData >> faction;

    if (faction > 1 || !_player->JoinPandarenFaction(faction == 1))
        return;

    // "The Healing of Shen-zin Su" ends with the faction choice
    if (_player->GetQuestStatus(31450) == QUEST_STATUS_INCOMPLETE)
        _player->KilledMonsterCredit(64594);

    _player->SendMovieStart(116);
}

void WorldSession::HandleRequestHonorStatsOpcode(WorldPackets::Inspect::RequestHonorStats& packet)
{
    Player* player = ObjectAccessor::GetPlayer(*_player, packet.TargetGUID);
    if (!player)
    {
        LOG_DEBUG("network", "CMSG_REQUEST_HONOR_STATS: No player found from {}", packet.TargetGUID.ToString());
        return;
    }

    if (!GetPlayer()->IsWithinDistInMap(player, INSPECT_DISTANCE, false))
        return;

    if (GetPlayer()->IsValidAttackTarget(player))
        return;

    WorldPackets::Inspect::InspectHonorStats inspectHonorStats;
    inspectHonorStats.PlayerGUID = player->GetGUID();
    inspectHonorStats.LifetimeHK = player->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS);
    inspectHonorStats.YesterdayHK = player->GetUInt16Value(PLAYER_FIELD_KILLS, 1);
    inspectHonorStats.TodayHK = player->GetUInt16Value(PLAYER_FIELD_KILLS, 0);
    SendPacket(inspectHonorStats.Write());
}

void WorldSession::HandleRequestInspectRatedBgStatsOpcode(WorldPackets::Inspect::RequestInspectRatedBgStats& packet)
{
    Player* player = ObjectAccessor::GetPlayer(*_player, packet.TargetGUID);
    if (!player)
    {
        LOG_DEBUG("network", "CMSG_REQUEST_INSPECT_RATED_BG_STATS: No player found from {}", packet.TargetGUID.ToString());
        return;
    }

    if (!GetPlayer()->IsWithinDistInMap(player, INSPECT_DISTANCE, false))
        return;

    if (GetPlayer()->IsValidAttackTarget(player))
        return;

    // rated battlegrounds keep no per-player statistics yet
    WorldPackets::Inspect::InspectRatedBgStats inspectRatedBgStats;
    inspectRatedBgStats.PlayerGUID = player->GetGUID();
    SendPacket(inspectRatedBgStats.Write());
}

void WorldSession::HandleWorldTeleportOpcode(WorldPacket& recv_data)
{
    uint32 time;
    uint32 mapid;
    float PositionX;
    float PositionY;
    float PositionZ;
    float Orientation;

    recv_data >> time;                                      // time in m.sec.
    recv_data >> mapid;
    recv_data >> PositionX;
    recv_data >> PositionY;
    recv_data >> PositionZ;
    recv_data >> Orientation;                               // o (3.141593 = 180 degrees)

    if (GetPlayer()->IsInFlight())
    {
        LOG_DEBUG("network", "Player '{}' ({}) in flight, ignore worldport command.", GetPlayer()->GetName(), GetPlayer()->GetGUID().ToString());
        return;
    }

    LOG_DEBUG("network", "CMSG_WORLD_TELEPORT: Player = {}, Time = {}, map = {}, x = {}, y = {}, z = {}, o = {}", GetPlayer()->GetName(), time, mapid, PositionX, PositionY, PositionZ, Orientation);

    if (HasPermission(rbac::RBAC_PERM_OPCODE_WORLD_TELEPORT))
        GetPlayer()->TeleportTo(mapid, PositionX, PositionY, PositionZ, Orientation);
    else
        ChatHandler(this).SendNotification(LANG_PERMISSION_DENIED);
}

void WorldSession::HandleWhoisOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "Received opcode CMSG_WHOIS");
    std::string charname;
    recv_data >> charname;

    if (!HasPermission(rbac::RBAC_PERM_OPCODE_WHOIS))
    {
        ChatHandler(this).SendNotification(LANG_PERMISSION_DENIED);
        return;
    }

    if (charname.empty() || !normalizePlayerName (charname))
    {
        ChatHandler(this).SendNotification(LANG_NEED_CHARACTER_NAME);
        return;
    }

    Player* player = ObjectAccessor::FindPlayerByName(charname, false);

    if (!player)
    {
        ChatHandler(this).SendNotification(LANG_PLAYER_NOT_EXIST_OR_OFFLINE, charname.c_str());
        return;
    }

    uint32 accid = player->GetSession()->GetAccountId();

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_WHOIS);

    stmt->SetData(0, accid);

    PreparedQueryResult result = LoginDatabase.Query(stmt);

    if (!result)
    {
        ChatHandler(this).SendNotification(LANG_ACCOUNT_FOR_PLAYER_NOT_FOUND, charname.c_str());
        return;
    }

    Field* fields = result->Fetch();
    std::string acc = fields[0].Get<std::string>();
    if (acc.empty())
        acc = "Unknown";
    std::string email = fields[1].Get<std::string>();
    if (email.empty())
        email = "Unknown";
    std::string lastip = fields[2].Get<std::string>();
    if (lastip.empty())
        lastip = "Unknown";

    std::string msg = charname + "'s " + "account is " + acc + ", e-mail: " + email + ", last ip: " + lastip;

    // 5.4.8 18414, from Project SkyFire
    WorldPacket data(SMSG_WHOIS, 2 + msg.size());
    data.WriteBits(msg.size(), 11);
    data.FlushBits();
    data.WriteString(msg);
    SendPacket(&data);

    LOG_DEBUG("network", "Received whois command from player {} for character {}", GetPlayer()->GetName(), charname);
}

void WorldSession::HandleComplainOpcode(WorldPackets::Misc::Complain& packet)
{
    LOG_DEBUG("network", "WORLD: CMSG_COMPLAIN");

    // NOTE: all chat messages from this spammer automatically ignored by spam reporter until logout in case chat spam.
    // if it's mail spam - ALL mails from this spammer automatically removed by client

    // Complaint Received message
    SendPacket(WorldPackets::Misc::ComplainResult().Write());

    if (sWorld->getBoolConfig(CONFIG_LOGSPAMREPORTS))
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_SPAM_REPORT);

        stmt->SetData(0, packet.SpamType);
        stmt->SetData(1, packet.SpammerGuid.GetCounter());
        stmt->SetData(2, packet.Unk1);
        stmt->SetData(3, packet.MailIdOrMessageType);
        stmt->SetData(4, packet.ChannelId);
        stmt->SetData(5, packet.SecondsSinceMessage);
        stmt->SetData(6, packet.Description);
        stmt->SetData(7, GameTime::GetGameTime().count());

        CharacterDatabase.Execute(stmt);
    }
}

void WorldSession::HandleRealmSplitOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "CMSG_REALM_SPLIT");

    uint32 unk;
    std::string split_date = "01/01/01";
    recv_data >> unk;

    // 5.4.8 18414, from Project SkyFire
    WorldPacket data(SMSG_REALM_SPLIT, 1 + 4 + 4 + split_date.size());
    data.WriteBits(split_date.size(), 7);
    data.FlushBits();
    data << uint32(0x00000000);                             // realm split state
    // split states:
    // 0x0 realm normal
    // 0x1 realm split
    // 0x2 realm split pending
    data << unk;
    data.WriteString(split_date);
    SendPacket(&data);
}

void WorldSession::HandleFarSightOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_FAR_SIGHT");

    bool apply = recvData.ReadBit();                        // 5.4.8, from Project SkyFire

    if (apply)
    {
        LOG_DEBUG("network", "Added FarSight {} to player {}", _player->GetGuidValue(PLAYER_FARSIGHT).ToString(), _player->GetGUID().ToString());
        if (WorldObject* target = _player->GetViewpoint())
            _player->SetSeer(target);
        else
            LOG_DEBUG("network.opcode", "Player {} requests non-existing seer {}", _player->GetName(), _player->GetGuidValue(PLAYER_FARSIGHT).ToString());
    }
    else
    {
        WorldObject* newFarsightobject = nullptr;
        if (WorldObject* viewpoint = _player->GetViewpoint())
        {
            if (DynamicObject* viewpointDynamicObject = viewpoint->ToDynObject())
            {
                newFarsightobject = ObjectAccessor::GetUnit(*viewpointDynamicObject, viewpointDynamicObject->GetOldFarsightGUID());
            }
            else if (DynamicObject* viewpointDynamicObject = _player->GetDynObject(_player->GetUInt32Value(UNIT_CHANNEL_SPELL)))
            {
                if (viewpointDynamicObject->IsViewpoint() && viewpointDynamicObject->GetCasterGUID() == _player->GetGUID())
                {
                    newFarsightobject = viewpointDynamicObject;
                }
            }
        }

        if (newFarsightobject)
        {
            LOG_DEBUG("network", "Player {} set vision to old farsight {}", _player->GetGUID().ToString(), newFarsightobject->GetGUID().ToString());
            _player->SetViewpoint(_player->GetViewpoint(), false);
            _player->SetViewpoint(newFarsightobject, true);
        }
        else
        {
            LOG_DEBUG("network", "Player {} set vision to self", _player->GetGUID().ToString());
            _player->SetSeer(_player);
        }
    }

    GetPlayer()->UpdateVisibilityForPlayer();
}

void WorldSession::HandleSetTitleOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "CMSG_SET_TITLE");

    int32 title;
    recv_data >> title;

    // -1 at none
    if (title > 0 && title < MAX_TITLE_INDEX)
    {
        if (!GetPlayer()->HasTitle(title))
            return;
    }
    else
        title = 0;

    GetPlayer()->SetUInt32Value(PLAYER_CHOSEN_TITLE, title);
}

void WorldSession::HandleResetInstancesOpcode(WorldPackets::Instance::ResetInstances& /*packet*/)
{
    LOG_DEBUG("network", "WORLD: CMSG_RESET_INSTANCES");

    if (Group* group = _player->GetGroup())
    {
        if (group->IsLeader(_player->GetGUID()))
            group->ResetInstances(INSTANCE_RESET_ALL, false, _player);
    }
    else
        Player::ResetInstances(_player->GetGUID(), INSTANCE_RESET_ALL, false);
}

void WorldSession::HandleSetDungeonDifficultyOpcode(WorldPackets::Instance::SetDungeonDifficultyClient& packet)
{
    LOG_DEBUG("network", "MSG_SET_DUNGEON_DIFFICULTY");

    if (packet.Mode >= MAX_DUNGEON_DIFFICULTY)
        return;

    if (Difficulty(packet.Mode) == _player->GetDungeonDifficulty())
        return;

    Group* group = _player->GetGroup();
    if (group)
    {
        if (group->IsLeader(_player->GetGUID()))
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupGuy = itr->GetSource();
                if (!groupGuy)
                    continue;

                if (!groupGuy->IsInWorld())
                {
                    _player->SendDungeonDifficulty(group != nullptr);
                    return;
                }

                if (groupGuy->GetGUID() == _player->GetGUID() ? groupGuy->GetMap()->IsDungeon() : groupGuy->GetMap()->IsNonRaidDungeon())
                {
                    _player->SendDungeonDifficulty(group != nullptr);
                    return;
                }
            }

            group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false, _player);
            group->SetDungeonDifficulty(Difficulty(packet.Mode));
        }
    }
    else
    {
        if (_player->FindMap() && _player->FindMap()->IsDungeon())
        {
            _player->SendDungeonDifficulty(group != nullptr);
            return;
        }
        Player::ResetInstances(_player->GetGUID(), INSTANCE_RESET_CHANGE_DIFFICULTY, false);
        _player->SetDungeonDifficulty(Difficulty(packet.Mode));
    }
}

void WorldSession::HandleSetRaidDifficultyOpcode(WorldPackets::Instance::SetRaidDifficultyClient& packet)
{
    LOG_DEBUG("network", "MSG_SET_RAID_DIFFICULTY");

    if (packet.Mode >= MAX_RAID_DIFFICULTY)
        return;

    if (Difficulty(packet.Mode) == _player->GetRaidDifficulty())
        return;

    Group* group = _player->GetGroup();
    if (group)
    {
        if (group->IsLeader(_player->GetGUID()))
        {
            std::set<uint32> foundMaps;
            std::set<Map*> foundMapsPtr;
            Map* currMap = nullptr;

            if (uint32 preventionTime = group->GetDifficultyChangePreventionTime())
            {
                switch (group->GetDifficultyChangePreventionReason())
                {
                    case DIFFICULTY_PREVENTION_CHANGE_BOSS_KILLED:
                        ChatHandler(this).PSendSysMessage("Raid was in combat recently and may not change difficulty again for {} sec.", preventionTime);
                        break;
                    case DIFFICULTY_PREVENTION_CHANGE_RECENTLY_CHANGED:
                    default:
                        ChatHandler(this).PSendSysMessage("Raid difficulty has changed recently, and may not change again for {} sec.", preventionTime);
                        break;
                }

                _player->SendRaidDifficulty(group != nullptr);
                return;
            }

            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupGuy = itr->GetSource();
                if (!groupGuy)
                    continue;

                if (!groupGuy->IsInWorld())
                {
                    _player->SendRaidDifficulty(group != nullptr);
                    return;
                }

                if (IsSharedDifficultyMap(groupGuy->GetMap()->GetId()) && (uint32(packet.Mode % 2) == uint32(_player->GetRaidDifficulty() % 2)) && group->isRaidGroup())
                {
                    if (!currMap)
                        currMap = groupGuy->GetMap();
                    foundMaps.insert(groupGuy->GetMap()->GetId());
                    foundMapsPtr.insert(groupGuy->GetMap());
                    if (foundMaps.size() > 1 || foundMapsPtr.size() > 1)
                    {
                        _player->SendRaidDifficulty(group != nullptr);
                        return;
                    }

                    if (!groupGuy->IsAlive() || groupGuy->IsInCombat() || groupGuy->GetVictim() || groupGuy->m_mover != groupGuy || groupGuy->IsNonMeleeSpellCast(true) || (!groupGuy->GetMotionMaster()->empty() && groupGuy->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                            || !groupGuy->movespline->Finalized() || !groupGuy->GetMap()->ToInstanceMap() || !groupGuy->GetMap()->ToInstanceMap()->GetInstanceScript() || groupGuy->GetMap()->ToInstanceMap()->GetInstanceScript()->IsEncounterInProgress()
                            || !groupGuy->Satisfy(sObjectMgr->GetAccessRequirement(groupGuy->GetMap()->GetId(), Difficulty(packet.Mode)), groupGuy->GetMap()->GetId(), false))
                    {
                        _player->SendRaidDifficulty(group != nullptr);
                        return;
                    }
                }
                else if (groupGuy->GetGUID() == _player->GetGUID() ? groupGuy->GetMap()->IsDungeon() : groupGuy->GetMap()->IsRaid())
                {
                    _player->SendRaidDifficulty(group != nullptr);
                    return;
                }
            }

            Map* homeMap571 = sMapMgr->CreateMap(MAP_NORTHREND, nullptr);
            Map* homeMap0 = sMapMgr->CreateMap(MAP_EASTERN_KINGDOMS, nullptr);
            ASSERT(homeMap0 && homeMap571);

            std::map<Player*, Position> playerTeleport;
            // handle here all players in the instance that are not in the group
            if (currMap)
                for (Map::PlayerList::const_iterator itr = currMap->GetPlayers().begin(); itr != currMap->GetPlayers().end(); ++itr)
                    if (Player* p = itr->GetSource())
                        if (p->GetGroup() != group)
                        {
                            if (!p->IsInWorld() || !p->IsAlive() || p->IsInCombat() || p->GetVictim() || p->m_mover != p || p->IsNonMeleeSpellCast(true) || (!p->GetMotionMaster()->empty() && p->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                                    || !p->movespline->Finalized() || !p->GetMap()->ToInstanceMap() || !p->GetMap()->ToInstanceMap()->GetInstanceScript() || p->GetMap()->ToInstanceMap()->GetInstanceScript()->IsEncounterInProgress())
                            {
                                _player->SendRaidDifficulty(group != nullptr);
                                return;
                            }
                            playerTeleport[p];
                        }
            for (std::map<Player*, Position>::iterator itr = playerTeleport.begin(); itr != playerTeleport.end(); ++itr)
            {
                Player* p = itr->first;
                Map* oldMap = p->GetMap();
                oldMap->RemovePlayerFromMap(p, false);
                p->ResetMap();
                oldMap->AfterPlayerUnlinkFromMap();
                p->SetMap(homeMap0);
                p->Relocate(0.0f, 0.0f, 0.0f, 0.0f);
                if (!p->TeleportTo(MAP_NORTHREND, 5790.20f, 2071.36f, 636.07f, 3.60f))
                    p->GetSession()->KickPlayer("HandleSetRaidDifficultyOpcode 1");
            }

            bool anyoneInside = false;
            playerTeleport.clear();
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupGuy = itr->GetSource();
                if (!groupGuy)
                    continue;

                if (IsSharedDifficultyMap(groupGuy->GetMap()->GetId()))
                {
                    anyoneInside = true;

                    float x, y, z, o;
                    groupGuy->GetPosition(x, y, z, o);
                    Map* oldMap = groupGuy->GetMap();
                    oldMap->RemovePlayerFromMap(groupGuy, false);
                    groupGuy->ResetMap();
                    oldMap->AfterPlayerUnlinkFromMap();
                    groupGuy->SetMap(homeMap571);
                    groupGuy->Relocate(5790.20f, 2071.36f, 636.07f, 3.60f);
                    Position dest = {x, y, z + 0.1f, o};
                    playerTeleport[groupGuy] = dest;
                }
            }

            if (!anyoneInside) // pussywizard: don't reset if changing ICC/RS difficulty while inside
                group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, _player);
            group->SetRaidDifficulty(Difficulty(packet.Mode));
            group->SetDifficultyChangePrevention(DIFFICULTY_PREVENTION_CHANGE_RECENTLY_CHANGED);

            for (std::map<Player*, Position>::iterator itr = playerTeleport.begin(); itr != playerTeleport.end(); ++itr)
            {
                itr->first->SetRaidDifficulty(Difficulty(packet.Mode)); // needed for teleport not to fail
                if (!itr->first->TeleportTo(*(foundMaps.begin()), itr->second.GetPositionX(), itr->second.GetPositionY(), itr->second.GetPositionZ(), itr->second.GetOrientation()))
                    itr->first->GetSession()->KickPlayer("HandleSetRaidDifficultyOpcode 2");
            }
        }
    }
    else
    {
        if (_player->FindMap() && _player->FindMap()->IsDungeon())
        {
            _player->SendRaidDifficulty(group != nullptr);
            return;
        }
        Player::ResetInstances(_player->GetGUID(), INSTANCE_RESET_CHANGE_DIFFICULTY, true);
        _player->SetRaidDifficulty(Difficulty(packet.Mode));
    }
}

void WorldSession::HandleCancelMountAuraOpcode(WorldPacket& /*recv_data*/)
{
    LOG_DEBUG("network", "WORLD: CMSG_CANCEL_MOUNT_AURA");

    //If player is not mounted, so go out :)
    if (!_player->IsMounted())                              // not blizz like; no any messages on blizz
    {
        ChatHandler(this).SendSysMessage(LANG_CHAR_NON_MOUNTED);
        return;
    }

    if (_player->IsInFlight())                               // not blizz like; no any messages on blizz
    {
        ChatHandler(this).SendSysMessage(LANG_YOU_IN_FLIGHT);
        return;
    }

    _player->Dismount();
    _player->RemoveAurasByType(SPELL_AURA_MOUNTED);
}

void WorldSession::HandleMoveFlagChangeOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "WORLD: {}", GetOpcodeNameForLogging(static_cast<OpcodeClient>(recv_data.GetOpcode())));

    OpcodeClient opcode = static_cast<OpcodeClient>(recv_data.GetOpcode());

    WorldPackets::Movement::MovementStatusElements const* sequence = WorldPackets::Movement::GetMovementFlagSequence(opcode);
    if (!sequence)
    {
        recv_data.rfinish();
        return;
    }

    uint32 counter = 0;
    MovementInfo movementInfo;
    WorldPackets::Movement::ReadGroundMovement(recv_data, movementInfo, sequence, &counter);
    SanitizeMovementFlags(&movementInfo);

    Unit* mover = _player->m_mover;
    if (movementInfo.guid != mover->GetGUID())
        return;

    // old map - async processing, ignore
    if (counter <= _player->GetMapChangeOrderCounter())
        return;

    sScriptMgr->AnticheatSetCanFlybyServer(_player, movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY));

    if (!ProcessMovementInfo(movementInfo, mover, mover->ToPlayer(), recv_data))
        return;

    if (opcode == CMSG_MOVE_SET_CAN_FLY_ACK && _player->GetPendingFlightChange() == counter)
        _player->SetPendingFlightChange(false);

    // 5.4.8 relays the new movement state as SMSG_PLAYER_MOVE
    WorldPacket data(SMSG_PLAYER_MOVE);
    WorldPackets::Movement::WriteMovementUpdate(data, movementInfo);
    mover->SendMessageToSet(&data, _player);
}

void WorldSession::HandleRequestPetInfo(WorldPackets::Pet::RequestPetInfo& /*packet*/)
{
    /*
        LOG_DEBUG("network.opcode", "WORLD: CMSG_REQUEST_PET_INFO");
        recv_data.hexlike();
    */

    if (_player->GetPet())
        _player->PetSpellInitialize();
    else if (Unit* charm = _player->GetCharm())
    {
        if (charm->HasUnitState(UNIT_STATE_POSSESSED))
            _player->PossessSpellInitialize();
        else if (charm->HasUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED) && charm->HasUnitFlag(UNIT_FLAG_POSSESSED))
            _player->VehicleSpellInitialize();
        else
            _player->CharmSpellInitialize();
    }
}

void WorldSession::HandleSetTaxiBenchmarkOpcode(WorldPacket& recv_data)
{
    uint8 mode;
    recv_data >> mode;

    mode ? _player->SetPlayerFlag(PLAYER_FLAGS_TAXI_BENCHMARK) : _player->RemovePlayerFlag(PLAYER_FLAGS_TAXI_BENCHMARK);

    LOG_DEBUG("network", "Client used \"/timetest {}\" command", mode);
}

void WorldSession::HandleQueryInspectAchievements(WorldPacket& recv_data)
{
    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recv_data.ReadGuidMask(guid, { 2, 7, 1, 5, 4, 0, 3, 6 });
    recv_data.ReadGuidBytes(guid, { 7, 2, 0, 4, 1, 5, 6, 3 });

    Player* player = ObjectAccessor::GetPlayer(*_player, guid);
    if (!player)
    {
        return;
    }

    if (!GetPlayer()->IsWithinDistInMap(player, INSPECT_DISTANCE, false))
    {
        return;
    }

    if (GetPlayer()->IsValidAttackTarget(player))
    {
        return;
    }

    player->SendRespondInspectAchievements(_player);
}

void WorldSession::HandleWorldStateUITimerUpdate(WorldPacket& /*recv_data*/)
{
    // empty opcode
    LOG_DEBUG("network", "WORLD: CMSG_WORLD_STATE_UI_TIMER_UPDATE");

    WorldPackets::Misc::UITime response;
    response.Time = GameTime::GetGameTime().count();
    SendPacket(response.Write());
}

void WorldSession::HandleReadyForAccountDataTimes(WorldPacket& /*recv_data*/)
{
    // empty opcode
    LOG_DEBUG("network", "WORLD: CMSG_READY_FOR_ACCOUNT_DATA_TIMES");

    SendAccountDataTimes(GLOBAL_CACHE_MASK);
}

//Battlefield and Battleground
void WorldSession::HandleAreaSpiritHealerQueryOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "WORLD: CMSG_AREA_SPIRIT_HEALER_QUERY");

    Battleground* bg = _player->GetBattleground();

    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recv_data.ReadGuidMask(guid, { 5, 6, 0, 4, 1, 2, 7, 3 });
    recv_data.ReadGuidBytes(guid, { 0, 2, 6, 7, 1, 5, 3, 4 });

    Creature* unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if (!unit->IsSpiritService())                            // it's not spirit service
        return;

    if (bg)
        sBattlegroundMgr->SendAreaSpiritHealerQueryOpcode(_player, bg, guid);

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(_player->GetZoneId()))
        bf->SendAreaSpiritHealerQueryOpcode(_player, guid);
}

void WorldSession::HandleAreaSpiritHealerQueueOpcode(WorldPacket& recv_data)
{
    LOG_DEBUG("network", "WORLD: CMSG_AREA_SPIRIT_HEALER_QUEUE");

    Battleground* bg = _player->GetBattleground();

    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recv_data.ReadGuidMask(guid, { 5, 4, 0, 2, 7, 1, 6, 3 });
    recv_data.ReadGuidBytes(guid, { 1, 7, 6, 2, 4, 3, 0, 5 });

    Creature* unit = GetPlayer()->GetMap()->GetCreature(guid);
    if (!unit)
        return;

    if (!unit->IsSpiritService())                            // it's not spirit service
        return;

    if (bg)
        bg->AddPlayerToResurrectQueue(guid, _player->GetGUID());

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(_player->GetZoneId()))
        bf->AddPlayerToResurrectQueue(guid, _player->GetGUID());
}

void WorldSession::HandleHearthAndResurrect(WorldPacket& /*recv_data*/)
{
    if (_player->IsInFlight())
        return;

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(_player->GetZoneId()))
    {
        bf->PlayerAskToLeave(_player);
        return;
    }

    AreaTableEntry const* atEntry = sAreaTableStore.LookupEntry(_player->GetAreaId());
    if (!atEntry || !(atEntry->flags & AREA_FLAG_WINTERGRASP_2))
        return;

    _player->BuildPlayerRepop();
    _player->ResurrectPlayer(1.0f);
    _player->SpawnCorpseBones();
    _player->TeleportTo(_player->m_homebindMapId, _player->m_homebindX, _player->m_homebindY, _player->m_homebindZ, _player->GetOrientation());
}

void WorldSession::HandleInstanceLockResponse(WorldPackets::Instance::InstanceLockResponse& packet)
{
    if (!_player->HasPendingBind() || _player->GetPendingBind() != _player->GetInstanceId() || (_player->GetGroup() && _player->GetGroup()->isLFGGroup() && _player->GetGroup()->IsLfgRandomInstance()))
    {
        LOG_DEBUG("network.opcode", "InstanceLockResponse: Player {} ({}) tried to bind himself/teleport to graveyard without a pending bind!",
            _player->GetName(), _player->GetGUID().ToString());
        return;
    }

    if (packet.Accept)
        _player->BindToInstance();
    else
        _player->RepopAtGraveyard();

    _player->SetPendingBind(0, 0);
}

void WorldSession::HandleUpdateMissileTrajectory(WorldPacket& recvPacket)
{
    LOG_DEBUG("network", "WORLD: CMSG_UPDATE_MISSILE_TRAJECTORY");

    ObjectGuid guid;
    uint32 spellId;
    uint32 moveMsgId;
    float pitch, speed;
    Position firePos, impactPos;

    recvPacket >> firePos.m_positionZ;
    recvPacket >> firePos.m_positionX;
    recvPacket >> pitch;
    recvPacket >> impactPos.m_positionX;
    recvPacket >> firePos.m_positionY;
    recvPacket >> speed;
    recvPacket >> impactPos.m_positionY;
    recvPacket >> moveMsgId;
    recvPacket >> spellId;
    recvPacket >> impactPos.m_positionZ;

    guid[5] = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[3] = recvPacket.ReadBit();
    guid[2] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();

    // The caster's movement status follows. Its layout only exists here, so it is read inline
    // and, as in TrinityCore, dropped afterwards.
    bool const hasStatus = recvPacket.ReadBit();
    MovementInfo status;
    bool hasFallData = false;
    bool hasFallVelocity = false;
    bool hasFacing = false;
    bool hasTransportData = false;
    bool hasTransportTime2 = false;
    bool hasVehicleId = false;
    bool hasSplineElevation = false;
    bool hasPitch = false;
    bool hasMoveTime = false;

    if (hasStatus)
    {
        status.guid[2] = recvPacket.ReadBit();
        recvPacket.ReadBit();                               // heightChangeFailed
        bool const hasMovementFlags = !recvPacket.ReadBit();
        recvPacket.ReadBit();                               // hasSpline
        status.guid[4] = recvPacket.ReadBit();

        if (hasMovementFlags)
            recvPacket.ReadBits(30);

        status.guid[3] = recvPacket.ReadBit();
        hasPitch = recvPacket.ReadBit();
        bool const hasMovementFlags2 = recvPacket.ReadBit();
        hasTransportData = recvPacket.ReadBit();
        status.guid[7] = recvPacket.ReadBit();

        if (hasTransportData)
        {
            status.transport.guid[0] = recvPacket.ReadBit();
            status.transport.guid[7] = recvPacket.ReadBit();
            status.transport.guid[2] = recvPacket.ReadBit();
            status.transport.guid[4] = recvPacket.ReadBit();
            hasVehicleId = recvPacket.ReadBit();
            status.transport.guid[5] = recvPacket.ReadBit();
            status.transport.guid[3] = recvPacket.ReadBit();
            status.transport.guid[6] = recvPacket.ReadBit();
            hasTransportTime2 = recvPacket.ReadBit();
            status.transport.guid[1] = recvPacket.ReadBit();
        }

        status.guid[6] = recvPacket.ReadBit();
        hasFacing = !recvPacket.ReadBit();
        hasMoveTime = !recvPacket.ReadBit();
        status.guid[5] = recvPacket.ReadBit();
        hasSplineElevation = !recvPacket.ReadBit();
        status.guid[1] = recvPacket.ReadBit();
        hasFallData = recvPacket.ReadBit();
        status.guid[0] = recvPacket.ReadBit();

        if (hasFallData)
            hasFallVelocity = recvPacket.ReadBit();

        if (hasMovementFlags2)
            recvPacket.ReadBits(12);
    }

    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[0]);
    recvPacket.ReadByteSeq(guid[5]);

    if (hasStatus)
    {
        if (hasFallData)
        {
            if (hasFallVelocity)
                recvPacket.read_skip(3 * sizeof(float));           // cos, sin, horizontal speed

            recvPacket.read_skip<float>();                  // vertical speed
            recvPacket.read_skip<uint32>();                 // fall time
        }

        if (hasFacing)
            recvPacket.read_skip<float>();

        if (hasTransportData)
        {
            recvPacket.read_skip<float>();                  // transport z
            recvPacket.ReadByteSeq(status.transport.guid[3]);
            recvPacket.ReadByteSeq(status.transport.guid[7]);
            recvPacket.read_skip(2 * sizeof(float));               // transport x, orientation

            if (hasTransportTime2)
                recvPacket.read_skip<uint32>();

            recvPacket.ReadByteSeq(status.transport.guid[2]);
            recvPacket.read_skip<uint32>();                 // transport time
            recvPacket.ReadByteSeq(status.transport.guid[0]);
            recvPacket.read_skip<float>();                  // transport y

            if (hasVehicleId)
                recvPacket.read_skip<uint32>();

            recvPacket.ReadByteSeq(status.transport.guid[4]);
            recvPacket.ReadByteSeq(status.transport.guid[6]);
            recvPacket.ReadByteSeq(status.transport.guid[1]);
            recvPacket.read_skip<int8>();                   // seat
            recvPacket.ReadByteSeq(status.transport.guid[5]);
        }

        if (hasSplineElevation)
            recvPacket.read_skip<float>();

        recvPacket.ReadByteSeq(status.guid[4]);

        if (hasPitch)
            recvPacket.read_skip<float>();

        recvPacket.ReadByteSeq(status.guid[7]);
        recvPacket.ReadByteSeq(status.guid[2]);

        if (hasMoveTime)
            recvPacket.read_skip<uint32>();

        recvPacket.read_skip<float>();                      // x
        recvPacket.ReadByteSeq(status.guid[5]);
        recvPacket.ReadByteSeq(status.guid[3]);
        recvPacket.ReadByteSeq(status.guid[1]);
        recvPacket.ReadByteSeq(status.guid[0]);
        recvPacket.read_skip(2 * sizeof(float));                   // y, z
        recvPacket.ReadByteSeq(status.guid[6]);
    }

    Unit* caster = ObjectAccessor::GetUnit(*_player, guid);
    Spell* spell = caster ? caster->GetCurrentSpell(CURRENT_GENERIC_SPELL) : nullptr;
    if (!spell || spell->m_spellInfo->Id != spellId || !spell->m_targets.HasDst() || !spell->m_targets.HasSrc())
        return;

    Position pos = *spell->m_targets.GetSrcPos();
    pos.Relocate(firePos.GetPositionX(), firePos.GetPositionY(), firePos.GetPositionZ());
    spell->m_targets.ModSrc(pos);

    pos = *spell->m_targets.GetDstPos();
    pos.Relocate(impactPos.GetPositionX(), impactPos.GetPositionY(), impactPos.GetPositionZ());
    spell->m_targets.ModDst(pos);

    spell->m_targets.SetElevation(pitch);
    spell->m_targets.SetSpeed(speed);
}

void WorldSession::HandleRequestResearchHistory(WorldPackets::Archaeology::RequestResearchHistory& /*packet*/)
{
    _player->NotifyRequestResearchHistory();
}

void WorldSession::HandleSaveCUFProfiles(WorldPacket& recvData)
{
    uint8 count = uint8(recvData.ReadBits(20));
    if (count > MAX_CUF_PROFILES)
    {
        LOG_ERROR("entities.player", "HandleSaveCUFProfiles - {} tried to save more than {} CUF profiles. Hacking attempt?", GetPlayerInfo(), MAX_CUF_PROFILES);
        recvData.rfinish();
        return;
    }

    std::array<std::unique_ptr<CUFProfile>, MAX_CUF_PROFILES> profiles;
    std::array<uint8, MAX_CUF_PROFILES> nameLengths = { };

    for (uint8 i = 0; i < count; ++i)
    {
        std::unique_ptr<CUFProfile>& profile = profiles[i];
        profile = std::make_unique<CUFProfile>();
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_SPEC_2, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_10_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_UNK_157, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_HEAL_PREDICTION, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_SPEC_1, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_PVP, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_POWER_BAR, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_15_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_40_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_PETS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_5_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_ONLY_DISPELLABLE_DEBUFFS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_2_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_UNK_156, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_NON_BOSS_DEBUFFS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_MAIN_TANK_AND_ASSIST, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_AGGRO_HIGHLIGHT, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_3_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_BORDER, recvData.ReadBit());
        profile->BoolOptions.set(CUF_USE_CLASS_COLORS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_UNK_145, recvData.ReadBit());
        nameLengths[i] = uint8(recvData.ReadBits(8));
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_PVE, recvData.ReadBit());
        profile->BoolOptions.set(CUF_DISPLAY_HORIZONTAL_GROUPS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_AUTO_ACTIVATE_25_PLAYERS, recvData.ReadBit());
        profile->BoolOptions.set(CUF_KEEP_GROUPS_TOGETHER, recvData.ReadBit());
    }

    for (uint8 i = 0; i < count; ++i)
    {
        std::unique_ptr<CUFProfile>& profile = profiles[i];
        recvData >> profile->TopPoint;
        profile->ProfileName = recvData.ReadString(nameLengths[i]);
        recvData >> profile->BottomOffset;
        recvData >> profile->FrameHeight;
        recvData >> profile->FrameWidth;
        recvData >> profile->TopOffset;
        recvData >> profile->HealthText;
        recvData >> profile->BottomPoint;
        recvData >> profile->SortBy;
        recvData >> profile->LeftOffset;
        recvData >> profile->LeftPoint;
    }

    // the client sends every profile it keeps; the rest were deleted
    for (uint8 i = 0; i < MAX_CUF_PROFILES; ++i)
        GetPlayer()->SaveCUFProfile(i, std::move(profiles[i]));
}
