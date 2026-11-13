#!/usr/bin/env bash
# Rebuild data/sql/base/db_world from the newest TDB 434 world release (AzerothCore squash style).
#
#   build_world_base.sh <TDB_full_world_434.sql> <TrinityCore sql/updates/world/4.3.4 dir>
#
# Environment:
#   DBC_DIR       4.3.4 client dbc directory (Item.db2, Item-sparse.db2, Phase.dbc, WorldSafeLocs.dbc ...)
#   CONTAINER     "local" (default) for a local MySQL/MariaDB server, or the name of a MySQL docker container
#   DB_USER       MySQL user (default root); DB_PASSWORD (default empty)
#   SOURCE_DB     schema the TDB dump is loaded into (default tdb_world)
#   TARGET_DB     schema the AzerothCore world is built in (default acore_world_build)
#   KEEP_UPDATES  set to 1 to leave data/sql/updates in place (no archive step)
#   ZONES_FILE    zone list for the quest import (default: every zone with TDB quests, list_quest_zones.sh)
#
# Steps:
#   1. load the TDB dump and every TrinityCore 4.3.4 world update after it into SOURCE_DB
#   2. build TARGET_DB from this repository's data/sql (base, updates, pending), as worldserver would
#   3. empty the world content tables; *_dbc, acore_string, player stats and spell overrides stay
#   4. convert the TDB content (build_clean_world.py), the client items (convert_items_from_db2.py)
#      and the zone quests (import_clean_zones.sh)
#   5. dump every TARGET_DB table to data/sql/base/db_world in the DatabaseExporter format
#   6. move data/sql/updates/db_world and pending_db_world to data/sql/old/db_world/cata and record
#      them as ARCHIVED in the dumped `updates` table, like an AzerothCore database squash
set -euo pipefail

TDB_SQL=${1:?TDB_full_world_434 .sql file}
TC_UPDATES=${2:?TrinityCore sql/updates/world/4.3.4 directory}
: "${DBC_DIR:?set DBC_DIR to the 4.3.4 client dbc directory}"
CONTAINER=${CONTAINER:-local}
DB_USER=${DB_USER:-root}
DB_PASSWORD=${DB_PASSWORD:-}
SOURCE_DB=${SOURCE_DB:-tdb_world}
TARGET_DB=${TARGET_DB:-acore_world_build}

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
[[ $SOURCE_DB =~ ^[A-Za-z0-9_]+$ && $TARGET_DB =~ ^[A-Za-z0-9_]+$ ]] || { echo "invalid schema name" >&2; exit 2; }

mysql_run() {
    local credentials=(-u"$DB_USER")
    [[ -n $DB_PASSWORD ]] && credentials+=(-p"$DB_PASSWORD")
    if [[ $CONTAINER == local ]]; then
        command mysql "${credentials[@]}" --max_allowed_packet=1G "$@"
    else
        docker exec -i "$CONTAINER" mysql "${credentials[@]}" --max_allowed_packet=1G "$@"
    fi
}

mysqldump_run() {
    local credentials=(-u"$DB_USER")
    [[ -n $DB_PASSWORD ]] && credentials+=(-p"$DB_PASSWORD")
    if [[ $CONTAINER == local ]]; then
        command mysqldump "${credentials[@]}" "$@"
    else
        docker exec "$CONTAINER" mysqldump "${credentials[@]}" "$@"
    fi
}

recreate() {
    mysql_run -e "DROP DATABASE IF EXISTS \`$1\`; CREATE DATABASE \`$1\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
}

# MySQL 8 dumps name a collation MariaDB does not know
load() {
    sed 's/utf8mb4_0900_ai_ci/utf8mb4_unicode_ci/g' "$2" | mysql_run "$1"
}

echo "== 1. $SOURCE_DB from $(basename "$TDB_SQL")"
recreate "$SOURCE_DB"
load "$SOURCE_DB" "$TDB_SQL"
while IFS= read -r -d '' update; do
    load "$SOURCE_DB" "$update" || echo "warning: $(basename "$update") failed" >&2
done < <(find "$TC_UPDATES" -maxdepth 1 -name '*.sql' -print0 | sort -z)

