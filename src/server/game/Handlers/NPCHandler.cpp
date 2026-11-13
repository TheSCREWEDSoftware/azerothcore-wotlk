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

#include "Battleground.h"
#include "ConditionMgr.h"
#include "BattlegroundMgr.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameGraveyard.h"
#include "NPCPackets.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "Player.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Trainer.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <cmath>

enum StableResultCode
{
    STABLE_ERR_MONEY        = 0x01,                         // "you don't have enough money"
    STABLE_ERR_STABLE       = 0x06,                         // currently used in most fail cases
    STABLE_SUCCESS_STABLE   = 0x08,                         // stable success
    STABLE_SUCCESS_UNSTABLE = 0x09,                         // unstable/swap success
    STABLE_SUCCESS_BUY_SLOT = 0x0A,                         // buy slot success
    STABLE_ERR_EXOTIC       = 0x0C,                         // "you are unable to control exotic creatures"
};

enum StablePetFlags : uint8
{
    PET_STABLE_FLAG_ACTIVE   = 0x01,
    PET_STABLE_FLAG_INACTIVE = 0x02
};

void WorldSession::HandleTabardVendorActivateOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleTabardVendorActivateOpcode - Unit ({}) not found or you can not interact with him.", guid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendTabardVendorActivate(guid);
}

void WorldSession::SendTabardVendorActivate(ObjectGuid guid)
{
    WorldPacket data(MSG_TABARDVENDOR_ACTIVATE, 8);
    data << guid;
    SendPacket(&data);
}

void WorldSession::SendShowMailBox(ObjectGuid guid)
{
    WorldPacket data(SMSG_SHOW_MAILBOX, 8);
    data << guid;
    SendPacket(&data);
}

void WorldSession::HandleTrainerListOpcode(WorldPackets::NPC::Hello& packet)
{
    Creature* npc = GetPlayer()->GetNPCIfCanInteractWith(packet.Unit, UNIT_NPC_FLAG_TRAINER);
    if (!npc)
    {
        LOG_DEBUG("network", "WorldSession: SendTrainerList - {} not found or you can not interact with him.", packet.Unit.ToString().c_str());
        return;
    }

    SendTrainerList(npc);
}

void WorldSession::SendTrainerList(Creature* npc)
{
    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    Trainer::Trainer const* trainer = sObjectMgr->GetTrainer(npc->GetEntry());
    if (!trainer)
    {
        LOG_DEBUG("network", "WorldSession: SendTrainerList - trainer spells not found for {}", npc->GetGUID().ToString().c_str());
        return;
    }

    if (!trainer->IsTrainerValidForPlayer(_player))
    {
        LOG_DEBUG("network", "WorldSession: SendTrainerList - trainer {} not valid for player {}", npc->GetGUID().ToString().c_str(), GetPlayerInfo().c_str());
        return;
    }

    npc->PauseMovementForInteraction();

    trainer->SendSpells(npc, _player, GetSessionDbLocaleIndex());
}

