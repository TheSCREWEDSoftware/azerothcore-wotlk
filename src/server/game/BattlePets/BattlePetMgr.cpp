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

// 6.x battle pet journal, from TrinityCore 6.x (BattlePets/BattlePetMgr.cpp). The journal is kept per account
// (TrinityCore keeps it per battle.net account).

#include "BattlePetMgr.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Timer.h"
#include "WorldSession.h"

std::unordered_map<uint16 /*BreedID*/, std::unordered_map<uint32 /*state*/, int32 /*value*/>> BattlePetMgr::_battlePetBreedStates;
std::unordered_map<uint32 /*SpeciesID*/, std::unordered_map<uint32 /*state*/, int32 /*value*/>> BattlePetMgr::_battlePetSpeciesStates;
std::unordered_map<uint32 /*SpeciesID*/, std::unordered_set<uint8 /*breed*/>> BattlePetMgr::_availableBreedsPerSpecies;
std::unordered_map<uint32 /*SpeciesID*/, uint8 /*quality*/> BattlePetMgr::_defaultQualityPerSpecies;

void BattlePetMgr::BattlePet::CalculateStats()
{
    float health = 0.0f;
    float power = 0.0f;
    float speed = 0.0f;

    // get base breed stats
    auto breedState = _battlePetBreedStates.find(PacketInfo.Breed);
    if (breedState == _battlePetBreedStates.end()) // non existing breed id
        return;

    health = float(breedState->second[STATE_STAT_STAMINA]);
    power = float(breedState->second[STATE_STAT_POWER]);
    speed = float(breedState->second[STATE_STAT_SPEED]);

    // modify stats depending on species - not all pets have this
    auto speciesState = _battlePetSpeciesStates.find(PacketInfo.Species);
    if (speciesState != _battlePetSpeciesStates.end())
    {
        health += float(speciesState->second[STATE_STAT_STAMINA]);
        power += float(speciesState->second[STATE_STAT_POWER]);
        speed += float(speciesState->second[STATE_STAT_SPEED]);
    }

    // modify stats by quality
    for (BattlePetBreedQualityEntry const* breedQuality : sBattlePetBreedQualityStore)
    {
        if (breedQuality->Quality == PacketInfo.Quality)
        {
            health *= breedQuality->Modifier;
            power *= breedQuality->Modifier;
            speed *= breedQuality->Modifier;
            break;
        }
    }

    // scale stats depending on level
    health *= PacketInfo.Level;
    power *= PacketInfo.Level;
    speed *= PacketInfo.Level;

    // set stats
    PacketInfo.MaxHealth = uint32((std::round(health / 20) + 100));
    PacketInfo.Power = uint32(std::round(power / 100));
    PacketInfo.Speed = uint32(std::round(speed / 100));
}

void BattlePetMgr::Initialize()
{
    uint32 oldMSTime = getMSTime();

    if (QueryResult result = LoginDatabase.Query("SELECT MAX(guid) FROM battle_pets"))
        sObjectMgr->GetGenerator<HighGuid::BattlePet>().Set((*result)[0].Get<uint64>() + 1);

    for (BattlePetBreedStateEntry const* breedState : sBattlePetBreedStateStore)
        _battlePetBreedStates[breedState->BreedID][breedState->State] = breedState->Value;

    for (BattlePetSpeciesStateEntry const* speciesState : sBattlePetSpeciesStateStore)
        _battlePetSpeciesStates[speciesState->SpeciesID][speciesState->State] = speciesState->Value;

    LoadAvailablePetBreeds();
    LoadDefaultPetQualities();

    LOG_INFO("server.loading", ">> Loaded battle pet data in {} ms", GetMSTimeDiffToNow(oldMSTime));
}

void BattlePetMgr::LoadAvailablePetBreeds()
{
    QueryResult result = WorldDatabase.Query("SELECT speciesId, breedId FROM battle_pet_breeds");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 battle pet breeds. DB table `battle_pet_breeds` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 speciesId = fields[0].Get<uint32>();
        uint16 breedId = fields[1].Get<uint16>();

        if (!sBattlePetSpeciesStore.LookupEntry(speciesId))
        {
            LOG_ERROR("sql.sql", "Non-existing BattlePetSpecies.db2 entry {} was referenced in `battle_pet_breeds` by row ({}, {}).", speciesId, speciesId, breedId);
            continue;
        }

        _availableBreedsPerSpecies[speciesId].insert(uint8(breedId));
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} battle pet breeds.", count);
}

