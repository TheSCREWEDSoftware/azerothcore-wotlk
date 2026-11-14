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

#include "Archaeology.h"
#include "DBCStores.h"
#include "GameObjectAI.h"
#include "Guild.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "MovementPackets.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellCastRequest.h"
#include "SpellMgr.h"
#include "SpellPackets.h"
#include "GameObjectPackets.h"
#include "Totem.h"
#include "TotemPackets.h"
#include "Vehicle.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::ApplyCastRequestResearchData(SpellCastRequest const& request)
{
    // Archaeology: fragment/keystone reagent substitution data
    if (request.hasResearchData)
        if (Player* player = GetPlayer())
            player->SetArchData(request.researchData);
}

void WorldSession::HandleUseItemOpcode(WorldPackets::Spells::UseItem& packet)
{
    /// @todo: add targets.read() check
    Player* pUser = _player;

    // ignore for remote control state
    if (pUser->m_mover != pUser)
        return;

    SpellCastRequest request = SpellCastRequest::FromPacket(packet.Cast);
    uint8 bagIndex = packet.PackSlot;
    uint8 slot = packet.Slot;
    uint8 castCount = request.castCount;                    // next cast if exists (single or not)
    ObjectGuid itemGUID = packet.CastItem;
    uint32 glyphIndex = request.glyphIndex;                 // something to do with glyphs?
    uint32 spellId = request.spellId;                       // casted spell id

    if (glyphIndex >= MAX_GLYPH_SLOT_INDEX)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
        return;
    }

    Item* pItem = pUser->GetUseableItemByPos(bagIndex, slot);
    if (!pItem)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
        return;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

    if (!spellInfo)
    {
        LOG_ERROR("network.opcode", "WORLD: unknown spell id {}", spellId);
        return;
    }

    // fail if we are cancelling pending request
    if (!_player->SpellQueue.empty())
    {
        PendingSpellCastRequest& request = _player->SpellQueue.front(); // Peek at the first spell
        if (request.cancelInProgress)
        {
            pUser->SendEquipError(EQUIP_ERR_NONE, pItem, nullptr);
                return;
        }
    }

    // try queue spell if it can't be executed right now
    if (!_player->CanExecutePendingSpellCastRequest(spellInfo))
        if (_player->CanRequestSpellCast(spellInfo))
        {
            WorldPacket packetCopy(*packet.GetRawPacket()); // Copy the packet
            packetCopy.rpos(0); // Reset read position to the start of the buffer.
            _player->SpellQueue.emplace_back(
                spellId,
                spellInfo->GetCategory(),
                std::move(packetCopy), // Move ownership of copied packet
                true // itemCast
            );
            return;
        }

    if (pItem->GetGUID() != itemGUID)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
        return;
    }

    LOG_DEBUG("network", "WORLD: CMSG_USE_ITEM packet, bagIndex: {}, slot: {}, castCount: {}, spellId: {}, Item: {}, glyphIndex: {}", bagIndex, slot, castCount, spellId, pItem->GetEntry(), glyphIndex);

    ItemTemplate const* proto = pItem->GetTemplate();
    if (!proto)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, nullptr);
        return;
    }

    // some item classes can be used only in equipped state
    if (proto->InventoryType != INVTYPE_NON_EQUIP && !pItem->IsEquipped())
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, nullptr);
        return;
    }

    InventoryResult msg = pUser->CanUseItem(pItem);
    if (msg != EQUIP_ERR_OK)
    {
        pUser->SendEquipError(msg, pItem, nullptr);
        return;
    }

    // only allow conjured consumable, bandage, poisons (all should have the 2^21 item flag set in DB)
    if (proto->Class == ITEM_CLASS_CONSUMABLE && !proto->HasFlag(ITEM_FLAG_IGNORE_DEFAULT_ARENA_RESTRICTIONS) && pUser->InArena())
    {
        pUser->SendEquipError(EQUIP_ERR_NOT_DURING_ARENA_MATCH, pItem, nullptr);
        return;
    }

    // don't allow items banned in arena
    if (proto->HasFlag(ITEM_FLAG_NOT_USEABLE_IN_ARENA) && pUser->InArena())
    {
        pUser->SendEquipError(EQUIP_ERR_NOT_DURING_ARENA_MATCH, pItem, nullptr);
        return;
    }

    if (pUser->IsInCombat())
    {
        for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Spells[i].SpellId))
            {
                if (!spellInfo->CanBeUsedInCombat())
                {
                    pUser->SendEquipError(EQUIP_ERR_NOT_IN_COMBAT, pItem, nullptr);
                    return;
                }
            }
        }
    }

    // check also  BIND_WHEN_PICKED_UP and BIND_QUEST_ITEM for .additem or .additemset case by GM (not binded at adding to inventory)
    if (pItem->GetTemplate()->Bonding == BIND_WHEN_USE || pItem->GetTemplate()->Bonding == BIND_WHEN_PICKED_UP || pItem->GetTemplate()->Bonding == BIND_QUEST_ITEM)
    {
        if (!pItem->IsSoulBound())
        {
            pItem->SetState(ITEM_CHANGED, pUser);
            pItem->SetBinding(true);
        }
    }

    SpellCastTargets targets;
    request.FillTargets(targets, pUser);
    ApplyCastRequestResearchData(request);

    // Note: If script stop casting it must send appropriate data to client to prevent stuck item in gray state.
    if (!sScriptMgr->OnItemUse(pUser, pItem, targets))
    {
        // no script or script not process request by self
        pUser->CastItemUseSpell(pItem, targets, castCount, glyphIndex);
    }
}

