"""Research-only matched workflows. Confirmatory execution is deliberately absent.

Reuses the accepted comparator and fixture harness. All entry points/records are
instrumentation validation, and carry no comparative elapsed-time observation.
"""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import uuid

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from ablation_runner import AblationHarness
from baseline_stack import ConventionalCAS, ConventionalTaskRunner, normalize_task_input_text

PURPOSE = "instrumentation_validation"
ROLES = ("model", "input", "policy", "tokenizer", "result", "provenance", "bundle")
EVIDENCE = ("sampled_token_ids", "sampled_token_scores", "selected_token_id",
            "prompt_token_history", "generated_token_history", "candidate_selection_mode",
            "candidate_selection_basis", "selection_policy_kind", "input_preview")
ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"


def identity(data):
    """Independent identity oracle: integer division, not comparator encoding."""
    value = int.from_bytes(hashlib.sha3_512(data).digest()[:32], "big")
    digits = ""
    while value:
        value, digit = divmod(value, 81)
        digits = ALPHABET[digit] + digits
    return "sha3-256:" + (digits or "0")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def json_bytes(data):
    return (json.dumps(data, indent=2) + "\n").encode()


class Journal:
    """Append + fsync each record before continuing; trusted sequential owner.

    One coordinator and its currently awaited worker may append. Each writes a
    complete record under the POSIX advisory lock. Other concurrency is outside
    scope. A partial/corrupt last record is retained and prevents continuation.
    """
    def __init__(self, path, *, create=False):
        self.path = Path(path).resolve()
        if os.name != "posix":
            raise RuntimeError("Durable journal currently requires POSIX; Windows unvalidated")
        if create:
            fd = os.open(self.path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
            try:
                os.fsync(fd)
            finally:
                os.close(fd)
            directory = os.open(self.path.parent, os.O_RDONLY)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)
        elif not self.path.is_file():
            raise ValueError("Missing attempt journal")

    def append(self, kind, **fields):
        import fcntl
        record = dict(schema="t81.i08.attempt.v1", purpose=PURPOSE,
                      confirmatory=False, event_id=uuid.uuid4().hex,
                      utc=datetime.now(timezone.utc).isoformat(),
                      monotonic_ns=time.monotonic_ns(), pid=os.getpid(), kind=kind, **fields)
        raw = (json.dumps(record, ensure_ascii=False, allow_nan=False) + "\n").encode()
        fd = os.open(self.path, os.O_RDWR | os.O_APPEND)
        try:
            fcntl.flock(fd, fcntl.LOCK_EX)
            existing = os.read(fd, os.fstat(fd).st_size)
            if existing and not existing.endswith(b"\n"):
                raise ValueError("Truncated journal retained; do not continue")
            for line in existing.splitlines():
                prior = json.loads(line)
                if prior.get("purpose") != PURPOSE or prior.get("confirmatory") is not False:
                    raise ValueError("Journal purpose mismatch")
            view = memoryview(raw)
            while view:
                count = os.write(fd, view)
                if count <= 0:
                    raise OSError("Incomplete journal write")
                view = view[count:]
            os.fsync(fd)
        finally:
            os.close(fd)
        return record


class LaunchFailure(RuntimeError):
    def __init__(self, code, message):
        super().__init__(message)
        self.returncode = code


def launch(command, journal, *, context=None, timeout=180):
    """Every launch is durable before spawn, including failure/timeout/interruption."""
    attempt = uuid.uuid4().hex
    context = context or {}
    command = [str(x) for x in command]
    journal.append("launch_start", attempt=attempt, command=command, context=context)
    process = None
    group_leader = os.environ.get("T81_I08_VALIDATION_WORKER") != "1"
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   env=dict(os.environ, T81_CANONFS_READ_VERIFY="1"),
                                   start_new_session=group_leader)
        out, err = process.communicate(timeout=timeout)
    except BaseException as error:
        out = err = b""
        if process is not None:
            # Kill the whole worker group, including any nested public CLI process.
            try:
                if group_leader:
                    os.killpg(process.pid, signal.SIGKILL)
                else:
                    process.kill()
            except ProcessLookupError:
                pass
            out, err = process.communicate()
        journal.append("launch_failure", attempt=attempt, context=context,
                       error_type=type(error).__name__, error=str(error),
                       returncode=process.returncode if process else None,
                       stdout_base64=base64.b64encode(out).decode(),
                       stderr_base64=base64.b64encode(err).decode())
        raise
    journal.append("launch_finish", attempt=attempt, context=context,
                   returncode=process.returncode,
                   stdout_base64=base64.b64encode(out).decode(),
                   stderr_base64=base64.b64encode(err).decode())
    if process.returncode:
        raise LaunchFailure(process.returncode, err.decode("utf-8", errors="replace"))
    return out