void BattlePetMgr::LoadDefaultPetQualities()
{
    QueryResult result = WorldDatabase.Query("SELECT speciesId, quality FROM battle_pet_quality");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 battle pet qualities. DB table `battle_pet_quality` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 speciesId = fields[0].Get<uint32>();
        uint8 quality = fields[1].Get<uint8>();

        if (!sBattlePetSpeciesStore.LookupEntry(speciesId))
        {
            LOG_ERROR("sql.sql", "Non-existing BattlePetSpecies.db2 entry {} was referenced in `battle_pet_quality` by row ({}, {}).", speciesId, speciesId, quality);
            continue;
        }

        _defaultQualityPerSpecies[speciesId] = quality;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} battle pet qualities.", uint32(_defaultQualityPerSpecies.size()));
}

uint16 BattlePetMgr::RollPetBreed(uint32 species)
{
    auto itr = _availableBreedsPerSpecies.find(species);
    if (itr == _availableBreedsPerSpecies.end() || itr->second.empty())
        return 3; // default B/B

    return Acore::Containers::SelectRandomContainerElement(itr->second);
}

uint8 BattlePetMgr::GetDefaultPetQuality(uint32 species)
{
    auto itr = _defaultQualityPerSpecies.find(species);
    if (itr == _defaultQualityPerSpecies.end())
        return 0; // default poor

    return itr->second;
}

BattlePetMgr::BattlePetMgr(WorldSession* owner) : _owner(owner)
{
    for (uint8 i = 0; i < MAX_PET_BATTLE_SLOTS; ++i)
    {
        WorldPackets::BattlePet::BattlePetSlot slot;
        slot.Index = i;
        _slots.push_back(slot);
    }
}

void BattlePetMgr::LoadFromDB(PreparedQueryResult pets, PreparedQueryResult slots)
{
    if (pets)
    {
        do
        {
            Field* fields = pets->Fetch();
            uint32 species = fields[1].Get<uint32>();

            if (BattlePetSpeciesEntry const* speciesEntry = sBattlePetSpeciesStore.LookupEntry(species))
            {
                if (GetPetCount(species) >= MAX_BATTLE_PETS_PER_SPECIES)
                {
                    LOG_ERROR("misc", "Account {} has more than 3 battle pets of species {}", _owner->GetBattlenetAccountId(), species);
                    continue;
                }

                BattlePet pet;
                pet.PacketInfo.Guid = ObjectGuid::Create<HighGuid::BattlePet>(fields[0].Get<uint64>());
                pet.PacketInfo.Species = species;
                pet.PacketInfo.Breed = fields[2].Get<uint16>();
                pet.PacketInfo.Level = fields[3].Get<uint16>();
                pet.PacketInfo.Exp = fields[4].Get<uint16>();
                pet.PacketInfo.Health = fields[5].Get<uint32>();
                pet.PacketInfo.Quality = fields[6].Get<uint8>();
                pet.PacketInfo.Flags = fields[7].Get<uint16>();
                pet.PacketInfo.Name = fields[8].Get<std::string>();
                pet.PacketInfo.CreatureID = speciesEntry->CreatureID;
                pet.SaveInfo = BATTLE_PET_UNCHANGED;
                pet.CalculateStats();
                _pets[pet.PacketInfo.Guid.GetCounter()] = pet;
            }
        } while (pets->NextRow());
    }

    if (slots)
    {
        do
        {
            Field* fields = slots->Fetch();
            uint8 index = fields[0].Get<uint8>();
            if (index >= _slots.size())
                continue;

            _slots[index].Index = index;
            auto itr = _pets.find(fields[1].Get<uint64>());
            if (itr != _pets.end())
                _slots[index].Pet = itr->second.PacketInfo;
            _slots[index].Locked = fields[2].Get<bool>();
        } while (slots->NextRow());
    }
}

