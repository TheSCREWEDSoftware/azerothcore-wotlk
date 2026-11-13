#!/usr/bin/env python3
"""Layer the 5.4.8 content of a SkyFire world database (SFDB 548) onto a 4.3.4 AzerothCore world.

The target is a world built by apps/cata/build_world_base.sh (TDB 434 converted to the AzerothCore
schema). SkyFire's world uses the 2013 TrinityCore schema; this script adds what 4.3.4 does not have:

- item_template: every SFDB item replaces the 4.3.4 row (5.4.8 stats, prices, spells). Weapon
  damage, armor and resistances are not SFDB columns (5.4.8 derives them from ItemDamage*/ItemArmor*
  DBCs); the 4.3.4 values are kept for items both have, new items get 0.
- templates that 4.3.4 does not have: creature_template (+ model, spell, resistance, movement,
  quest items, addon, equipment, immunities), creature_model_info, gameobject_template (+ addon,
  quest items).
- spawns: every SFDB spawn on a map the target has no spawns on (Pandaria, the Wandering Isle, the
  5.x dungeons and scenarios) and every SFDB spawn of a new template, with their addons; spawn guids
  are shifted by GUID_OFFSET.
- quests that 4.3.4 does not have, with their objectives (quest_objective -> RequiredNpcOrGo,
  RequiredItem, currency and reputation columns), texts, starters and enders.
- loot, vendor, SmartAI, creature text, waypoint and condition rows that belong to the new templates,
  spawns and quests.
- character creation for the Monk class and the Pandaren races (playercreateinfo, actions, items,
  spells) and their stats (player_class_stats / player_race_stats from player_levelstats).

Rows the target already has are never replaced, except item_template.

  import_sfdb.py --source sfdb548 --target acore_world_build [--container local --user root --password ""]
"""

import argparse
import re
import subprocess
import sys

GUID_OFFSET = 20_000_000
PATH_OFFSET = 200_000_000
NEW_RACES = (24, 25, 26)
MONK = 10
IMMUNITY_ID_BASE = 2_000_000
IMMUNITY_COMMENT = "SFDB 548 template immunity masks"

ARGS = None


def mysql_command(*extra, stdin=False):
    credentials = [f"-u{ARGS.user}"] + ([f"-p{ARGS.password}"] if ARGS.password else [])
    if ARGS.container == "local":
        return ["mysql", *credentials, "--max_allowed_packet=1G", "--database=mysql", *extra]
    return ["docker", "exec", *(["-i"] if stdin else []), ARGS.container, "mysql", *credentials, *extra]


def run(sql):
    # SFDB leaves some NOT NULL columns NULL; a non-strict session stores the column default
    result = subprocess.run(mysql_command("--default-character-set=utf8mb4", "-N", "-B", "-e",
                                          "SET SESSION sql_mode='NO_ENGINE_SUBSTITUTION'; " + sql),
                            capture_output=True, text=True, encoding="utf-8", check=False)
    if result.returncode:
        raise RuntimeError(f"query failed: {sql}\n{result.stderr.strip()}")
    return [line.split("\t") for line in result.stdout.split("\n") if line]


def execute(sql):
    rows = run(sql + "; SELECT ROW_COUNT()")
    return int(rows[-1][0]) if rows else 0


def columns(schema, table):
    return [row[0] for row in run(f"SELECT COLUMN_NAME FROM information_schema.columns WHERE TABLE_SCHEMA='{schema}' "
                                  f"AND TABLE_NAME='{table}' ORDER BY ORDINAL_POSITION")]


def insert(table, mapping, source_from, where="1", ignore=True, source_table=None):
    """INSERT the target columns of `mapping` (target column -> source expression)."""
    target_columns = set(columns(ARGS.target, table))
    pairs = [(column, expression) for column, expression in mapping.items() if column in target_columns]
    sql = (f"INSERT {'IGNORE ' if ignore else ''}INTO `{ARGS.target}`.`{table}` "
           f"({','.join(f'`{column}`' for column, _ in pairs)}) "
           f"SELECT {','.join(expression for _, expression in pairs)} FROM {source_from} WHERE {where}")
    return execute(sql)


