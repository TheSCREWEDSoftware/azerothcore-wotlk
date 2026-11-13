#!/usr/bin/env bash
# Write a ZONES_FILE for import_clean_zones.sh with every zone that has TDB 434 quests.
#   list_quest_zones.sh [source_schema] > zones.txt
# Each zone's map is the map most of its creature spawns are on; zones without spawns are skipped.
set -euo pipefail
SOURCE=${1:-tdb_world}
[[ $SOURCE =~ ^[A-Za-z0-9_]+$ ]] || { echo "invalid schema name" >&2; exit 2; }
credentials=(-u"${DB_USER:-root}")
[[ -n ${DB_PASSWORD:-} ]] && credentials+=(-p"$DB_PASSWORD")
if [[ ${CONTAINER:-local} == local ]]; then
    run() { mysql "${credentials[@]}" -N -B -e "$1"; }
else
    run() { docker exec "$CONTAINER" mysql "${credentials[@]}" -N -B -e "$1"; }
fi
run "SELECT m.zone, m.map, CONCAT('zone', m.zone)
     FROM (SELECT zoneId AS zone, map, ROW_NUMBER() OVER (PARTITION BY zoneId ORDER BY COUNT(*) DESC) AS rank_in_zone
           FROM \`$SOURCE\`.creature WHERE zoneId<>0 GROUP BY zoneId, map) m
     JOIN (SELECT DISTINCT QuestSortID AS zone FROM \`$SOURCE\`.quest_template WHERE QuestSortID>0) z ON z.zone=m.zone
     WHERE m.rank_in_zone=1 ORDER BY m.zone"
