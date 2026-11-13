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

#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "GameTime.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "PetitionMgr.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::HandlePetitionBuyOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode CMSG_PETITION_BUY");

    ObjectGuid guidNPC;
    uint32 clientIndex;                                     // 1 for guild and arenaslot+1 for arenas in client
    std::string name;

    // 5.4.8 18414, from Project SkyFire: guild charters only, there is no arena slot index
    recvData.ReadGuidMask(guidNPC, { 5, 2, 3 });
    uint32 nameLength = recvData.ReadBits(7);
    recvData.ReadGuidMask(guidNPC, { 4, 1, 7, 0, 6 });
    name = recvData.ReadString(nameLength);
    recvData.ReadGuidBytes(guidNPC, { 1, 7, 4, 6, 0, 5, 2, 3 });
    clientIndex = 1;

    LOG_DEBUG("network", "Petitioner ({}) tried sell petition: name {}", guidNPC.ToString(), name);

    // prevent cheating
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guidNPC, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandlePetitionBuyOpcode - Unit ({}) not found or you can't interact with him.", guidNPC.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    uint32 charterid = 0;
    uint32 cost = 0;
    uint32 type = 0;
    if (creature->IsTabardDesigner())
    {
        // if tabard designer, then trying to buy a guild charter.
        // do not let if already in guild.
        if (_player->GetGuildId())
            return;

        if (sWorld->getBoolConfig(CONFIG_TRIAL_RESTRICTION_GUILD) && IsTrialAccount())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
            return;
        }

        charterid = GUILD_CHARTER;
        cost = sWorld->getIntConfig(CONFIG_CHARTER_COST_GUILD);
        type = GUILD_CHARTER_TYPE;
    }
    else
    {
        /// @todo: find correct opcode
        if (_player->GetLevel() < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, "", _player->GetName(), ERR_ARENA_TEAM_TARGET_TOO_LOW_S);
            return;
        }

        switch (clientIndex)                                 // arenaSlot+1 as received from client (1 from 3 case)
        {
            case 1:
                charterid = ARENA_TEAM_CHARTER_2v2;
                cost = sWorld->getIntConfig(CONFIG_CHARTER_COST_ARENA_2v2);
                type = ARENA_TEAM_CHARTER_2v2_TYPE;
                break;
            case 2:
                charterid = ARENA_TEAM_CHARTER_3v3;
                cost = sWorld->getIntConfig(CONFIG_CHARTER_COST_ARENA_3v3);
                type = ARENA_TEAM_CHARTER_3v3_TYPE;
                break;
            case 3:
                charterid = ARENA_TEAM_CHARTER_5v5;
                cost = sWorld->getIntConfig(CONFIG_CHARTER_COST_ARENA_5v5);
                type = ARENA_TEAM_CHARTER_5v5_TYPE;
                break;
            default:
                LOG_DEBUG("network", "unknown selection at buy arena petition: {}", clientIndex);
                return;
        }

        if (_player->GetArenaTeamId(clientIndex - 1))        // arenaSlot+1 as received from client
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, name, "", ERR_ALREADY_IN_ARENA_TEAM);
            return;
        }
    }

    sScriptMgr->OnPlayerPetitionBuy(_player, creature, charterid, cost, type);

    if (type == GUILD_CHARTER_TYPE)
    {
        if (sGuildMgr->GetGuildByName(name))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, name);
            return;
        }

        if (!ObjectMgr::IsValidCharterName(name))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_INVALID, name);
            return;
        }
    }
    else
    {
        if (sArenaTeamMgr->GetArenaTeamByName(name))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, name, "", ERR_ARENA_TEAM_NAME_EXISTS_S);
            return;
        }
        if (!ObjectMgr::IsValidCharterName(name))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, name, "", ERR_ARENA_TEAM_NAME_INVALID);
            return;
        }
    }

    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(charterid);
    if (!pProto)
    {
        _player->SendBuyError(BUY_ERR_CANT_FIND_ITEM, nullptr, charterid, 0);
        return;
    }

    if (!_player->HasEnoughMoney(cost))
    {
        //player hasn't got enough money
        _player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, creature, charterid, 0);
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, charterid, pProto->BuyCount);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, nullptr, nullptr, charterid);
        return;
    }

    _player->ModifyMoney(-(int32)cost);
    Item* charter = _player->StoreNewItem(dest, charterid, true);
    if (!charter)
        return;

    // Use a 31-bit safe petition id instead of the raw item guid
    uint32 petitionId = sPetitionMgr->GeneratePetitionId();
    charter->SetEnchantmentValue(PERM_ENCHANTMENT_SLOT, ENCHANTMENT_ID_OFFSET, petitionId);
    // ITEM_FIELD_ENCHANTMENT_1_1 is guild/arenateam id
    // ITEM_FIELD_ENCHANTMENT_1_1+1 is current signatures count (showed on item)
    charter->SetState(ITEM_CHANGED, _player);
    _player->SendNewItem(charter, 1, true, false);

    // a petition is invalid, if both the owner and the type matches
    // we checked above, if this player is in an arenateam, so this must be
    // datacorruption
    Petition const* petition = sPetitionMgr->GetPetitionByOwnerWithType(_player->GetGUID(), type);

    CharacterDatabase.EscapeString(name);
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    if (petition)
    {
        LOG_DEBUG("network", "Invalid petition: {}", petition->petitionGuid.ToString());

        trans->Append("DELETE FROM petition WHERE petitionguid = {}", petition->petitionGuid.GetCounter());
        trans->Append("DELETE FROM petition_sign WHERE petitionguid = {}", petition->petitionGuid.GetCounter());

        // xinef: clear petition store
        sPetitionMgr->RemovePetition(petition->petitionGuid);
    }

    // xinef: petition pointer is invalid from now on

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION);
    // petition_id, ownerguid, petitionguid(item guid), name, type
    stmt->SetData(0, petitionId);
    stmt->SetData(1, _player->GetGUID().GetCounter());
    stmt->SetData(2, charter->GetGUID().GetCounter());
    stmt->SetData(3, name);
    stmt->SetData(4, uint8(type));
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    // xinef: fill petition store (include petitionId)
    sPetitionMgr->AddPetition(charter->GetGUID(), _player->GetGUID(), name, uint8(type), petitionId);
}