def common(table, source_table=None, renames=None, overrides=None):
    """Mapping of the columns both schemas share (case-insensitive), plus renames and expressions."""
    source_columns = {name.lower(): name for name in columns(ARGS.source, source_table or table)}
    mapping = {}
    for column in columns(ARGS.target, table):
        name = (renames or {}).get(column, column)
        if overrides and column in overrides:
            mapping[column] = overrides[column]
        elif name.lower() in source_columns:
            mapping[column] = f"s.`{source_columns[name.lower()]}`"
    return mapping


def src(table):
    return f"`{ARGS.source}`.`{table}` s"


def tgt(table):
    return f"`{ARGS.target}`.`{table}`"


# --- items --------------------------------------------------------------------------------------

def import_items():
    keep = ("dmg_min1", "dmg_max1", "dmg_type1", "dmg_min2", "dmg_max2", "dmg_type2", "armor", "holy_res",
            "fire_res", "nature_res", "frost_res", "shadow_res", "arcane_res", "block", "ScalingStatValue",
            "DisenchantID", "RequiredDisenchantSkill", "FoodType", "minMoneyLoot", "maxMoneyLoot", "ScriptName")
    mapping = common("item_template", overrides={"VerifiedBuild": "18414"})
    for column in keep:
        mapping[column] = f"IFNULL(o.`{column}`, {repr('') if column == 'ScriptName' else 0})"
    mapping["dmg_type1"] = "IFNULL(o.dmg_type1, s.DamageType)"
    run(f"DROP TABLE IF EXISTS `{ARGS.target}`.`_sfdb_item_backup`")
    run(f"CREATE TABLE `{ARGS.target}`.`_sfdb_item_backup` (PRIMARY KEY (entry)) AS SELECT * FROM {tgt('item_template')}")
    pairs = list(mapping.items())
    sql = (f"REPLACE INTO {tgt('item_template')} ({','.join(f'`{c}`' for c, _ in pairs)}) "
           f"SELECT {','.join(e for _, e in pairs)} FROM {src('item_template')} "
           f"LEFT JOIN `{ARGS.target}`.`_sfdb_item_backup` o ON o.entry=s.entry")
    count = execute(sql)
    run(f"DROP TABLE `{ARGS.target}`.`_sfdb_item_backup`")
    return {"item_template": count}


# --- templates ----------------------------------------------------------------------------------

def new_entries(table, key="entry"):
    return f"s.`{key}` NOT IN (SELECT `{key}` FROM {tgt(table)})"


