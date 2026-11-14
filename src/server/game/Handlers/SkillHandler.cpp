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

#include "Creature.h"
#include "DBCStores.h"
#include "Log.h"
#include "LootPackets.h"
#include "Opcodes.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellPackets.h"
#include "TalentPackets.h"
#include "WorldSession.h"

void WorldSession::HandleLearnTalentsOpcode(WorldPackets::Talent::LearnTalents& packet)
{
    // one talent per tier
    if (packet.Talents.size() > MAX_TALENT_TIERS)
        return;

    bool anythingLearned = false;
    for (uint32 talentId : packet.Talents)
        if (_player->LearnTalent(talentId))
            anythingLearned = true;

    if (anythingLearned)
        _player->SendTalentsInfoData(false);
}

void WorldSession::HandleSetSpecializationOpcode(WorldPackets::Talent::SetSpecialization& packet)
{
    // SpecGroupIndex is the tab page of the specialization (ChrSpecialization OrderIndex)
    if (packet.SpecGroupIndex >= MAX_SPECIALIZATIONS)
    {
        LOG_DEBUG("network", "WORLD: HandleSetSpecializationOpcode - specialization index {} out of range", packet.SpecGroupIndex);
        return;
    }

    if (!_player->LearnPrimaryTalentSpecialization(uint8(packet.SpecGroupIndex)))
        return;

    _player->SendTalentsInfoData(false);
}

void WorldSession::HandleSetLootSpecialization(WorldPackets::Loot::SetLootSpecialization& packet)
{
    // 0 follows the current specialization
    if (packet.SpecID)
    {
        ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(packet.SpecID);
        if (!specialization || specialization->ClassID != _player->getClass())
            return;
    }

    _player->SetUInt32Value(PLAYER_FIELD_LOOT_SPEC_ID, packet.SpecID);
}

void WorldSession::HandleConfirmRespecWipeOpcode(WorldPackets::Talent::ConfirmRespecWipe& confirmRespecWipe)
{
    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(confirmRespecWipe.RespecMaster, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleConfirmRespecWipeOpcode - Unit ({}) not found or you can't interact with him.", confirmRespecWipe.RespecMaster.ToString());
        return;
    }

    if (!unit->CanResetTalents(_player))
        return;

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // 1: forget the specialization (the talents stay)
    if (confirmRespecWipe.RespecType != 0)
    {
        if (!_player->GetSpecializationId(_player->GetActiveSpec()))
            return;

        _player->RemoveSpecializationSpells();
        _player->SetSpecializationId(_player->GetActiveSpec(), 0);
        _player->SendTalentsInfoData(false);
        unit->CastSpell(_player, 14867, true);              //spell: "Untalent Visual Effect"
        return;
    }

    if (!_player->resetTalents())
    {
        _player->SendRespecWipeConfirm(ObjectGuid::Empty, 0); // you have no talents
        return;
    }

    _player->SendTalentsInfoData(false);
    unit->CastSpell(_player, 14867, true);                  //spell: "Untalent Visual Effect"
}

void WorldSession::HandleUnlearnSkillOpcode(WorldPackets::Spells::UnlearnSkill& packet)
{
    SkillRaceClassInfoEntry const* rcEntry = GetSkillRaceClassInfo(packet.SkillLine, _player->getRace(), _player->getClass());
    if (!rcEntry || !(rcEntry->Flags & SKILL_FLAG_UNLEARNABLE))
        return;

    _player->SetSkill(packet.SkillLine, 0, 0, 0);
}