class Store:
    def __init__(self, arm, root, cli, journal, scratch):
        if arm not in ("A", "B"):
            raise ValueError("Unknown arm")
        self.arm, self.root, self.cli, self.journal = arm, Path(root), str(cli), journal
        self.scratch = Path(scratch)
        self.cas = ConventionalCAS(str(root)) if arm == "B" else None

    def put(self, raw):
        if self.arm == "B":
            ref = self.cas.put(raw)
        else:
            with tempfile.TemporaryDirectory(dir=self.scratch) as tmp:
                path = Path(tmp)/"object.bin"
                path.write_bytes(raw)
                ref = launch([self.cli, "canonfs", "put-file", path,
                              "--canonfs-root", self.root], self.journal,
                             context=dict(arm=self.arm, operation="put")).decode().strip()
        if ref != identity(raw):
            raise ValueError("Independent byte-identity mismatch on put")
        return ref

    def get(self, ref):
        if self.arm == "B":
            raw = self.cas.get(ref)
            if raw is None:
                raise ValueError("Missing object")
        else:
            with tempfile.TemporaryDirectory(dir=self.scratch) as tmp:
                path = Path(tmp)/"object.bin"
                launch([self.cli, "canonfs", "get", ref, "--out", path,
                        "--canonfs-root", self.root], self.journal,
                       context=dict(arm=self.arm, operation="get"))
                raw = path.read_bytes()
        if identity(raw) != ref:
            raise ValueError("Independent byte-identity mismatch on get")
        return raw


def readback(store, bundle_ref):
    bundle_raw = store.get(bundle_ref)
    bundle = json.loads(bundle_raw)
    if bundle.get("schema") != "t81.ai.task.assess-fixed.bundle.v1" or bundle.get("task") != "assess_fixed":
        raise ValueError("Wrong recovery descriptor")
    refs = dict(model=bundle["model_ref"], input=bundle["input_ref"],
                policy=bundle["policy_ref"], tokenizer=bundle["tokenizer_ref"],
                result=bundle["source_result_ref"], provenance=bundle["source_provenance_ref"],
                bundle=bundle_ref)
    raw = {role: store.get(ref) for role, ref in refs.items() if role != "bundle"}
    raw["bundle"] = bundle_raw
    result, prov = json.loads(raw["result"]), json.loads(raw["provenance"])
    tokenizer = json.loads(raw["tokenizer"])
    if not isinstance(tokenizer, str):
        raise ValueError("Exact tokenizer text must be recovered reversibly")
    prompt = raw["input"].decode("utf-8")
    if normalize_task_input_text(prompt) != prompt:
        raise ValueError("Persisted input is not normalized")
    if bundle["model_hash"] != "sha3-512:"+hashlib.sha3_512(raw["model"]).hexdigest():
        raise ValueError("Model fingerprint mismatch")
    if bundle["input_hash"] != identity(raw["input"]):
        raise ValueError("Input fingerprint mismatch")
    for key in ("model_hash", "input_hash", "decision", "reason_code", "decision_token_ids"):
        if bundle[key] != result[key]:
            raise ValueError("Descriptor/result mismatch: "+key)
    for key in ("model_hash", "input_hash"):
        if prov[key] != result[key]:
            raise ValueError("Provenance/result mismatch: "+key)
    if prov["result_ref"] != refs["result"] or prov["policy_hash"] != identity(raw["policy"]):
        raise ValueError("Provenance linkage mismatch")
    if prov["policy_result"] != "allowed" or prov["determinism_class"] != "strict_deterministic":
        raise ValueError("Wrong policy/mode evidence")
    evidence = prov["execution_evidence"]
    common = {key: evidence[key] for key in EVIDENCE}
    if common["selected_token_id"] != result["decision_token_ids"][0] or common["input_preview"] != prompt:
        raise ValueError("Execution evidence/result mismatch")
    ids, scores = common["sampled_token_ids"], common["sampled_token_scores"]
    if not ids or len(ids) != len(scores) or any(not math.isfinite(s) for s in scores):
        raise ValueError("Invalid candidate evidence")
    if ids[max(range(len(scores)), key=lambda i: scores[i])] != common["selected_token_id"]:
        raise ValueError("Selection evidence mismatch")
    files = [path for path in store.root.rglob("*") if path.is_file()]
    stats = [path.stat() for path in files]
    allocated = sum(s.st_blocks*512 for s in stats) if all(hasattr(s, "st_blocks") for s in stats) else None
    response = dict(purpose=PURPOSE, measurement_eligible=False, verified=True,
                    artifact_roles=refs, common_evidence=common,
                    summary={key: result[key] for key in ("model_hash", "input_hash", "decision", "reason_code", "decision_token_ids")},
                    input_identities=dict(model_sha256=sha256(raw["model"]),
                                          input_sha256=sha256(raw["input"]),
                                          policy_sha256=sha256(raw["policy"]),
                                          tokenizer_text_sha256=sha256(tokenizer.encode("utf-8"))),
                    storage=dict(logical_role_bytes={k: len(v) for k, v in raw.items()},
                                 unique_payload_bytes=sum(len(raw[next(k for k,v in refs.items() if v==ref)]) for ref in set(refs.values())),
                                 file_bytes=sum(s.st_size for s in stats), allocated_bytes=allocated,
                                 file_count=len(files)))
    return response, bundle, raw, tokenizer