def import_creature_templates():
    counts = {}
    run(f"DROP TABLE IF EXISTS `{ARGS.target}`.`_sfdb_new_creature`")
    run(f"CREATE TABLE `{ARGS.target}`.`_sfdb_new_creature` (entry INT UNSIGNED PRIMARY KEY) "
        f"SELECT s.entry FROM {src('creature_template')} WHERE {new_entries('creature_template')}")
    new = f"s.entry IN (SELECT entry FROM `{ARGS.target}`.`_sfdb_new_creature`)"
    mapping = common("creature_template", overrides={
        "faction": "s.faction_A", "rank": "s.npc_rank", "DamageModifier": "s.dmg_multiplier",
        "BaseAttackTime": "s.baseattacktime", "RangeAttackTime": "s.rangeattacktime",
        "BaseVariance": "1", "RangeVariance": "1", "speed_swim": "1", "speed_flight": "1",
        "HealthModifier": "s.Health_mod", "ManaModifier": "s.Mana_mod", "ArmorModifier": "s.Armor_mod",
        "ExperienceModifier": "1", "CreatureImmunitiesId": "0", "VerifiedBuild": "18414"})
    counts["creature_template"] = insert("creature_template", mapping, src("creature_template"), new)
    for idx in range(4):
        counts[f"model{idx + 1}"] = insert(
            "creature_template_model",
            {"CreatureID": "s.entry", "Idx": str(idx), "CreatureDisplayID": f"s.modelid{idx + 1}",
             "DisplayScale": "1", "Probability": "1", "VerifiedBuild": "18414"},
            src("creature_template"), f"{new} AND s.modelid{idx + 1}<>0")
    for index in range(8):
        insert("creature_template_spell",
               {"CreatureID": "s.entry", "Index": str(index), "Spell": f"s.spell{index + 1}", "VerifiedBuild": "18414"},
               src("creature_template"), f"{new} AND s.spell{index + 1}<>0")
    for school in range(1, 7):
        insert("creature_template_resistance",
               {"CreatureID": "s.entry", "School": str(school), "Resistance": f"s.resistance{school}",
                "VerifiedBuild": "18414"},
               src("creature_template"), f"{new} AND s.resistance{school}<>0")
    for idx in range(6):
        insert("creature_questitem",
               {"CreatureEntry": "s.entry", "Idx": str(idx), "ItemId": f"s.questItem{idx + 1}", "VerifiedBuild": "18414"},
               src("creature_template"), f"{new} AND s.questItem{idx + 1}<>0")
    # InhabitType: 1 ground, 2 water, 4 air, 8 rooted
    counts["movement"] = insert(
        "creature_template_movement",
        {"CreatureId": "s.entry", "Ground": "IF(s.InhabitType & 1, 1, 0)", "Swim": "IF(s.InhabitType & 2, 1, 0)",
         "Flight": "IF(s.InhabitType & 4, 1, 0)", "Rooted": "IF(s.InhabitType & 8, 1, 0)"},
        src("creature_template"), f"{new} AND s.InhabitType<>3")
    # TC mechanic bit 1 << (mechanic - 1), AC MechanicsMask bit 1 << mechanic
    counts["immunities"] = execute(
        f"INSERT IGNORE INTO {tgt('creature_immunities')} "
        f"(ID, SchoolMask, DispelTypeMask, MechanicsMask, Effects, Auras, ImmuneAoE, ImmuneChain, Comment) "
        f"SELECT {IMMUNITY_ID_BASE} + DENSE_RANK() OVER (ORDER BY m), 0, 0, m << 1, '', '', 0, 0, '{IMMUNITY_COMMENT}' "
        f"FROM (SELECT DISTINCT mechanic_immune_mask AS m FROM {src('creature_template')} "
        f"WHERE {new} AND mechanic_immune_mask<>0) x")
    execute(f"UPDATE {tgt('creature_template')} t JOIN `{ARGS.source}`.creature_template s ON s.entry=t.entry "
            f"JOIN {tgt('creature_immunities')} i ON i.Comment='{IMMUNITY_COMMENT}' "
            f"AND i.MechanicsMask=(s.mechanic_immune_mask << 1) SET t.CreatureImmunitiesId=i.ID "
            f"WHERE {new}")
    counts["creature_template_addon"] = insert("creature_template_addon", common("creature_template_addon"),
                                               src("creature_template_addon"), new)
    counts["creature_equip_template"] = insert(
        "creature_equip_template",
        common("creature_equip_template", renames={"CreatureID": "entry", "ID": "id", "ItemID1": "itemEntry1",
                                                   "ItemID2": "itemEntry2", "ItemID3": "itemEntry3"},
               overrides={"VerifiedBuild": "18414"}),
        src("creature_equip_template"), new)
    counts["creature_model_info"] = insert(
        "creature_model_info",
        {"DisplayID": "s.modelid", "BoundingRadius": "s.bounding_radius", "CombatReach": "s.combat_reach",
         "Gender": "s.gender", "DisplayID_Other_Gender": "s.modelid_other_gender", "VerifiedBuild": "18414"},
        src("creature_model_info"), "s.modelid NOT IN (SELECT DisplayID FROM " + tgt("creature_model_info") + ")")
    return counts


