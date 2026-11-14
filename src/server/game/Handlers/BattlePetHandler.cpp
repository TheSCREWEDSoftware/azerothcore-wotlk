/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// 6.x battle pet journal handlers, from TrinityCore 6.x (BattlePetHandler.cpp)

#include "BattlePetMgr.h"
#include "BattlePetPackets.h"
#include "DB2Stores.h"
#include "Item.h"
#include "ItemPackets.h"
#include "Player.h"
#include "WorldSession.h"

void WorldSession::HandleBattlePetRequestJournal(WorldPackets::BattlePet::BattlePetRequestJournal& /*battlePetRequestJournal*/)
{
    WorldPackets::BattlePet::BattlePetJournal battlePetJournal;
    battlePetJournal.Trap = GetBattlePetMgr()->GetTrapLevel();

    for (BattlePetMgr::BattlePet const& pet : GetBattlePetMgr()->GetLearnedPets())
        battlePetJournal.Pets.push_back(pet.PacketInfo);

    battlePetJournal.Slots = GetBattlePetMgr()->GetSlots();
    SendPacket(battlePetJournal.Write());
}

void WorldSession::HandleBattlePetSetBattleSlot(WorldPackets::BattlePet::BattlePetSetBattleSlot& battlePetSetBattleSlot)
{
    if (BattlePetMgr::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetSetBattleSlot.PetGuid))
        if (WorldPackets::BattlePet::BattlePetSlot* slot = GetBattlePetMgr()->GetSlot(battlePetSetBattleSlot.Slot))
            slot->Pet = pet->PacketInfo;
}

void WorldSession::HandleBattlePetModifyName(WorldPackets::BattlePet::BattlePetModifyName& battlePetModifyName)
{
    if (BattlePetMgr::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetModifyName.PetGuid))
    {
        pet->PacketInfo.Name = battlePetModifyName.Name;

        if (pet->SaveInfo != BATTLE_PET_NEW)
            pet->SaveInfo = BATTLE_PET_CHANGED;
    }
}

void WorldSession::HandleBattlePetDeletePet(WorldPackets::BattlePet::BattlePetDeletePet& battlePetDeletePet)
{
    GetBattlePetMgr()->RemovePet(battlePetDeletePet.PetGuid);
}

void WorldSession::HandleBattlePetSetFlags(WorldPackets::BattlePet::BattlePetSetFlags& battlePetSetFlags)
{
    if (BattlePetMgr::BattlePet* pet = GetBattlePetMgr()->GetPet(battlePetSetFlags.PetGuid))
    {
        if (battlePetSetFlags.ControlType == FLAGS_CONTROL_TYPE_APPLY)
            pet->PacketInfo.Flags |= battlePetSetFlags.Flags;
        else // FLAGS_CONTROL_TYPE_REMOVE
            pet->PacketInfo.Flags &= ~battlePetSetFlags.Flags;

        if (pet->SaveInfo != BATTLE_PET_NEW)
            pet->SaveInfo = BATTLE_PET_CHANGED;
    }
}

void WorldSession::HandleCageBattlePet(WorldPackets::BattlePet::CageBattlePet& cageBattlePet)
{
    GetBattlePetMgr()->CageBattlePet(cageBattlePet.PetGuid);
}

void WorldSession::HandleBattlePetSummon(WorldPackets::BattlePet::BattlePetSummon& battlePetSummon)
{
    GetBattlePetMgr()->SummonPet(battlePetSummon.PetGuid);
}

void WorldSession::HandleUseCritterItem(WorldPackets::Item::UseCritterItem& useCritterItem)
{
    Item* item = _player->GetItemByGuid(useCritterItem.ItemGuid);
    if (!item)
        return;

    ItemToBattlePetSpeciesEntry const* itemToBattlePetSpecies = sItemToBattlePetSpeciesStore.LookupEntry(item->GetEntry());
    if (!itemToBattlePetSpecies)
        return;

    BattlePetSpeciesEntry const* battlePetSpecies = sBattlePetSpeciesStore.LookupEntry(itemToBattlePetSpecies->BattlePetSpeciesID);
    if (!battlePetSpecies)
        return;

    if (GetBattlePetMgr()->GetPetCount(battlePetSpecies->ID) >= MAX_BATTLE_PETS_PER_SPECIES)
    {
        GetBattlePetMgr()->SendError(BATTLEPETRESULT_CANT_HAVE_MORE_PETS_OF_THAT_TYPE, battlePetSpecies->CreatureID);
        return;
    }

    GetBattlePetMgr()->AddPet(battlePetSpecies->ID, battlePetSpecies->CreatureID, BattlePetMgr::RollPetBreed(battlePetSpecies->ID), BattlePetMgr::GetDefaultPetQuality(battlePetSpecies->ID));
    _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
}
