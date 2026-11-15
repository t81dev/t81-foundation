"""Durable process apparatus. Real measurement orchestration remains frozen.

The only executable worker in this increment is a fixed synthetic diagnostic;
it cannot import codecs, accept arbitrary commands or produce phase timings.
"""
import base64
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import uuid

from .measurement_engine import CONFIGURATION, ProtocolError, canonical_json, paired_orders, run_measurements
from .retention import process_reclamation


def _digest(value):
    return hashlib.sha256(canonical_json(value).encode("ascii")).hexdigest()


def make_plan(seed, commit):
    if type(seed) is not int or not 0 <= seed < 2**64:
        raise ProtocolError("prospectively pinned uint64 order seed required")
    if not isinstance(commit, str) or len(commit) != 40 or any(c not in "0123456789abcdef" for c in commit):
        raise ProtocolError("full source revision required")
    return {"schema": "t81.h04-r03.process-plan.v1", "commit": commit, "seed": seed,
            "configuration": json.loads(canonical_json(CONFIGURATION)),
            "slots": [{"block": block, "position": position, "arm": arm}
                      for block, order in enumerate(paired_orders(seed)) for position, arm in enumerate(order)]}


def validate_plan(plan):
    if not isinstance(plan, dict) or plan != make_plan(plan.get("seed"), plan.get("commit")):
        raise ProtocolError("process plan differs from frozen configuration/order")


def linux_identity(pid, stat_text, boot_id):
    """Parse /proc/PID/stat field 22 without splitting the parenthesized comm field."""
    if type(pid) is not int or pid <= 0 or not stat_text.startswith(f"{pid} ("):
        raise ProtocolError("PID identity mismatch")
    try:
        end = stat_text.rindex(")")
        ticks = int(stat_text[end + 1:].split()[19])
        boot = str(uuid.UUID(boot_id.strip()))
    except (ValueError, IndexError) as exc:
        raise ProtocolError("incomplete Linux process birth identity") from exc
    if ticks < 0:
        raise ProtocolError("negative process start time")
    return {"pid": pid, "start_ticks": ticks, "boot_id": boot}


def observe_identity(pid):
    if sys.platform != "linux":
        return {"pid": pid, "os_birth_identity": None, "scope": "synthetic-nonce-only"}
    return linux_identity(pid, Path(f"/proc/{pid}/stat").read_text(),
                          Path("/proc/sys/kernel/random/boot_id").read_text())


