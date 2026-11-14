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

#ifndef ACORE_DBCSFRM_H
#define ACORE_DBCSFRM_H

// x - skip<uint32>, X - skip<uint8>, s - char*, f - float, i - uint32, b - uint8, d - index (not included)
// n - index (included), l - uint64, p - field present in sql dbc, a - field absent in sql dbc

char constexpr AnimKitfmt[] = "nxxx";
char constexpr AreaTablefmt[] = "niiiiixxxxxxisiiiiixxxxxxxxxx";
char constexpr AreaTriggerfmt[] = "nifffxxxfffffxxxx";
char constexpr ArmorLocationfmt[] = "nfffff";
char constexpr BankBagSlotPricesfmt[] = "ni";
char constexpr BannedAddOnsfmt[] = "nxxxxxxxxxx";
char constexpr BattlemasterListfmt[] = "niiiiiiiiiiiiiiiiixsiiiixxxxxxx";
char constexpr CharSectionsfmt[] = "diiixxxiii";
char constexpr CharTitlesfmt[] = "nxssix";
char constexpr ChatChannelsfmt[] = "nixsx";
char constexpr ChrClassesfmt[] = "nixsxxxixiiiiixxxxx";
char constexpr ChrRacesfmt[] = "niixiixxxxxxiisxxxxxxxxxxxxxxxxxxxxxxxxx";
char constexpr ChrSpecializationfmt[] = "nxiiiiiiiiixxxii";
char constexpr CreatureDisplayInfoExtrafmt[] = "dixxxxxxxxxxxxxxxxxxxx";
char constexpr CreatureFamilyfmt[] = "nfifiiiiixsx";
char constexpr CreatureModelDatafmt[] = "niixfxxxxxxxxxfffxxxxxxxxxxxxxxxxx";
char constexpr DifficultyFmt[] = "niiiixiixxxxix";
char constexpr DungeonEncounterfmt[] = "niiixsxxx";
char constexpr DurabilityCostsfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiiiiiii";
char constexpr Emotesfmt[] = "nxxiiixx";
char constexpr EmotesTextfmt[] = "nxixxxxxxxxxxxxxxxx";
char constexpr EmotesTextSoundEntryfmt[] = "niiii";
char constexpr Factionfmt[] = "niiiiiiiiiiiiiiiiiiffixsxixx";
char constexpr FactionTemplatefmt[] = "niiiiiiiiiiiii";
char constexpr GameObjectDisplayInfofmt[] = "nixxxxxxxxxxffffffxxx";
char constexpr GemPropertiesfmt[] = "nixxii";
char constexpr GlyphPropertiesfmt[] = "niiix";
char constexpr GtArmorMitigationByLvlfmt[] = "xf";
char constexpr GtBarberShopCostBasefmt[] = "xf";
char constexpr GtCombatRatingsfmt[] = "xf";
char constexpr GtOCTHpPerStaminafmt[] = "df";
char constexpr GtOCTLevelExperiencefmt[] = "xf";
char constexpr GtChanceToMeleeCritBasefmt[] = "xf";
char constexpr GtChanceToMeleeCritfmt[] = "xf";
char constexpr GtChanceToSpellCritBasefmt[] = "xf";
char constexpr GtChanceToSpellCritfmt[] = "xf";
char constexpr GtItemSocketCostPerLevelfmt[] = "xf";
char constexpr GtNPCManaCostScalerfmt[] = "xf";
char constexpr GtNpcTotalHpfmt[] = "xf";
char constexpr GtNpcTotalHpExp1fmt[] = "xf";
char constexpr GtNpcTotalHpExp2fmt[] = "xf";
char constexpr GtNpcTotalHpExp3fmt[] = "xf";
char constexpr GtNpcTotalHpExp4fmt[] = "xf";
char constexpr GtNpcTotalHpExp5fmt[] = "xf";
char constexpr GtRegenMPPerSptfmt[] = "xf";
char constexpr GtSpellScalingfmt[] = "df";
char constexpr GtOCTBaseHPByClassfmt[] = "df";
char constexpr GtOCTBaseMPByClassfmt[] = "df";
char constexpr GuildColorBackgroundfmt[] = "nXXX";
char constexpr GuildColorBorderfmt[] = "nXXX";
char constexpr GuildColorEmblemfmt[] = "nXXX";
char constexpr ItemBagFamilyfmt[] = "nx";
char constexpr ItemArmorQualityfmt[] = "nfffffffi";
char constexpr ItemArmorShieldfmt[] = "nifffffff";
char constexpr ItemArmorTotalfmt[] = "niffff";
char constexpr ItemDamagefmt[] = "nfffffffi";
char constexpr ItemSetfmt[] = "nsiiiiiiiiiiiiiiiiiii";
char constexpr ItemSetSpellfmt[] = "niiii";
char constexpr LFGDungeonfmt[] = "nsiiixxiiiixxixixxiiixxxxixxxx";
char constexpr Lightfmt[] = "nifffxxxxxxxxxx";
char constexpr LiquidTypefmt[] = "nxxixixxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
char constexpr Lockfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiixxxxxxxx";
char constexpr Mapfmt[] = "nxiixxsixxixiffxiiiiix";
char constexpr MapDifficultyfmt[] = "diisiiii";
char constexpr MinorTalentfmt[] = "niii";
char constexpr Moviefmt[] = "nxxxx";
char constexpr Phasefmt[] = "ni";
char constexpr QuestFactionRewardfmt[] = "niiiiiiiiii";
char constexpr PowerDisplayfmt[] = "nixXXX";
char constexpr PvpDifficultyfmt[] = "diiii";
char constexpr RandPropPointsfmt[] = "niiiiiiiiiiiiiii";
char constexpr SkillLinefmt[] = "nisxixixx";
char constexpr SkillLineAbilityfmt[] = "niiiiiiiiiiii";
char constexpr SkillRaceClassInfofmt[] = "diiiiiii";
char constexpr SpellCategoriesfmt[] = "diiiiiiiii";
char constexpr SpellCategoryfmt[] = "nixxii";
char constexpr SpellEffectfmt[] = "niifiiiffiiiiiifiifiiiiifiiiiif";
char constexpr Spellfmt[] = "nssxxiiiiiiiiiiiiiiiiiii";
char constexpr SpellEffectScalingfmt[] = "nfffi";
char constexpr SpellFocusObjectfmt[] = "nx";
char constexpr SpellItemEnchantmentfmt[] = "niiiiiiiiiixiiiiiiiiiiifff";
char constexpr SpellScalingfmt[] = "niiiifiii";
char constexpr SpellTargetRestrictionsfmt[] = "niiffiiii";
char constexpr SpellInterruptsfmt[] = "diiiiiii";
char constexpr SpellEquippedItemsfmt[] = "diiiii";
char constexpr SpellAuraOptionsfmt[] = "niiiiiiii";
char constexpr SpellCooldownsfmt[] = "diiiii";
char constexpr SpellLevelsfmt[] = "diiiii";
char constexpr SpellShapeshiftfmt[] = "niiiix";
char constexpr SpellShapeshiftFormfmt[] = "nxxiixiiiiiiiiiiiiixx";
char constexpr SummonPropertiesfmt[] = "niiiii";
char constexpr Talentfmt[] = "niiiiiiiiix";
char constexpr Vehiclefmt[] = "niiffffiiiiiiiifffffffffffffffxxxxfifiiii";
char constexpr VehicleSeatfmt[] = "niiffffffffffiiiiiifffffffiiifffiiiiiiiffiiiiffffffffffffiiiiiiiii";
char constexpr WMOAreaTablefmt[] = "niiixxxxxiixxxx";
char constexpr WorldMapAreafmt[] = "xinxffffixxxxx";
char constexpr WorldMapTransformsfmt[] = "diffffffiffxxxf";
char constexpr WorldSafeLocsfmt[] = "niffffx";

// AzerothCore / 5.4.8 formats of files the 6.x client still ships that TrinityCore 6.x does not load
char constexpr AchievementCategoryfmt[] = "nixx";
char constexpr CinematicCameraEntryfmt[] = "nsiffff";
char constexpr CreatureSpellDatafmt[] = "niiiixxxx";
char constexpr GameObjectArtKitfmt[] = "nxxxxxxx";
char constexpr ResearchBranchfmt[] = "nsiisi";
char constexpr ResearchFieldfmt[] = "nsi";
char constexpr ResearchProjectfmt[] = "nssiiiisi";
char constexpr ResearchSitefmt[] = "niiss";

#endif