void WorldSession::HandlePetitionShowSignOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode CMSG_PETITION_SHOW_SIGNATURES");

    ObjectGuid petitionguid;
    // 5.4.8 18414, from Project SkyFire
    recvData.ReadGuidMask(petitionguid, { 3, 7, 2, 4, 5, 6, 0, 1 });
    recvData.ReadGuidBytes(petitionguid, { 2, 4, 5, 7, 1, 0, 3, 6 });

    // solve (possible) some strange compile problems with explicit use petition low guid at some GCC versions (wrong code optimization in compiler?)
    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
        return;

    uint32 type = petition->petitionType;

    // if guild petition and has guild => error, return;
    if (type == GUILD_CHARTER_TYPE && _player->GetGuildId())
        return;

    Signatures const* signatures = sPetitionMgr->GetSignature(petitionguid);
    uint8 signs = signatures ? signatures->signatureMap.size() : 0;

    LOG_DEBUG("network", "CMSG_PETITION_SHOW_SIGNATURES petition {}", petitionguid.ToString());

    (void)signs;
    WorldPacket data = BuildPetitionShowSignatures(petitionguid, _player->GetGUID(), petition->petitionId, signatures);
    SendPacket(&data);
}

void WorldSession::HandlePetitionQueryOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode CMSG_PETITION_QUERY");   // ok

    ObjectGuid::LowType guildguid;
    ObjectGuid petitionguid;
    // 5.4.8 18414, from Project SkyFire
    recvData >> guildguid;                                 // in Trinity always same as petition low guid
    recvData.ReadGuidMask(petitionguid, { 2, 3, 1, 0, 4, 7, 6, 5 });
    recvData.ReadGuidBytes(petitionguid, { 0, 4, 7, 5, 1, 6, 3, 2 });
    LOG_DEBUG("network", "CMSG_PETITION_QUERY Petition ({}) Guild GUID {}", petitionguid.ToString(), guildguid);

    SendPetitionQueryOpcode(petitionguid);
}

