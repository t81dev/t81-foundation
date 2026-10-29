#!/usr/bin/env python3
"""Fail-closed PR triage. No merges; closures require explicit pinned authorization."""
import argparse
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone

API = "https://api.github.com"

def api(method, path, payload=None):
    request = urllib.request.Request(
        API + path,
        data=None if payload is None else json.dumps(payload).encode(),
        method=method,
        headers={"Authorization": "Bearer " + os.environ["GITHUB_TOKEN"],
                 "Accept": "application/vnd.github+json",
                 "X-GitHub-Api-Version": "2022-11-28",
                 "Content-Type": "application/json"},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)

def pages(path):
    items = []
    for page in range(1, 51):
        sep = "&" if "?" in path else "?"
        batch = api("GET", f"{path}{sep}per_page=100&page={page}")
        items.extend(batch)
        if len(batch) < 100:
            return items
    raise RuntimeError("pagination limit exceeded: refusing partial inventory")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--authorizations", default="docs/governance/pr-triage-closures.json")
    parser.add_argument("--output", default="pr-triage-report.json")
    args = parser.parse_args()
    repo = os.environ["GITHUB_REPOSITORY"]
    root = "/repos/" + repo
    prs = pages(root + "/pulls?state=open")
    authorizations = json.load(open(args.authorizations, encoding="utf-8"))
    if authorizations.get("schema_version") != 1 or not isinstance(authorizations.get("closures"), list):
        raise ValueError("invalid authorization document")
    by_number = {p["number"]: p for p in prs}
    decisions = []
    for p in prs:
        number = p["number"]
        decisions.append({"number": number, "head_sha": p["head"]["sha"],
                          "draft": p["draft"], "base": p["base"]["ref"],
                          "decision": "HOLD_REVIEW", "reason": "No conclusive authorized supersession"})
    by_decision = {d["number"]: d for d in decisions}
    seen = set()
    for entry in authorizations["closures"]:
        number = entry["pr"]
        successor_number = entry["merged_successor"]
        expected = entry["head_sha"]
        if number in seen:
            raise ValueError("duplicate closure authorization")
        seen.add(number)
        if not isinstance(number, int) or not isinstance(successor_number, int):
            raise ValueError("invalid PR numbers")
        if number not in by_number:
            continue
        p = by_number[number]
        d = by_decision[number]
        if p["head"]["sha"] != expected or p["draft"] is False and p["base"]["ref"] != "main":
            d["reason"] = "Authorization head mismatch or unexpected base; manual review"
            continue
        successor = api("GET", root + f"/pulls/{successor_number}")
        if not successor["merged"] or not successor.get("merge_commit_sha") or successor_number == number:
            d["reason"] = "Successor not verified merged"
            continue
        # GitHub compare: HEAD of old PR must be an ancestor of the merged successor commit.
        # Squash/reimplemented changes cannot pass this proof; they require manual disposition.
        old_sha = urllib.parse.quote(expected, safe="")
        merged_sha = urllib.parse.quote(successor["merge_commit_sha"], safe="")
        comparison = api("GET", root + f"/compare/{old_sha}...{merged_sha}")
        if comparison.get("behind_by") != 0 or comparison.get("status") not in ("ahead", "identical"):
            d["reason"] = "No strict Git ancestry proof of inclusion"
            continue
        d["decision"] = "VERIFIED_SUPERSEDED"
        d["reason"] = f"Explicit pinned authorization; head ancestor of merged PR #{successor_number}"
        if args.apply:
            # Recheck immediately before mutation, avoiding stale-head closure.
            current = api("GET", root + f"/pulls/{number}")
            if current["state"] != "open" or current["head"]["sha"] != expected:
                d["decision"] = "HOLD_REVIEW"
                d["reason"] = "PR changed during audit; no mutation"
                continue
            comment = (f"Administrative closure: merged PR #{successor_number} contains "
                       f"this exact PR head \`{expected}\` in its Git ancestry. "
                       "Closure authorized by docs/governance/pr-triage-closures.json. "
                       "Historical commits remain in Git. No merge or research activation is implied.")
            api("POST", root + f"/issues/{number}/comments", {"body": comment})
            api("PATCH", root + f"/pulls/{number}", {"state": "closed"})
            d["decision"] = "CLOSED_SUPERSEDED"
    report = {"generated_at": datetime.now(timezone.utc).isoformat(), "repository": repo,
              "apply": args.apply, "count": len(prs), "decisions": decisions}
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, sort_keys=True)
        f.write("\n")
    print(f"Audited {len(prs)} open PRs; closed {sum(d['decision']=='CLOSED_SUPERSEDED' for d in decisions)}")
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, RuntimeError, urllib.error.HTTPError) as exc:
        print(f"Fail-closed: {exc}", file=sys.stderr)
        sys.exit(1)
