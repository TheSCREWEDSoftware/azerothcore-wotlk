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
#include "PetitionPackets.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::HandlePetitionBuy(WorldPackets::Petition::PetitionBuy& packet)
{
    ObjectGuid guidNPC = packet.Unit;
    std::string name = packet.Title;

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
        // 6.x has no arena charters
        LOG_DEBUG("network", "WORLD: HandlePetitionBuy - {} is not a guild charter seller.", guidNPC.ToString());
        return;
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

void WorldSession::HandlePetitionShowSignatures(WorldPackets::Petition::PetitionShowSignatures& packet)
{
    ObjectGuid petitionguid = packet.Item;

    // solve (possible) some strange compile problems with explicit use petition low guid at some GCC versions (wrong code optimization in compiler?)
    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
        return;

    uint32 type = petition->petitionType;

    // if guild petition and has guild => error, return;
    if (type == GUILD_CHARTER_TYPE && _player->GetGuildId())
        return;

    Signatures const* signatures = sPetitionMgr->GetSignature(petitionguid);
    LOG_DEBUG("network", "CMSG_PETITION_SHOW_SIGNATURES petition {}", petitionguid.ToString());

    WorldPacket data = BuildPetitionShowSignatures(petitionguid, _player->GetGUID(), petition->petitionId, signatures);
    SendPacket(&data);
}

void WorldSession::HandleQueryPetition(WorldPackets::Petition::QueryPetition& packet)
{
    ObjectGuid petitionguid = packet.ItemGUID;
    LOG_DEBUG("network", "CMSG_QUERY_PETITION Petition ({}) PetitionID {}", petitionguid.ToString(), packet.PetitionID);

    SendPetitionQueryOpcode(petitionguid);
}

void WorldSession::SendPetitionQueryOpcode(ObjectGuid petitionguid)
{
    WorldPackets::Petition::QueryPetitionResponse responsePacket;

    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
    {
        responsePacket.Allow = false;
        SendPacket(responsePacket.Write());
        LOG_DEBUG("network", "CMSG_QUERY_PETITION failed for petition ({})", petitionguid.ToString());
        return;
    }

    int32 reqSignatures = int32(sWorld->getIntConfig(CONFIG_MIN_PETITION_SIGNS));

    responsePacket.PetitionID = petition->petitionId;
    responsePacket.Allow = true;
    responsePacket.Info.PetitionID = int32(petition->petitionId);
    responsePacket.Info.Petitioner = petition->ownerGuid;
    responsePacket.Info.MinSignatures = reqSignatures;
    responsePacket.Info.MaxSignatures = reqSignatures;
    responsePacket.Info.Title = petition->petitionName;
    SendPacket(responsePacket.Write());
}

void WorldSession::HandlePetitionRenameGuild(WorldPackets::Petition::PetitionRenameGuild& packet)
{
    ObjectGuid petitionGuid = packet.PetitionGuid;
    std::string newName = packet.NewGuildName;

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
    WorldPackets::Petition::PetitionRenameGuildResponse renameResponse;
    renameResponse.PetitionGuid = petitionGuid;
    renameResponse.NewGuildName = newName;
    SendPacket(renameResponse.Write());
}

void WorldSession::HandleSignPetition(WorldPackets::Petition::SignPetition& packet)
{
    ObjectGuid petitionGuid = packet.PetitionGUID;

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

void WorldSession::HandleDeclinePetition(WorldPackets::Petition::DeclinePetition& packet)
{
    ObjectGuid petitionguid = packet.PetitionGUID;
    ObjectGuid ownerguid;
    LOG_DEBUG("network", "Petition {} declined by {}", petitionguid.ToString(), _player->GetGUID().ToString());

    Petition const* petition = sPetitionMgr->GetPetition(petitionguid);
    if (!petition)
        return;

    // 6.x: the client does not handle SMSG_PETITION_DECLINED, the owner gets no notice
    (void)ownerguid;
}

void WorldSession::HandleOfferPetition(WorldPackets::Petition::OfferPetition& packet)
{
    ObjectGuid petitionguid = packet.ItemGUID;
    ObjectGuid plguid = packet.TargetPlayer;
    Player* player;

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

    WorldPacket data = BuildPetitionShowSignatures(petitionguid, _player->GetGUID(), petition->petitionId, signatures);
    player->SendDirectMessage(&data);
}

void WorldSession::HandleTurnInPetition(WorldPackets::Petition::TurnInPetition& packet)
{
    ObjectGuid petitionGuid = packet.Item;

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
            SendTurnInPetitionResult(PETITION_TURN_ALREADY_IN_GUILD);
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
        SendTurnInPetitionResult(PETITION_TURN_NEED_MORE_SIGNATURES);
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
        // 6.x has no arena team charters
        return;
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

    SendTurnInPetitionResult(PETITION_TURN_OK);
}

void WorldSession::HandlePetitionShowList(WorldPackets::Petition::PetitionShowList& packet)
{
    ObjectGuid guid = packet.PetitionUnit;

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

    // 6.x sells guild charters only, arena teams are gone
    if (!creature->IsTabardDesigner())
    {
        LOG_DEBUG("network", "WORLD: SendPetitionShowList - {} sells arena charters, which 6.x has no list for.", guid.ToString());
        return;
    }

    uint32 CharterEntry = GUILD_CHARTER;
    uint32 CharterDispayID = CHARTER_DISPLAY_ID;
    uint32 CharterCost = sWorld->getIntConfig(CONFIG_CHARTER_COST_GUILD);
    sScriptMgr->OnPlayerPetitionShowList(_player, creature, CharterEntry, CharterDispayID, CharterCost);

    WorldPackets::Petition::ServerPetitionShowList packet;
    packet.Unit = guid;
    packet.Price = CharterCost;
    SendPacket(packet.Write());
}

WorldPacket WorldSession::BuildPetitionShowSignatures(ObjectGuid petitionGuid, ObjectGuid ownerGuid, uint32 petitionId, Signatures const* signatures)
{
    WorldPackets::Petition::ServerPetitionShowSignatures signaturesPacket;
    signaturesPacket.Item = petitionGuid;
    signaturesPacket.Owner = ownerGuid;
    signaturesPacket.OwnerAccountID = ObjectGuid::Create<HighGuid::WowAccount>(sCharacterCache->GetCharacterAccountIdByGuid(ownerGuid));
    signaturesPacket.PetitionID = int32(petitionId);

    if (signatures)
    {
        signaturesPacket.Signatures.reserve(signatures->signatureMap.size());
        for (auto const& [signerGuid, accountId] : signatures->signatureMap)
        {
            WorldPackets::Petition::ServerPetitionShowSignatures::PetitionSignature signature;
            signature.Signer = signerGuid;
            signature.Choice = 0;
            signaturesPacket.Signatures.push_back(signature);
        }
    }

    signaturesPacket.Write();
    return signaturesPacket.Move();
}

WorldPacket WorldSession::BuildPetitionSignResults(ObjectGuid petitionGuid, ObjectGuid playerGuid, uint32 result)
{
    WorldPackets::Petition::PetitionSignResults signResult;
    signResult.Item = petitionGuid;
    signResult.Player = playerGuid;
    signResult.Error = int32(result);
    signResult.Write();
    return signResult.Move();
}

void WorldSession::SendTurnInPetitionResult(uint32 result)
{
    WorldPackets::Petition::TurnInPetitionResult packet;
    packet.Result = int32(result);
    SendPacket(packet.Write());
}