void WorldSession::SendPetitionQueryOpcode(ObjectGuid petitionguid)
{
    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
    {
        LOG_DEBUG("network", "CMSG_PETITION_QUERY failed for petition ({})", petitionguid.ToString());
        return;
    }

    // 5.4.8 18414, from Project SkyFire
    uint8 type = petition->petitionType;
    ObjectGuid ownerGuid = petition->ownerGuid;
    WorldPacket data(SMSG_PETITION_QUERY_RESPONSE, (4 + 1 + 8 + petition->petitionName.size() + 4 * 16 + 2 + 10));
    data << uint32(petition->petitionId);                   // guild id (was item low guid)
    data.WriteBit(true);                                    // has data
    for (int i = 0; i < 10; ++i)
        data.WriteBits(0, 6);                               // choice texts
    data.WriteGuidMask(ownerGuid, { 2, 4 });
    data.WriteBits(0, 12);                                  // body text
    data.WriteGuidMask(ownerGuid, { 0, 7, 3, 6, 5 });
    data.WriteBits(petition->petitionName.size(), 7);
    data.WriteGuidMask(ownerGuid, { 1 });
    data.FlushBits();

    data.WriteGuidBytes(ownerGuid, { 5 });
    data << uint32(0);
    data.WriteString(petition->petitionName);
    data << uint32(0);
    data.WriteGuidBytes(ownerGuid, { 4 });
    data << uint32(type);                                   // 4 = guild
    data.WriteGuidBytes(ownerGuid, { 6 });
    data << uint32(0);
    data << uint32(sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS)); // required signatures
    data.WriteGuidBytes(ownerGuid, { 1, 7, 0 });
    data << uint32(0);
    data << uint32(0);
    data.WriteGuidBytes(ownerGuid, { 2 });
    data << uint32(GameTime::GetGameTime().count() + YEAR); // deadline
    data << uint16(0);
    data << uint32(0);
    data.WriteGuidBytes(ownerGuid, { 3 });
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    SendPacket(&data);
}

void WorldSession::HandlePetitionRenameOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode MSG_PETITION_RENAME");   // ok

    ObjectGuid petitionGuid;
    std::string newName;

    // 5.4.8 18414, from Project SkyFire
    uint32 nameLength = recvData.ReadBits(7);
    recvData.ReadGuidMask(petitionGuid, { 7, 4, 6, 2, 0, 5, 3, 1 });
    recvData.ReadGuidBytes(petitionGuid, { 4, 1, 7 });
    newName = recvData.ReadString(nameLength);
    recvData.ReadGuidBytes(petitionGuid, { 0, 3, 2, 6, 5 });

    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
        return;

    Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
    if (!petition)
    {
        LOG_DEBUG("network", "CMSG_PETITION_QUERY failed for petition ({})", petitionGuid.ToString());
        return;
    }

    if (petition->petitionType == GUILD_CHARTER_TYPE)
    {
        if (sGuildMgr->GetGuildByName(newName))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, newName);
            return;
        }
        if (!ObjectMgr::IsValidCharterName(newName))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_INVALID, newName);
            return;
        }
    }
    else
    {
        if (sArenaTeamMgr->GetArenaTeamByName(newName))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, newName, "", ERR_ARENA_TEAM_NAME_EXISTS_S);
            return;
        }
        if (!ObjectMgr::IsValidCharterName(newName))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, newName, "", ERR_ARENA_TEAM_NAME_INVALID);
            return;
        }
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_PETITION_NAME);

    stmt->SetData(0, newName);
    stmt->SetData(1, petition->petitionId);

    CharacterDatabase.Execute(stmt);

    // xinef: update petition container
    const_cast<Petition*>(petition)->petitionName = newName;

    LOG_DEBUG("network", "Petition ({}) renamed to {}", petitionGuid.ToString(), newName);
    // 5.4.8 18414, from Project SkyFire
    WorldPacket data(SMSG_PETITION_RENAME_RESULT, (1 + 8 + 1 + newName.size()));
    data.WriteBits(newName.length(), 7);
    data.WriteGuidMask(petitionGuid, { 0, 3, 4, 2, 6, 5, 7, 1 });
    data.FlushBits();
    data.WriteGuidBytes(petitionGuid, { 4, 3, 6, 0, 5, 2, 1, 7 });
    data.WriteString(newName);
    SendPacket(&data);
}

