"""Offline diagnostic verification; imports no apparatus/collector/statistics code."""
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
    for name, expected in manifest["preserved_previous_packet_sha256"].items():
        require(hashlib.sha256((HERE.parents[3] / name).read_bytes()).hexdigest() == expected, "Preserved previous packet: " + name)
    with tarfile.open(HERE / "raw-run.tar.gz") as archive:
        def read(name):
            return archive.extractfile(name).read()
        members = {m.name for m in archive.getmembers() if m.isfile()}
        require(members == set(manifest["archive_members_sha256"]), "Archive inventory")
        for name, expected in manifest["archive_members_sha256"].items():
            require(hashlib.sha256(read(name)).hexdigest() == expected, "Member hash: " + name)
        data = json.loads(read("run/observations.json"))
        require(json.loads((HERE / "observations.json").read_text()) == data, "Standalone raw observations")
        expected_flat = [dict(pair=row["index"]+1, order=row["order"], arm=arm, **n)
                         for row in data["pairs"] for arm in row["order"] for n in row["arms"][arm]["nested"]]
        require([json.loads(line) for line in (HERE / "per-command-timings.jsonl").read_text().splitlines()] == expected_flat, "Standalone command timings")
        plan = json.loads(read("run/plan.json"))
        require(data["confirmatory"] is False and data["eligible_for_preregistered_dataset"] is False, "Exploratory isolation")
        require(len(data["pairs"]) == 10 and data["collection_seconds"] <= 1800, "Count/budget")
        events = [json.loads(line) for line in read("run/attempts.jsonl").splitlines()]
        require(not any(e["kind"] in ("stopped", "arm_failure") for e in events), "Collection failure")
        require(sum(e["kind"] == "arm_start" for e in events) == 20, "Launch count")
        require(sum(e["kind"] == "pair_complete" for e in events) == 10, "Complete pairs")
        count = 0
        accounting = []
        previous_roles = json.loads((HERE.parent / "exploratory_2026_10_28/artifact-identities.json").read_text())["roles_stable_in_all_ten_pairs"]
        for row, order in zip(data["pairs"], plan["orders"]):
            require(row["order"] == order, "Planned order")
            original_events = [json.loads(line) for line in read(f"run/pair-{row['index']:02d}/worker-diagnostics.jsonl").splitlines()]
            require(len(original_events) == 26, "Original launch journal count")
            for arm in ("A", "B"):
                record = row["arms"][arm]
                require(record["returncode"] == 0 and record["elapsed_ns"] == record["clock_end_ns"]-record["clock_start_ns"] > 0, "Clock/exit")
                finish = [e for e in events if e["kind"] == "arm_finish" and e["index"] == row["index"] and e["arm"] == arm]
                require(len(finish) == 1 and finish[0]["elapsed_ns"] == record["elapsed_ns"], "Durable timing")
                nested = [json.loads(line) for line in read(f"run/pair-{row['index']:02d}/nested-{arm}.jsonl").splitlines()]
                require(nested == record["nested"], "Standalone command records")
                expected = ["execute"]+["put"]*5+["get"]*7 if arm == "A" else []
                require([n["context"]["operation"] for n in nested] == expected, "Command order/count")
                previous_end = record["clock_start_ns"]
                sums = {"execute":0, "put":0, "get":0}
                for sequence, invocation in enumerate(nested, 1):
                    require(invocation["sequence"] == sequence and invocation["error"] is None and invocation["returncode"] == 0, "Nested success/order")
                    launch_start, launch_finish = original_events[2*(sequence-1):2*sequence]
                    require(launch_start["kind"] == "launch_start" and launch_finish["kind"] == "launch_finish" and
                            launch_start["attempt"] == launch_finish["attempt"] and launch_finish["returncode"] == 0 and
                            launch_start["command"] == invocation["command"], "Original command/exit journal")
                    require(previous_end <= invocation["start_ns"] < invocation["end_ns"] <= record["clock_end_ns"], "Sequential containment")
                    duration = invocation["end_ns"]-invocation["start_ns"]
                    require(duration == invocation["elapsed_ns"], "Nested clock")
                    sums[invocation["context"]["operation"]] += duration
                    previous_end = invocation["end_ns"]
                total = sum(sums.values())
                computed = dict(execution_ns=sums["execute"], persistence_ns=sums["put"], readback_ns=sums["get"],
                                nested_total_ns=total, residual_ns=record["elapsed_ns"]-total,
                                nested_share=total/record["elapsed_ns"], sequential_nonoverlapping=True)
                require(computed == record["accounting"], "Exact per-pair accounting")
                if arm == "A":
                    accounting.append(computed)
                roles = record["correctness_response"]["artifact_roles"]
                require(len(roles) == 7, "Seven roles")
                require(roles == previous_roles[arm], "Prior exploratory artifact identities")
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
        decomposition = {}
        for key in ("execution_ns", "persistence_ns", "readback_ns", "nested_total_ns", "residual_ns", "nested_share"):
            decomposition[key] = statistics.median(r[key] for r in accounting)
            require(decomposition[key] == data["decomposition"][key]["median"], "Decomposition median")
        controls = data["version_controls"]
        require(len(controls) == 10, "Control count")
        require(sum(e["kind"] == "version_control_start" for e in events) == 10, "Control attempts")
        for control in controls:
            require(control["returncode"] == 0 and control["elapsed_ns"] == control["end_ns"]-control["start_ns"] > 0, "Control clock/exit")
        control_median = statistics.median(c["elapsed_ns"] for c in controls)
        require(control_median == data["version_control_summary"]["median"], "Control median")
        print(json.dumps(dict(label="EXPLORATORY — NONCONFIRMATORY", verified_role_objects=count,
                              pairs=10, nested_invocations=130, all_nested_intervals_sequential_and_contained=True,
                              median_outer_ns=summary, medians=decomposition, version_control_median_ns=control_median), indent=2))


if __name__ == "__main__":
    main()
