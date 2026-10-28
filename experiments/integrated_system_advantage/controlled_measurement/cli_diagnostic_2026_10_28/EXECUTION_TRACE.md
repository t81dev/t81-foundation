# EXPLORATORY — NONCONFIRMATORY: source-grounded command trace

The apparatus and comparator are unchanged from candidate
`05639a677b11ea1f4ef0328b3f67140784a4ead1`, retained by evidence commit
`96db9bb4a758ab6d9efa46356bc0aa91ff94e477`. This is a source/call-path trace,
not a syscall trace. OS executable/library loading, caches and metadata accesses
are not individually instrumented.

Notation: `CLI` is the pinned `t81` executable; `S` is the arm-A independent
initially empty store; `T` denotes a fresh temporary directory in that pair.
All actual absolute paths, refs and complete argv lists appear in the raw timing
rows and original lossless launch journals. None of the placeholders below is
an actual execution argument.

`EXEC`:

```text
CLI ai task assess-fixed --model investigation08 --model-file T/model.t81w
    --policy T/policy.apl --input "greet hello" --mode strict_deterministic
    --canonfs-root S
```

`PUT(bytes)`: the worker writes exact bytes to `T/object.bin`, then invokes:

```text
CLI canonfs put-file T/object.bin --canonfs-root S
```

`GET(ref)`: the worker invokes:

```text
CLI canonfs get ref --out T/object.bin --canonfs-root S
```

| Sequence | Command / role | Inputs → outputs / purpose | Corresponding conventional operation |
| --- | --- | --- | --- |
| 1 | EXEC | Model, tokenizer sidecar, input, exact APL → task JSON stdout; persists result/provenance. Admission and bounded computation. | `ConventionalTaskRunner.run_assess_fixed` executes same bounded computation/admission in-process and serializes its evidence. |
| 2 | PUT model | 2,037 exact model bytes → CAS ref stdout. | `self.cas.put(model_bytes)` |
| 3 | PUT input | 11 normalized UTF-8 bytes → CAS ref stdout. | `self.cas.put(normalized_input.encode(...))` |
| 4 | PUT policy | 245 exact APL bytes → CAS ref stdout. | `self.cas.put(policy_raw_text.encode(...))` |
| 5 | PUT tokenizer | 256 bytes of reversible JSON-string encoding of exact tokenizer text → CAS ref stdout. | `self.cas.put(json.dumps(tokenizer_data,...).encode(...))` |
| 6 | PUT bundle | A's 1,095-byte descriptor linking six role refs → CAS ref stdout. | `self.cas.put(bundle_bytes)`; backend descriptor bytes/ref differ. |
| 7 | GET bundle | Bundle ref → temporary raw descriptor file; decode and schema/task/linkage checks. | `Store.get` → `ConventionalCAS.get(bundle_ref)`; decoded by shared `readback`. |
| 8 | GET model | Model ref → temporary 2,037-byte model file; hash verification. | `ConventionalCAS.get(model_ref)` |
| 9 | GET input | Input ref → temporary 11-byte normalized input file; hash and normalization checks. | `ConventionalCAS.get(input_ref)` |
| 10 | GET policy | Policy ref → temporary 245-byte exact APL file; hash/linkage checks. | `ConventionalCAS.get(policy_ref)` |
| 11 | GET tokenizer | Tokenizer ref → temporary 256-byte reversible text representation; hash/type checks. | `ConventionalCAS.get(tokenizer_ref)` |
| 12 | GET result | Result ref → temporary 442-byte result file; hash/linkage/decision checks. | `ConventionalCAS.get(result_ref)` |
| 13 | GET provenance | Provenance ref → temporary 1,703-byte evidence file; hash/policy/mode/candidate linkage checks. | `ConventionalCAS.get(provenance_ref)`; backend provenance bytes/ref differ. |

Every logical operation above is required by this matched instrumented boundary:
one task execution, seven persisted roles and readback of all seven. B additionally
persists result/provenance in its seven in-process `cas.put` calls. The boundary
requires the obligations, not a fundamental minimum of 13 operating-system
processes; alternative implementations would be a separate experiment.

For every A invocation, the original `apparatus.launch` fsyncs a launch-start
record, creates a subprocess with captured stdout/stderr and
`T81_CANONFS_READ_VERIFY=1`, waits synchronously with a timeout, and fsyncs its
completion/failure row. Nested processes inherit the outer worker process group.
The CLI starts afresh, parses arguments, runs its command, serializes outputs and
terminates. The observer clocks Popen through communicate return at the invoking
worker. Original journal I/O and added observer fsync are outside that nested
window but inside the outer worker interval. No two nested calls run concurrently.

`Store.put` independently hashes the original byte buffer and checks returned ref;
each temp directory is removed after its call. `Store.get` reads the CLI's output
file, independently hashes bytes against the requested ref, then removes its temp
directory. Shared `readback` verifies model fingerprint, input normalization,
policy/result/provenance links, strict mode, candidate scores/selection and common
evidence, then enumerates store files for storage accounting.

Source-grounded filesystem behavior:

- `tools/cli/core/driver.cpp::canonfs_put_file` checks/reads the source file,
  constructs a persistent driver and calls `write_object`, printing the ref.
- `fs/persistent_driver.cpp` constructs `objects`, `caps` and `parity` directories,
  checks capabilities/hooks, hashes bytes and writes a `.blk` file with exclusive
  create (or accepts an existing ref). Its writes shown here are not fsynced;
  durable **journal** fsync must not be misreported as durable payload fsync.
- `canonfs_get` calls `load_canonfs_object` / `read_object_bytes`, which checks
  permissions, opens/stats/reads (POSIX mmap path), verifies its content hash,
  and writes the requested temporary output file. Driver-local caches end with
  each CLI process. Kernel/filesystem caches may persist across commands/pairs.
- The worker materializes model/policy/tokenizer temporary files before EXEC,
  constructs the normalized input and bundle outside EXEC, and performs its
  independent hashes/JSON checks and temporary cleanup outside nested clocks.
- B's `ConventionalCAS` makes `objects`, hashes bytes, uses Python exclusive-file
  writes or reads existing objects, and verifies byte identity on reads, all in
  its single worker process. It creates no corresponding CLI subprocesses.

The separate `CLI --version` control executes argument parsing and `print_version`
then returns before task/CAS dispatch in `tools/cli/core/main.cpp::main`. It has no
artifact-store obligation or store argument. It still includes dynamic/static
initialization, command handling, output, wait and teardown. It is not a substitute
for any of the 13 real operations and cannot measure pure process startup alone.
