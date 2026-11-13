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

char constexpr Achievementfmt[] = "niixSxiixixxiii";
char constexpr AchievementCategoryfmt[] = "nixx";
char constexpr AchievementCriteriafmt[] = "niiiqixxxxiixixxxiiiiii"; // 4.3.4 layout, built from Criteria/CriteriaTree/ModifierTree on 5.4.8
char constexpr Criteriafmt[] = "niiiiiiiiiii";
char constexpr CriteriaTreefmt[] = "niixiiix";
char constexpr ModifierTreefmt[] = "niiiiii";
char constexpr AreaTableEntryfmt[] = "niiiixxxxxxxiSiiiiixxxxxxxxxxx";
char constexpr AreaGroupEntryfmt[] = "niiiiiii";
char constexpr AreaPOIEntryfmt[] = "nxiiiiiiiiixffixixxixx";
char constexpr AuctionHouseEntryfmt[] = "niiix";
char constexpr BankBagSlotPricesEntryfmt[] = "ni";
char constexpr BarberShopStyleEntryfmt[] = "nixxxiii";
char constexpr BattlemasterListEntryfmt[] = "niiiiiiiiiiiiiiiiixSiixxxxxxxx";
char constexpr CharStartOutfitEntryfmt[] = "dbbbXiiiiiiiiiiiiiiiiiiiiiiiixxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxii";
char constexpr CharSectionsEntryfmt[] = "diiixxxiii";
char constexpr CharTitlesEntryfmt[] = "nxSSix";
char constexpr ChatChannelsEntryfmt[] = "nixSx"; // ChatChannelsEntryfmt, index not used (more compact store)
char constexpr ChrClassesEntryfmt[] = "nixSxxxixixxxxxxxx";
char constexpr ChrClassesXPowerTypesfmt[] = "nii";
char constexpr ChrRacesEntryfmt[] = "niixiixixxxxiiSxxxxxxxxxxxxxxxxxxxxx";
char constexpr CinematicCameraEntryfmt[] = "nsiffff";
char constexpr CinematicSequencesEntryfmt[] = "nxixxxxxxx";
char constexpr CreatureDisplayInfofmt[] = "nixifxxxxxxxxxxxxxxx";
char constexpr CreatureDisplayInfoExtrafmt[] = "dixxxxxxxxxxxxxxxxxxx";
char constexpr CreatureFamilyfmt[] = "nfifiiiiixSx";
char constexpr CreatureModelDatafmt[] = "nixxfxxxxxxxxxfffxxxxxxxxxxxxxxxxx";
char constexpr CreatureSpellDatafmt[] = "niiiixxxx";
char constexpr CreatureTypefmt[] = "nxx";
char constexpr CurrencyTypesfmt[] = "nixixxxiiixx";
char constexpr DestructibleModelDatafmt[] = "nxxixxxixxxixxxixxxxxxxx";
char constexpr DungeonEncounterfmt[] = "niixiSxxx";
char constexpr DurabilityCostsfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiiiiiii";
char constexpr DurabilityQualityfmt[] = "nf";
char constexpr EmotesEntryfmt[] = "nxxiiixx";
char constexpr EmotesTextEntryfmt[] = "nxixxxxxxxxxxxxxxxx";
char constexpr EmotesTextSoundEntryfmt[] = "niiii";
char constexpr FactionEntryfmt[] = "niiiiiiiiiiiiiiiiiiffixSxxxx";
char constexpr FactionTemplateEntryfmt[] = "niiiiiiiiiiiii";
char constexpr GameObjectArtKitfmt[] = "nxxxxxxx";
char constexpr GameObjectDisplayInfofmt[] = "nsxxxxxxxxxxffffffxxx";
char constexpr GemPropertiesEntryfmt[] = "nixxix";
char constexpr GlyphPropertiesfmt[] = "niix";
char constexpr GlyphSlotfmt[] = "nii";
// The one-column gt*.dbc files are indexed by row; their gt*_dbc tables add that row as ID
char constexpr GtDatabasefmt[] = "df";
char constexpr GtBarberShopCostBasefmt[] = "xf";
char constexpr GtCombatRatingsfmt[] = "xf";
char constexpr GtChanceToMeleeCritBasefmt[] = "xf";
char constexpr GtChanceToMeleeCritfmt[] = "xf";
char constexpr GtChanceToSpellCritBasefmt[] = "xf";
char constexpr GtChanceToSpellCritfmt[] = "xf";
char constexpr GtNPCManaCostScalerfmt[] = "xf";
char constexpr GtOCTClassCombatRatingScalarfmt[] = "df";
char constexpr GtOCTRegenHPfmt[] = "f";
//char constexpr GtOCTRegenMPfmt[] = "f";
char constexpr GtRegenHPPerSptfmt[] = "f";
char constexpr GtRegenMPPerSptfmt[] = "xf";
char constexpr GtSpellScalingfmt[] = "df";
char constexpr GuildPerkSpellsfmt[] = "dii";
char constexpr Holidaysfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiixxsiix";
char constexpr ImportPriceArmorfmt[] = "nffff";
char constexpr ImportPriceQualityfmt[] = "nf";
char constexpr ImportPriceShieldfmt[] = "nf";
char constexpr ImportPriceWeaponfmt[] = "nf";
char constexpr Itemfmt[] = "niiiiiii";
char constexpr Phasefmt[] = "nsi";
char constexpr PhaseGroupfmt[] = "nii";
char constexpr ItemBagFamilyfmt[] = "nx";
char constexpr ItemClassfmt[] = "nxfx";
char constexpr ItemDisplayTemplateEntryfmt[] = "nxxxxsxxxxxxxxxxxxxxxxxxx";
//char constexpr ItemCondExtCostsEntryfmt[] = "xiii";
char constexpr ItemExtendedCostEntryfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiiiiiiii";
char constexpr ItemLimitCategoryEntryfmt[] = "nxii";
char constexpr ItemPriceBasefmt[] = "diff";
char constexpr ItemReforgefmt[] = "nifif";
char constexpr ItemRandomPropertiesfmt[] = "nxiiiiiS";
char constexpr ItemRandomSuffixfmt[] = "nSxiiiiiiiiii";
char constexpr ItemSetEntryfmt[] = "dSiiiiiiiiiixxxxxxxiiiiiiiiiiiiiiiiii";
char constexpr LFGDungeonEntryfmt[] = "nSiiiiiiiiixxixixxiiixxxxixxx";
char constexpr LightEntryfmt[] = "nifffxxxxxxxxxx";
char constexpr LiquidTypefmt[] = "nxxixixxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
char constexpr LockEntryfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiixxxxxxxx";
char constexpr MailTemplateEntryfmt[] = "nxS";
char constexpr MapEntryfmt[] = "nxiixSixxxxiffxixix";
char constexpr MapDifficultyEntryfmt[] = "diisiix";
char constexpr MovieEntryfmt[] = "nxxxx";
char constexpr NameGenfmt[] = "dsii";
char constexpr NamesReservedfmt[] = "xsx";
char constexpr NamesProfanityfmt[] = "xsx";
char constexpr OverrideSpellDatafmt[] = "niiiiiiiiiixx";
char constexpr PowerDisplayfmt[] = "nixXXX";
char constexpr QuestSortEntryfmt[] = "nx";
char constexpr QuestPOIBlobfmt[] = "niii";
char constexpr QuestPOIPointfmt[] = "niii";
char constexpr QuestXPfmt[] = "niiiiiiiiii";
char constexpr QuestFactionRewardfmt[] = "niiiiiiiiii";
char constexpr PvPDifficultyfmt[] = "diiiii";
char constexpr RandomPropertiesPointsfmt[] = "niiiiiiiiiiiiiii";
char constexpr ScalingStatDistributionfmt[] = "niiiiiiiiiiiiiiiiiiiiix";
char constexpr ScalingStatValuesfmt[] = "inxiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiix";
char constexpr SkillLinefmt[] = "niSxixixx";
char constexpr SkillLineAbilityfmt[] = "niiiiiiiiiiix";
char constexpr SkillRaceClassInfofmt[] = "diiiiiiix";
char constexpr ResearchBranchfmt[] = "nsiisi";
char constexpr ResearchFieldfmt[] = "nsi";
char constexpr ResearchProjectfmt[] = "nssiiiisi";
char constexpr ResearchSitefmt[] = "niiss";
char constexpr AnimKitfmt[] = "nxx";
char constexpr SkillTiersfmt[] = "nxxxxxxxxxxxxxxxxiiiiiiiiiiiiiiii";
char constexpr SoundEntriesfmt[] = "nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
char constexpr SpellCastTimefmt[] = "nixx";
char constexpr SpellCategoryfmt[] = "nixxii";
char constexpr SpellDifficultyfmt[] = "niiii";
char constexpr SpellDurationfmt[] = "niii";
char constexpr SpellEntryfmt[] = "niiiiiiiiiiiiiiifiiiiSSxxiixxifiiiiiiixiiiiiiiii"; // spell_cata_dbc (server-side rows)
char constexpr SpellDbcEntryfmt[] = "nSSxxixxfiiiiiiiiiiiiiiii"; // 5.4.8 Spell.dbc
char constexpr SpellMiscEntryfmt[] = "nixiiiiiiiiiiixxxiiifiiiii"; // 5.4.8 SpellMisc.dbc
char constexpr SpellEffectScalingEntryfmt[] = "nfffxi";
char constexpr SpellEffectEntryfmt[] = "nifiiiffiiiiiifiifiiiiiiiix"; // spelleffect_dbc (server-side rows)
char constexpr SpellEffectDbcEntryfmt[] = "niifiiiffiiiiiifiifiiixixiiiix"; // 5.4.8 SpellEffect.dbc
char constexpr SpellAuraOptionsEntryfmt[] = "nxxiiiixx";
char constexpr SpellAuraRestrictionsEntryfmt[] = "dxxiiiiiiii";
char constexpr SpellCastingRequirementsEntryfmt[] = "niiiiii";
char constexpr SpellCategoriesEntryfmt[] = "dxxiiiiiix";
char constexpr SpellClassOptionsEntryfmt[] = "dxiiixi";
char constexpr SpellCooldownsEntryfmt[] = "dxxiii";
char constexpr SpellEquippedItemsEntryfmt[] = "dxxiii";
char constexpr SpellInterruptsEntryfmt[] = "dxxiiiii";
char constexpr SpellLevelsEntryfmt[] = "dxxiii";
char constexpr SpellPowerEntryfmt[] = "niiiiiiixffxx";
char constexpr SpellReagentsEntryfmt[] = "diiiiiiiiiiiiiiiixx";
char constexpr SpellScalingEntryfmt[] = "diiiifiii";
char constexpr SpellShapeshiftEntryfmt[] = "niiiix";
char constexpr SpellTargetRestrictionsEntryfmt[] = "nxxfxiiii";
char constexpr SpellTotemsEntryfmt[] = "niiii";
char constexpr SpellFocusObjectfmt[] = "nx";
char constexpr SpellItemEnchantmentfmt[] = "niiiiiiiiiiSiiiiiiixxxxxxx";
char constexpr SpellItemEnchantmentConditionfmt[] = "nbbbXxxxxxxbbbXXbbbxiiixxXXXXXX";
char constexpr SpellRadiusfmt[] = "nffxf";
char constexpr SpellRangefmt[] = "nffffixx";
char constexpr SpellRuneCostfmt[] = "niiixi";
char constexpr SpellShapeshiftFormEntryfmt[] = "nxxiixiiixxiiiiiiiixx";
char constexpr SpellVisualfmt[] = "dxxxxxxiixxxxxxxxxxxxxxxxxxxxxxxx";
char constexpr StableSlotPricesfmt[] = "ni";
char constexpr SummonPropertiesfmt[] = "niiiii";
char constexpr TalentEntryfmt[] = "nxiiixxxiix";
char constexpr ChrSpecializationfmt[] = "nxiiiiiiiiiSxx";
char constexpr SpecializationSpellsfmt[] = "niiix";
char constexpr TaxiNodesEntryfmt[] = "nifffSiixxxx";
char constexpr TaxiPathEntryfmt[] = "niii";
char constexpr TaxiPathNodeEntryfmt[] = "diiifffiiii";
char constexpr TeamContributionPointsfmt[] = "df";
char constexpr TotemCategoryEntryfmt[] = "nxii";
char constexpr TransportAnimationfmt[] = "diifffx";
char constexpr TransportRotationfmt[] = "diiffff";
char constexpr VehicleEntryfmt[] = "nixffffiiiiiiiifffffffffffffffssssfifiixx";
char constexpr VehicleSeatEntryfmt[] = "niiffffffffffiiiiiifffffffiiifffiiiiiiiffiiiiixxxxxxxxxxxxxxxxxxxx";
char constexpr WMOAreaTableEntryfmt[] = "niiixxxxxiixxxx";
char constexpr WorldMapAreaEntryfmt[] = "xinxffffixxxxx";
char constexpr WorldMapOverlayEntryfmt[] = "nxiiiixxxxxxxxxx";

#endif
