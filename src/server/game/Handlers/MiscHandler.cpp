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
#include "BattlegroundPackets.h"
#include "ArchaeologyPackets.h"
#include "ClientConfigPackets.h"
#include "GameGraveyard.h"
#include "MiscPackets.h"
#include "SpellPackets.h"
#include "WhoPackets.h"
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

void WorldSession::HandleRepopRequest(WorldPackets::Misc::RepopRequest& /*packet*/)
{

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

void WorldSession::HandleViolenceLevel(WorldPackets::Misc::ViolenceLevel& /*violenceLevel*/)
{
    // do something?
}

void WorldSession::HandleWhoOpcode(WorldPackets::Who::WhoRequestPkt& whoRequest)
{
    uint32 matchCount = 0;
    WorldPackets::Who::WhoRequest& request = whoRequest.Request;

    if (whoRequest.Areas.size() > 10)                       // can't be received from real client or broken packet
        return;

    if (request.Words.size() > 4)                           // can't be received from real client or broken packet
        return;

    uint32 levelMin = uint32(std::max(request.MinLevel, 0));
    uint32 levelMax = uint32(std::max(request.MaxLevel, 0));
    uint32 racemask = uint32(request.RaceFilter);
    uint32 classmask = uint32(request.ClassFilter);
    uint32 zonesCount = uint32(whoRequest.Areas.size());
    uint32 strCount = uint32(request.Words.size());
    std::string const& packetPlayerName = request.Name;
    std::string const& packetGuildName = request.Guild;

    std::array<uint32, 10> zoneids = {};
    for (uint32 i = 0; i < zonesCount; ++i)
        zoneids[i] = uint32(whoRequest.Areas[i]);

    std::wstring str[4];
    for (uint32 i = 0; i < strCount; ++i)
    {
        if (!Utf8toWStr(request.Words[i].Word, str[i]))
            continue;

        wstrToLower(str[i]);
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

    WorldPackets::Who::WhoResponsePkt response;

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
            aname = areaEntry->AreaName_lang;
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

        WorldPackets::Who::WhoEntry& whoEntry = response.Response.Entries.emplace_back();
        whoEntry.PlayerData.GuidActual = target.GetGuid();
        whoEntry.PlayerData.AccountID = ObjectGuid::Create<HighGuid::WowAccount>(target.GetAccountId());
        if (Player* targetPlayer = ObjectAccessor::FindConnectedPlayer(target.GetGuid()))
            whoEntry.PlayerData.BnetAccountID = targetPlayer->GetSession()->GetBattlenetAccountGUID();
        whoEntry.PlayerData.Name = target.GetPlayerName();
        whoEntry.PlayerData.VirtualRealmAddress = GetVirtualRealmAddress();
        whoEntry.PlayerData.Race = uint8(race);
        whoEntry.PlayerData.Sex = gender;
        whoEntry.PlayerData.ClassID = class_;
        whoEntry.PlayerData.Level = lvl;

        if (target.GetGuildId())
        {
            whoEntry.GuildGUID = ObjectGuid::Create<HighGuid::Guild>(target.GetGuildId());
            whoEntry.GuildVirtualRealmAddress = GetVirtualRealmAddress();
            whoEntry.GuildName = target.GetGuildName();
        }

        whoEntry.AreaID = int32(playerZoneId);
        whoEntry.IsGM = target.GetSecurity() > SEC_PLAYER;
    }

    SendPacket(response.Write());

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

void WorldSession::HandleTogglePvP(WorldPackets::Misc::TogglePvP& /*packet*/)
{
    // 6.x: CMSG_TOGGLE_PVP toggles, CMSG_SET_PVP carries the new state
    GetPlayer()->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);

    if (GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(true, true);
    else if (!GetPlayer()->pvpInfo.IsHostile && GetPlayer()->IsPvP()) // pussywizard: in pvp mode, but doesn't want to be and not in hostile territory, so start timer
        GetPlayer()->UpdatePvP(true, false);

    //if (OutdoorPvP* pvp = _player->GetOutdoorPvP())
    //    pvp->HandlePlayerActivityChanged(_player);
}

void WorldSession::HandleSetPvP(WorldPackets::Misc::SetPvP& packet)
{
    bool newPvPStatus = packet.EnablePVP;
    GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP, newPvPStatus);

    if (GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(true, true);
    else if (!GetPlayer()->pvpInfo.IsHostile && GetPlayer()->IsPvP())
        GetPlayer()->UpdatePvP(true, false);
}

void WorldSession::HandlePortGraveyard(WorldPackets::Misc::PortGraveyard& /*packet*/)
{
    if (GetPlayer()->IsAlive() || !GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_GHOST))
        return;

    GetPlayer()->RepopAtGraveyard();
}

void WorldSession::HandleRequestCemeteryList(WorldPackets::Misc::RequestCemeteryList& /*packet*/)
{
    uint32 zoneId = _player->GetZoneId();
    TeamId teamId = _player->GetTeamId();

    WorldPackets::Misc::RequestCemeteryListResponse packet;
    packet.IsGossipTriggered = false;

    GraveyardMapBounds range = sGraveyard->GetGraveyardLinks(zoneId);
    for (auto it = range.first; it != range.second && packet.CemeteryID.size() < 16; ++it) // client max
        if (it->second.IsNeutralOrFriendlyToTeam(teamId))
            packet.CemeteryID.push_back(it->second.safeLocId);

    if (packet.CemeteryID.empty())
    {
        LOG_DEBUG("network", "No graveyards found for zone {} for {} (team {}) in CMSG_REQUEST_CEMETERY_LIST", zoneId, _player->GetGUID().ToString(), teamId);
        return;
    }

    SendPacket(packet.Write());
}

void WorldSession::HandleSetSelectionOpcode(WorldPackets::Misc::SetSelection& packet)
{
    ObjectGuid guid = packet.Selection;

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

void WorldSession::HandleStandStateChangeOpcode(WorldPackets::Misc::StandStateChange& packet)
{
    uint32 animstate = packet.StandState;

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

void WorldSession::HandleReclaimCorpse(WorldPackets::Misc::ReclaimCorpse& /*packet*/)
{
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

void WorldSession::HandleResurrectResponse(WorldPackets::Misc::ResurrectResponse& packet)
{
    ObjectGuid guid = packet.Resurrecter;
    uint32 status = packet.Response;

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

void WorldSession::HandleAreaTriggerOpcode(WorldPackets::Misc::AreaTrigger& packet)
{
    uint32 triggerId = uint32(packet.AreaTriggerID);

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

void WorldSession::HandleUpdateAccountData(WorldPackets::ClientConfig::UserClientUpdateAccountData& packet)
{
    LOG_DEBUG("network", "UAD: type {}, time {}, decompressedSize {}", packet.DataType, packet.Time, packet.Size);

    if (packet.DataType >= NUM_ACCOUNT_DATA_TYPES)
        return;

    if (packet.Size == 0)                                   // erase
    {
        SetAccountData(AccountDataType(packet.DataType), 0, "");
        return;
    }

    if (packet.Size > 0xFFFF)
    {
        LOG_ERROR("network.opcode", "UAD: Account data packet too big, size {}", packet.Size);
        return;
    }

    ByteBuffer dest;
    dest.resize(packet.Size);

    uLongf realSize = packet.Size;
    if (uncompress(dest.contents(), &realSize, packet.CompressedData.contents(), packet.CompressedData.size()) != Z_OK)
    {
        LOG_ERROR("network.opcode", "UAD: Failed to decompress account data");
        return;
    }

    std::string adata = dest.ReadString(realSize);
    SetAccountData(AccountDataType(packet.DataType), packet.Time, adata);
}

void WorldSession::HandleRequestAccountData(WorldPackets::ClientConfig::RequestAccountData& request)
{
    LOG_DEBUG("network", "RAD: type {}", request.DataType);

    if (request.DataType >= NUM_ACCOUNT_DATA_TYPES)
        return;

    AccountData const* adata = GetAccountData(AccountDataType(request.DataType));

    WorldPackets::ClientConfig::UpdateAccountData data;
    data.Player = _player ? _player->GetGUID() : ObjectGuid::Empty;
    data.Time = uint32(adata->Time);
    data.Size = uint32(adata->Data.size());
    data.DataType = request.DataType;

    uLongf destSize = compressBound(data.Size);
    data.CompressedData.resize(destSize);

    if (data.Size && compress(data.CompressedData.contents(), &destSize, (uint8 const*)adata->Data.c_str(), data.Size) != Z_OK)
    {
        LOG_DEBUG("network", "RAD: Failed to compress account data");
        return;
    }

    data.CompressedData.resize(destSize);
    SendPacket(data.Write());
}

void WorldSession::HandleSetActionButtonOpcode(WorldPackets::Spells::SetActionButton& packet)
{
    // 6.x: the action in the low four bytes, the type in the high byte
    uint8 button = packet.Index;
    uint64 packetData = packet.Action;
    uint32 action = uint32(packetData & 0xFFFFFFFF);
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

void WorldSession::HandleCompleteCinematic(WorldPackets::Misc::CompleteCinematic& /*packet*/)
{
    // End the current cinematic and restore the normal player view
    GetPlayer()->GetCinematicMgr().EndCinematic();
}

void WorldSession::HandleCompleteMovie(WorldPackets::Misc::CompleteMovie& /*packet*/)
{
    if (!GetPlayer()->GetMovie())
        return;

    GetPlayer()->SetMovie(0);
}

void WorldSession::HandleNextCinematicCamera(WorldPackets::Misc::NextCinematicCamera& /*packet*/)
{
    // Sent by client when cinematic actually begun. So we begin the server side process
    GetPlayer()->GetCinematicMgr().StartCinematicCamera();
}

void WorldSession::HandleSetActionBarToggles(WorldPackets::Character::SetActionBarToggles& packet)
{
    uint8 ActionBar = packet.Mask;

    if (!GetPlayer())                                        // ignore until not logged (check needed because STATUS_AUTHED)
    {
        if (ActionBar != 0)
            LOG_ERROR("network.opcode", "WorldSession::HandleSetActionBarToggles in not logged state with value: {}, ignored", uint32(ActionBar));
        return;
    }

    GetPlayer()->SetByteValue(PLAYER_FIELD_BYTES, 2, ActionBar);
}

void WorldSession::HandlePlayedTime(WorldPackets::Character::RequestPlayedTime& packet)
{
    WorldPackets::Character::PlayedTime playedTime;
    playedTime.TotalTime = _player->GetTotalPlayedTime();
    playedTime.LevelTime = _player->GetLevelPlayedTime();
    playedTime.TriggerEvent = packet.TriggerScriptEvent;    // 0-1 - will not show in chat frame
    SendPacket(playedTime.Write());
}

void WorldSession::HandleSelectFactionOpcode(WorldPackets::Misc::NeutralPlayerSelectFaction& packet)
{
    // 0 = Horde, 1 = Alliance
    uint32 faction = packet.Faction;

    if (faction > 1 || !_player->JoinPandarenFaction(faction == 1))
        return;

    // "The Healing of Shen-zin Su" ends with the faction choice
    if (_player->GetQuestStatus(31450) == QUEST_STATUS_INCOMPLETE)
        _player->KilledMonsterCredit(64594);

    _player->SendMovieStart(116);
}

void WorldSession::HandleWorldTeleportOpcode(WorldPackets::Misc::WorldTeleport& worldTeleport)
{
    uint32 mapid = worldTeleport.MapID;
    float PositionX = worldTeleport.Pos.x;
    float PositionY = worldTeleport.Pos.y;
    float PositionZ = worldTeleport.Pos.z;
    float Orientation = worldTeleport.Facing;

    if (GetPlayer()->IsInFlight())
    {
        LOG_DEBUG("network", "Player '{}' ({}) in flight, ignore worldport command.", GetPlayer()->GetName(), GetPlayer()->GetGUID().ToString());
        return;
    }

    LOG_DEBUG("network", "CMSG_WORLD_TELEPORT: Player = {}, map = {}, x = {}, y = {}, z = {}, o = {}", GetPlayer()->GetName(), mapid, PositionX, PositionY, PositionZ, Orientation);

    if (HasPermission(rbac::RBAC_PERM_OPCODE_WORLD_TELEPORT))
        GetPlayer()->TeleportTo(mapid, PositionX, PositionY, PositionZ, Orientation);
    else
        ChatHandler(this).SendNotification(LANG_PERMISSION_DENIED);
}

void WorldSession::HandleWhoIsOpcode(WorldPackets::Who::WhoIsRequest& packet)
{
    std::string charname = packet.CharName;

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

    WorldPackets::Who::WhoIsResponse response;
    response.AccountName = msg;
    SendPacket(response.Write());

    LOG_DEBUG("network", "Received whois command from player {} for character {}", GetPlayer()->GetName(), charname);
}

void WorldSession::HandleFarSightOpcode(WorldPackets::Misc::FarSight& packet)
{
    bool apply = packet.Enable;

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

void WorldSession::HandleSetTitleOpcode(WorldPackets::Character::SetTitle& packet)
{
    int32 title = packet.TitleID;

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

void WorldSession::HandleSetDungeonDifficultyOpcode(WorldPackets::Misc::SetDungeonDifficulty& setDungeonDifficulty)
{
    DifficultyEntry const* difficultyEntry = sDifficultyStore.LookupEntry(setDungeonDifficulty.DifficultyID);
    if (!difficultyEntry)
    {
        LOG_DEBUG("network", "WorldSession::HandleSetDungeonDifficultyOpcode: {} sent an invalid instance mode {}!", _player->GetGUID().ToString(), setDungeonDifficulty.DifficultyID);
        return;
    }

    if (difficultyEntry->InstanceType != MAP_INSTANCE)
    {
        LOG_DEBUG("network", "WorldSession::HandleSetDungeonDifficultyOpcode: {} sent an non-dungeon instance mode {}!", _player->GetGUID().ToString(), difficultyEntry->ID);
        return;
    }

    if (!(difficultyEntry->Flags & DIFFICULTY_FLAG_CAN_SELECT))
    {
        LOG_DEBUG("network", "WorldSession::HandleSetDungeonDifficultyOpcode: {} sent unselectable instance mode {}!", _player->GetGUID().ToString(), difficultyEntry->ID);
        return;
    }

    Difficulty difficultyID = Difficulty(difficultyEntry->ID);
    if (difficultyID == _player->GetDungeonDifficulty())
        return;

    // cannot reset while in an instance
    Map* map = _player->FindMap();
    if (map && map->IsDungeon())
    {
        _player->SendDungeonDifficulty(_player->GetGroup() != nullptr);
        return;
    }

    if (Group* group = _player->GetGroup())
    {
        if (!group->IsLeader(_player->GetGUID()))
            return;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupGuy = itr->GetSource();
            if (!groupGuy)
                continue;

            if (!groupGuy->IsInWorld() || groupGuy->GetMap()->IsNonRaidDungeon())
            {
                _player->SendDungeonDifficulty(true);
                return;
            }
        }

        // the difficulty is set even if the instances can't be reset
        group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false, _player);
        group->SetDungeonDifficulty(difficultyID);
    }
    else
    {
        Player::ResetInstances(_player->GetGUID(), INSTANCE_RESET_CHANGE_DIFFICULTY, false);
        _player->SetDungeonDifficulty(difficultyID);
        _player->SendDungeonDifficulty(false);
    }
}

void WorldSession::HandleSetRaidDifficultyOpcode(WorldPackets::Misc::SetRaidDifficulty& setRaidDifficulty)
{
    DifficultyEntry const* difficultyEntry = sDifficultyStore.LookupEntry(setRaidDifficulty.DifficultyID);
    if (!difficultyEntry)
    {
        LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: {} sent an invalid instance mode {}!", _player->GetGUID().ToString(), setRaidDifficulty.DifficultyID);
        return;
    }

    if (difficultyEntry->InstanceType != MAP_RAID)
    {
        LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: {} sent an non-raid instance mode {}!", _player->GetGUID().ToString(), difficultyEntry->ID);
        return;
    }

    if (!(difficultyEntry->Flags & DIFFICULTY_FLAG_CAN_SELECT))
    {
        LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: {} sent unselectable instance mode {}!", _player->GetGUID().ToString(), difficultyEntry->ID);
        return;
    }

    bool legacy = setRaidDifficulty.Legacy != 0;
    if (((difficultyEntry->Flags & DIFFICULTY_FLAG_LEGACY) != 0) != legacy)
    {
        LOG_DEBUG("network", "WorldSession::HandleSetRaidDifficultyOpcode: {} sent not matching legacy difficulty {}!", _player->GetGUID().ToString(), difficultyEntry->ID);
        return;
    }

    Difficulty difficultyID = Difficulty(difficultyEntry->ID);
    if (difficultyID == (legacy ? _player->GetLegacyRaidDifficulty() : _player->GetRaidDifficulty()))
        return;

    // cannot reset while in an instance
    Map* map = _player->FindMap();
    if (map && map->IsDungeon())
    {
        _player->SendRaidDifficulty(_player->GetGroup() != nullptr);
        return;
    }

    if (Group* group = _player->GetGroup())
    {
        if (!group->IsLeader(_player->GetGUID()))
            return;

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

            _player->SendRaidDifficulty(true);
            return;
        }

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupGuy = itr->GetSource();
            if (!groupGuy)
                continue;

            if (!groupGuy->IsInWorld() || groupGuy->GetMap()->IsRaid())
            {
                _player->SendRaidDifficulty(true);
                return;
            }
        }

        // the difficulty is set even if the instances can't be reset
        group->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, _player);
        if (legacy)
            group->SetLegacyRaidDifficulty(difficultyID);
        else
            group->SetRaidDifficulty(difficultyID);

        group->SetDifficultyChangePrevention(DIFFICULTY_PREVENTION_CHANGE_RECENTLY_CHANGED);
    }
    else
    {
        Player::ResetInstances(_player->GetGUID(), INSTANCE_RESET_CHANGE_DIFFICULTY, true);
        if (legacy)
            _player->SetLegacyRaidDifficulty(difficultyID);
        else
            _player->SetRaidDifficulty(difficultyID);

        _player->SendRaidDifficulty(false);
    }
}

void WorldSession::HandleCancelMountAuraOpcode(WorldPackets::Spells::CancelMountAura& /*cancelMountAura*/)
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

void WorldSession::HandleSetTaxiBenchmark(WorldPackets::Misc::SetTaxiBenchmarkMode& packet)
{
    uint8 mode = packet.Enable ? 1 : 0;

    mode ? _player->SetPlayerFlag(PLAYER_FLAGS_TAXI_BENCHMARK) : _player->RemovePlayerFlag(PLAYER_FLAGS_TAXI_BENCHMARK);

    LOG_DEBUG("network", "Client used \"/timetest {}\" command", mode);
}

void WorldSession::HandleUITimeRequest(WorldPackets::Misc::UITimeRequest& /*request*/)
{
    WorldPackets::Misc::UITime response;
    response.Time = GameTime::GetGameTime().count();
    SendPacket(response.Write());
}

//Battlefield and Battleground
void WorldSession::HandleAreaSpiritHealerQueryOpcode(WorldPackets::Battleground::AreaSpiritHealerQuery& areaSpiritHealerQuery)
{
    Battleground* bg = _player->GetBattleground();
    ObjectGuid guid = areaSpiritHealerQuery.HealerGuid;

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

void WorldSession::HandleAreaSpiritHealerQueueOpcode(WorldPackets::Battleground::AreaSpiritHealerQueue& areaSpiritHealerQueue)
{
    Battleground* bg = _player->GetBattleground();
    ObjectGuid guid = areaSpiritHealerQueue.HealerGuid;

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

void WorldSession::HandleHearthAndResurrect(WorldPackets::Battleground::HearthAndResurrect& /*hearthAndResurrect*/)
{
    if (_player->IsInFlight())
        return;

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(_player->GetZoneId()))
    {
        bf->PlayerAskToLeave(_player);
        return;
    }

    AreaTableEntry const* atEntry = sAreaTableStore.LookupEntry(_player->GetAreaId());
    if (!atEntry || !(atEntry->Flags[0] & AREA_FLAG_CAN_HEARTH_AND_RESURRECT))
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

    if (packet.AcceptLock)
        _player->BindToInstance();
    else
        _player->RepopAtGraveyard();

    _player->SetPendingBind(0, 0);
}

void WorldSession::HandleUpdateMissileTrajectory(WorldPackets::Spells::UpdateMissileTrajectory& packet)
{
    Unit* caster = ObjectAccessor::GetUnit(*_player, packet.Guid);
    Spell* spell = caster ? caster->GetCurrentSpell(CURRENT_GENERIC_SPELL) : nullptr;
    if (!spell || spell->m_spellInfo->Id != uint32(packet.SpellID) || !spell->m_targets.HasDst() || !spell->m_targets.HasSrc())
        return;

    Position pos = *spell->m_targets.GetSrcPos();
    pos.Relocate(packet.FirePos.x, packet.FirePos.y, packet.FirePos.z);
    spell->m_targets.ModSrc(pos);

    pos = *spell->m_targets.GetDstPos();
    pos.Relocate(packet.ImpactPos.x, packet.ImpactPos.y, packet.ImpactPos.z);
    spell->m_targets.ModDst(pos);

    spell->m_targets.SetElevation(packet.Pitch);
    spell->m_targets.SetSpeed(packet.Speed);

    if (packet.Status)
    {
        // TC 6.x relays the status as a movement update and drops it otherwise; keep it as the mover's latest movement info
        LOG_DEBUG("network", "CMSG_UPDATE_MISSILE_TRAJECTORY: movement status from {} ignored", packet.Guid.ToString());
    }
}

void WorldSession::HandleRequestResearchHistory(WorldPackets::Archaeology::RequestResearchHistory& /*packet*/)
{
    _player->NotifyRequestResearchHistory();
}

void WorldSession::HandleSaveCUFProfiles(WorldPackets::Misc::SaveCUFProfiles& packet)
{
    if (packet.CUFProfiles.size() > MAX_CUF_PROFILES)
    {
        LOG_ERROR("entities.player", "HandleSaveCUFProfiles - {} tried to save more than {} CUF profiles. Hacking attempt?", GetPlayerInfo(), MAX_CUF_PROFILES);
        return;
    }

    // the client sends every profile it keeps; the rest were deleted
    for (uint8 i = 0; i < MAX_CUF_PROFILES; ++i)
        GetPlayer()->SaveCUFProfile(i, i < packet.CUFProfiles.size() ? std::move(packet.CUFProfiles[i]) : nullptr);
}

void WorldSession::HandleObjectUpdateRescuedOpcode(WorldPackets::Misc::ObjectUpdateRescued& objectUpdateRescued)
{
    LOG_ERROR("network", "Object update rescued for {} for player {}", objectUpdateRescued.ObjectGUID.ToString(), GetPlayerInfo());

    // Client received values update after destroying object; AC's visibility container sends the
    // DestroyObject again on the next visibility update of that object
}

void WorldSession::HandleSetAdvancedCombatLogging(WorldPackets::ClientConfig::SetAdvancedCombatLogging& setAdvancedCombatLogging)
{
    _player->SetAdvancedCombatLogging(setAdvancedCombatLogging.Enable);
}
