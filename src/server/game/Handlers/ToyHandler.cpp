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

// 6.x toy box, from TrinityCore 6.x (ToyHandler.cpp)

#include "CollectionMgr.h"
#include "DB2Stores.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Spell.h"
#include "SpellCastRequest.h"
#include "SpellMgr.h"
#include "ToyPackets.h"
#include "WorldSession.h"

void WorldSession::HandleAddToy(WorldPackets::Toy::AddToy& packet)
{
    if (!packet.Guid)
        return;

    Item* item = _player->GetItemByGuid(packet.Guid);
    if (!item)
    {
        _player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, nullptr, nullptr);
        return;
    }

    if (!sDB2Manager.IsToyItem(item->GetEntry()))
        return;

    InventoryResult msg = _player->CanUseItem(item);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, item, nullptr);
        return;
    }

    if (_collectionMgr->AddToy(item->GetEntry(), false))
        _player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
}

void WorldSession::HandleUseToy(WorldPackets::Toy::UseToy& packet)
{
    ItemTemplate const* item = sObjectMgr->GetItemTemplate(packet.ItemID);
    if (!item)
        return;

    if (!_collectionMgr->HasToy(packet.ItemID))
        return;

    auto effect = std::find_if(item->Effects.begin(), item->Effects.end(), [&packet](ItemEffectEntry const* itemEffect)
    {
        return uint32(packet.Cast.SpellID) == itemEffect->SpellID;
    });

    if (effect == item->Effects.end())
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(packet.Cast.SpellID);
    if (!spellInfo)
    {
        LOG_ERROR("network.opcode", "HandleUseToy: unknown spell id: {} used by Toy Item entry {}", packet.Cast.SpellID, packet.ItemID);
        return;
    }

    if (_player->isPossessing())
        return;

    SpellCastTargets targets;
    SpellCastRequest::FromPacket(packet.Cast).FillTargets(targets, _player);

    Spell* spell = new Spell(_player, spellInfo, TRIGGERED_NONE, ObjectGuid::Empty, false);
    spell->m_castItemEntry = packet.ItemID;
    spell->m_cast_count = packet.Cast.CastID;
    spell->m_castFlagsEx |= CAST_FLAG_EX_USE_TOY_SPELL;
    spell->prepare(&targets);
}

void WorldSession::HandleToySetFavorite(WorldPackets::Toy::ToySetFavorite& packet)
{
    _collectionMgr->ToySetFavorite(packet.ItemID, packet.Favorite);
}