void WorldSession::HandleOpenItemOpcode(WorldPackets::Spells::OpenItem& packet)
{
    Player* pUser = _player;

    // ignore for remote control state
    if (pUser->m_mover != pUser)
        return;

    uint8 bagIndex = packet.PackSlot;
    uint8 slot = packet.Slot;

    LOG_DEBUG("network.opcode", "bagIndex: {}, slot: {}", bagIndex, slot);

    Item* item = pUser->GetItemByPos(bagIndex, slot);
    if (!item)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
        return;
    }

    // The client locks the item when it sends CMSG_OPEN_ITEM and only unlocks it on a loot
    // response for that item's guid, so a plain equip error would leave it grayed out until /reload.
    if (!pUser->IsAlive())
    {
        pUser->SendEquipError(EQUIP_ERR_YOU_ARE_DEAD, item, nullptr);
        pUser->SendLootRelease(item->GetGUID());
        return;
    }

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, item, nullptr);
        return;
    }

    // Verify that the bag is an actual bag or wrapped item that can be used "normally"
    if (!proto->HasFlag(ITEM_FLAG_HAS_LOOT) && !item->IsWrapped())
    {
        pUser->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW, item, nullptr);
        LOG_ERROR("network.opcode", "Possible hacking attempt: Player {} [{}] tried to open item [{}, entry: {}] which is not openable!",
                       pUser->GetName(), pUser->GetGUID().ToString(), item->GetGUID().ToString(), proto->ItemId);
        return;
    }

    // locked item
    uint32 lockId = proto->LockID;
    if (lockId)
    {
        LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);

        if (!lockInfo)
        {
            pUser->SendEquipError(EQUIP_ERR_ITEM_LOCKED, item, nullptr);
            LOG_ERROR("network.opcode", "WORLD::OpenItem: item [{}] has an unknown lockId: {}!", item->GetGUID().ToString(), lockId);
            return;
        }

        // was not unlocked yet
        if (item->IsLocked())
        {
            pUser->SendEquipError(EQUIP_ERR_ITEM_LOCKED, item, nullptr);
            return;
        }
    }

    if (sScriptMgr->OnPlayerBeforeOpenItem(pUser, item))
    {
        if (item->IsWrapped())// wrapped?
        {
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GIFT_BY_ITEM);
            stmt->SetData(0, item->GetGUID().GetCounter());
            _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt)
                .WithPreparedCallback(std::bind(&WorldSession::HandleOpenWrappedItemCallback, this, bagIndex, slot, item->GetGUID().GetCounter(), std::placeholders::_1)));
        }
        else
        {
            pUser->SendLoot(item->GetGUID(), LOOT_CORPSE);
        }
    }
}