void WorldSession::HandleTrainerBuySpellOpcode(WorldPackets::NPC::TrainerBuySpell& packet)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_TRAINER_BUY_SPELL {}, learn spell id is: {}", packet.TrainerGUID.ToString().c_str(), packet.SpellID);

    Creature* npc = GetPlayer()->GetNPCIfCanInteractWith(packet.TrainerGUID, UNIT_NPC_FLAG_TRAINER);
    if (!npc)
    {
        LOG_DEBUG("network", "WORLD: HandleTrainerBuySpellOpcode - {} not found or you can not interact with him.", packet.TrainerGUID.ToString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    Trainer::Trainer* trainer = sObjectMgr->GetTrainer(npc->GetEntry());
    if (!trainer)
        return;

    // 4.x carries the trainer id in the request; it must name the trainer this NPC currently offers
    // through AC's creature -> default trainer mapping (the list a client was previously shown is not
    // tracked, unlike the reference's interaction history)
    if (trainer->GetTrainerId() != uint32(packet.TrainerID))
    {
        LOG_DEBUG("network", "WORLD: HandleTrainerBuySpellOpcode - trainer id {} does not match {} for {}",
            packet.TrainerID, trainer->GetTrainerId(), packet.TrainerGUID.ToString());
        return;
    }

    trainer->TeachSpell(npc, _player, packet.SpellID);
}

void WorldSession::HandleGossipHelloOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleGossipHelloOpcode - Unit ({}) not found or you can not interact with him.", guid.ToString());
        return;
    }

    // xinef: check if we have ANY npc flags
    if (unit->GetNpcFlags() == UNIT_NPC_FLAG_NONE)
        return;

    // Check GossipHello conditions - block gossip opening if conditions not met
    ConditionList gossipConditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_GOSSIP_HELLO, unit->GetEntry());
    if (!sConditionMgr->IsObjectMeetToConditions(_player, unit, gossipConditions))
        return;

    // set faction visible if needed
    if (FactionTemplateEntry const* factionTemplateEntry = sFactionTemplateStore.LookupEntry(unit->GetFaction()))
        _player->GetReputationMgr().SetVisible(factionTemplateEntry);

    GetPlayer()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);
    // remove fake death
    //if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
    //    GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    unit->PauseMovementForInteraction();

    // Update home position for patrolling NPCs only (prevents drift for stationary NPCs)
    if (unit->GetDefaultMovementType() == WAYPOINT_MOTION_TYPE ||
        unit->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
        unit->SetHomePosition(unit->GetPosition());

    // If spiritguide, no need for gossip menu, just put player into resurrect queue
    if (unit->IsSpiritGuide())
    {
        Battleground* bg = _player->GetBattleground();
        if (bg)
        {
            bg->AddPlayerToResurrectQueue(unit->GetGUID(), _player->GetGUID());
            sBattlegroundMgr->SendAreaSpiritHealerQueryOpcode(_player, bg, unit->GetGUID());
            return;
        }
    }

    if (!sScriptMgr->OnGossipHello(_player, unit))
    {
        //        _player->TalkedToCreature(unit->GetEntry(), unit->GetGUID());
        _player->PrepareGossipMenu(unit, unit->GetGossipMenuId(), true);
        _player->SendPreparedGossip(unit);
    }
    unit->AI()->sGossipHello(_player);
}

/*void WorldSession::HandleGossipSelectOptionOpcode(WorldPacket & recvData)
{
    LOG_DEBUG("network.opcode", "WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 option;
    uint32 unk;
    ObjectGuid guid;
    std::string code = "";

    recvData >> guid >> unk >> option;

    if (_player->PlayerTalkClass->GossipOptionCoded(option))
    {
        LOG_DEBUG("network.opcode", "reading string");
        recvData >> code;
        LOG_DEBUG("network.opcode", "string read: {}", code);
    }

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!unit)
    {
        LOG_DEBUG("network.opcode", "WORLD: HandleGossipSelectOptionOpcode - Unit ({}) not found or you can't interact with him.", guid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    if (!code.empty())
    {
        if (!Script->GossipSelectWithCode(_player, unit, _player->PlayerTalkClass->GossipOptionSender (option), _player->PlayerTalkClass->GossipOptionAction(option), code.c_str()))
            unit->OnGossipSelect (_player, option);
    }
    else
    {
        if (!Script->OnGossipSelect (_player, unit, _player->PlayerTalkClass->GossipOptionSender (option), _player->PlayerTalkClass->GossipOptionAction (option)))
           unit->OnGossipSelect (_player, option);
    }
}*/

void WorldSession::HandleSpiritHealerActivateOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_SPIRIT_HEALER_ACTIVATE");

    ObjectGuid guid;

    recvData >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_SPIRITHEALER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleSpiritHealerActivateOpcode - Unit ({}) not found or you can not interact with him.", guid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendSpiritResurrect();
}