void WorldSession::HandlePetitionSignOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode CMSG_PETITION_SIGN");    // ok

    ObjectGuid petitionGuid;
    // 5.4.8 18414, from Project SkyFire
    recvData.read_skip<uint8>();
    recvData.ReadGuidMask(petitionGuid, { 4, 2, 0, 1, 5, 3, 6, 7 });
    recvData.ReadGuidBytes(petitionGuid, { 6, 1, 7, 2, 5, 3, 0, 4 });

    Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
    if (!petition)
    {
        LOG_ERROR("network.opcode", "Petition {} is not found for player {} (Name: {})", petitionGuid.ToString(), GetPlayer()->GetGUID().ToString(), GetPlayer()->GetName());
        return;
    }

    uint8 type = petition->petitionType;

    ObjectGuid playerGuid = _player->GetGUID();
    if (petition->ownerGuid == playerGuid)
        return;

    Signatures const* signatures = sPetitionMgr->GetSignature(petitionGuid);
    if (!signatures)
        return;

    if (type != GUILD_CHARTER_TYPE)
    {
        if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_ARENA) && GetPlayer()->GetTeamId() != sCharacterCache->GetCharacterTeamByGuid(petition->ownerGuid))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_INVITE_SS, "", "", ERR_ARENA_TEAM_NOT_ALLIED);
            return;
        }

        if (_player->GetLevel() < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, "", _player->GetName().c_str(), ERR_ARENA_TEAM_TARGET_TOO_LOW_S);
            return;
        }

        uint8 slot = ArenaTeam::GetSlotByType(type);
        if (slot >= MAX_ARENA_SLOT)
            return;

        if (_player->GetArenaTeamId(slot))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_INVITE_SS, "", _player->GetName().c_str(), ERR_ALREADY_IN_ARENA_TEAM_S);
            return;
        }

        if (_player->GetArenaTeamIdInvited())
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_INVITE_SS, "", _player->GetName().c_str(), ERR_ALREADY_INVITED_TO_ARENA_TEAM_S);
            return;
        }
    }
    else
    {
        if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && GetPlayer()->GetTeamId() != sCharacterCache->GetCharacterTeamByGuid(petition->ownerGuid))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NOT_ALLIED);
            return;
        }

        if (sWorld->getBoolConfig(CONFIG_TRIAL_RESTRICTION_GUILD) && IsTrialAccount())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_GUILD_PERMISSIONS);
            return;
        }

        if (_player->GetGuildId())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_IN_GUILD_S, _player->GetName());
            return;
        }
        if (_player->GetGuildIdInvited())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
            return;
        }
    }

    uint32 signs = signatures->signatureMap.size();
    if (++signs > type)                                        // client signs maximum
        return;

    // Client doesn't allow to sign petition two times by one character, but not check sign by another character from same account
    // not allow sign another player from already sign player account

    bool found = false;
    for (SignatureMap::const_iterator itr = signatures->signatureMap.begin(); itr != signatures->signatureMap.end(); ++itr)
        if (itr->second == GetAccountId())
        {
            found = true;
            break;
        }

    sScriptMgr->OnPlayerBeforePetitionSign(_player, petitionGuid, found);

    if (found)
    {
        WorldPacket data = BuildPetitionSignResults(petitionGuid, playerGuid, PETITION_SIGN_ALREADY_SIGNED);

        // close at signer side
        SendPacket(&data);

        // update for owner if online
        if (Player* owner = ObjectAccessor::FindConnectedPlayer(petition->ownerGuid))
            owner->SendDirectMessage(&data);
        return;
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION_SIGNATURE);

    stmt->SetData(0, petition->ownerGuid.GetCounter());
    stmt->SetData(1, petition->petitionId);
    stmt->SetData(2, playerGuid.GetCounter());
    stmt->SetData(3, GetAccountId());

    CharacterDatabase.Execute(stmt);

    // xinef: fill petition store
    sPetitionMgr->AddSignature(petitionGuid, GetAccountId(), playerGuid);

    LOG_DEBUG("network", "PETITION SIGN: {} by player: {} ({}, Account: {})", petitionGuid.ToString(), _player->GetName(), playerGuid.ToString(), GetAccountId());

    WorldPacket data = BuildPetitionSignResults(petitionGuid, playerGuid, PETITION_SIGN_OK);

    // close at signer side
    SendPacket(&data);

    // update signs count on charter, required testing...
    //Item* item = _player->GetItemByGuid(petitionguid));
    //if (item)
    //    item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1+1, signs);

    // update for owner if online
    if (Player* owner = ObjectAccessor::FindConnectedPlayer(petition->ownerGuid))
        owner->SendDirectMessage(&data);
}

