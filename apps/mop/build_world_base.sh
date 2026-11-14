#!/usr/bin/env bash
# Rebuild data/sql/base/db_world for the 5.4.8 branch: the 4.3.4 world from the newest TDB 434 (apps/cata)
# plus the 5.4.8 content of the newest SkyFire world release (SFDB 548), AzerothCore squash style.
#
#   build_world_base.sh <TDB_full_world_434.sql> <TC sql/updates/world/4.3.4 dir> \
#                       <SFDB_full_548.sql> <SkyFire sql/updates/world dir>
#
# Environment: DBC_DIR (4.3.4 client dbc directory, for the 4.3.4 phase), CONTAINER, DB_USER, DB_PASSWORD,
# TARGET_DB, KEEP_UPDATES as in apps/cata/build_world_base.sh; SFDB_DB (default sfdb_world) is the schema
# the SkyFire dump is loaded into. DBC_DIR_548 (5.4.8 client DBFilesClient directory) lets import_sfdb.py
# derive item armor/damage and BaseHP/BaseMana from the 5.4.8 tables.
#
# BASE_DB skips the 4.3.4 phase: that schema (a finished apps/cata build, for example the one the 4.3.4
# branch exported its base files from) is copied into TARGET_DB and the pending world updates it has not
# recorded are applied on top.
set -euo pipefail

TDB_SQL=${1:?TDB_full_world_434 .sql file}
TC_UPDATES=${2:?TrinityCore sql/updates/world/4.3.4 directory}
SFDB_SQL=${3:?SFDB_full_548 .sql file}
SF_UPDATES=${4:?SkyFire sql/updates/world directory}
CONTAINER=${CONTAINER:-local}
DB_USER=${DB_USER:-root}
DB_PASSWORD=${DB_PASSWORD:-}
TARGET_DB=${TARGET_DB:-acore_world_build}
SFDB_DB=${SFDB_DB:-sfdb_world}

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
[[ $SFDB_DB =~ ^[A-Za-z0-9_]+$ ]] || { echo "invalid schema name" >&2; exit 2; }

credentials=(-u"$DB_USER")
[[ -n $DB_PASSWORD ]] && credentials+=(-p"$DB_PASSWORD")
if [[ $CONTAINER == local ]]; then
    mysql_run() { command mysql "${credentials[@]}" --max_allowed_packet=1G "$@"; }
else
    mysql_run() { docker exec -i "$CONTAINER" mysql "${credentials[@]}" --max_allowed_packet=1G "$@"; }
fi

# SkyFire dumps come from MySQL 8 and select their own schema
load_sfdb() {
    sed 's/utf8mb4_0900_ai_ci/utf8mb4_unicode_ci/g; /^CREATE DATABASE/d; /^USE `/d' "$1" | mysql_run "$SFDB_DB"
}

mysqldump_run() {
    if [[ $CONTAINER == local ]]; then
        command mysqldump "${credentials[@]}" "$@"
    else
        docker exec "$CONTAINER" mysqldump "${credentials[@]}" "$@"
    fi
}

mysql_run -e "SET GLOBAL max_allowed_packet = 1073741824" \
    || echo "warning: could not raise max_allowed_packet; set it to 1G on the server" >&2

if [[ -n ${BASE_DB:-} ]]; then
    [[ $BASE_DB =~ ^[A-Za-z0-9_]+$ && $TARGET_DB =~ ^[A-Za-z0-9_]+$ ]] || { echo "invalid schema name" >&2; exit 2; }
    echo "== 4.3.4 world copied from $BASE_DB"
    mysql_run -e "DROP DATABASE IF EXISTS \`$TARGET_DB\`; CREATE DATABASE \`$TARGET_DB\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
    mysqldump_run --skip-tz-utc --max_allowed_packet=1G "$BASE_DB" | mysql_run "$TARGET_DB"
    recorded="$(mysql_run -N -B -e "SELECT name FROM \`$TARGET_DB\`.updates")"
    for update in "$root"/data/sql/updates/pending_db_world/*.sql; do
        [[ -e $update ]] || continue
        name="$(basename "$update")"
        grep -qxF -e "$name" -e "cata_$name" <<< "$recorded" && continue
        echo "applying $name"
        mysql_run "$TARGET_DB" < "$update"
    done
else
    echo "== 4.3.4 world"
    SKIP_EXPORT=1 CONTAINER="$CONTAINER" DB_USER="$DB_USER" DB_PASSWORD="$DB_PASSWORD" TARGET_DB="$TARGET_DB" \
        bash "$root/apps/cata/build_world_base.sh" "$TDB_SQL" "$TC_UPDATES"
fi

echo "== $SFDB_DB from $(basename "$SFDB_SQL")"
mysql_run -e "DROP DATABASE IF EXISTS \`$SFDB_DB\`; CREATE DATABASE \`$SFDB_DB\` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
load_sfdb "$SFDB_SQL"
while IFS= read -r -d '' update; do
    load_sfdb "$update" || echo "warning: $(basename "$update") failed" >&2
done < <(find "$SF_UPDATES" -maxdepth 1 -name '*.sql' -print0 | sort -z)

echo "== 5.4.8 content"
python3 "$here/import_sfdb.py" --container "$CONTAINER" --user "$DB_USER" --password "$DB_PASSWORD" \
    --source "$SFDB_DB" --target "$TARGET_DB" ${DBC_DIR_548:+--dbc-dir "$DBC_DIR_548"}

echo "== exporting data/sql/base/db_world"
ARCHIVE_NAME=mop CONTAINER="$CONTAINER" DB_USER="$DB_USER" DB_PASSWORD="$DB_PASSWORD" KEEP_UPDATES="${KEEP_UPDATES:-0}" \
    bash "$root/apps/cata/export_world_base.sh" "$TARGET_DB"