def import_gameobject_templates():
    counts = {}
    run(f"DROP TABLE IF EXISTS `{ARGS.target}`.`_sfdb_new_gameobject`")
    run(f"CREATE TABLE `{ARGS.target}`.`_sfdb_new_gameobject` (entry INT UNSIGNED PRIMARY KEY) "
        f"SELECT s.entry FROM {src('gameobject_template')} WHERE {new_entries('gameobject_template')}")
    new = f"s.entry IN (SELECT entry FROM `{ARGS.target}`.`_sfdb_new_gameobject`)"
    mapping = common("gameobject_template", renames={f"Data{i}": f"data{i}" for i in range(24)},
                     overrides={"VerifiedBuild": "18414"})
    counts["gameobject_template"] = insert("gameobject_template", mapping, src("gameobject_template"), new)
    counts["gameobject_template_addon"] = insert(
        "gameobject_template_addon", {"entry": "s.entry", "faction": "s.faction", "flags": "s.flags"},
        src("gameobject_template"), f"{new} AND (s.faction<>0 OR s.flags<>0)")
    for idx in range(6):
        insert("gameobject_questitem",
               {"GameObjectEntry": "s.entry", "Idx": str(idx), "ItemId": f"s.questItem{idx + 1}", "VerifiedBuild": "18414"},
               src("gameobject_template"), f"{new} AND s.questItem{idx + 1}<>0")
    return counts


# --- spawns -------------------------------------------------------------------------------------

def import_spawns():
    counts = {}
    for kind, table in (("creature", "creature"), ("gameobject", "gameobject")):
        new_maps = f"s.map NOT IN (SELECT DISTINCT map FROM {tgt(table)})"
        new_templates = f"s.id IN (SELECT entry FROM `{ARGS.target}`.`_sfdb_new_{kind}`)"
        overrides = {"guid": f"s.guid + {GUID_OFFSET}", "phaseMask": "1", "PhaseId": "s.phaseId",
                     "PhaseGroup": "s.phaseGroup", "zoneId": "0", "areaId": "0", "VerifiedBuild": "18414",
                     "Comment": "'SFDB 548'"}
        if kind == "creature":
            overrides["wander_distance"] = "s.spawndist"
        counts[table] = insert(table, common(table, overrides=overrides), src(table),
                               f"({new_maps} OR {new_templates})")
    imported = f"s.guid + {GUID_OFFSET} IN (SELECT guid FROM {tgt('creature')} WHERE Comment='SFDB 548')"
    counts["creature_addon"] = insert("creature_addon", common("creature_addon", overrides={
        "guid": f"s.guid + {GUID_OFFSET}", "path_id": f"IF(s.path_id <> 0, s.path_id + {PATH_OFFSET}, 0)"}),
        src("creature_addon"), imported)
    return counts


# --- quests -------------------------------------------------------------------------------------

