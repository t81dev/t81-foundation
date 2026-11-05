# CPA006 exact commands and replay

The experiment ran in `/private/tmp/t81-cpa006` on macOS ARM64. Python tooling
used `/private/tmp/t81-h04-enforcement-venv/bin/python` with jsonschema 4.23.0.
Paths are actual recorded paths; relocate consistently for another checkout.
No hosted library or EFI binary came from the user's dirty shared checkout.

Source setup used a separate shared clone, checked out main
`ea6d031a8117341fe03013cc9d5473de1b07727d`, and applied the exact CPA005
integration patch. The resulting starting tree was
`70734fd5bcfe06978cea13b4d2d595d4dda71a12`. It is retained as normal single-parent
commit `f7e089590020508077607580da271b6570fa8f60` on the review-only base branch
`experiment/cpa005-integration-base`; this is not a merge or production activation.
The focused CPA006 draft targets that baseline branch, so unchanged PR709
implementation/evidence is a visible dependency rather than repeated in its diff.
Neither original PR nor main was changed.

The baseline driver (`baseline.py`, initially executed from an identical scratch
copy) first checked every CPA005 packaged hash, the reused compiled-binary hashes,
then ran three fresh hosted processes and guests for each CPA005 source snapshot.
Its exact arguments and outcomes are under `evidence/baseline/commands.json`.
`evidence/baseline/cpa005-reference.tar.gz` retains CPA005's complete original
manifest-listed packet and tools; `cpa003-packet.tar.gz` retains the original
CPA003 oracle packet. Extract these archives into scratch, never over historical
records. Baseline guest staging/vars files were generated and then removed after
hashing; their raw serial streams remain.

Freeze and independent integer derivation preceded CPA006 implementation and
execution. Reproduce that derivation without running either VM:

```sh
python3 -B experiments/cpa006/freeze_suite.py --check
```

The full initial configure/build arguments and successful exits are retained in
`evidence/initial-build-commands.json`; full output is `configure.*` and `build.*`.
A fresh build directory was used. To repeat the initial build:

```sh
cmake -S . -B /private/tmp/cpa006-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DT81_BUILD_TESTS=OFF -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=ON -DT81_ENABLE_DPE=OFF -DT81_BUILD_CPA006_EXPERIMENT=ON -DT81_CPA006_CASE=C01
cmake --build /private/tmp/cpa006-build --target t81_ternaryos_qemu_slice6_efi t81_vm -j 4
/private/tmp/t81-h04-enforcement-venv/bin/python -B experiments/cpa006/native_checks.py
/private/tmp/t81-h04-enforcement-venv/bin/python -B experiments/cpa006/run_suite.py
python3 experiments/cpa006/negatives.py
/private/tmp/t81-h04-enforcement-venv/bin/python -B experiments/cpa006/mutation_check.py
python3 experiments/cpa006/off.py
/private/tmp/t81-h04-enforcement-venv/bin/python -B experiments/cpa006/verify.py
# Expected nonzero, without changing frozen expectations:
/private/tmp/t81-h04-enforcement-venv/bin/python -B experiments/cpa006/verify.py --corrupt-expected
python3 -B scripts/ci/check_tisc_freeze_integrity.py
git diff --check
```

`run_suite.py` compiles a new observer against freshly built libraries, runs 57
hosted processes, then configures/builds one actual-input EFI per sealed case
and runs three fresh boots per case, with at most three simultaneous guests.
All exact per-case build and hosted/QEMU argument arrays are retained. Every
bootstrap stages `TERNOS`, as the unchanged firmware entry requires it. Each
guest has fresh firmware vars, 512MiB RAM, one TCG cortex-a57 CPU and a 30-second
deadline. The published data contain no timeout; any future timeout is
INCONCLUSIVE. Exit/stop status alone is never computational Halt.

Before repeating live execution, archive old output and use fresh build/output
paths (including QEMU `boot-N` directories), preserving existing evidence.
The scripts deliberately do not delete failed old runs or overwrite frozen
artifacts. The two setup drivers `off.py` and `negatives.py` were originally
run from identical `/private/tmp/cpa006-*.py` copies. `off.py` compares against
the historical CPA005 main-control sections; extract the reference archive
into a consistently relocated scratch path when reproducing elsewhere.

`verify.py` can replay all packaged raw CPA006 streams without QEMU, native
binaries or old workspace state; it validates the freeze receipt, every frozen
hash, oracle regeneration, input bytes, guest markers, raw-stream hashes,
terminal classification, full schema records and cross-profile equality.
`native_checks.py` and `negatives.py` record explicit NDEBUG, Debug, Release and
sanitizer compile/run commands. These are direct executions, not CTest or remote
CI claims. The unchanged freeze-integrity script's reported 209 registry count
does not resolve the 174/209/244 inventory discrepancies.