void WorldSession::HandleOpenWrappedItemCallback(uint8 bagIndex, uint8 slot, ObjectGuid::LowType itemLowGUID, PreparedQueryResult result)
{
    if (!GetPlayer())
        return;

    Item* item = GetPlayer()->GetItemByPos(bagIndex, slot);
    if (!item)
        return;

    if (item->GetGUID().GetCounter() != itemLowGUID || !item->IsWrapped()) // during getting result, gift was swapped with another item
        return;

    if (!result)
    {
        LOG_ERROR("network", "Wrapped item {} don't have record in character_gifts table and will deleted", item->GetGUID().ToString());
        GetPlayer()->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        return;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    Field* fields = result->Fetch();
    uint32 entry = fields[0].Get<uint32>();
    uint32 flags = fields[1].Get<uint32>();

    item->SetGuidValue(ITEM_FIELD_GIFTCREATOR, ObjectGuid::Empty);
    item->SetEntry(entry);
    item->SetUInt32Value(ITEM_FIELD_FLAGS, flags);
    item->SetUInt32Value(ITEM_FIELD_MAXDURABILITY, item->GetTemplate()->MaxDurability);

    item->SetState(ITEM_CHANGED, GetPlayer());
    GetPlayer()->SaveInventoryAndGoldToDB(trans);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GIFT);
    stmt->SetData(0, item->GetGUID().GetCounter());
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandleGameObjectUseOpcode(WorldPackets::GameObject::GameObjUse& packet)
{
    ObjectGuid guid = packet.Guid;

    LOG_DEBUG("network", "WORLD: Recvd CMSG_GAMEOBJ_USE Message [{}]", guid.ToString());

    if (GameObject* obj = GetPlayer()->GetMap()->GetGameObject(guid))
    {
        // despawned objects cannot be used (they are still visible and clickable for gamemasters)
        if (!obj->isSpawned())
            return;

        if (!obj->IsWithinDistInMap(GetPlayer(), obj->GetInteractionDistance()))
            return;

        // ignore for remote control state
        if (GetPlayer()->m_mover != GetPlayer())
            if (!(GetPlayer()->IsOnVehicle(GetPlayer()->m_mover) || GetPlayer()->IsMounted()) && !obj->GetGOInfo()->IsUsableMounted())
                return;

        obj->Use(GetPlayer());
    }
}

void WorldSession::HandleGameobjectReportUse(WorldPackets::GameObject::GameObjReportUse& packet)
{
    ObjectGuid guid = packet.Guid;

    LOG_DEBUG("network", "WORLD: Recvd CMSG_GAMEOBJ_REPORT_USE Message [{}]", guid.ToString());

    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    GameObject* go = GetPlayer()->GetMap()->GetGameObject(guid);
    if (!go)
        return;

    // despawned objects cannot be used (they are still visible and clickable for gamemasters)
    if (!go->isSpawned())
        return;

    // Prevent use of GameObject if it is not selectable. Fixes hack.
    if (go->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE))
        return;

    if (!go->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
        return;

    if (go->AI()->GossipHello(_player, true))
        return;

    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT, go->GetEntry());
}

