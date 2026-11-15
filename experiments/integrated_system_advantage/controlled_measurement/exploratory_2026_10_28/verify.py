"""Offline integrity/recalculation of EXPLORATORY — NONCONFIRMATORY evidence."""
import hashlib
import json
from pathlib import Path
import statistics
import tarfile

HERE = Path(__file__).resolve().parent
ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def identity(raw):
    number = int.from_bytes(hashlib.sha3_512(raw).digest()[:32], "big")
    digits = ""
    while number:
        number, remainder = divmod(number, 81)
        digits = ALPHABET[remainder] + digits
    return "sha3-256:" + (digits or "0")


def main():
    manifest = json.loads((HERE / "manifest.json").read_text())
    for name, expected in manifest["files_sha256"].items():
        require(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == expected, "File hash: " + name)
    for name, expected in manifest["preserved_candidate_sha256"].items():
        require(hashlib.sha256((HERE.parents[3] / name).read_bytes()).hexdigest() == expected, "Preserved candidate: " + name)
    with tarfile.open(HERE / "raw-run.tar.gz") as archive:
        def read(name):
            return archive.extractfile(name).read()
        members = {m.name for m in archive.getmembers() if m.isfile()}
        require(members == set(manifest["archive_members_sha256"]), "Archive inventory")
        for name, expected in manifest["archive_members_sha256"].items():
            require(hashlib.sha256(read(name)).hexdigest() == expected, "Member hash: " + name)
        data = json.loads(read("run/observations.json"))
        plan = json.loads(read("run/plan.json"))
        require(data["confirmatory"] is False and data["eligible_for_preregistered_dataset"] is False, "Exploratory isolation")
        require(len(data["pairs"]) == 10 and data["collection_seconds"] <= 1800, "Count/budget")
        events = [json.loads(line) for line in read("run/attempts.jsonl").splitlines()]
        require(not any(e["kind"] in ("stopped", "arm_failure") for e in events), "Collection failure")
        require(sum(e["kind"] == "arm_start" for e in events) == 20, "Launch count")
        require(sum(e["kind"] == "pair_complete" for e in events) == 10, "Complete pairs")
        count = 0
        for row, order in zip(data["pairs"], plan["orders"]):
            require(row["order"] == order, "Planned order")
            for arm in ("A", "B"):
                record = row["arms"][arm]
                require(record["returncode"] == 0 and record["elapsed_ns"] == record["clock_end_ns"]-record["clock_start_ns"] > 0, "Clock/exit")
                finish = [e for e in events if e["kind"] == "arm_finish" and e["index"] == row["index"] and e["arm"] == arm]
                require(len(finish) == 1 and finish[0]["elapsed_ns"] == record["elapsed_ns"], "Durable timing")
                roles = record["correctness_response"]["artifact_roles"]
                require(len(roles) == 7, "Seven roles")
                for role, ref in roles.items():
                    filename = ref.split(":", 1)[1] + (".blk" if arm == "A" else "")
                    raw = read(f"run/pair-{row['index']:02d}/{arm}/objects/{filename}")
                    require(identity(raw) == ref, "Stored byte identity")
                    count += 1
            a, b = [row["arms"][arm]["correctness_response"] for arm in ("A", "B")]
            for key in ("summary", "common_evidence", "input_identities"):
                require(a[key] == b[key], "Parity: " + key)
            for role in ("model", "input", "policy", "tokenizer", "result"):
                require(a["artifact_roles"][role] == b["artifact_roles"][role], "Common identity")
        summary = {arm: statistics.median(r["arms"][arm]["elapsed_ns"] for r in data["pairs"]) for arm in ("A", "B")}
        for arm in ("A", "B"):
            require(summary[arm] == data["summary"][arm]["median"], "Median")
        ratio = statistics.median(r["arms"]["A"]["elapsed_ns"]/r["arms"]["B"]["elapsed_ns"] for r in data["pairs"])
        require(ratio == data["summary"]["paired_ratio_A_over_B"]["median"], "Paired ratio")
        print(json.dumps(dict(label="EXPLORATORY — NONCONFIRMATORY", verified_role_objects=count,
                              pairs=10, median_ns=summary, median_paired_A_over_B=ratio), indent=2))


if __name__ == "__main__":
    main()