void WorldSession::HandlePetitionDeclineOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode MSG_PETITION_DECLINE");  // ok

    ObjectGuid petitionguid;
    ObjectGuid ownerguid;
    // 5.4.8 18414, from Project SkyFire
    recvData.ReadGuidMask(petitionguid, { 5, 6, 4, 3, 1, 7, 0, 2 });
    recvData.ReadGuidBytes(petitionguid, { 6, 2, 1, 5, 0, 7, 4, 3 });
    LOG_DEBUG("network", "Petition {} declined by {}", petitionguid.ToString(), _player->GetGUID().ToString());

    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
        return;

    // 5.4.8 has no decline notice for the owner
    (void)ownerguid;
}

void WorldSession::HandleOfferPetitionOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode CMSG_OFFER_PETITION");   // ok

    ObjectGuid petitionguid, plguid;
    Player* player;
    // 5.4.8 18414, from Project SkyFire
    recvData.read_skip<uint32>();                          // this is not petition type!
    recvData.ReadGuidMask(plguid, { 4, 1 });
    recvData.ReadGuidMask(petitionguid, { 2 });
    recvData.ReadGuidMask(plguid, { 6 });
    recvData.ReadGuidMask(petitionguid, { 1 });
    recvData.ReadGuidMask(plguid, { 2 });
    recvData.ReadGuidMask(petitionguid, { 4 });
    recvData.ReadGuidMask(plguid, { 3, 7 });
    recvData.ReadGuidMask(petitionguid, { 0, 6 });
    recvData.ReadGuidMask(plguid, { 5, 0 });
    recvData.ReadGuidMask(petitionguid, { 3, 5, 7 });
    recvData.ReadGuidBytes(plguid, { 7 });
    recvData.ReadGuidBytes(petitionguid, { 1, 4, 2 });
    recvData.ReadGuidBytes(plguid, { 6 });
    recvData.ReadGuidBytes(petitionguid, { 3, 0, 5 });
    recvData.ReadGuidBytes(plguid, { 0, 2, 5, 3, 4 });
    recvData.ReadGuidBytes(petitionguid, { 7 });
    recvData.ReadGuidBytes(plguid, { 1 });
    recvData.ReadGuidBytes(petitionguid, { 6 });

    player = ObjectAccessor::FindConnectedPlayer(plguid);
    if (!player)
        return;

    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
        return;

    if (petition->petitionType != GUILD_CHARTER_TYPE)
    {
        if (GetPlayer()->GetTeamId() != player->GetTeamId() && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_ARENA))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_INVITE_SS, "", "", ERR_ARENA_TEAM_NOT_ALLIED);
            return;
        }

        if (player->GetLevel() < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            // player is too low level to join an arena team
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, player->GetName().c_str(), "", ERR_ARENA_TEAM_TARGET_TOO_LOW_S);
            return;
        }

        uint8 slot = ArenaTeam::GetSlotByType(petition->petitionType);
        if (slot >= MAX_ARENA_SLOT)
            return;

        if (player->GetArenaTeamId(slot))
        {
            // player is already in an arena team
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, player->GetName().c_str(), "", ERR_ALREADY_IN_ARENA_TEAM_S);
            return;
        }

        if (player->GetArenaTeamIdInvited())
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_INVITE_SS, "", _player->GetName().c_str(), ERR_ALREADY_INVITED_TO_ARENA_TEAM_S);
            return;
        }
    }
    else
    {
        if (GetPlayer()->GetTeamId() != player->GetTeamId() && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NOT_ALLIED);
            return;
        }

        if (player->GetGuildId())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_IN_GUILD_S, _player->GetName());
            return;
        }

        if (player->GetGuildIdInvited())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_INVITE, ERR_ALREADY_INVITED_TO_GUILD_S, _player->GetName());
            return;
        }
    }

    Signatures const* signatures = sPetitionMgr->GetSignature(petitionguid);
    uint8 signs = signatures ? signatures->signatureMap.size() : 0;

    (void)signs;
    WorldPacket data = BuildPetitionShowSignatures(petitionguid, _player->GetGUID(), petition->petitionId, signatures);
    player->SendDirectMessage(&data);
}