void WorldSession::HandleCastSpellOpcode(WorldPackets::Spells::CastSpell& castRequest)
{
    SpellCastRequest request = SpellCastRequest::FromPacket(castRequest.Cast);
    uint32 spellId = request.spellId;
    uint8 castCount = request.castCount;
    uint8 castFlags = request.castFlags;
    TriggerCastFlags triggerFlag = TRIGGERED_NONE;

    uint32 oldSpellId = spellId;

    LOG_DEBUG("network", "WORLD: got cast spell packet, castCount: {}, spellId: {}, castFlags: {}", castCount, spellId, castFlags);

    // ignore for remote control state (for player case)
    Unit* mover = _player->m_mover;
    if (mover != _player && mover->IsPlayer())
    {
        return;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

    if (!spellInfo)
    {
        LOG_ERROR("network.opcode", "WORLD: unknown spell id {}", spellId);
        return;
    }

    // fail if we are cancelling pending request
    if (!_player->SpellQueue.empty())
    {
        PendingSpellCastRequest& request = _player->SpellQueue.front(); // Peek at the first spell
        if (request.cancelInProgress)
        {
            Spell* spell = new Spell(_player, spellInfo, TRIGGERED_NONE);
            spell->m_cast_count = castCount;
            spell->SendCastResult(SPELL_FAILED_DONT_REPORT);
            spell->finish(false);
            return;
        }
    }

    // try queue spell if it can't be executed right now
    if (!_player->CanExecutePendingSpellCastRequest(spellInfo))
    {
        if (_player->CanRequestSpellCast(spellInfo))
        {
            WorldPacket packetCopy(*castRequest.GetRawPacket()); // Copy the packet
            packetCopy.rpos(0); // Reset read position to the start of the buffer.
            _player->SpellQueue.emplace_back(
                spellId,
                spellInfo->GetCategory(),
                std::move(packetCopy) // Move ownership of copied packet
            );
            return;
        }
    }

    // client provided targets
    SpellCastTargets targets;
    request.FillTargets(targets, mover);
    ApplyCastRequestResearchData(request);

    // not have spell in spellbook
    if (mover->IsPlayer())
    {
        // not have spell in spellbook or spell passive and not casted by client
        if (!(spellInfo->Targets & TARGET_FLAG_GAMEOBJECT_ITEM) && (!mover->ToPlayer()->HasActiveSpell(spellId) || spellInfo->IsPassive()))
        {
            // artifact solve spells are not in the spellbook
            bool allow = !spellInfo->IsPassive() && spellInfo->ResearchProjectId && mover->ToPlayer()->HasArchProject(uint16(spellInfo->ResearchProjectId));

            // allow casting of unknown spells for special lock cases
            if (GameObject* go = targets.GetGOTarget())
            {
                if (go->GetSpellForLock(mover->ToPlayer()) == spellInfo)
                {
                    allow = true;
                }
            }

            /// @todo: Preparation for #23204
            // allow casting of spells triggered by clientside periodic trigger auras
            /*
             if (caster->HasAuraTypeWithTriggerSpell(SPELL_AURA_PERIODIC_TRIGGER_SPELL_FROM_CLIENT, spellId))
            {
                allow = true;
                triggerFlag = TRIGGERED_FULL_MASK;
            }
            */

            if (!allow)
                return;
        }
    }
    else
    {
        // pussywizard: casting player's spells from vehicle when seat allows it
        // if ANYTHING CHANGES in this function, INFORM ME BEFORE applying!!!
        if (Vehicle* veh = mover->GetVehicleKit())
            if (VehicleSeatEntry const* seat = veh->GetSeatForPassenger(_player))
                if (seat->Flags & VEHICLE_SEAT_FLAG_CAN_ATTACK || spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_OPEN_LOCK /*allow looting from vehicle, but only if player has required spell (all necessary opening spells are in playercreateinfo_spell)*/)
                    if ((mover->IsCreature() && !mover->ToCreature()->HasSpell(spellId)) || spellInfo->IsPassive()) // the creature can't cast that spell, check player instead
                    {
                        if (!(spellInfo->Targets & TARGET_FLAG_GAMEOBJECT_ITEM) && (!_player->HasActiveSpell (spellId) || spellInfo->IsPassive()))
                        {
                            //cheater? kick? ban?
                            return;
                        }

                        // at this point, player is a valid caster
                        // swapping the mover will stop the check below at IsUnit, so everything works fine
                        mover = _player;
                    }

        // not have spell in spellbook or spell passive and not casted by client
        if ((mover->IsCreature() && !mover->ToCreature()->HasSpell(spellId)) || spellInfo->IsPassive())
        {
            //cheater? kick? ban?
            return;
        }
    }

    sScriptMgr->ValidateSpellAtCastSpell(_player, oldSpellId, spellId, castCount, castFlags);

    if (oldSpellId != spellId)
        spellInfo = sSpellMgr->GetSpellInfo(spellId);

    // Client is resending autoshot cast opcode when other spell is casted during shoot rotation
    // Skip it to prevent "interrupt" message
    if (spellInfo->IsAutoRepeatRangedSpell() && _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)
            && _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)->m_spellInfo == spellInfo)
    {
        return;
    }

    // can't use our own spells when we're in possession of another unit,
    if (_player->isPossessing())
    {
        return;
    }

    // pussywizard: caster not in map will crash at GetMap() for spell difficulty in Spell constructor.
    if (!mover->FindMap())
    {
        return;
    }

    // auto-selection buff level base at target level (in spellInfo)
    if (spellInfo->IsPositive())
        if (Unit* target = targets.GetUnitTarget())
            if (mover->IsFriendlyTo(target))
            {
                SpellInfo const* actualSpellInfo = spellInfo->GetAuraRankForLevel(target->GetLevel());

                // if rank not found then function return nullptr but in explicit cast case original spell can be casted and later failed with appropriate error message
                if (actualSpellInfo)
                    spellInfo = actualSpellInfo;
            }

    Spell* spell = new Spell(mover, spellInfo, triggerFlag, ObjectGuid::Empty, false);

    sScriptMgr->ValidateSpellAtCastSpellResult(_player, mover, spell, oldSpellId, spellId);

    spell->m_cast_count = castCount;                       // set count of casts
    spell->prepare(&targets);
}

