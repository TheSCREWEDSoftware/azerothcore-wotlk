#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

if [[ "$PROJECT_ROOT" =~ ^/([a-zA-Z])/(.*) ]]; then
  DRIVE_LETTER="${BASH_REMATCH[1]}"
  PATH_REMAINDER="${BASH_REMATCH[2]}"
  PROJECT_ROOT="${DRIVE_LETTER^^}:/${PATH_REMAINDER}"
fi

BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build/dbimport-package}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$PROJECT_ROOT/env/dist}"
ARCHIVE_DIR="${ARCHIVE_DIR:-$PROJECT_ROOT/var/db-archives}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
JOBS="${JOBS:-}"

DB_HOST="${DB_HOST:-localhost}"
DB_PORT="${DB_PORT:-3306}"
DB_USER="${DB_USER:-}"
DB_PASS="${DB_PASS:-}"
DB_AUTH="${DB_AUTH:-acore_auth}"
DB_CHARACTERS="${DB_CHARACTERS:-acore_characters}"
DB_WORLD="${DB_WORLD:-acore_world}"

prompt_if_empty() {
  local __prompt="$1"
  local __value="$2"

  if [[ -n "$__value" ]]; then
    printf '%s' "$__value"
    return 0
  fi

  read -r -p "$__prompt" __value
  printf '%s' "$__value"
}

get_acore_major_version() {
  python - "$PROJECT_ROOT/acore.json" <<'PY'
import json
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
data = json.loads(path.read_text(encoding='utf-8'))
version = str(data.get('version', ''))
major = version.split('.', 1)[0]
print(major)
PY
}

create_zip_archive() {
  local archive_path="$1"
  local label_text="$2"
  local git_short_hash="$3"
  local date_label="$4"
  local source_dir="$5"

  python - "$PROJECT_ROOT" "$archive_path" "$label_text" "$git_short_hash" "$date_label" "$source_dir" <<'PY'
import pathlib
import sys
import zipfile

project_root = pathlib.Path(sys.argv[1])
archive_path = pathlib.Path(sys.argv[2])
label_text = sys.argv[3]
git_short_hash = sys.argv[4]
date_label = sys.argv[5]
source_dir = pathlib.Path(sys.argv[6])

manifest = (
    f'{label_text}\n'
    f'Revision: {git_short_hash}\n'
    f'Date: {date_label}\n'
    f'Package: dbimport install tree\n'
)

archive_path.parent.mkdir(parents=True, exist_ok=True)

with zipfile.ZipFile(archive_path, 'w', compression=zipfile.ZIP_DEFLATED) as zip_file:
  for file_path in source_dir.rglob('*'):
        if file_path.is_file():
            zip_file.write(file_path, file_path.relative_to(project_root))

    zip_file.writestr('db-package.txt', manifest)
PY
}

DB_USER="$(prompt_if_empty "Enter MySQL username: " "$DB_USER")"
DB_PASS="$(prompt_if_empty "Enter MySQL password: " "$DB_PASS")"
DB_HOST="$(prompt_if_empty "Enter MySQL host (default: localhost): " "$DB_HOST")"
DB_PORT="$(prompt_if_empty "Enter MySQL port (default: 3306): " "$DB_PORT")"
DB_AUTH="$(prompt_if_empty "Enter name of Auth database [default: acore_auth]: " "$DB_AUTH")"
DB_CHARACTERS="$(prompt_if_empty "Enter name of Characters database [default: acore_characters]: " "$DB_CHARACTERS")"
DB_WORLD="$(prompt_if_empty "Enter name of World database [default: acore_world]: " "$DB_WORLD")"

mkdir -p "$BUILD_DIR" "$INSTALL_PREFIX" "$ARCHIVE_DIR"

cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DMODULES=none \
  -DAPPS_BUILD=db-only \
  -DTOOLS_BUILD=none \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"

if [[ -n "$JOBS" ]]; then
  cmake --build "$BUILD_DIR" --config "$CMAKE_BUILD_TYPE" --target dbimport -j "$JOBS"
else
  cmake --build "$BUILD_DIR" --config "$CMAKE_BUILD_TYPE" --target dbimport
fi

cmake --install "$BUILD_DIR" --config "$CMAKE_BUILD_TYPE"

CONFIG_DIR=""
for candidate in \
  "$INSTALL_PREFIX/etc" \
  "$INSTALL_PREFIX/configs"
do
  if [[ -f "$candidate/dbimport.conf.dist" ]]; then
    CONFIG_DIR="$candidate"
    break
  fi
done

if [[ -z "$CONFIG_DIR" ]]; then
  echo "Unable to find dbimport.conf.dist under the install prefix."
  exit 1
fi

if [[ ! -f "$CONFIG_DIR/dbimport.conf" ]]; then
  cp "$CONFIG_DIR/dbimport.conf.dist" "$CONFIG_DIR/dbimport.conf"
fi

DBIMPORT_EXE="$INSTALL_PREFIX/bin/dbimport"
if [[ ! -x "$DBIMPORT_EXE" && -x "$INSTALL_PREFIX/bin/dbimport.exe" ]]; then
  DBIMPORT_EXE="$INSTALL_PREFIX/bin/dbimport.exe"
fi

if [[ ! -x "$DBIMPORT_EXE" && -x "$INSTALL_PREFIX/dbimport.exe" ]]; then
  DBIMPORT_EXE="$INSTALL_PREFIX/dbimport.exe"
fi

if [[ ! -x "$DBIMPORT_EXE" ]]; then
  echo "Unable to find the dbimport executable under the install prefix."
  exit 1
fi

export AC_UPDATES_ALLOWED_MODULES=""
export AC_LOGIN_DATABASE_INFO="${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};${DB_AUTH}"
export AC_CHARACTER_DATABASE_INFO="${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};${DB_CHARACTERS}"
export AC_WORLD_DATABASE_INFO="${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};${DB_WORLD}"

if [[ "${RUN_DBIMPORT:-1}" != "0" ]]; then
  "$DBIMPORT_EXE" --config "$CONFIG_DIR/dbimport.conf"
fi

GIT_SHORT_HASH="$(git -C "$PROJECT_ROOT" rev-parse --short HEAD)"
DATE_LABEL="$(date +%d-%m-%Y)"
ACORE_MAJOR_VERSION="$(get_acore_major_version)"
ARCHIVE_NAME="${ARCHIVE_NAME:-db-${GIT_SHORT_HASH}.zip}"
ARCHIVE_PATH="$ARCHIVE_DIR/$ARCHIVE_NAME"
LABEL_TEXT="Development DB ${DATE_LABEL} - ACDB ${ACORE_MAJOR_VERSION}"

create_zip_archive "$ARCHIVE_PATH" "$LABEL_TEXT" "$GIT_SHORT_HASH" "$DATE_LABEL" "$INSTALL_PREFIX"

echo "Created archive: $ARCHIVE_PATH"