def import_quests():
    counts = {}
    run(f"DROP TABLE IF EXISTS `{ARGS.target}`.`_sfdb_new_quest`")
    run(f"CREATE TABLE `{ARGS.target}`.`_sfdb_new_quest` (ID INT UNSIGNED PRIMARY KEY) "
        f"SELECT s.Id AS ID FROM {src('quest_template')} WHERE s.Id NOT IN (SELECT ID FROM {tgt('quest_template')})")
    new = f"s.Id IN (SELECT ID FROM `{ARGS.target}`.`_sfdb_new_quest`)"
    mapping = {
        "ID": "s.Id", "QuestType": "s.Method", "QuestLevel": "s.Level", "MinLevel": "s.MinLevel",
        "QuestSortID": "s.ZoneOrSort", "QuestInfoID": "s.Type", "SuggestedGroupNum": "s.SuggestedPlayers",
        "RewardNextQuest": "s.NextQuestIdChain", "RewardXPDifficulty": "s.RewardXPId", "RewardMoney": "s.RewardMoney",
        "RewardBonusMoney": "s.RewardMoneyMaxLevel", "RewardDisplaySpell": "s.RewardSpell",
        "RewardSpell": "s.RewardSpellCast", "RewardHonor": "s.RewardHonor", "RewardKillHonor": "s.RewardHonorMultiplier",
        "StartItem": "s.SourceItemId", "Flags": "s.Flags", "POIContinent": "s.PointMapId", "POIx": "s.PointX",
        "POIy": "s.PointY", "POIPriority": "s.PointOption", "RewardTitle": "s.RewardTitleId",
        "RewardTalents": "s.RewardTalents", "RewardArenaPoints": "s.RewardArenaPoints", "TimeAllowed": "s.LimitTime",
        "AllowableRaces": "s.RequiredRaces", "LogTitle": "s.Title", "LogDescription": "s.Objectives",
        "QuestDescription": "s.Details", "AreaDescription": "s.EndText", "QuestCompletionLog": "s.CompletedText",
        "RewardReputationMask": "s.RewardReputationMask", "SoundAccept": "s.SoundAccept",
        "SoundTurnIn": "s.SoundTurnIn", "VerifiedBuild": "18414"}
    for i in range(1, 5):
        mapping[f"RewardItem{i}"] = f"s.RewardItemId{i}"
        mapping[f"RewardAmount{i}"] = f"s.RewardItemCount{i}"
        mapping[f"ItemDrop{i}"] = f"s.RequiredSourceItemId{i}"
        mapping[f"ItemDropQuantity{i}"] = f"s.RequiredSourceItemCount{i}"
        mapping[f"RewardCurrencyId{i}"] = f"s.RewardCurrencyId{i}"
        mapping[f"RewardCurrencyCount{i}"] = f"s.RewardCurrencyCount{i}"
    for i in range(1, 7):
        mapping[f"RewardChoiceItemID{i}"] = f"s.RewardChoiceItemId{i}"
        mapping[f"RewardChoiceItemQuantity{i}"] = f"s.RewardChoiceItemCount{i}"
    for i in range(1, 6):
        mapping[f"RewardFactionID{i}"] = f"s.RewardFactionId{i}"
        mapping[f"RewardFactionValue{i}"] = f"s.RewardFactionValueId{i}"
        mapping[f"RewardFactionOverride{i}"] = f"s.RewardFactionValueIdOverride{i}"
    counts["quest_template"] = insert("quest_template", mapping, src("quest_template"), new)
    counts["quest_template_addon"] = insert("quest_template_addon", {
        "ID": "s.Id", "MaxLevel": "s.MaxLevel", "AllowableClasses": "s.RequiredClasses",
        "SourceSpellID": "s.SourceSpellId", "PrevQuestID": "s.PrevQuestId", "NextQuestID": "s.NextQuestId",
        "ExclusiveGroup": "s.ExclusiveGroup", "RewardMailTemplateID": "s.RewardMailTemplateId",
        "RewardMailDelay": "s.RewardMailDelay", "RequiredSkillID": "s.RequiredSkillId",
        "RequiredSkillPoints": "s.RequiredSkillPoints", "RequiredMinRepFaction": "s.RequiredMinRepFaction",
        "RequiredMaxRepFaction": "s.RequiredMaxRepFaction", "RequiredMinRepValue": "s.RequiredMinRepValue",
        "RequiredMaxRepValue": "s.RequiredMaxRepValue", "SpecialFlags": "s.SpecialFlags"},
        src("quest_template"), new)
    counts["quest_details"] = insert("quest_details", {
        "ID": "s.Id", **{f"Emote{i}": f"s.DetailsEmote{i}" for i in range(1, 5)},
        **{f"EmoteDelay{i}": f"s.DetailsEmoteDelay{i}" for i in range(1, 5)}, "VerifiedBuild": "18414"},
        src("quest_template"), f"{new} AND (s.DetailsEmote1<>0 OR s.DetailsEmote2<>0)")
    counts["quest_offer_reward"] = insert("quest_offer_reward", {
        "ID": "s.Id", **{f"Emote{i}": f"s.OfferRewardEmote{i}" for i in range(1, 5)},
        **{f"EmoteDelay{i}": f"s.OfferRewardEmoteDelay{i}" for i in range(1, 5)},
        "RewardText": "s.OfferRewardText", "VerifiedBuild": "18414"},
        src("quest_template"), f"{new} AND IFNULL(s.OfferRewardText,'')<>''")
    counts["quest_request_items"] = insert("quest_request_items", {
        "ID": "s.Id", "EmoteOnComplete": "s.EmoteOnComplete", "EmoteOnIncomplete": "s.EmoteOnIncomplete",
        "CompletionText": "s.RequestItemsText", "VerifiedBuild": "18414"},
        src("quest_template"), f"{new} AND IFNULL(s.RequestItemsText,'')<>''")
    counts["objectives"] = import_quest_objectives(new)
    for table in ("creature_queststarter", "creature_questender", "gameobject_queststarter", "gameobject_questender"):
        counts[table] = insert(table, {"id": "s.id", "quest": "s.quest"}, src(table),
                               "s.quest IN (SELECT ID FROM `{0}`.`_sfdb_new_quest`)".format(ARGS.target))
    return counts


