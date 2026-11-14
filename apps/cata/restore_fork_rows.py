#!/usr/bin/env python3
"""Put the fork's script bindings and 4.3.4 phasing back into a converted world (build_world_base.sh step 4b).

build_world_base.sh snapshots these into _fork_* tables before it empties the content tables:

- _fork_scripts: creature, gameobject and item template ScriptName values. A converted template whose
  ScriptName is empty (TDB names TrinityCore scripts this core does not register, and
  build_clean_world.py strips those) gets the fork's name back when the core registers it.
- _fork_phase_area, _fork_phase_conditions (conditions source type 26), _fork_spell_area: rows the
  converted world does not have are added; TDB rows win where both have the same key.
- _fork_smart_scripts (creature, gameobject and action list scripts by entry): the fork's script of a
  template or action list the converted world has no rows for is added (AIName set to SmartAI where the
  template has no C++ script), with the fork's creature_text groups of creatures that have none.

  restore_fork_rows.py --target acore_world_build [--container local --user root --password ""]
"""

import argparse
import json
import re
import subprocess
import sys

from import_starter_content import mysql_command
from starter_content_inventory import registered_script_names

SCRIPT_TABLES = ((0, "creature_template"), (1, "gameobject_template"), (2, "item_template"))
COPIED = (("phase_area", "_fork_phase_area"), ("conditions", "_fork_phase_conditions"),
          ("spell_area", "_fork_spell_area"))


def execute(args, sql):
    result = subprocess.run(mysql_command(args.container, args.user, args.password, "--database=mysql", "-N", "-B",
                                          stdin=True),
                            input=sql + ";\nSELECT ROW_COUNT();", capture_output=True, text=True, check=False)
    if result.returncode:
        raise RuntimeError(f"query failed: {sql}\n{result.stderr.strip()}")
    lines = [line for line in result.stdout.split("\n") if line]
    return int(lines[-1]) if lines else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--container", default="local")
    parser.add_argument("--user", default="root")
    parser.add_argument("--password", default="")
    parser.add_argument("--target", required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"\w+", args.target):
        parser.error(f"invalid schema name: {args.target!r}")
    db = f"`{args.target}`"

    registered = sorted(registered_script_names())
    names = ",".join(f"X'{name.encode().hex()}'" for name in registered)
    report = {}
    for kind, table in SCRIPT_TABLES:
        report[f"{table}.ScriptName"] = execute(
            args, f"UPDATE {db}.`{table}` t JOIN {db}._fork_scripts f ON f.kind={kind} AND f.entry=t.entry "
                  f"SET t.ScriptName=f.ScriptName WHERE t.ScriptName='' AND BINARY f.ScriptName IN ({names})")
    for table, snapshot in COPIED:
        report[table] = execute(args, f"INSERT IGNORE INTO {db}.`{table}` SELECT * FROM {db}.`{snapshot}`")

    # SmartAI: whole scripts only, never a merge of two scripts for the same owner
    existing = (f"NOT EXISTS (SELECT 1 FROM {db}.smart_scripts s WHERE s.entryorguid=f.entryorguid "
                f"AND s.source_type=f.source_type)")
    owners = (f"(f.source_type=9 OR (f.source_type=0 AND f.entryorguid IN (SELECT entry FROM {db}.creature_template)) "
              f"OR (f.source_type=1 AND f.entryorguid IN (SELECT entry FROM {db}.gameobject_template)))")
    execute(args, f"DROP TABLE IF EXISTS {db}._fork_smart_restore")
    execute(args, f"CREATE TABLE {db}._fork_smart_restore AS SELECT f.* FROM {db}._fork_smart_scripts f "
                  f"WHERE {existing} AND {owners}")
    report["smart_scripts"] = execute(args, f"INSERT IGNORE INTO {db}.smart_scripts SELECT * FROM {db}._fork_smart_restore")
    for source_type, table in ((0, "creature_template"), (1, "gameobject_template")):
        report[f"{table}.AIName"] = execute(
            args, f"UPDATE {db}.`{table}` t SET t.AIName='SmartAI' WHERE t.ScriptName='' AND t.AIName<>'SmartAI' "
                  f"AND t.entry IN (SELECT entryorguid FROM {db}._fork_smart_restore WHERE source_type={source_type})")
    report["creature_text"] = execute(
        args, f"INSERT IGNORE INTO {db}.creature_text SELECT f.* FROM {db}._fork_creature_text f "
              f"WHERE f.CreatureID IN (SELECT entry FROM {db}.creature_template) AND NOT EXISTS "
              f"(SELECT 1 FROM {db}.creature_text t WHERE t.CreatureID=f.CreatureID)")
    execute(args, f"DROP TABLE {db}._fork_smart_restore")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
