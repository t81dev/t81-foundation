# CanonFS identity/storage portability audit

An additive opt-in storage-map v2 candidate, based on latest main without merging
#554 or #536. [RFC-00D6](../../spec/rfcs/RFC-00D6-canonfs-portable-storage-mapping.md)
defines compatibility; [REPORT.md](REPORT.md) records native evidence and limits.
The legacy factory/format remain the default. No migration is implemented.

From the repository root:

```sh
cmake -S . -B /tmp/t81-portable-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_CANONFS_PORTABILITY_AUDIT=ON -DT81_BUILD_TESTS=OFF \
  -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_TUI=OFF \
  -DT81_ENABLE_TERNARYOS=OFF
cmake --build /tmp/t81-portable-build --target \
  t81_canonfs_portability_probe t81_canonfs_portable_contract_test -j 4
/tmp/t81-portable-build/t81_canonfs_portable_contract_test /tmp/t81-portable-contract
python3 experiments/canonfs_portability/replay.py \
  --probe /tmp/t81-portable-build/t81_canonfs_portability_probe \
  --output /tmp/t81-portable-replay
```

C++23, CMake, Ninja and Python >=3.10; no Python packages. Windows uses the native
MSVC environment and `.exe` suffix. Outputs must be fresh and are retained.
`raw.jsonl` flushes process output, actual file bytes and errors before verification.
Every expected legacy failure is re-executed; invalid filenames are not skipped.
The tarball under `original-554/` is byte-for-byte original evidence with a pinned
SHA-256, not an approved expectation substituted for a new native run.

The new factory accepts an absent/empty root or a complete v2 root. It refuses
legacy stores instead of migrating them. Current CLI/interchange paths continue
to use the legacy factory. Choose a different new root when adopting v2 from C++:

```cpp
auto portable = t81::canonfs::make_portable_persistent_driver(new_root);
```

Linux/macOS CI additionally runs six unchanged legacy regression suites. Native
Windows legacy behavior is measured by the original before/after corpus, which
contains its known failures. The dedicated workflow uploads raw directories and
logs inside a tarball to preserve native Base81 filenames on artifact services.