def execute(store, model, prompt, policy, tokenizer):
    """Accepted computation unchanged; A adds exactly the missing obligations."""
    if store.arm == "B":
        output = ConventionalTaskRunner(str(store.root)).run_assess_fixed(model, prompt, policy, tokenizer)
        bundle_ref = output["bundle_ref"]
    else:
        with tempfile.TemporaryDirectory(dir=store.scratch) as tmp:
            path = Path(tmp)
            (path/"model.t81w").write_bytes(model)
            (path/"policy.apl").write_bytes(policy.encode("utf-8"))
            (path/"tokenizer.json").write_bytes(tokenizer.encode("utf-8"))
            output = json.loads(launch([store.cli, "ai", "task", "assess-fixed",
                                       "--model", "investigation08", "--model-file", path/"model.t81w",
                                       "--policy", path/"policy.apl", "--input", prompt,
                                       "--mode", "strict_deterministic", "--canonfs-root", store.root],
                                      store.journal, context=dict(arm="A", operation="execute")))
        # Denial/invalid-model exits above, before additional persistence.
        model_ref = store.put(model)
        input_ref = store.put(normalize_task_input_text(prompt).encode("utf-8"))
        policy_ref = store.put(policy.encode("utf-8"))
        tokenizer_ref = store.put(json.dumps(tokenizer, ensure_ascii=False, sort_keys=True).encode("utf-8"))
        bundle = dict(schema="t81.ai.task.assess-fixed.bundle.v1", task="assess_fixed",
                      source_result_ref=output["result_ref"], model_ref=model_ref,
                      input_ref=input_ref, policy_ref=policy_ref, tokenizer_ref=tokenizer_ref,
                      source_provenance_ref=output["provenance_ref"], model_hash=output["model_hash"],
                      input_hash=output["input_hash"], decision=output["decision"],
                      reason_code=output["reason_code"], decision_token_ids=output["decision_token_ids"])
        bundle_ref = store.put(json_bytes(bundle))
    response, _, _, _ = readback(store, bundle_ref)
    return response


def recover(store, bundle_ref):
    original, bundle, raw, tokenizer = readback(store, bundle_ref)
    if store.arm == "B":
        # Exercise the accepted no-override CAS recovery implementation unchanged.
        replay = ConventionalTaskRunner(str(store.root)).verify_and_replay(bundle_ref)
        if replay["valid"] is not True:
            raise ValueError("Accepted comparator recovery rejected evidence")
        repeated, _, _, _ = readback(store, bundle_ref)
    else:
        repeated = execute(store, raw["model"], raw["input"].decode("utf-8"),
                           raw["policy"].decode("utf-8"), tokenizer)
    if repeated["artifact_roles"] != original["artifact_roles"] or repeated["summary"] != original["summary"]:
        raise ValueError("Recovered execution/evidence differs")
    return repeated


