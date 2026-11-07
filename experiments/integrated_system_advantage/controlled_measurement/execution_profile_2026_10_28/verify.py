"""Offline EXPLORATORY — NONCONFIRMATORY verifier; never launches a process."""
import hashlib
import json
from pathlib import Path
import re
import statistics
import tarfile

HERE = Path(__file__).resolve().parent
ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def identity(raw):
    n = int.from_bytes(hashlib.sha3_512(raw).digest()[:32], "big")
    digits = ""
    while n:
        n, r = divmod(n,81)
        digits = ALPHABET[r]+digits
    return "sha3-256:"+(digits or "0")


def describe(values):
    median = statistics.median(values)
    q1,_,q3 = statistics.quantiles(values,method="inclusive")
    return dict(n=len(values),median=median,min=min(values),max=max(values),
                sample_sd=statistics.stdev(values),iqr=q3-q1,
                mad=statistics.median(abs(v-median) for v in values))


def profiles(text):
    graph = text.split("Call graph:\n",1)[1].split("Total number in stack",1)[0]
    roots = [int(x) for x in re.findall(r"^\s+(\d+) Thread_",graph,re.M)]
    require(len(roots)==1,"Expected one sampled thread; denominator needs reassessment")
    table = text.split("Sort by top of stack, same collapsed (when >= 5):",1)[1].split("Binary Images:",1)[0]
    leaves = [(line.strip(),int(re.search(r"(\d+)\s*$",line)[1]))
              for line in table.splitlines() if re.search(r"\s\d+\s*$",line)]
    divisions = sum(n for name,n in leaves if "t81::div_mod<384ul>" in name)
    require(divisions<=roots[0],"Do not add inclusive stack counts")
    required = ["Interpreter::run_to_halt", "canonical_fixed_from_float_", "build_canonical_fixed_cache_",
                "initialize_canonical_storage_mode_", "Fixed<192ul, 80ul>::operator/", "div_mod<384ul>"]
    require(all(s in graph for s in required),"Resolved attribution chain missing")
    return dict(thread_samples=roots[0],div_mod_384_top_samples=divisions,
                div_mod_384_top_share=divisions/roots[0],sampled_threads=len(roots),
                unresolved_markers=graph.count("???"),function_chain_resolved=True)


def main():
    manifest = json.loads((HERE/"manifest.json").read_text())
    for name,expected in manifest["files_sha256"].items():
        require(hashlib.sha256((HERE/name).read_bytes()).hexdigest()==expected,"Packet hash: "+name)
    for name,expected in manifest["preserved_sha256"].items():
        require(hashlib.sha256((HERE.parents[3]/name).read_bytes()).hexdigest()==expected,"Preserved evidence/source: "+name)
    with tarfile.open(HERE/"raw-profile-run.tar.gz") as archive:
        def raw(name):
            return archive.extractfile(name).read()
        require({m.name for m in archive.getmembers() if m.isfile()} == set(manifest["archive_members_sha256"]),"Archive inventory")
        for name,expected in manifest["archive_members_sha256"].items():
            require(hashlib.sha256(raw(name)).hexdigest()==expected,"Archive hash: "+name)
        data = json.loads(raw("run/observations.json"))
        require(data==json.loads((HERE/"observations.json").read_text()),"Standalone raw observations")
        require(data["confirmatory"] is False and data["eligible_for_preregistered_dataset"] is False,"Exploratory isolation")
        require(data["collection_seconds"]<=1800,"Collection budget")
        plan = json.loads(raw("run/plan.json"))
        rows = data["observations"]
        require([(r["pair"],r["treatment"]) for r in rows]==[(i,t) for i,o in enumerate(plan["orders"]) for t in o],"Frozen diagnostic order")
        require(len(rows)==20 and sum(r["treatment"]=="P" for r in rows)==10,"Bounded count")
        events = [json.loads(line) for line in raw("run/attempts.jsonl").splitlines()]
        require(sum(e["kind"]=="execution_start" for e in events)==20 and not any(e["kind"]=="stopped" for e in events),"All attempts retained/completed")
        results = []
        for row in rows:
            prefix = f"run/pair-{row['pair']:02d}-{row['treatment']}"
            require(row["returncode"]==0 and row["elapsed_ns"]==row["end_ns"]-row["start_ns"]>0,"Exit/clock")
            finish = [e for e in events if e["kind"]=="execution_finish" and e["pair"]==row["pair"] and e["treatment"]==row["treatment"]]
            require(len(finish)==1 and finish[0]["elapsed_ns"]==row["elapsed_ns"],"Journal timing")
            response = json.loads(raw(prefix+"/stdout.bin"))
            model,policy,tok = [raw(prefix+"/"+name) for name in ("model.t81w","policy.apl","tokenizer.json")]
            expected = json.loads((HERE.parent/"exploratory_2026_10_28/artifact-identities.json").read_text())
            for data_bytes,key in ((model,"model_sha256"),(policy,"policy_sha256"),(tok,"tokenizer_text_sha256")):
                require(hashlib.sha256(data_bytes).hexdigest()==expected["input_sha256"][key],"Identical fixture bytes")
            objects = {}
            for role,key in (("result","result_ref"),("provenance","provenance_ref")):
                ref = response[key]
                payload = raw(prefix+"/store/objects/"+ref.split(":",1)[1]+".blk")
                require(identity(payload)==ref==plan["expected_artifact_roles"][role],"Prior artifact identity")
                require(hashlib.sha256(payload).hexdigest()==row["verified_artifacts"][role]["sha256"],"Artifact manifest")
                objects[role]=json.loads(payload)
            result,prov=objects["result"],objects["provenance"]
            require(result["decision"]=="ALLOW" and result["decision_token_ids"]==[42],"Decision")
            require(result["model_hash"]==prov["model_hash"]=="sha3-512:"+hashlib.sha3_512(model).hexdigest(),"Model linkage")
            require(prov["policy_hash"]==identity(policy) and prov["result_ref"]==response["result_ref"],"Policy/result linkage")
            require(result["input_hash"]==prov["input_hash"]==identity(b"greet hello"),"Input linkage")
            if row["treatment"]=="P":
                require(row["profiler_returncode"]==0,"Profiler exit")
                results.append(dict(pair=row["pair"]+1,**profiles(raw(prefix+"/sample.txt").decode())))
        pairs = [{r["treatment"]:r["elapsed_ns"] for r in rows if r["pair"]==i} for i in range(10)]
        summary = dict(label="EXPLORATORY — NONCONFIRMATORY",profiled=describe([r["P"] for r in pairs]),
                       controls=describe([r["C"] for r in pairs]),
                       paired_P_over_C=describe([r["P"]/r["C"] for r in pairs]),
                       paired_P_minus_C_ns=describe([r["P"]-r["C"] for r in pairs]),profiles=results,
                       verified_artifacts=40,collection_seconds=data["collection_seconds"])
        require(summary==json.loads((HERE/"summary.json").read_text()),"Independent recomputation")
        print(json.dumps(summary,indent=2))


if __name__ == "__main__":
    main()