class Journal:
    """Exclusive append-only, fsynced hash chain. Identity does not prove authenticity."""
    def __init__(self, path):
        self.path = Path(path)
        self.fd = os.open(self.path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        self.sequence, self.previous = 0, "0" * 64
        # Persist directory entry before launching any child.
        try:
            descriptor = os.open(self.path.parent, os.O_RDONLY)
            try:
                os.fsync(descriptor)
            finally:
                os.close(descriptor)
        except BaseException:
            os.close(self.fd)
            self.fd = None
            raise

    def append(self, event):
        body = {"sequence": self.sequence, "previous": self.previous, "event": event}
        digest = _digest(body)
        data = canonical_json(dict(body, sha256=digest)).encode("ascii")
        view = memoryview(data)
        while view:
            written = os.write(self.fd, view)
            if written <= 0:
                raise OSError("short journal write")
            view = view[written:]
        os.fsync(self.fd)
        self.previous, self.sequence = digest, self.sequence + 1
        return digest

    def close(self):
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None


def recover_journal(path, expected_sha256=None):
    """Read-only recovery: retain valid prefix AND exact corrupt/truncated tail."""
    data = Path(path).read_bytes()
    records, consumed, previous = [], 0, "0" * 64
    failure = None
    for line in data.splitlines(keepends=True):
        try:
            if not line.endswith(b"\n"):
                raise ProtocolError("truncated journal tail")
            record = json.loads(line)
            if set(record) != {"sequence", "previous", "event", "sha256"}:
                raise ProtocolError("unknown journal record fields")
            if not isinstance(record["event"], dict):
                raise ProtocolError("journal event must be an object")
            body = {k: record[k] for k in ("sequence", "previous", "event")}
            if type(record["sequence"]) is not int or record["sequence"] != len(records) or record["previous"] != previous or _digest(body) != record["sha256"]:
                raise ProtocolError("journal identity/sequence mismatch")
            if canonical_json(record).encode("ascii") != line:
                raise ProtocolError("noncanonical or duplicate-key journal JSON")
        except (ValueError, TypeError, KeyError, ProtocolError) as exc:
            failure = str(exc)
            break
        records.append(record)
        consumed += len(line)
        previous = record["sha256"]
    digest = hashlib.sha256(data).hexdigest()
    if expected_sha256 is not None and digest != expected_sha256:
        failure = failure or "journal file digest mismatch"
    started = {r["event"]["attempt"] for r in records if r["event"].get("kind") == "attempt_started"}
    finished = {r["event"]["attempt"] for r in records if r["event"].get("kind") == "attempt_finished"}
    return {"integrity": failure is None, "failure": failure, "records": records,
            "incomplete_attempts": sorted(started - finished), "sha256": digest,
            "unparsed_tail_base64": base64.b64encode(data[consumed:]).decode("ascii")}


def run_process_plan(*args, **kwargs):
    """No caller field or synthetic helper can authorize a measurement worker."""
    return run_measurements(*args, **kwargs)


def _kill_and_reap(process):
    if process.poll() is None:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    return process.wait()


def _synthetic_attempt(slot, directory, journal, action, timeout, expected_worker_sha256=None):
    nonce = uuid.uuid4().hex
    attempt = f"{slot['block']:02d}-{slot['position']}-{slot['arm']}"
    journal.append({"kind": "attempt_started", "attempt": attempt, "slot": slot,
                    "nonce": nonce, "evidence": "SYNTHETIC"})
    worker = Path(__file__).with_name("synthetic_worker.py").resolve()
    source_hash = hashlib.sha256(worker.read_bytes()).hexdigest()
    process, result, identity, failure = None, None, None, None
    stdout_path, stderr_path = directory / (attempt + ".stdout"), directory / (attempt + ".stderr")
    try:
        if expected_worker_sha256 is not None and source_hash != expected_worker_sha256:
            raise ProtocolError("worker source differs from plan identity")
        with stdout_path.open("xb") as out, stderr_path.open("xb") as err:
            os.chmod(stdout_path, 0o600)
            os.chmod(stderr_path, 0o600)
            process = subprocess.Popen([sys.executable, "-I", "-S", "-B", str(worker)],
                                       stdin=subprocess.PIPE, stdout=out, stderr=err,
                                       cwd=directory, env={"PATH": os.defpath, "LC_ALL": "C"},
                                       close_fds=True, start_new_session=True)
            # The child blocks on stdin until birth identity has been observed.
            identity = observe_identity(process.pid)
            journal.append({"kind": "worker_spawned", "attempt": attempt, "nonce": nonce,
                            "identity": identity, "worker_sha256": source_hash})
            request = canonical_json({"evidence": "SYNTHETIC", "action": action, "nonce": nonce}).encode("ascii")
            process.communicate(request, timeout=timeout)
            result = process.returncode
    except Exception as exc:
        failure = f"{type(exc).__name__}: {exc}"
    finally:
        if process is not None:
            result = _kill_and_reap(process)
            if process.stdin is not None:
                process.stdin.close()
        files = {}
        for path in (stdout_path, stderr_path):
            if path.exists():
                with path.open("rb") as stream:
                    os.fsync(stream.fileno())
                    data = stream.read()
                files[path.suffix[1:]] = {"path": path.name, "size": len(data),
                                         "sha256": hashlib.sha256(data).hexdigest()}
        valid = False
        try:
            lines = stdout_path.read_text().splitlines()
            payload = json.loads(lines[-1])
            valid = (result == 0 and failure is None and payload == {"evidence": "SYNTHETIC", "nonce": nonce,
                     "pid": process.pid, "status": "synthetic-only-complete"})
        except (OSError, ValueError, IndexError, AttributeError):
            pass
        if hashlib.sha256(worker.read_bytes()).hexdigest() != source_hash:
            valid, failure = False, "worker source changed during attempt"
        record = {"kind": "attempt_finished", "attempt": attempt, "slot": slot, "nonce": nonce,
                  "identity": identity, "returncode": result, "valid_synthetic_result": valid,
                  "failure": failure or (None if valid else "missing, failed or killed synthetic worker"),
                  "files": files, "reclamation": process_reclamation(result, process is not None and result is not None),
                  "evidence": "SYNTHETIC", "measurement_eligible": False}
        journal.append(record)
    return record


def execute_synthetic_plan(plan, directory, *, actions=None, timeout=5):
    """Test fresh-process sequencing and failures without a codec/timing worker.

    Every scheduled slot executes once, including failures: no replacement runs
    or favorable-case filtering. A journal failure stops further spawning.
    """
    validate_plan(plan)
    if os.name != "posix":
        raise ProtocolError("process-group isolation requires POSIX; no Windows fallback")
    if type(timeout) not in (int, float) or not 0 < timeout <= 60:
        raise ProtocolError("bounded synthetic-worker timeout required")
    actions = actions or {}
    allowed = {"ok", "nonzero", "kill", "malformed", "wait"}
    attempts = {f"{s['block']:02d}-{s['position']}-{s['arm']}" for s in plan["slots"]}
    if set(actions) - attempts or any(value not in allowed for value in actions.values()):
        raise ProtocolError("only fixed synthetic worker fault actions permitted")
    directory = Path(directory).resolve()
    directory.mkdir(mode=0o700, exist_ok=False)
    journal = Journal(directory / "attempts.jsonl")
    try:
        worker_sha = hashlib.sha256(Path(__file__).with_name("synthetic_worker.py").read_bytes()).hexdigest()
        journal.append({"kind": "plan", "plan": plan, "plan_sha256": _digest(plan),
                        "worker_sha256": worker_sha, "evidence": "SYNTHETIC", "execution_frozen": True})
        results = []
        for slot in plan["slots"]:
            attempt = f"{slot['block']:02d}-{slot['position']}-{slot['arm']}"
            results.append(_synthetic_attempt(slot, directory, journal, actions.get(attempt, "ok"), timeout, worker_sha))
        journal.append({"kind": "plan_finished", "attempts": len(results),
                        "failures": sum(not r["valid_synthetic_result"] for r in results),
                        "measurement_eligible": False})
    finally:
        journal.close()
    return recover_journal(directory / "attempts.jsonl")


def verify_attempt_files(directory, recovery):
    """Verify retained stdout/stderr identities; never trust worker-declared hashes."""
    if not recovery["integrity"] or recovery["incomplete_attempts"]:
        raise ProtocolError("incomplete or corrupt process journal")
    validate_process_recovery(recovery)
    root = Path(directory).resolve()
    for record in recovery["records"]:
        for entry in record["event"].get("files", {}).values():
            name = entry["path"]
            if not isinstance(name, str) or Path(name).name != name:
                raise ProtocolError("invalid attempt artifact path")
            path = root / name
            if path.is_symlink():
                raise ProtocolError("symlink attempt artifact")
            data = path.read_bytes()
            if len(data) != entry["size"] or hashlib.sha256(data).hexdigest() != entry["sha256"]:
                raise ProtocolError("attempt artifact identity mismatch")
        event = record["event"]
        if event.get("valid_synthetic_result") is True:
            try:
                payload = json.loads((root / event["files"]["stdout"]["path"]).read_text().splitlines()[-1])
                if payload != {"evidence": "SYNTHETIC", "nonce": event["nonce"],
                               "pid": event["identity"]["pid"], "status": "synthetic-only-complete"}:
                    raise ProtocolError("successful worker payload identity mismatch")
            except (ValueError, IndexError, KeyError) as exc:
                raise ProtocolError("successful worker payload missing or malformed") from exc
    return True


def validate_process_recovery(recovery):
    """Check plan chronology, identities and completion counts, not just hashes."""
    if not recovery["integrity"] or recovery["incomplete_attempts"]:
        raise ProtocolError("incomplete or corrupt process journal")
    events = [r["event"] for r in recovery["records"]]
    if not events or events[0].get("kind") != "plan":
        raise ProtocolError("missing process plan")
    plan = events[0]["plan"]
    validate_plan(plan)
    if events[0].get("plan_sha256") != _digest(plan) or events[0].get("execution_frozen") is not True:
        raise ProtocolError("plan identity/freeze mismatch")
    worker_sha = events[0].get("worker_sha256")
    if not isinstance(worker_sha, str) or len(worker_sha) != 64 or any(c not in "0123456789abcdef" for c in worker_sha):
        raise ProtocolError("missing worker source identity")
    pos, nonces, births, failures = 1, set(), set(), 0
    try:
        for slot in plan["slots"]:
            attempt = f"{slot['block']:02d}-{slot['position']}-{slot['arm']}"
            start = events[pos]
            pos += 1
            if start.get("kind") != "attempt_started" or start.get("slot") != slot or start.get("attempt") != attempt:
                raise ProtocolError("attempt chronology differs from plan")
            nonce = start["nonce"]
            if not isinstance(nonce, str) or len(nonce) != 32 or any(c not in "0123456789abcdef" for c in nonce) or nonce in nonces:
                raise ProtocolError("duplicate or invalid attempt identity")
            nonces.add(nonce)
            spawned = events[pos] if events[pos].get("kind") == "worker_spawned" else None
            if spawned is not None:
                pos += 1
                if spawned.get("attempt") != attempt or spawned.get("nonce") != nonce:
                    raise ProtocolError("worker identity/attempt mismatch")
                if spawned.get("worker_sha256") != worker_sha:
                    raise ProtocolError("worker source differs from process plan")
                identity = spawned["identity"]
                if "start_ticks" in identity:
                    birth = canonical_json(identity)
                    if birth in births:
                        raise ProtocolError("OS process birth identity reused")
                    births.add(birth)
            finish = events[pos]
            pos += 1
            if finish.get("kind") != "attempt_finished" or finish.get("attempt") != attempt or finish.get("slot") != slot or finish.get("nonce") != nonce:
                raise ProtocolError("missing or mismatched worker completion")
            if finish.get("evidence") != "SYNTHETIC" or finish.get("measurement_eligible") is not False:
                raise ProtocolError("synthetic receipt cannot be measured evidence")
            if spawned is not None and finish.get("identity") != spawned["identity"]:
                raise ProtocolError("completion process identity mismatch")
            if finish.get("valid_synthetic_result") is True:
                if spawned is None or type(finish.get("returncode")) is not int or finish["returncode"] != 0 or finish.get("failure") is not None:
                    raise ProtocolError("failed worker labeled successful")
                if set(finish.get("files", {})) != {"stdout", "stderr"}:
                    raise ProtocolError("successful worker output files missing")
            else:
                failures += 1
            for stream, entry in finish.get("files", {}).items():
                if stream not in ("stdout", "stderr") or entry.get("path") != attempt + "." + stream:
                    raise ProtocolError("worker output path mismatch")
                sha = entry.get("sha256")
                if type(entry.get("size")) is not int or entry["size"] < 0 or not isinstance(sha, str) or len(sha) != 64 or any(c not in "0123456789abcdef" for c in sha):
                    raise ProtocolError("worker output identity malformed")
        ending = events[pos]
        if (pos + 1 != len(events) or ending.get("kind") != "plan_finished"
                or ending.get("attempts") != 120 or ending.get("failures") != failures
                or ending.get("measurement_eligible") is not False):
            raise ProtocolError("missing or inconsistent plan completion")
    except (IndexError, KeyError, TypeError) as exc:
        raise ProtocolError("incomplete process receipt") from exc
    return True
