#!/usr/bin/env python3
"""Load dp_cli output (CSV on stdin) into a SQLite database.

Example:
    ./build/apps/dp_cli < data/btc_options_sample.csv \
        | python3 scripts/load_results.py --db results.db --snapshot sample
    sqlite3 results.db < sql/reports.sql
"""

import argparse
import csv
import pathlib
import sqlite3
import sys

SCHEMA = pathlib.Path(__file__).resolve().parent.parent / "sql" / "schema.sql"

# dp_cli column -> table column, where the names differ.
RENAMED = {"instrument_name": "instrument", "type": "option_type"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--db", required=True, help="SQLite file, created if missing")
    parser.add_argument("--snapshot", required=True, help="label for this market snapshot")
    args = parser.parse_args()

    sys.stdin.reconfigure(encoding="utf-8-sig")  # tolerate a BOM from Windows tools
    rows = list(csv.DictReader(sys.stdin))
    if not rows:
        print("load_results: no rows on stdin", file=sys.stderr)
        return 1

    columns = ["snapshot"] + [RENAMED.get(name, name) for name in rows[0]]
    placeholders = ", ".join("?" for _ in columns)
    statement = f"INSERT OR REPLACE INTO option_results ({', '.join(columns)}) VALUES ({placeholders})"

    with sqlite3.connect(args.db) as connection:
        connection.executescript(SCHEMA.read_text(encoding="utf-8"))
        connection.executemany(statement, ([args.snapshot, *row.values()] for row in rows))

    print(f"load_results: stored {len(rows)} rows in {args.db}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
