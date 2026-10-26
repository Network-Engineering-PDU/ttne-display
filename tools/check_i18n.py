#!/usr/bin/env python3
"""Checks the touchscreen translations (src/i18n_strings.c).

- Every tr("...") and N_("...") text in the sources has an entry.
- Every translation keeps the printf conversions (%s, %d, %.1f...) of the
  English text in the same order: a mismatch would crash the program.
- Every translation has as many '#' as the English text, so LVGL recolor
  commands ("#RRGGBB text#") stay balanced.
- Entries no source uses are reported, so the table doesn't grow stale.

Run from the repository root: python3 tools/check_i18n.py
"""
import glob
import re
import sys

MACROS = {"TT_COLOR_GREEN_NE_STR": '"#A1C61A"'}
LITERAL = r'"(?:[^"\\]|\\.)*"'
SEQUENCE = r'(?:\s*(?:' + LITERAL + r'|' + "|".join(MACROS) + r'))+'
CALL = re.compile(r'\b(?:tr|N_)\((' + SEQUENCE + r')\s*\)')
CONVERSION = re.compile(r'%[-+ #0]*\*?\d*(?:\.\*?\d+)?(?:hh|h|ll|l|z)?[diouxXeEfFgGcsp%]')


def join_literals(seq):
    for name, value in MACROS.items():
        seq = seq.replace(name, value)
    parts = re.findall(LITERAL, seq)
    return "".join(bytes(p[1:-1], "utf-8").decode("unicode_escape").encode("latin-1").decode("utf-8")
                   for p in parts)


def source_keys():
    keys = {}
    for path in glob.glob("src/*.c") + glob.glob("backend/*.c") + glob.glob("app/*.c"):
        if path.endswith("i18n_strings.c"):
            continue
        text = open(path, encoding="utf-8").read()
        for m in CALL.finditer(text):
            keys.setdefault(join_literals(m.group(1)), path)
    return keys


def table():
    text = open("src/i18n_strings.c", encoding="utf-8").read()
    body = text[text.index("I18N_STRINGS[] = {"):]
    entries = []
    for block in re.findall(r'\{((?:\s*' + SEQUENCE + r'\s*,){5})\s*\}', body):
        cols = [join_literals(c) for c in re.findall(SEQUENCE + r'(?=\s*,)', block)]
        entries.append(cols)
    return entries


def main():
    errors = []
    entries = table()
    keys = {e[0]: e for e in entries}
    if len(keys) != len(entries):
        errors.append("duplicate English keys in the table")
    used = source_keys()
    for key, path in sorted(used.items()):
        if key not in keys:
            errors.append(f"{path}: no translation entry for {key!r}")
    names = ["es", "de", "zh", "ar"]
    for e in entries:
        conv = CONVERSION.findall(e[0])
        for name, text in zip(names, e[1:]):
            if CONVERSION.findall(text) != conv:
                errors.append(f"{name}: conversions differ from English in {e[0]!r}")
            if text.count("#") != e[0].count("#"):
                errors.append(f"{name}: '#' count differs from English in {e[0]!r}")
    unused = sorted(set(keys) - set(used))
    for key in unused:
        print(f"note: entry not used by the sources: {key!r}")
    for err in errors:
        print("error:", err)
    print(f"{len(entries)} entries, {len(used)} texts in the sources, {len(errors)} errors")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
