#!/usr/bin/env python3
"""Download a snapshot of Deribit option quotes as CSV for dp_cli.

Uses the public REST endpoint public/get_book_summary_by_currency,
which needs no API key. Only the Python standard library is used.

Example:
    python3 scripts/fetch_deribit.py --currency BTC > data/btc_options.csv
"""

import argparse
import csv
import json
import sys
import urllib.request

API_URL = "https://www.deribit.com/api/v2/public/get_book_summary_by_currency"
COLUMNS = ["instrument_name", "underlying_price", "mark_price", "mark_iv", "creation_timestamp"]


def fetch_option_summaries(currency: str) -> list[dict]:
    url = f"{API_URL}?currency={currency}&kind=option"
    with urllib.request.urlopen(url, timeout=30) as response:
        payload = json.load(response)
    if "result" not in payload:
        raise RuntimeError(f"unexpected Deribit response: {payload.get('error')}")
    return payload["result"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--currency", default="BTC", help="BTC, ETH, ... (default: BTC)")
    parser.add_argument("--limit", type=int, default=0, help="keep only the first N instruments")
    args = parser.parse_args()

    summaries = sorted(fetch_option_summaries(args.currency), key=lambda s: s["instrument_name"])
    if args.limit > 0:
        summaries = summaries[: args.limit]

    writer = csv.writer(sys.stdout, lineterminator="\n")
    writer.writerow(COLUMNS)
    for summary in summaries:
        writer.writerow(summary[column] for column in COLUMNS)
    return 0


if __name__ == "__main__":
    sys.exit(main())
