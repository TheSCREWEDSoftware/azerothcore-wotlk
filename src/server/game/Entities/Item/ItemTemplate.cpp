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

#include "ItemTemplate.h"
#include "DBCStores.h"
#include "Errors.h"
#include <cmath>

uint32 ItemTemplate::GetArmor(uint32 itemLevel) const
{
    uint32 quality = ItemQualities(Quality) != ITEM_QUALITY_HEIRLOOM ? ItemQualities(Quality) : ITEM_QUALITY_RARE;
    if (quality > ITEM_QUALITY_ARTIFACT)
        return 0;

    // all items but shields
    if (Class != ITEM_CLASS_ARMOR || SubClass != ITEM_SUBCLASS_ARMOR_SHIELD)
    {
        ItemArmorQualityEntry const* armorQuality = sItemArmorQualityStore.LookupEntry(itemLevel);
        ItemArmorTotalEntry const* armorTotal = sItemArmorTotalStore.LookupEntry(itemLevel);
        if (!armorQuality || !armorTotal)
            return 0;

        uint32 inventoryType = InventoryType;
        if (inventoryType == INVTYPE_ROBE)
            inventoryType = INVTYPE_CHEST;

        ArmorLocationEntry const* location = sArmorLocationStore.LookupEntry(inventoryType);
        if (!location)
            return 0;

        if (SubClass < ITEM_SUBCLASS_ARMOR_CLOTH || SubClass > ITEM_SUBCLASS_ARMOR_PLATE)
            return 0;

        return uint32(armorQuality->QualityMod[quality] * armorTotal->Value[SubClass - 1] * location->Modifier[SubClass - 1] + 0.5f);
    }

    // shields
    ItemArmorShieldEntry const* shield = sItemArmorShieldStore.LookupEntry(itemLevel);
    if (!shield)
        return 0;

    return uint32(shield->Quality[quality] + 0.5f);
}

void ItemTemplate::GetDamage(uint32 itemLevel, float& minDamage, float& maxDamage) const
{
    minDamage = maxDamage = 0.0f;
    uint32 quality = ItemQualities(Quality) != ITEM_QUALITY_HEIRLOOM ? ItemQualities(Quality) : ITEM_QUALITY_RARE;
    if (Class != ITEM_CLASS_WEAPON || quality > ITEM_QUALITY_ARTIFACT)
        return;

    if (InventoryType > INVTYPE_RANGEDRIGHT)
        return;

    DBCStorage<ItemDamageEntry>* store = nullptr;
    switch (InventoryType)
    {
        case INVTYPE_AMMO:
            store = &sItemDamageAmmoStore;
            break;
        case INVTYPE_2HWEAPON:
            store = HasFlag2(ITEM_FLAG2_CASTER_WEAPON) ? &sItemDamageTwoHandCasterStore : &sItemDamageTwoHandStore;
            break;
        case INVTYPE_RANGED:
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
            switch (SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_WAND:
                    store = &sItemDamageWandStore;
                    break;
                case ITEM_SUBCLASS_WEAPON_THROWN:
                    store = &sItemDamageThrownStore;
                    break;
                case ITEM_SUBCLASS_WEAPON_BOW:
                case ITEM_SUBCLASS_WEAPON_GUN:
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    store = &sItemDamageRangedStore;
                    break;
                default:
                    return;
            }
            break;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
            store = HasFlag2(ITEM_FLAG2_CASTER_WEAPON) ? &sItemDamageOneHandCasterStore : &sItemDamageOneHandStore;
            break;
        default:
            return;
    }

    ASSERT(store);

    ItemDamageEntry const* damageInfo = store->LookupEntry(itemLevel);
    if (!damageInfo)
        return;

    float dps = damageInfo->DPS[quality];
    float avgDamage = dps * Delay * 0.001f;
    minDamage = (StatScalingFactor * -0.5f + 1.0f) * avgDamage;
    maxDamage = std::floor(float(avgDamage * (StatScalingFactor * 0.5f + 1.0f) + 0.5f));
}

std::size_t ItemTemplate::CalculateItemSpecBit(ChrSpecializationEntry const* spec)
{
    return (spec->ClassID - 1) * MAX_SPECIALIZATIONS + spec->OrderIndex;
}