void WorldSession::SendSpiritResurrect()
{
    if (_player->IsAlive())
        return;

    _player->ResurrectPlayer(0.5f, true);

    float durabilityLossOnSpiritResurrect = sWorld->getRate(RATE_DURABILITY_LOSS_ON_SPIRIT_RESURRECT) / 100.0f;
    if (durabilityLossOnSpiritResurrect)
        _player->DurabilityLossAll(durabilityLossOnSpiritResurrect, true);

    // get corpse nearest graveyard
    GraveyardStruct const* corpseGrave = nullptr;

    // Search for any graveyards near the player's corpse.
    corpseGrave = sGraveyard->GetClosestGraveyard(_player, _player->GetTeamId(), _player->HasCorpse());

    // now can spawn bones
    _player->SpawnCorpseBones();

    // teleport to nearest from corpse graveyard, if different from nearest to player ghost
    if (corpseGrave)
    {
        GraveyardStruct const* ghostGrave = sGraveyard->GetClosestGraveyard(_player, _player->GetTeamId());

        if (corpseGrave != ghostGrave)
            _player->TeleportTo(corpseGrave->Map, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
        // or update at original position
        //else
        //    _player->UpdateObjectVisibility(); // xinef: not needed, called in ResurrectPlayer
    }
    // or update at original position
    //else
    //    _player->UpdateObjectVisibility(); // xinef: not needed, called in ResurrectPlayer
}

void WorldSession::HandleBinderActivateOpcode(WorldPacket& recvData)
{
    ObjectGuid npcGUID;
    recvData >> npcGUID;

    if (!GetPlayer()->IsInWorld() || !GetPlayer()->IsAlive())
        return;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID, UNIT_NPC_FLAG_INNKEEPER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleBinderActivateOpcode - Unit ({}) not found or you can not interact with him.", npcGUID.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendBindPoint(unit);
}

void WorldSession::SendBindPoint(Creature* npc)
{
    // prevent set homebind to instances in any case
    if (GetPlayer()->GetMap()->Instanceable())
        return;

    uint32 bindspell = 3286;

    // send spell for homebinding (3286)
    npc->CastSpell(_player, bindspell, true);

    WorldPacket data(SMSG_TRAINER_BUY_SUCCEEDED, (8 + 4));
    data << npc->GetGUID();
    data << uint32(bindspell);
    SendPacket(&data);

    _player->PlayerTalkClass->SendCloseGossip();
}

void WorldSession::HandleListStabledPetsOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Recv MSG_LIST_STABLED_PETS");
    ObjectGuid npcGUID;

    recvData >> npcGUID;

    if (!CheckStableMaster(npcGUID))
        return;

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // remove mounts this fix bug where getting pet from stable while mounted deletes pet.
    if (GetPlayer()->IsMounted())
        GetPlayer()->RemoveAurasByType(SPELL_AURA_MOUNTED);

    SendStablePet(npcGUID);
}

void WorldSession::SendStablePet(ObjectGuid guid)
{
    WorldPacket data(MSG_LIST_STABLED_PETS, 200);
    data << guid;
    std::size_t countPos = data.wpos();
    data << uint8(0);                                       // pet count, set below
    data << uint8(PET_STABLE_LAST_CLIENT_SLOT);             // stable slots

    uint8 count = 0;
    auto appendPet = [&](PetStable::PetInfo const& pet, uint8 clientSlot)
    {
        uint8 flags = PET_STABLE_FLAG_ACTIVE;
        if (clientSlot >= PET_STABLE_FIRST_CLIENT_SLOT)
            flags |= PET_STABLE_FLAG_INACTIVE;

        data << int32(clientSlot);
        data << uint32(pet.PetNumber);
        data << uint32(pet.CreatureId);
        data << uint32(pet.Level);
        data << pet.Name;
        data << uint8(flags);
        ++count;
    };

    if (PetStable const* petStable = GetPlayer()->GetPetStable())
    {
        // active pets in Call Pet order, then the stable
        for (uint8 callSlot = 0; callSlot < PET_CALL_SLOT_COUNT; ++callSlot)
            if (PetStable::PetInfo const* pet = petStable->GetPetInCallSlot(callSlot))
                appendPet(*pet, callSlot);

        for (std::size_t stableSlot = 0; stableSlot < petStable->StabledPets.size(); ++stableSlot)
            if (Optional<PetStable::PetInfo> const& pet = petStable->StabledPets[stableSlot])
                appendPet(*pet, uint8(PET_STABLE_FIRST_CLIENT_SLOT + stableSlot));
    }

    data.put<uint8>(countPos, count);
    SendPacket(&data);
}

void WorldSession::SendStableResult(uint8 res)
{
    WorldPacket data(SMSG_STABLE_RESULT, 1);
    data << uint8(res);
    SendPacket(&data);
}