void BattlePetMgr::SaveToDB(LoginDatabaseTransaction trans)
{
    LoginDatabasePreparedStatement* stmt = nullptr;

    for (auto itr = _pets.begin(); itr != _pets.end();)
    {
        switch (itr->second.SaveInfo)
        {
            case BATTLE_PET_NEW:
                stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BATTLE_PETS);
                stmt->SetData(0, itr->first);
                stmt->SetData(1, _owner->GetBattlenetAccountId());
                stmt->SetData(2, itr->second.PacketInfo.Species);
                stmt->SetData(3, itr->second.PacketInfo.Breed);
                stmt->SetData(4, itr->second.PacketInfo.Level);
                stmt->SetData(5, itr->second.PacketInfo.Exp);
                stmt->SetData(6, itr->second.PacketInfo.Health);
                stmt->SetData(7, itr->second.PacketInfo.Quality);
                stmt->SetData(8, itr->second.PacketInfo.Flags);
                stmt->SetData(9, itr->second.PacketInfo.Name);
                trans->Append(stmt);
                itr->second.SaveInfo = BATTLE_PET_UNCHANGED;
                ++itr;
                break;
            case BATTLE_PET_CHANGED:
                stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_BATTLE_PETS);
                stmt->SetData(0, itr->second.PacketInfo.Level);
                stmt->SetData(1, itr->second.PacketInfo.Exp);
                stmt->SetData(2, itr->second.PacketInfo.Health);
                stmt->SetData(3, itr->second.PacketInfo.Quality);
                stmt->SetData(4, itr->second.PacketInfo.Flags);
                stmt->SetData(5, itr->second.PacketInfo.Name);
                stmt->SetData(6, _owner->GetBattlenetAccountId());
                stmt->SetData(7, itr->first);
                trans->Append(stmt);
                itr->second.SaveInfo = BATTLE_PET_UNCHANGED;
                ++itr;
                break;
            case BATTLE_PET_REMOVED:
                stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_BATTLE_PETS);
                stmt->SetData(0, _owner->GetBattlenetAccountId());
                stmt->SetData(1, itr->first);
                trans->Append(stmt);
                itr = _pets.erase(itr);
                break;
            default:
                ++itr;
                break;
        }
    }

    stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_BATTLE_PET_SLOTS);
    stmt->SetData(0, _owner->GetBattlenetAccountId());
    trans->Append(stmt);

    for (WorldPackets::BattlePet::BattlePetSlot const& slot : _slots)
    {
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_BATTLE_PET_SLOTS);
        stmt->SetData(0, slot.Index);
        stmt->SetData(1, _owner->GetBattlenetAccountId());
        stmt->SetData(2, slot.Pet.Guid.GetCounter());
        stmt->SetData(3, slot.Locked);
        trans->Append(stmt);
    }
}

BattlePetMgr::BattlePet* BattlePetMgr::GetPet(ObjectGuid guid)
{
    auto itr = _pets.find(guid.GetCounter());
    if (itr != _pets.end() && itr->second.SaveInfo != BATTLE_PET_REMOVED)
        return &itr->second;

    return nullptr;
}

void BattlePetMgr::AddPet(uint32 species, uint32 creatureId, uint16 breed, uint8 quality, uint16 level /*= 1*/)
{
    BattlePetSpeciesEntry const* battlePetSpecies = sBattlePetSpeciesStore.LookupEntry(species);
    if (!battlePetSpecies) // should never happen
        return;

    BattlePet pet;
    pet.PacketInfo.Guid = ObjectGuid::Create<HighGuid::BattlePet>(sObjectMgr->GetGenerator<HighGuid::BattlePet>().Generate());
    pet.PacketInfo.Species = species;
    pet.PacketInfo.CreatureID = creatureId;
    pet.PacketInfo.Level = level;
    pet.PacketInfo.Exp = 0;
    pet.PacketInfo.Flags = 0;
    pet.PacketInfo.Breed = breed;
    pet.PacketInfo.Quality = quality;
    pet.PacketInfo.Name = "";
    pet.CalculateStats();
    pet.PacketInfo.Health = pet.PacketInfo.MaxHealth;
    pet.SaveInfo = BATTLE_PET_NEW;

    _pets[pet.PacketInfo.Guid.GetCounter()] = pet;

    SendUpdates({ pet }, true);

    if (Player* player = _owner->GetPlayer())
        player->UpdateAchievementCriteria(AchievementCriteriaTypes(CRITERIA_TYPE_OWN_BATTLE_PET), species);
}

void BattlePetMgr::RemovePet(ObjectGuid guid)
{
    BattlePet* pet = GetPet(guid);
    if (!pet)
        return;

    pet->SaveInfo = BATTLE_PET_REMOVED;

    // a slotted pet leaves its slot
    for (WorldPackets::BattlePet::BattlePetSlot& slot : _slots)
        if (slot.Pet.Guid == guid)
            slot.Pet = WorldPackets::BattlePet::BattlePet();

    // spell is not unlearned on retail
}

uint8 BattlePetMgr::GetPetCount(uint32 species) const
{
    uint8 count = 0;
    for (auto const& [guid, pet] : _pets)
        if (pet.PacketInfo.Species == species && pet.SaveInfo != BATTLE_PET_REMOVED)
            ++count;

    return count;
}

