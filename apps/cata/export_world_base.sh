#!/usr/bin/env bash
# Write a built world schema to data/sql/base/db_world (AzerothCore DatabaseExporter format) and archive the
# applied updates the way an AzerothCore database squash does.
#   export_world_base.sh <world_schema>
# CONTAINER ("local" default), DB_USER, DB_PASSWORD as in build_world_base.sh; KEEP_UPDATES=1 skips the archive.
set -euo pipefail

TARGET_DB=${1:?world schema}
CONTAINER=${CONTAINER:-local}
DB_USER=${DB_USER:-root}
DB_PASSWORD=${DB_PASSWORD:-}
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
[[ $TARGET_DB =~ ^[A-Za-z0-9_]+$ ]] || { echo "invalid schema name" >&2; exit 2; }

credentials=(-u"$DB_USER")
[[ -n $DB_PASSWORD ]] && credentials+=(-p"$DB_PASSWORD")
if [[ $CONTAINER == local ]]; then
    mysql_run() { command mysql "${credentials[@]}" "$@"; }
    mysqldump_run() { command mysqldump "${credentials[@]}" "$@"; }
else
    mysql_run() { docker exec -i "$CONTAINER" mysql "${credentials[@]}" "$@"; }
    mysqldump_run() { docker exec "$CONTAINER" mysqldump "${credentials[@]}" "$@"; }
fi

base="$root/data/sql/base/db_world"
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

tables=$(mysql_run -N -B -e "SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA='$TARGET_DB' AND TABLE_TYPE='BASE TABLE' AND TABLE_NAME NOT LIKE '\\_%' ORDER BY TABLE_NAME")
find "$base" -maxdepth 1 -name '*.sql' -delete
for table in $tables; do
    mysqldump_run --skip-tz-utc --extended-insert "$TARGET_DB" "$table" |
        sed -E 's/ VALUES \(/ VALUES\n(/; :a; s/\),\(/\),\n\(/g; ta' > "$base/$table.sql"
done

echo "== done: $(echo "$tables" | wc -w) tables in data/sql/base/db_world"