def import_quest_objectives(new):
    """5.4.8 quest_objective rows -> the 4.x objective columns of quest_template (in index order)."""
    count = 0
    slots = {0: ("RequiredNpcOrGo", "RequiredNpcOrGoCount", 4, ""), 2: ("RequiredNpcOrGo", "RequiredNpcOrGoCount", 4, "-"),
             3: ("RequiredNpcOrGo", "RequiredNpcOrGoCount", 4, ""), 1: ("RequiredItemId", "RequiredItemCount", 6, ""),
             4: ("RequiredCurrencyId", "RequiredCurrencyCount", 4, "")}
    rows = run(f"SELECT o.questId, o.`type`, o.objectId, o.amount, IFNULL(o.description,'') "
               f"FROM `{ARGS.source}`.quest_objective o WHERE o.questId IN "
               f"(SELECT ID FROM `{ARGS.target}`.`_sfdb_new_quest`) ORDER BY o.questId, o.`index`")
    used = {}
    for quest, objective_type, object_id, amount, description in rows:
        quest, objective_type = int(quest), int(objective_type)
        sets = []
        if objective_type in slots:
            column, count_column, limit, sign = slots[objective_type]
            # creature and gameobject objectives share one set of columns
            key = (quest, column)
            slot = used.get(key, 0) + 1
            if slot > limit:
                continue
            used[key] = slot
            sets = [f"`{column}{slot}`={sign}{int(object_id)}", f"`{count_column}{slot}`={int(amount)}"]
            if column == "RequiredNpcOrGo" and description:
                sets.append(f"`ObjectiveText{slot}`='{escape(description)}'")
        elif objective_type in (6, 7):
            slot = used.get((quest, "faction"), 0) + 1
            if slot > 2:
                continue
            used[(quest, "faction")] = slot
            sets = [f"`RequiredFactionId{slot}`={int(object_id)}", f"`RequiredFactionValue{slot}`={int(amount)}"]
        elif objective_type == 9:
            sets = [f"`RequiredPlayerKills`={int(amount)}"]
        elif objective_type == 5:
            sets = [f"`RequiredSpell`={int(object_id)}"]
        if sets:
            count += execute(f"UPDATE {tgt('quest_template')} SET {','.join(sets)} WHERE ID={quest}")
    return count


def escape(text):
    return text.replace("\\", "\\\\").replace("'", "\\'")


# --- relations of the new content ---------------------------------------------------------------

def loot_mapping():
    return {"Entry": "s.entry", "Item": "s.item",
            "Reference": "IF(s.mincountOrRef < 0, -s.mincountOrRef, 0)",
            "Chance": "ABS(s.ChanceOrQuestChance)", "QuestRequired": "IF(s.ChanceOrQuestChance < 0, 1, 0)",
            "LootMode": "s.lootmode", "GroupId": "s.groupid",
            "MinCount": "IF(s.mincountOrRef > 0, s.mincountOrRef, 1)", "MaxCount": "s.maxcount",
            "Comment": "'SFDB 548'"}