void WorldSession::HandleCancelCastOpcode(WorldPackets::Spells::CancelCast& packet)
{
    _player->SpellQueue.clear();

    _player->InterruptSpell(CURRENT_MELEE_SPELL);
    if (_player->IsNonMeleeSpellCast(false))
        _player->InterruptNonMeleeSpells(false, packet.SpellID, false, true);
}

void WorldSession::HandleCancelAuraOpcode(WorldPackets::Spells::CancelAura& cancelAura)
{
    uint32 spellId = uint32(cancelAura.SpellID);

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return;

    // not allow remove spells with attr SPELL_ATTR0_NO_AURA_CANCEL
    if (spellInfo->HasAttribute(SPELL_ATTR0_NO_AURA_CANCEL))
    {
        return;
    }

    // channeled spell case (it currently casted then)
    if (spellInfo->IsChanneled())
    {
        if (Spell* curSpell = _player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (curSpell->m_spellInfo->Id == spellId)
                _player->InterruptSpell(CURRENT_CHANNELED_SPELL);
        return;
    }

    // non channeled case:
    // don't allow remove non positive spells
    // don't allow cancelling passive auras (some of them are visible)
    if (!spellInfo->IsPositive() || spellInfo->IsPassive())
    {
        return;
    }

    // 6.x names the caster of the aura to cancel (TC 6.x)
    _player->RemoveOwnedAura(spellId, cancelAura.CasterGUID, 0, AURA_REMOVE_BY_CANCEL);
}

void WorldSession::HandlePetCancelAuraOpcode(WorldPackets::Spells::PetCancelAura& packet)
{
    ObjectGuid guid = packet.PetGUID;
    uint32 spellId = packet.SpellID;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        LOG_ERROR("network.opcode", "WORLD: unknown PET spell id {}", spellId);
        return;
    }

    Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!pet)
    {
        LOG_ERROR("network.opcode", "HandlePetCancelAura: Attempt to cancel an aura for non-existant pet {} by player {}", guid.ToString(), GetPlayer()->GetName());
        return;
    }

    if (pet != GetPlayer()->GetGuardianPet() && pet != GetPlayer()->GetCharm())
    {
        LOG_ERROR("network.opcode", "HandlePetCancelAura: Pet {} is not a pet of player {}", guid.ToString(), GetPlayer()->GetName());
        return;
    }

    if (!pet->IsAlive())
    {
        pet->SendPetActionFeedback(FEEDBACK_PET_DEAD);
        return;
    }

    pet->RemoveOwnedAura(spellId, ObjectGuid::Empty, 0, AURA_REMOVE_BY_CANCEL);
}

// removes the player's positive, cancelable, non passive auras of a type (TC 6.x CMSG_CANCEL_GROWTH_AURA / CMSG_CANCEL_MOUNT_AURA)
static void RemoveCancelableAurasByType(Player* player, AuraType auraType)
{
    std::vector<std::pair<uint32, ObjectGuid>> auras;
    for (AuraEffect const* aurEff : player->GetAuraEffectsByType(auraType))
    {
        SpellInfo const* spellInfo = aurEff->GetSpellInfo();
        if (spellInfo->HasAttribute(SPELL_ATTR0_NO_AURA_CANCEL) || !spellInfo->IsPositive() || spellInfo->IsPassive())
            continue;

        auras.emplace_back(spellInfo->Id, aurEff->GetCasterGUID());
    }

    for (auto const& [spellId, casterGuid] : auras)
        player->RemoveOwnedAura(spellId, casterGuid, 0, AURA_REMOVE_BY_CANCEL);
}