void WorldSession::HandleTurnInPetitionOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received opcode CMSG_TURN_IN_PETITION");

    // Get petition guid from packet
    WorldPacket data;
    ObjectGuid petitionGuid;

    // 5.4.8 18414, from Project SkyFire
    recvData.ReadGuidMask(petitionGuid, { 1, 2, 3, 0, 5, 7, 4, 6 });
    recvData.ReadGuidBytes(petitionGuid, { 2, 1, 4, 6, 0, 7, 5, 3 });

    // Check if player really has the required petition charter
    Item* item = _player->GetItemByGuid(petitionGuid);
    if (!item)
        return;

    LOG_DEBUG("network", "Petition {} turned in by {}", petitionGuid.ToString(), _player->GetGUID().ToString());

    Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
    if (!petition)
    {
        LOG_ERROR("network.opcode", "Player {} ({}) tried to turn in petition ({}) that is not present in the database",
            _player->GetName(), _player->GetGUID().ToString(), petitionGuid.ToString());
        return;
    }

    ObjectGuid ownerGuid = petition->ownerGuid;
    uint8 type = petition->petitionType;
    std::string name = petition->petitionName;

    // Only the petition owner can turn in the petition
    if (_player->GetGUID() != ownerGuid)
        return;

    // Petition type (guild/arena) specific checks
    if (type == GUILD_CHARTER_TYPE)
    {
        if (sWorld->getBoolConfig(CONFIG_TRIAL_RESTRICTION_GUILD) && IsTrialAccount())
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
            return;
        }

        // Check if player is already in a guild
        if (_player->GetGuildId())
        {
            // 5.4.8 18414, from Project SkyFire
            data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
            data.WriteBits(PETITION_TURN_ALREADY_IN_GUILD, 4);
            data.FlushBits();
            SendPacket(&data);
            return;
        }

        // Check if guild name is already taken
        if (sGuildMgr->GetGuildByName(name))
        {
            Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_EXISTS_S, name);
            return;
        }
    }
    else
    {
        // Check for valid arena bracket (2v2, 3v3, 5v5)
        uint8 slot = ArenaTeam::GetSlotByType(type);
        if (slot >= MAX_ARENA_SLOT)
            return;

        // Check if player is already in an arena team
        if (_player->GetArenaTeamId(slot))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, name, "", ERR_ALREADY_IN_ARENA_TEAM);
            return;
        }

        // Check if arena team name is already taken
        if (sArenaTeamMgr->GetArenaTeamByName(name))
        {
            SendArenaTeamCommandResult(ERR_ARENA_TEAM_CREATE_S, name, "", ERR_ARENA_TEAM_NAME_EXISTS_S);
            return;
        }
    }

    // Get petition signatures from db
    Signatures const* signatures = sPetitionMgr->GetSignature(petitionGuid);
    uint8 signs = signatures ? signatures->signatureMap.size() : 0;
    SignatureMap signatureCopy;
    if (signs)
        signatureCopy = signatures->signatureMap;

    uint32 requiredSignatures;
    if (type == GUILD_CHARTER_TYPE)
        requiredSignatures = sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS);
    else
        requiredSignatures = type - 1;

    // Notify player if signatures are missing
    if (signs < requiredSignatures)
    {
        // 5.4.8 18414, from Project SkyFire
        data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
        data.WriteBits(PETITION_TURN_NEED_MORE_SIGNATURES, 4);
        data.FlushBits();
        SendPacket(&data);
        return;
    }

    // Proceed with guild/arena team creation

    // Delete charter item
    _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    if (type == GUILD_CHARTER_TYPE)
    {
        // Create guild
        Guild* guild = new Guild;

        if (!guild->Create(_player, name))
        {
            delete guild;
            return;
        }

        // Register guild and add guild master
        sGuildMgr->AddGuild(guild);

        Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_COMMAND_SUCCESS, name);

        // Add members from signatures
        if (signs)
            for (SignatureMap::const_iterator itr = signatureCopy.begin(); itr != signatureCopy.end(); ++itr)
                guild->AddMember(itr->first);
    }
    else
    {
        // Receive the rest of the packet in arena team creation case
        uint32 background, icon, iconcolor, border, bordercolor;
        recvData >> background >> icon >> iconcolor >> border >> bordercolor;

        // Create arena team
        ArenaTeam* arenaTeam = new ArenaTeam();

        if (!arenaTeam->Create(_player->GetGUID(), type, name, background, icon, iconcolor, border, bordercolor))
        {
            delete arenaTeam;
            return;
        }

        // Register arena team
        sArenaTeamMgr->AddArenaTeam(arenaTeam);
        LOG_DEBUG("network", "PetitonsHandler: Arena team (guid: {}) added to ObjectMgr", arenaTeam->GetId());

        // Add members
        if (signs)
            for (SignatureMap::const_iterator itr = signatureCopy.begin(); itr != signatureCopy.end(); ++itr)
            {
                LOG_DEBUG("network", "PetitionsHandler: Adding arena team (guid: {}) member {}", arenaTeam->GetId(), itr->first.ToString());
                arenaTeam->AddMember(itr->first);
            }
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_BY_ID);
    stmt->SetData(0, petition->petitionId);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PETITION_SIGNATURE_BY_ID);
    stmt->SetData(0, petition->petitionId);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    // xinef: clear petition store (petition and signatures)
    sPetitionMgr->RemovePetition(petitionGuid);

    // created
    LOG_DEBUG("network", "TURN IN PETITION {}", petitionGuid.ToString());

    // 5.4.8 18414, from Project SkyFire
    data.Initialize(SMSG_TURN_IN_PETITION_RESULTS, 1);
    data.WriteBits(PETITION_TURN_OK, 4);
    data.FlushBits();
    SendPacket(&data);
}