def import_relations():
    counts = {}
    # loot ids that the target does not use yet
    for table in ("creature_loot_template", "gameobject_loot_template", "reference_loot_template",
                  "pickpocketing_loot_template", "skinning_loot_template", "item_loot_template",
                  "disenchant_loot_template", "prospecting_loot_template", "milling_loot_template",
                  "spell_loot_template", "mail_loot_template", "fishing_loot_template"):
        counts[table] = insert(table, loot_mapping(), src(table),
                               f"s.entry NOT IN (SELECT DISTINCT Entry FROM {tgt(table)})")
    new_creature = f"(SELECT entry FROM `{ARGS.target}`.`_sfdb_new_creature`)"
    counts["npc_vendor"] = insert("npc_vendor", common("npc_vendor", overrides={"VerifiedBuild": "18414"}),
                                  src("npc_vendor"), f"s.entry IN {new_creature}")
    counts["creature_text"] = insert("creature_text", common("creature_text", renames={"CreatureID": "entry",
                                                                                      "GroupID": "groupid", "ID": "id",
                                                                                      "Text": "text", "Type": "type",
                                                                                      "Language": "language",
                                                                                      "Probability": "probability",
                                                                                      "Emote": "emote",
                                                                                      "Duration": "duration",
                                                                                      "Sound": "sound"}),
                                     src("creature_text"), f"s.entry IN {new_creature}")
    imported_guids = f"(SELECT guid - {GUID_OFFSET} FROM {tgt('creature')} WHERE Comment='SFDB 548')"
    counts["smart_scripts"] = insert(
        "smart_scripts", common("smart_scripts", overrides={
            "entryorguid": f"IF(s.entryorguid < 0, s.entryorguid - {GUID_OFFSET}, s.entryorguid)"}),
        src("smart_scripts"),
        f"(s.source_type=0 AND ((s.entryorguid > 0 AND s.entryorguid IN {new_creature}) "
        f"OR (s.entryorguid < 0 AND -s.entryorguid IN {imported_guids}))) "
        f"OR (s.source_type=1 AND s.entryorguid IN (SELECT entry FROM `{ARGS.target}`.`_sfdb_new_gameobject`))")
    counts["waypoint_data"] = insert(
        "waypoint_data", common("waypoint_data", overrides={"id": f"s.id + {PATH_OFFSET}", "wpguid": "0"}),
        src("waypoint_data"),
        f"s.id IN (SELECT path_id FROM `{ARGS.source}`.creature_addon WHERE path_id<>0 AND guid IN {imported_guids})")
    counts["conditions"] = insert("conditions", common("conditions"), src("conditions"),
                                  f"(s.SourceTypeOrReferenceId IN (19, 20) AND s.SourceEntry IN "
                                  f"(SELECT ID FROM `{ARGS.target}`.`_sfdb_new_quest`)) OR "
                                  f"(s.SourceTypeOrReferenceId=22 AND s.SourceEntry IN {new_creature})")
    return counts


# --- character creation -------------------------------------------------------------------------

