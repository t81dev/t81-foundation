#!/usr/bin/env python3
"""Verify selected byte-identity links without inferring historical authorship."""
import argparse
import hashlib
import json
import subprocess

FOUNDATION = "f6d4d680709c5c6c98b305628e04346f5ef61620"
HANOI = "59205d76db4aca3da7c0bdc521924b8f9bc52cc3"
PARSER_FOUNDATION = "77c77098ad821ecd2c8452c3e7ab4f5eefae1760"
LANG = "dc9ec92044876651aa2531b2879413285b388961"
LEGACY = "fe00396831c8bb249fde06a04186fa02f6018caf"


def git(root, *args):
    return subprocess.check_output(["git", "-C", root, *args])


parser = argparse.ArgumentParser(description=__doc__)
for name in ["foundation", "hanoi", "lang"]:
    parser.add_argument("--" + name + "-root", required=True)
a = parser.parse_args()
old = git(a.foundation_root, "show", LEGACY + ":legacy/hanoivm/src/hanoivm_core/t81recursion.cweb")
new = git(a.hanoi_root, "show", HANOI + ":src/t81recursion.cweb")
assert old == new
p_old = git(a.foundation_root, "show", PARSER_FOUNDATION + ":src/frontend/parser.cpp")
p_new = git(a.lang_root, "show", LANG + ":src/frontend/parser.cpp")
assert p_old == p_new
subprocess.run(["git", "-C", a.foundation_root, "merge-base", "--is-ancestor", LEGACY, FOUNDATION], check=True)
objects = {}
for line in git(a.foundation_root, "rev-list", "--objects", FOUNDATION).decode().splitlines():
    fields = line.split(" ", 1)
    if len(fields) == 2:
        objects.setdefault(fields[0], []).append(fields[1])
shared = []
for line in git(a.hanoi_root, "ls-tree", "-r", HANOI).decode().splitlines():
    meta, path = line.split("\t", 1)
    blob = meta.split()[2]
    if path.endswith((".cweb", ".h", ".c", ".cpp", ".rs", ".td")) and blob in objects:
        shared.append({"hanoi_path": path, "blob": blob, "foundation_paths": objects[blob]})
assert len(shared) == 41
print(json.dumps({"foundation_scope": FOUNDATION + " reachable history only",
                  "hanoi_pin": HANOI, "shared_source_blob_count": len(shared),
                  "recursion_sha256": hashlib.sha256(old).hexdigest(),
                  "parser_sha256": hashlib.sha256(p_old).hexdigest(),
                  "legacy_is_ancestor_of_main_pin": True,
                  "shared_sources": shared,
                  "limit": "source identity and recorded ancestry, not original invention/authorship or whole-system equivalence"}, indent=2))