void WorldSession::HandlePetitionShowListOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "Received CMSG_PETITION_SHOWLIST");

    ObjectGuid guid;
    // 5.4.8 18414, from Project SkyFire
    recvData.ReadGuidMask(guid, { 1, 7, 2, 5, 4, 0, 3, 6 });
    recvData.ReadGuidBytes(guid, { 6, 3, 2, 4, 1, 7, 5, 0 });

    SendPetitionShowList(guid);
}

void WorldSession::SendPetitionShowList(ObjectGuid guid)
{
    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_PETITIONER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandlePetitionShowListOpcode - Unit ({}) not found or you can't interact with him.", guid.ToString());
        return;
    }

    // 5.4.8 sells guild charters only, arena teams are gone
    if (!creature->IsTabardDesigner())
    {
        LOG_DEBUG("network", "WORLD: SendPetitionShowList - {} sells arena charters, which 5.4.8 has no list for.", guid.ToString());
        return;
    }

    uint32 CharterEntry = GUILD_CHARTER;
    uint32 CharterDispayID = CHARTER_DISPLAY_ID;
    uint32 CharterCost = sWorld->getIntConfig(CONFIG_CHARTER_COST_GUILD);
    sScriptMgr->OnPlayerPetitionShowList(_player, creature, CharterEntry, CharterDispayID, CharterCost);

    // 5.4.8 18414, from Project SkyFire
    WorldPacket data(SMSG_PETITION_SHOWLIST, 1 + 8 + 4);
    data.WriteGuidMask(guid, { 3, 5, 7, 6, 1, 0, 2, 4 });
    data.FlushBits();
    data.WriteGuidBytes(guid, { 6, 0, 1 });
    data << uint32(CharterCost);
    data.WriteGuidBytes(guid, { 4, 3, 5, 2, 7 });
    SendPacket(&data);
    LOG_DEBUG("network", "Sent SMSG_PETITION_SHOWLIST");
}