def import_character_creation():
    counts = {}
    new = f"(s.race IN {NEW_RACES} OR s.class={MONK})"
    for table in ("playercreateinfo", "playercreateinfo_action", "playercreateinfo_item"):
        counts[table] = insert(table, common(table), src(table), new)
    race_mask = sum(1 << (race - 1) for race in NEW_RACES)
    counts["playercreateinfo_spell_custom"] = insert(
        "playercreateinfo_spell_custom", {"racemask": "s.racemask", "classmask": "s.classmask", "Spell": "s.Spell",
                                          "Note": "s.Note"},
        src("playercreateinfo_spell"), f"(s.racemask & {race_mask}) OR (s.classmask & {1 << (MONK - 1)})")
    # player_levelstats keeps race + class stats per level; AC splits them into class stats per level and a
    # race bonus. SFDB's Pandaren rows equal its human rows, so the Pandaren racial modifiers of the 5.4.8
    # client (str 0, agi -2, sta +1, int -1, spi +2) are written directly. Class stats come from SFDB's
    # human rows (human bonus 0) for every class and level, so the 5.4.8 base stats line up; the Monk
    # and levels 86-90 are new rows.
    stats = ("Strength", "Agility", "Stamina", "Intellect", "Spirit")
    sf_stats = ("str", "agi", "sta", "inte", "spi")

    def signed(expression):
        return f"CAST({expression} AS SIGNED)"

    for race in NEW_RACES:
        counts[f"race_{race}"] = execute(
            f"INSERT IGNORE INTO {tgt('player_race_stats')} (Race,{','.join(stats)}) VALUES ({race},0,-2,1,-1,2)")
    counts["class_stats_updated"] = execute(
        f"UPDATE {tgt('player_class_stats')} c JOIN `{ARGS.source}`.player_levelstats l "
        f"ON l.race=1 AND l.class=c.Class AND l.level=c.Level "
        f"SET " + ",".join(f"c.{ac}=l.{sf}" for sf, ac in zip(sf_stats, stats)))
    # BaseHP/BaseMana are not SFDB data (5.4.8 takes them from gtOCTBaseHPByClass/MPByClass): a new
    # level keeps the class's highest known level, the Monk uses the rogue's health and the shaman's mana.
    class_stats = ",".join(f"GREATEST(0, {signed(f'l.{sf}')} - {signed(f'r.{ac}')})" for sf, ac in zip(sf_stats, stats))
    counts["class_stats"] = execute(
        f"INSERT IGNORE INTO {tgt('player_class_stats')} (Class,Level,BaseHP,BaseMana,{','.join(stats)}) "
        f"SELECT l.class, l.level, IFNULL(hp.BaseHP, 0), IFNULL(mana.BaseMana, 0), {class_stats} "
        f"FROM `{ARGS.source}`.player_levelstats l JOIN {tgt('player_race_stats')} r ON r.Race=l.race "
        f"LEFT JOIN {tgt('player_class_stats')} hp ON hp.Class=IF(l.class={MONK}, 4, l.class) "
        f"AND hp.Level=(SELECT MAX(Level) FROM {tgt('player_class_stats')} m "
        f"WHERE m.Class=IF(l.class={MONK}, 4, l.class) AND m.Level<=l.level) "
        f"LEFT JOIN {tgt('player_class_stats')} mana ON mana.Class=IF(l.class={MONK}, 7, l.class) "
        f"AND mana.Level=(SELECT MAX(Level) FROM {tgt('player_class_stats')} m "
        f"WHERE m.Class=IF(l.class={MONK}, 7, l.class) AND m.Level<=l.level) "
        f"WHERE l.race=1")
    return counts


def cleanup():
    for name in ("_sfdb_new_creature", "_sfdb_new_gameobject", "_sfdb_new_quest"):
        run(f"DROP TABLE IF EXISTS `{ARGS.target}`.`{name}`")


PHASES = (("items", import_items), ("creature_templates", import_creature_templates),
          ("gameobject_templates", import_gameobject_templates), ("spawns", import_spawns),
          ("quests", import_quests), ("relations", import_relations),
          ("character_creation", import_character_creation))


def main():
    global ARGS
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--container", default="local")
    parser.add_argument("--user", default="root")
    parser.add_argument("--password", default="")
    parser.add_argument("--source", default="sfdb548")
    parser.add_argument("--target", required=True)
    ARGS = parser.parse_args()
    for name in (ARGS.source, ARGS.target):
        if not re.fullmatch(r"\w+", name):
            parser.error(f"invalid schema name: {name!r}")
    try:
        for name, phase in PHASES:
            print(f"== {name}")
            for table, count in phase().items():
                print(f"{table}: {count}")
    finally:
        cleanup()


if __name__ == "__main__":
    sys.exit(main())
