"""Fixed diagnostic worker. No package imports, codec dispatch or timing API.

Invocation with -I -S ignores user imports/site packages. No supplied path,
module, callable, workload parameters or arbitrary command can be executed.
"""
import json
import os
import signal
import sys
import time


def main():
    request = json.loads(sys.stdin.buffer.readline(4096))
    if set(request) != {"evidence", "action", "nonce"} or request["evidence"] != "SYNTHETIC":
        raise ValueError("synthetic diagnostic requests only")
    nonce, action = request["nonce"], request["action"]
    if not isinstance(nonce, str) or len(nonce) != 32 or any(c not in "0123456789abcdef" for c in nonce):
        raise ValueError("invalid attempt nonce")
    print(json.dumps({"status": "synthetic-start", "nonce": nonce}), flush=True)
    if action == "kill":
        os.kill(os.getpid(), signal.SIGKILL)
    elif action == "nonzero":
        print("intentional synthetic failure", file=sys.stderr, flush=True)
        return 7
    elif action == "malformed":
        print("{truncated", flush=True)
        return 0
    elif action == "wait":
        time.sleep(120)  # failure-injection timeout only; no elapsed value recorded
        return 8
    elif action != "ok":
        raise ValueError("unknown diagnostic action")
    print(json.dumps({"evidence": "SYNTHETIC", "nonce": nonce, "pid": os.getpid(),
                      "status": "synthetic-only-complete"}, sort_keys=True), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
