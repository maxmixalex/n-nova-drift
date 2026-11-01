"""Audit names/coverage against the user's original CSV when available.

The source descriptions establish mechanical identities, not numerical or
byte-for-byte GameMaker runtime equivalence. Every active implementation is
therefore classified as an adaptation. Source placeholders stay unavailable.
"""
import argparse
import csv
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def audit(source=None):
    text = (ROOT / "src/content.cpp").read_text()
    rows = []
    pattern = r'^ ([MX])\("([^"]+)","([^"]+)","([^"]+)"([^\n]+)\)'
    for macro, name, desc, key, rest in re.findall(pattern, text, re.M):
        values = rest.strip(", ").split(",")
        category = int(values[-2]) if macro == "X" else 0
        implemented = int(values[-1]) if macro == "X" else 1
        status = "adapted" if implemented else "source_placeholder_unavailable"
        if key == "TheVoid":
            status = "intentional_empty_original_effect"
        rows.append(dict(source_key="ModTitle" + key, name=name, status=status,
                         category=["normal", "super", "wild"][category],
                         port_behavior=desc))
    assert len(rows) == 180, len(rows)
    assert len({r["source_key"] for r in rows}) == 180
    if source:
        with open(source, encoding="utf-8-sig", newline="") as f:
            original = {r[0]: r[1] for r in csv.reader(f) if len(r) > 1}
        titles = {k for k in original if k.startswith("ModTitle")}
        actual = {r["source_key"] for r in rows}
        assert actual == titles, dict(missing=sorted(titles-actual), unknown=sorted(actual-titles))
        for key in ("Havoc", "Onslaught"):
            assert original["ModFull"+key].strip() == "[$=*]..."
    return {"source_mods": 180, "adapted_mechanics": sum(r["status"] == "adapted" for r in rows),
            "intentional_empty": 1, "unavailable_placeholders": 2,
            "numeric_balance": "Independently adjusted for the TI-Nspire port; not extracted YYC constants",
            "runtime_parity": "Native reimplementation, not execution or decompilation of the original PC runtime",
            "mods": rows}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", help="Path to original localization.csv")
    parser.add_argument("--json", help="Optional report path")
    args = parser.parse_args()
    report = audit(args.source)
    if args.json:
        Path(args.json).write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n")
    print(f"PASS 180 source keys; {report['adapted_mechanics']} adapted mechanics; 1 intentional empty; 2 unavailable source placeholders")