echo "== 2. $TARGET_DB from data/sql"
recreate "$TARGET_DB"
for file in "$root"/data/sql/base/db_world/*.sql; do
    load "$TARGET_DB" "$file"
done
for dir in updates/db_world updates/pending_db_world; do
    while IFS= read -r -d '' update; do
        load "$TARGET_DB" "$update" || echo "warning: $dir/$(basename "$update") failed" >&2
    done < <(find "$root/data/sql/$dir" -maxdepth 1 -name '*.sql' -print0 | sort -z)
done

echo "== 3. emptying the world content tables"
# The tables build_clean_world.py and the zone import refill from TDB. Everything else (*_dbc,
# acore_string, player stats, spell overrides, game events, instance and dungeon data) stays.
content_tables="gameobject_template gameobject_template_addon creature_template creature_template_addon
creature_template_model creature_template_movement creature_template_spell creature_template_resistance
creature_model_info creature_equip_template creature_onkill_reputation creature_default_trainer
npc_spellclick_spells vehicle_template_accessory vehicle_seat_addon creature gameobject creature_addon
gameobject_addon creature_formations waypoint_data waypoint_scripts waypoints creature_text
game_event_creature game_event_gameobject smart_scripts phase_area spell_area conditions
creature_loot_template gameobject_loot_template reference_loot_template pickpocketing_loot_template
skinning_loot_template disenchant_loot_template prospecting_loot_template milling_loot_template
spell_loot_template item_loot_template mail_loot_template fishing_loot_template broadcast_text npc_vendor
trainer trainer_spell pool_template pool_creature pool_gameobject pool_pool npc_text page_text gossip_menu
gossip_menu_option points_of_interest spell_target_position quest_template quest_template_addon
quest_details quest_offer_reward quest_request_items quest_poi quest_poi_points creature_queststarter
creature_questender gameobject_queststarter gameobject_questender creature_questitem gameobject_questitem
item_template"
for table in $content_tables; do echo "DELETE FROM \`$TARGET_DB\`.\`$table\`;"; done | mysql_run
mysql_run -e "DELETE FROM \`$TARGET_DB\`.creature_immunities WHERE ID >= 1000000"

echo "== 4. converting"
python3 "$here/build_clean_world.py" --container "$CONTAINER" --user "$DB_USER" --password "$DB_PASSWORD" \
    --source "$SOURCE_DB" --target "$TARGET_DB" --dbc-dir "$DBC_DIR"
python3 "$here/convert_items_from_db2.py" "$DBC_DIR/Item.db2" "$DBC_DIR/Item-sparse.db2" --all --dbc-dir "$DBC_DIR" |
    mysql_run "$TARGET_DB"
# every zone with TDB quests unless ZONES_FILE names a smaller set
zones_file=${ZONES_FILE:-$(mktemp)}
[[ -n ${ZONES_FILE:-} ]] || CONTAINER="$CONTAINER" DB_USER="$DB_USER" DB_PASSWORD="$DB_PASSWORD" \
    bash "$here/list_quest_zones.sh" "$SOURCE_DB" > "$zones_file"
CONTAINER="$CONTAINER" DB_USER="$DB_USER" DB_PASSWORD="$DB_PASSWORD" DBC_DIR="$DBC_DIR" SOURCE_DB="$SOURCE_DB" \
    ZONES_FILE="$zones_file" bash "$here/import_clean_zones.sh" "$TARGET_DB"

echo "== 5. exporting data/sql/base/db_world"
base="$root/data/sql/base/db_world"
tables=$(mysql_run -N -B -e "SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA='$TARGET_DB' AND TABLE_TYPE='BASE TABLE' ORDER BY TABLE_NAME")
archive="$root/data/sql/old/db_world/cata"
if [[ ${KEEP_UPDATES:-0} != 1 ]]; then
    # record every applied update as ARCHIVED before the `updates` table is dumped
    mkdir -p "$archive"
    for dir in updates/db_world updates/pending_db_world; do
        while IFS= read -r -d '' update; do
            name=$(basename "$update")
            [[ $dir == updates/pending_db_world ]] && name="cata_$name"
            hash=$(sha1sum "$update" | cut -c1-40 | tr a-f A-F)
            mysql_run -e "REPLACE INTO \`$TARGET_DB\`.updates (name, hash, state, speed) VALUES ('$name', '$hash', 'ARCHIVED', 0)"
            git -C "$root" mv "$update" "$archive/$name" 2>/dev/null || mv "$update" "$archive/$name"
        done < <(find "$root/data/sql/$dir" -maxdepth 1 -name '*.sql' -print0 | sort -z)
    done
fi
find "$base" -maxdepth 1 -name '*.sql' -delete
for table in $tables; do
    mysqldump_run --skip-tz-utc --extended-insert "$TARGET_DB" "$table" |
        sed -E 's/VALUES[[:space:]]*/VALUES\n/; :a; s/\),\(/\),\n\(/g; ta' > "$base/$table.sql"
done

echo "== done: $(echo "$tables" | wc -w) tables in data/sql/base/db_world"
