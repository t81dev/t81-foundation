# T81 Python SDK

T81 provides a T81Lang compiler and VM, ternary numbers and tensors, read-only
VM audit events, and CanonFS content-addressed storage from Python. It is an
experimental systems research project.

```sh
python -m pip install t81
```

```python
import t81

assert t81.compile_and_run("fn main() -> i32 { return 42; }") == 42
```

## Save a fixture and recover it in another process

Write a byte fixture with Python. Keep the printed digest and the store path.

```python
from pathlib import Path
import shlex

import t81

store = str(Path("fixture-store").resolve())
driver = t81.make_persistent_driver(store)
ref = driver.write_object(t81.ObjectType.RawBlock, b"123\n")
print(ref.hex())
print(shlex.join(["t81", "canonfs", "verify", ref.to_cli_handle(), "--canonfs-root", store]))
print(shlex.join(["t81", "canonfs", "get", ref.to_cli_handle(), "--canonfs-root", store]))
```

In a fresh Python process started from the same directory, recover the bytes
using the printed hex digest.

```python
from pathlib import Path

import t81

store = str(Path("fixture-store").resolve())
ref = t81.CanonRef.from_hex(input("Saved digest: ").strip())
assert t81.make_persistent_driver(store).read_object_bytes(ref) == b"123\n"
```

The native CLI and MCP server are separate builds. If you have the native
`t81` CLI on your path, copy the printed `verify` and `get` commands. The CLI
reads the original store and `get` writes the original bytes to stdout.

## Execution limits and audit events

`VM.run_to_halt()` and `compile_and_run()` raise `t81.StepLimitExceeded` when
their step budget expires before the VM halts. An explicit VM can resume from
where it stopped. Use `vm.halted` to check its state. After a successful run,
`vm.axion_events` provides read-only snapshots with the opcode, program
counter, verdict, reason, and reason code. `vm.axion_log` still provides reason
strings. These events describe the VM run under its configured policy.

- [Native CLI installation](https://github.com/t81dev/t81-foundation/blob/main/docs/user-guide/quickstart/INSTALL.md)
- [Python decision-table quickstart](https://github.com/t81dev/t81-foundation/blob/main/docs/user-guide/quickstart/PYTHON_SDK_QUICKSTART.md)
- [Project repository](https://github.com/t81dev/t81-foundation)