void WorldSession::HandleSetPetSlot(WorldPacket& recvData)
{
    uint32 petNumber;
    uint8 newSlot;
    ObjectGuid npcGuid;

    recvData >> petNumber >> newSlot;

    npcGuid[3] = recvData.ReadBit();
    npcGuid[2] = recvData.ReadBit();
    npcGuid[0] = recvData.ReadBit();
    npcGuid[7] = recvData.ReadBit();
    npcGuid[5] = recvData.ReadBit();
    npcGuid[6] = recvData.ReadBit();
    npcGuid[1] = recvData.ReadBit();
    npcGuid[4] = recvData.ReadBit();

    recvData.ReadByteSeq(npcGuid[5]);
    recvData.ReadByteSeq(npcGuid[3]);
    recvData.ReadByteSeq(npcGuid[1]);
    recvData.ReadByteSeq(npcGuid[7]);
    recvData.ReadByteSeq(npcGuid[4]);
    recvData.ReadByteSeq(npcGuid[0]);
    recvData.ReadByteSeq(npcGuid[6]);
    recvData.ReadByteSeq(npcGuid[2]);

    if (!GetPlayer()->IsAlive() || !CheckStableMaster(npcGuid) || newSlot > PET_STABLE_LAST_CLIENT_SLOT)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    PetStable* petStable = GetPlayer()->GetPetStable();
    PetStable::PetInfo const* petInfo = petStable ? petStable->FindPet(petNumber) : nullptr;
    if (!petInfo || petInfo->Type != HUNTER_PET)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(petInfo->CreatureId);
    if (!creatureInfo || !creatureInfo->IsTameable(true))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (newSlot < PET_CALL_SLOT_COUNT && !creatureInfo->IsTameable(_player->CanTameExoticPets()))
    {
        SendStableResult(STABLE_ERR_EXOTIC);
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // where the pet comes from, the pet in the target slot moves there
    uint8 fromSlot = petInfo->CallSlot;
    for (std::size_t stableSlot = 0; stableSlot < petStable->StabledPets.size(); ++stableSlot)
        if (petStable->StabledPets[stableSlot] && petStable->StabledPets[stableSlot]->PetNumber == petNumber)
            fromSlot = uint8(PET_STABLE_FIRST_CLIENT_SLOT + stableSlot);

    if (fromSlot >= PET_CALL_SLOT_COUNT && fromSlot < PET_STABLE_FIRST_CLIENT_SLOT)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (fromSlot == newSlot)
    {
        SendStableResult(STABLE_SUCCESS_STABLE);
        return;
    }

    uint32 occupantNumber = 0;
    if (newSlot < PET_CALL_SLOT_COUNT)
    {
        if (PetStable::PetInfo const* occupant = petStable->GetPetInCallSlot(newSlot))
            occupantNumber = occupant->PetNumber;
    }
    else if (Optional<PetStable::PetInfo> const& occupant = petStable->StabledPets[newSlot - PET_STABLE_FIRST_CLIENT_SLOT])
        occupantNumber = occupant->PetNumber;

    // a pet that goes into the stable cannot stay out with the player
    auto dismissIfOut = [&](uint32 number)
    {
        if (Pet* pet = _player->GetPet())
        {
            if (pet->GetCharmInfo()->GetPetNumber() == number)
                _player->RemovePet(pet, PET_SAVE_NOT_IN_SLOT);
        }
        else if (petStable->CurrentPet && petStable->CurrentPet->PetNumber == number)
            _player->RemovePet(nullptr, PET_SAVE_NOT_IN_SLOT);
    };

    if (newSlot >= PET_STABLE_FIRST_CLIENT_SLOT)
        dismissIfOut(petNumber);

    if (occupantNumber && fromSlot >= PET_STABLE_FIRST_CLIENT_SLOT)
        dismissIfOut(occupantNumber);

    // take both pets out of the stable, then put them into their new slots
    auto extract = [&](uint32 number, bool& wasCurrent) -> PetStable::PetInfo
    {
        PetStable::PetInfo pet;
        wasCurrent = false;
        if (petStable->CurrentPet && petStable->CurrentPet->PetNumber == number)
        {
            pet = std::move(*petStable->CurrentPet);
            petStable->CurrentPet.reset();
            wasCurrent = true;
            return pet;
        }

        for (Optional<PetStable::PetInfo>& stabled : petStable->StabledPets)
        {
            if (stabled && stabled->PetNumber == number)
            {
                pet = std::move(*stabled);
                stabled.reset();
                return pet;
            }
        }

        auto itr = std::find_if(petStable->UnslottedPets.begin(), petStable->UnslottedPets.end(), [number](PetStable::PetInfo const& unslotted)
        {
            return unslotted.PetNumber == number;
        });

        pet = std::move(*itr);
        petStable->UnslottedPets.erase(itr);
        return pet;
    };

    auto place = [&](PetStable::PetInfo&& pet, uint8 clientSlot, bool wasCurrent)
    {
        uint8 dbSlot;
        if (clientSlot < PET_CALL_SLOT_COUNT)
        {
            pet.CallSlot = clientSlot;
            dbSlot = wasCurrent ? uint8(PET_SAVE_AS_CURRENT) : uint8(PET_SAVE_NOT_IN_SLOT);
        }
        else
        {
            pet.CallSlot = PET_CALL_SLOT_NONE;
            dbSlot = uint8(PET_SAVE_FIRST_STABLE_SLOT + clientSlot - PET_STABLE_FIRST_CLIENT_SLOT);
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_PET_SLOTS_BY_ID);
        stmt->SetData(0, dbSlot);
        stmt->SetData(1, pet.CallSlot);
        stmt->SetData(2, _player->GetGUID().GetCounter());
        stmt->SetData(3, pet.PetNumber);
        CharacterDatabase.Execute(stmt);

        if (clientSlot >= PET_CALL_SLOT_COUNT)
            petStable->StabledPets[clientSlot - PET_STABLE_FIRST_CLIENT_SLOT] = std::move(pet);
        else if (wasCurrent)
            petStable->CurrentPet = std::move(pet);
        else
            petStable->UnslottedPets.push_back(std::move(pet));
    };

    bool petWasCurrent = false;
    bool occupantWasCurrent = false;
    PetStable::PetInfo pet = extract(petNumber, petWasCurrent);
    Optional<PetStable::PetInfo> occupant;
    if (occupantNumber)
        occupant = extract(occupantNumber, occupantWasCurrent);

    place(std::move(pet), newSlot, petWasCurrent);
    if (occupant)
        place(std::move(*occupant), fromSlot, occupantWasCurrent);

    SendPetSlotUpdated(petNumber, newSlot, occupantNumber, fromSlot);
    SendStableResult(STABLE_SUCCESS_STABLE);
}

void WorldSession::SendPetSlotUpdated(int32 petNumberA, int32 petSlotA, int32 petNumberB, int32 petSlotB)
{
    WorldPacket data(SMSG_PET_SLOT_UPDATED, 4 + 4 + 4 + 4);
    data << int32(petSlotA);
    data << uint32(petNumberA);
    data << uint32(petNumberB);
    data << int32(petSlotB);
    SendPacket(&data);
}

void WorldSession::HandleRepairItemOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: CMSG_REPAIR_ITEM");

    ObjectGuid npcGUID, itemGUID;
    uint8 guildBank;                                        // new in 2.3.2, bool that means from guild bank money

    recvData >> npcGUID >> itemGUID >> guildBank;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID, UNIT_NPC_FLAG_REPAIR);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleRepairItemOpcode - Unit ({}) not found or you can not interact with him.", npcGUID.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // reputation discount
    float discountMod = _player->GetReputationPriceDiscount(unit);

    sScriptMgr->OnPlayerBeforeDurabilityRepair(_player, npcGUID, itemGUID, discountMod, guildBank);

    if (itemGUID)
    {
        LOG_DEBUG("network", "ITEM: Repair item, item {}, npc {}", itemGUID.ToString(), npcGUID.ToString());

        Item* item = _player->GetItemByGuid(itemGUID);
        if (item)
            _player->DurabilityRepair(item->GetPos(), true, discountMod, guildBank);
    }
    else
    {
        LOG_DEBUG("network", "ITEM: Repair all items, npc {}", npcGUID.ToString());
        _player->DurabilityRepairAll(true, discountMod, guildBank);
    }
}