class ControlledHarness(AblationHarness):
    """Validation-only extension of the accepted harness, not another comparator."""
    def validate_pair(self, fixtures, prompt="greet hello", condition="empty_store", order=("A", "B")):
        if condition not in ("empty_store", "populated_store") or tuple(order) not in (("A", "B"), ("B", "A")):
            raise ValueError("Invalid validation condition/order")
        pair = Path(tempfile.mkdtemp(prefix="validation-", dir=self.work_dir))
        journal = Journal(pair/"attempts.jsonl", create=True)
        roots = {arm: pair/arm for arm in ("A", "B")}
        for root in roots.values():
            root.mkdir()
        completed = []
        journal.append("pair_start", pair_id=pair.name, condition=condition, order=list(order),
                       roots={a: str(p) for a,p in roots.items()})
        try:
            def worker(arm, operation, stage, bundle=None):
                command = [sys.executable, str(HERE/"apparatus.py"), "worker", "--arm", arm,
                           "--operation", operation, "--store", str(roots[arm]),
                           "--cli", self.t81_bin, "--journal", str(journal.path), "--scratch", str(pair)]
                if operation == "run":
                    command += ["--model", fixtures["model_path"], "--policy", fixtures["apl_policy_path"],
                                "--tokenizer", fixtures["tokenizer_path"], "--input", prompt]
                else:
                    command += ["--bundle", bundle]
                return json.loads(launch(command, journal, context=dict(arm=arm, stage=stage)))
            if any(list(root.iterdir()) for root in roots.values()):
                raise ValueError("Stores not initially empty")
            if condition == "populated_store":
                for arm in order:
                    worker(arm, "run", "prime")
            before = {a: len(list(r.rglob("*"))) for a,r in roots.items()}
            results = {}
            for arm in order:
                results[arm] = worker(arm, "run", "execute")
                if (results[arm].get("verified") is not True or
                    results[arm].get("purpose") != PURPOSE or
                    results[arm].get("measurement_eligible") is not False or
                    set(results[arm].get("artifact_roles", {})) != set(ROLES)):
                    raise ValueError("Worker did not verify seven obligations")
                completed.append(arm)
            for key in ("summary", "common_evidence", "input_identities"):
                if results["A"][key] != results["B"][key]:
                    raise ValueError("Workflow parity mismatch: "+key)
            for role in ("model", "input", "policy", "tokenizer", "result"):
                if results["A"]["artifact_roles"][role] != results["B"]["artifact_roles"][role]:
                    raise ValueError("Common byte identity differs: "+role)
            for arm in order:
                replayed = worker(arm, "replay", "fixture_independent_replay",
                                  results[arm]["artifact_roles"]["bundle"])
                if replayed["artifact_roles"] != results[arm]["artifact_roles"]:
                    raise ValueError("Replay references differ")
            record = journal.append("pair_complete", pair_id=pair.name, status="complete",
                                    equivalent=True, condition=condition, entries_before=before,
                                    arms=results, order=list(order))
            return dict(pair_root=str(pair), journal=str(journal.path), record=record)
        except BaseException as error:
            journal.append("pair_incomplete", pair_id=pair.name, completed_arms=completed,
                           error_type=type(error).__name__, error=str(error))
            raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    worker = sub.add_parser("worker", help="single correctness/instrumentation workflow only")
    worker.add_argument("--arm", choices=("A", "B"), required=True)
    worker.add_argument("--operation", choices=("run", "lookup", "replay"), required=True)
    for key in ("store", "cli", "journal", "scratch"):
        worker.add_argument("--"+key, required=True)
    for key in ("model", "policy", "tokenizer", "input", "bundle"):
        worker.add_argument("--"+key)
    validate = sub.add_parser("validate", help="one validation pair; never executes the confirmatory schedule")
    validate.add_argument("--build-dir", required=True)
    validate.add_argument("--work-dir", required=True)
    validate.add_argument("--condition", choices=("empty_store", "populated_store"), default="empty_store")
    args = parser.parse_args()
    if args.command == "validate":
        harness = ControlledHarness(HERE.parents[2], args.work_dir, args.build_dir)
        fixtures = harness.prepare_fixtures()
        result = harness.validate_pair(fixtures, condition=args.condition)
    else:
        os.environ["T81_I08_VALIDATION_WORKER"] = "1"
        if args.operation == "run":
            if any(getattr(args, k) is None for k in ("model", "policy", "tokenizer", "input")) or args.bundle:
                parser.error("run needs fixture inputs and no bundle")
        elif not args.bundle or any(getattr(args, k) is not None for k in ("model", "policy", "tokenizer", "input")):
            parser.error("lookup/replay require only persisted bundle; overrides forbidden")
        journal = Journal(args.journal)
        store = Store(args.arm, args.store, args.cli, journal, args.scratch)
        if args.operation == "run":
            result = execute(store, Path(args.model).read_bytes(), args.input,
                             Path(args.policy).read_bytes().decode("utf-8"),
                             Path(args.tokenizer).read_bytes().decode("utf-8"))
        elif args.operation == "lookup":
            result = readback(store, args.bundle)[0]
        else:
            result = recover(store, args.bundle)
    print(json.dumps(result, ensure_ascii=False, allow_nan=False))


if __name__ == "__main__":
    try:
        main()
    except PermissionError as error:
        print(str(error), file=sys.stderr)
        sys.exit(13)
    except LaunchFailure as error:
        print(str(error), file=sys.stderr)
        sys.exit(error.returncode if error.returncode in (1, 13) else 1)
