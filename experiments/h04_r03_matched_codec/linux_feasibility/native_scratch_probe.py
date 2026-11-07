"""Bounded native-allocation counterexample; no codec calls or timing APIs."""
import ctypes
import gc
import weakref
import json
import sys
import tracemalloc

if sys.platform != "linux":
    print(json.dumps({"status": "NOT TESTED", "reason": "Linux diagnostic required"}))
    raise SystemExit(77)

class Buffer(bytearray):
    pass

owner = Buffer(b"synthetic object lifetime")
alias = owner
reference = weakref.ref(owner)
del owner
gc.collect()
retained_with_alias = reference() is not None
del alias
gc.collect()
print(json.dumps({"scope": "one-CPython-object-only", "retained_with_alias": retained_with_alias,
                  "dead_after_last_alias": reference() is None, "physical_reclamation_proven": False}, sort_keys=True))

libc = ctypes.CDLL(None)
libc.malloc.argtypes, libc.malloc.restype = [ctypes.c_size_t], ctypes.c_void_p
libc.memset.argtypes, libc.memset.restype = [ctypes.c_void_p, ctypes.c_int, ctypes.c_size_t], ctypes.c_void_p
libc.free.argtypes, libc.free.restype = [ctypes.c_void_p], None
size = 128 * 1024
tracemalloc.start(1)
tracemalloc.reset_peak()
pointer = libc.malloc(size)
if not pointer:
    tracemalloc.stop()
    raise MemoryError("synthetic native allocation failed")
try:
    libc.memset(pointer, 0x5A, size)
finally:
    libc.free(pointer)
current, peak = tracemalloc.get_traced_memory()
tracemalloc.stop()
result = {"scope": "synthetic-native-malloc-not-Phase-C", "requested_native_bytes": size,
          "scratch_limit_bytes": 65536, "traced_current_bytes": current, "traced_peak_bytes": peak,
          "native_buffer_freed": True, "complete_coverage": False}
print(json.dumps(result, sort_keys=True))
if not peak < 65536 < size:
    raise SystemExit("counterexample not demonstrated in this environment")