void WorldSession::HandleCancelGrowthAuraOpcode(WorldPackets::Spells::CancelGrowthAura& /*cancelGrowthAura*/)
{
    RemoveCancelableAurasByType(_player, SPELL_AURA_MOD_SCALE);
}

void WorldSession::HandleCancelMountAuraOpcode(WorldPackets::Spells::CancelMountAura& /*cancelMountAura*/)
{
    RemoveCancelableAurasByType(_player, SPELL_AURA_MOUNTED);
}

void WorldSession::HandleCancelAutoRepeatSpellOpcode(WorldPackets::Spells::CancelAutoRepeatSpell& /*cancelAutoRepeatSpell*/)
{
    // may be better send SMSG_CANCEL_AUTO_REPEAT?
    // cancel and prepare for deleting
    _player->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
}

void WorldSession::HandleCancelChanneling(WorldPackets::Spells::CancelChannelling& cancelChanneling)
{
    uint32 spellID = uint32(cancelChanneling.ChannelSpell);

    // ignore for remote control state (for player case)
    Unit* mover = _player->m_mover;
    if (!mover)
    {
        return;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
    if (!spellInfo)
    {
        return;
    }

    // not allow remove spells with attr SPELL_ATTR0_NO_AURA_CANCEL
    if (spellInfo->HasAttribute(SPELL_ATTR0_NO_AURA_CANCEL))
    {
        return;
    }

    Spell* spell = mover->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (!spell || spell->GetSpellInfo()->Id != spellInfo->Id)
    {
        return;
    }

    mover->InterruptSpell(CURRENT_CHANNELED_SPELL);
}

void WorldSession::HandleTotemDestroyed(WorldPackets::Totem::TotemDestroyed& totemDestroyed)
{
    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    uint8 slotId = totemDestroyed.Slot;
    slotId += SUMMON_SLOT_TOTEM_FIRE;

    if (slotId >= MAX_TOTEM_SLOT)
        return;

    if (!_player->m_SummonSlot[slotId])
        return;

    Creature* totem = GetPlayer()->GetMap()->GetCreature(_player->m_SummonSlot[slotId]);
    // Don't unsummon sentry totem
    if (totem && totem->IsTotem() && (!totemDestroyed.TotemGUID || totem->GetGUID() == totemDestroyed.TotemGUID))
        totem->ToTotem()->UnSummon();
}

void WorldSession::HandleSelfResOpcode(WorldPackets::Spells::SelfRes& /*packet*/)
{
    if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(_player->GetUInt32Value(PLAYER_SELF_RES_SPELL)))
    {
        if (_player->HasPreventResurectionAura() && !spell->HasAttribute(SPELL_ATTR7_BYPASS_NO_RESURRECTION_AURA))
        {
            return; // silent return, client should display error by itself and not send this opcode
        }

        _player->CastSpell(_player, spell->Id);
        _player->SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);
    }
}

void WorldSession::HandleSpellClick(WorldPackets::Spells::SpellClick& spellClick)
{
    ObjectGuid guid = spellClick.SpellClickUnitGuid;

    // this will get something not in world. crash
    Creature* unit = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!unit)
        return;

    /// @todo: Unit::SetCharmedBy: 28782 is not in world but 0 is trying to charm it! -> crash
    if (!unit->IsInWorld())
        return;

    unit->HandleSpellClick(_player);
}

