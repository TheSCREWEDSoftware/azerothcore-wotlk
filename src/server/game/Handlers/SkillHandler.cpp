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

#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "SpellMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::HandleLearnTalentOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire: a list of Talent.dbc ids
    uint32 talentCount = recvData.ReadBits(23);

    // one talent per tier
    if (talentCount > MAX_TALENT_TIERS)
    {
        recvData.rfinish();
        return;
    }

    bool learned = false;
    for (uint32 i = 0; i < talentCount; ++i)
    {
        uint16 talentId;
        recvData >> talentId;
        if (_player->LearnTalent(talentId))
            learned = true;
    }

    if (learned)
        _player->SendTalentsInfoData(false);
}

void WorldSession::HandleSetSpecializationOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire (CMSG_SET_PRIMARY_TALENT_TREE): the tab page of the specialization
    uint32 specializationTab;
    recvData >> specializationTab;

    if (specializationTab >= MAX_TALENT_TABS)
        return;

    if (!_player->LearnPrimaryTalentSpecialization(uint8(specializationTab)))
        return;

    _player->SendTalentsInfoData(false);
}

void WorldSession::HandleSetLootSpecializationOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire: 0 follows the current specialization
    uint32 lootSpecialization;
    recvData >> lootSpecialization;

    if (lootSpecialization)
    {
        ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(lootSpecialization);
        if (!specialization || specialization->ClassID != _player->getClass())
            return;
    }

    _player->SetUInt32Value(PLAYER_FIELD_LOOT_SPEC_ID, lootSpecialization);
}

void WorldSession::HandleLearnPreviewTalents(WorldPacket& recvPacket)
{
    // 4.3.4 talent preview; the 5.4.8 client sends CMSG_LEARN_TALENT instead
    LOG_DEBUG("network", "CMSG_LEARN_PREVIEW_TALENTS");
    recvPacket.rfinish();
}

void WorldSession::HandleTalentWipeConfirmOpcode(WorldPacket& recvData)
{
    LOG_DEBUG("network", "CMSG_CONFIRM_RESPEC_WIPE");
    // 5.4.8 18414, from Project SkyFire: 0 = talents, 1 = specialization
    uint8 respecType;
    recvData >> respecType;
    ObjectGuid guid;
    recvData.ReadGuidMask(guid, { 7, 2, 6, 1, 4, 0, 3, 5 });
    recvData.ReadGuidBytes(guid, { 2, 4, 1, 3, 0, 5, 7, 6 });
    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleTalentWipeConfirmOpcode - Unit ({}) not found or you can't interact with him.", guid.ToString());
        return;
    }

    if (!unit->CanResetTalents(_player))
        return;

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // 1: forget the specialization (the talents stay)
    if (respecType != 0)
    {
        if (!_player->GetSpecializationId(_player->GetActiveSpec()))
            return;

        _player->RemoveSpecializationSpells();
        _player->SetSpecializationId(_player->GetActiveSpec(), 0);
        _player->SendTalentsInfoData(false);
        unit->CastSpell(_player, 14867, true);              //spell: "Untalent Visual Effect"
        return;
    }

    if (!(_player->resetTalents()))
    {
        WorldPacket data = Player::BuildRespecWipeConfirm(ObjectGuid::Empty, 0); // you have no talents
        SendPacket(&data);
        return;
    }

    _player->SendTalentsInfoData(false);
    unit->CastSpell(_player, 14867, true);                  //spell: "Untalent Visual Effect"
}

void WorldSession::HandleUnlearnSkillOpcode(WorldPacket& recvData)
{
    uint32 skillId;
    recvData >> skillId;

    if (!IsPrimaryProfessionSkill(skillId))
        return;

    GetPlayer()->SetSkill(skillId, 0, 0, 0);
}