void BattlePetMgr::UnlockSlot(uint8 slot)
{
    if (slot >= _slots.size() || !_slots[slot].Locked)
        return;

    _slots[slot].Locked = false;

    WorldPackets::BattlePet::PetBattleSlotUpdates updates;
    updates.Slots.push_back(_slots[slot]);
    updates.AutoSlotted = false;
    updates.NewSlot = true;                                 // causes the "new slot unlocked" bubble to appear
    _owner->SendPacket(updates.Write());
}

std::vector<BattlePetMgr::BattlePet> BattlePetMgr::GetLearnedPets() const
{
    std::vector<BattlePet> pets;
    for (auto const& [guid, pet] : _pets)
        if (pet.SaveInfo != BATTLE_PET_REMOVED)
            pets.push_back(pet);

    return pets;
}

void BattlePetMgr::CageBattlePet(ObjectGuid guid)
{
    BattlePet* pet = GetPet(guid);
    if (!pet)
        return;

    Player* player = _owner->GetPlayer();
    if (!player)
        return;

    ItemPosCountVec dest;
    if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, BATTLE_PET_CAGE_ITEM_ID, 1) != EQUIP_ERR_OK)
        return;

    Item* item = player->StoreNewItem(dest, BATTLE_PET_CAGE_ITEM_ID, true);
    if (!item)
        return;

    item->SetModifier(ITEM_MODIFIER_BATTLE_PET_SPECIES_ID, pet->PacketInfo.Species);
    item->SetModifier(ITEM_MODIFIER_BATTLE_PET_BREED_DATA, pet->PacketInfo.Breed | (pet->PacketInfo.Quality << 24));
    item->SetModifier(ITEM_MODIFIER_BATTLE_PET_LEVEL, pet->PacketInfo.Level);
    item->SetModifier(ITEM_MODIFIER_BATTLE_PET_DISPLAY_ID, pet->PacketInfo.CreatureID);

    player->SendNewItem(item, 1, true, true);

    RemovePet(guid);

    WorldPackets::BattlePet::BattlePetDeleted deletePet;
    deletePet.PetGuid = guid;
    _owner->SendPacket(deletePet.Write());
}

void BattlePetMgr::HealBattlePetsPct(uint8 pct)
{
    std::vector<BattlePet> updates;

    for (auto& [guid, pet] : _pets)
    {
        if (pet.SaveInfo == BATTLE_PET_REMOVED || pet.PacketInfo.Health == pet.PacketInfo.MaxHealth)
            continue;

        pet.PacketInfo.Health += CalculatePct(pet.PacketInfo.MaxHealth, pct);
        // don't allow Health to be greater than MaxHealth
        pet.PacketInfo.Health = std::min(pet.PacketInfo.Health, pet.PacketInfo.MaxHealth);
        if (pet.SaveInfo != BATTLE_PET_NEW)
            pet.SaveInfo = BATTLE_PET_CHANGED;
        updates.push_back(pet);
    }

    SendUpdates(updates, false);
}

void BattlePetMgr::SummonPet(ObjectGuid guid)
{
    BattlePet* pet = GetPet(guid);
    if (!pet)
        return;

    BattlePetSpeciesEntry const* speciesEntry = sBattlePetSpeciesStore.LookupEntry(pet->PacketInfo.Species);
    if (!speciesEntry)
        return;

    Player* player = _owner->GetPlayer();
    if (!player)
        return;

    // TODO: set proper CreatureID for spell DEFAULT_SUMMON_BATTLE_PET_SPELL (default EffectMiscValueA is 40721 - Murkimus the Gladiator)
    player->CastSpell(player, speciesEntry->SummonSpellID ? speciesEntry->SummonSpellID : uint32(DEFAULT_SUMMON_BATTLE_PET_SPELL));
}

void BattlePetMgr::SendUpdates(std::vector<BattlePet> const& pets, bool petAdded)
{
    WorldPackets::BattlePet::BattlePetUpdates updates;
    for (BattlePet const& pet : pets)
        updates.Pets.push_back(pet.PacketInfo);

    updates.PetAdded = petAdded;
    _owner->SendPacket(updates.Write());
}

void BattlePetMgr::SendError(BattlePetError error, uint32 creatureId)
{
    WorldPackets::BattlePet::BattlePetError battlePetError;
    battlePetError.Result = error;
    battlePetError.CreatureID = creatureId;
    _owner->SendPacket(battlePetError.Write());
}