void WorldSession::HandleMirrorImageDataRequest(WorldPackets::Spells::GetMirrorImageData& getMirrorImageData)
{
    ObjectGuid guid = getMirrorImageData.UnitGUID;

    // Get unit for which data is needed by client
    Unit* unit = ObjectAccessor::GetUnit(*_player, guid);
    if (!unit)
        return;

    if (!unit->HasCloneCasterAura())
        return;

    // Get creator of the unit (SPELL_AURA_CLONE_CASTER does not stack)
    Unit* creator = unit->GetAuraEffectsByType(SPELL_AURA_CLONE_CASTER).front()->GetCaster();
    if (!creator)
        return;

    Player* player = creator->ToPlayer();
    if (!player)
    {
        WorldPackets::Spells::MirrorImageCreatureData mirrorImageCreatureData;
        mirrorImageCreatureData.UnitGUID = guid;
        mirrorImageCreatureData.DisplayID = creator->GetDisplayId();
        SendPacket(mirrorImageCreatureData.Write());
        return;
    }

    static EquipmentSlots const itemSlots[] =
    {
        EQUIPMENT_SLOT_HEAD,
        EQUIPMENT_SLOT_SHOULDERS,
        EQUIPMENT_SLOT_BODY,
        EQUIPMENT_SLOT_CHEST,
        EQUIPMENT_SLOT_WAIST,
        EQUIPMENT_SLOT_LEGS,
        EQUIPMENT_SLOT_FEET,
        EQUIPMENT_SLOT_WRISTS,
        EQUIPMENT_SLOT_HANDS,
        EQUIPMENT_SLOT_TABARD,
        EQUIPMENT_SLOT_BACK
    };

    WorldPackets::Spells::MirrorImageComponentedData mirrorImageComponentedData;
    mirrorImageComponentedData.UnitGUID = guid;
    mirrorImageComponentedData.DisplayID = creator->GetDisplayId();
    mirrorImageComponentedData.RaceID = creator->getRace();
    mirrorImageComponentedData.Gender = creator->getGender();
    mirrorImageComponentedData.ClassID = creator->getClass();
    mirrorImageComponentedData.SkinColor = player->GetByteValue(PLAYER_BYTES, 0);
    mirrorImageComponentedData.FaceVariation = player->GetByteValue(PLAYER_BYTES, 1);
    mirrorImageComponentedData.HairVariation = player->GetByteValue(PLAYER_BYTES, 2);
    mirrorImageComponentedData.HairColor = player->GetByteValue(PLAYER_BYTES, 3);
    mirrorImageComponentedData.BeardVariation = player->GetByteValue(PLAYER_BYTES_2, 0);

    Guild* guild = player->GetGuild();
    mirrorImageComponentedData.GuildGUID = guild ? guild->GetGUID() : ObjectGuid::Empty;

    mirrorImageComponentedData.ItemDisplayID.reserve(std::size(itemSlots));

    // Display items in visible slots
    for (EquipmentSlots slot : itemSlots)
    {
        uint32 itemDisplayId = 0;
        if ((slot == EQUIPMENT_SLOT_HEAD && player->HasPlayerFlag(PLAYER_FLAGS_HIDE_HELM)) ||
            (slot == EQUIPMENT_SLOT_BACK && player->HasPlayerFlag(PLAYER_FLAGS_HIDE_CLOAK)))
            itemDisplayId = 0;
        else if (Item const* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            itemDisplayId = item->GetDisplayId();
            sScriptMgr->OnGlobalMirrorImageDisplayItem(item, itemDisplayId);
        }

        mirrorImageComponentedData.ItemDisplayID.push_back(itemDisplayId);
    }

    SendPacket(mirrorImageComponentedData.Write());
}

void WorldSession::HandleMissileTrajectoryCollision(WorldPackets::Spells::MissileTrajectoryCollision& packet)
{
    Unit* caster = ObjectAccessor::GetUnit(*_player, packet.Target);
    if (!caster)
        return;

    Spell* spell = caster->FindCurrentSpellBySpellId(packet.SpellID);
    if (!spell || !spell->m_targets.HasDst())
        return;

    Position pos = *spell->m_targets.GetDstPos();
    pos.Relocate(packet.CollisionPos.x, packet.CollisionPos.y, packet.CollisionPos.z);
    spell->m_targets.ModDst(pos);

    // we changed dest, recalculate flight time
    spell->RecalculateDelayMomentForDst();

    WorldPackets::Spells::NotifyMissileTrajectoryCollision notify;
    notify.Caster = packet.Target;
    notify.CastID = packet.CastID;
    notify.CollisionPos = packet.CollisionPos;
    caster->SendMessageToSet(notify.Write(), true);
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

    // the client stops when it updates the trajectory; the movement status is not relayed (TC 6.x)
}

void WorldSession::HandleRequestCategoryCooldowns(WorldPackets::Spells::RequestCategoryCooldowns& /*requestCategoryCooldowns*/)
{
    _player->SendSpellCategoryCooldowns();
}