WorldPacket WorldSession::BuildPetitionShowSignatures(ObjectGuid petitionGuid, ObjectGuid ownerGuid, uint32 petitionId, Signatures const* signatures)
{
    // 5.4.8 18414, from Project SkyFire
    std::vector<ObjectGuid> signers;
    if (signatures)
        for (auto const& [signerGuid, accountId] : signatures->signatureMap)
            signers.push_back(signerGuid);

    WorldPacket data(SMSG_PETITION_SHOW_SIGNATURES, (8 + 8 + 4 + 4 + signers.size() * (8 + 4)));
    data.WriteGuidMask(ownerGuid, { 1 });
    data.WriteGuidMask(petitionGuid, { 3 });
    data.WriteGuidMask(ownerGuid, { 3 });
    data.WriteGuidMask(petitionGuid, { 4, 0 });
    data.WriteGuidMask(ownerGuid, { 7, 5 });
    data.WriteGuidMask(petitionGuid, { 1, 5, 7 });
    data.WriteGuidMask(ownerGuid, { 0, 6 });
    data.WriteGuidMask(petitionGuid, { 6 });
    data.WriteGuidMask(ownerGuid, { 2, 4 });
    data.WriteBits(signers.size(), 21);
    for (ObjectGuid const& signer : signers)
        data.WriteGuidMask(signer, { 2, 0, 4, 7, 5, 1, 6, 3 });
    data.WriteGuidMask(petitionGuid, { 2 });
    data.FlushBits();

    for (ObjectGuid const& signer : signers)
    {
        data.WriteGuidBytes(signer, { 6, 0, 1, 3, 2, 5, 7, 4 });
        data << uint32(0);                                  // choice, 0 = signed
    }

    data.WriteGuidBytes(petitionGuid, { 6, 5, 4 });
    data.WriteGuidBytes(ownerGuid, { 4 });
    data.WriteGuidBytes(petitionGuid, { 1 });
    data << uint32(petitionId);                             // guild id
    data.WriteGuidBytes(petitionGuid, { 2, 3, 7 });
    data.WriteGuidBytes(ownerGuid, { 5, 6, 3, 7, 1, 0 });
    data.WriteGuidBytes(petitionGuid, { 0 });
    data.WriteGuidBytes(ownerGuid, { 2 });
    return data;
}

WorldPacket WorldSession::BuildPetitionSignResults(ObjectGuid petitionGuid, ObjectGuid playerGuid, uint32 result)
{
    // 5.4.8 18414, from Project SkyFire
    WorldPacket data(SMSG_PETITION_SIGN_RESULTS, 8 + 8 + 1);
    data.WriteGuidMask(playerGuid, { 2, 0 });
    data.WriteGuidMask(petitionGuid, { 0 });
    data.WriteGuidMask(playerGuid, { 1 });
    data.WriteGuidMask(petitionGuid, { 5, 2, 4 });
    data.WriteGuidMask(playerGuid, { 6 });
    data.WriteGuidMask(petitionGuid, { 1, 6 });
    data.WriteGuidMask(playerGuid, { 4 });
    data.WriteGuidMask(petitionGuid, { 3 });
    data.WriteGuidMask(playerGuid, { 5, 3 });
    data.WriteGuidMask(petitionGuid, { 7 });
    data.WriteGuidMask(playerGuid, { 7 });
    data.WriteBits(result, 4);
    data.FlushBits();

    data.WriteGuidBytes(petitionGuid, { 0, 5 });
    data.WriteGuidBytes(playerGuid, { 3, 0 });
    data.WriteGuidBytes(petitionGuid, { 3 });
    data.WriteGuidBytes(playerGuid, { 2, 6, 4, 1 });
    data.WriteGuidBytes(petitionGuid, { 6, 7 });
    data.WriteGuidBytes(playerGuid, { 7 });
    data.WriteGuidBytes(petitionGuid, { 2, 1 });
    data.WriteGuidBytes(playerGuid, { 5 });
    data.WriteGuidBytes(petitionGuid, { 4 });
    return data;
}
