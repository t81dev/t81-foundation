# CPA-004 commands and environment

Commands below ran from `/tmp/t81-cpa-004` unless paths specify otherwise.
`/tmp` and `/private/tmp` resolve to the same location on this host. Commands
completed with exit 0 except the intentional NDEBUG failure and the initial
guest timeout supervisor run (exit 1). Exact compiler/link command lines are in
`evidence/freestanding-build-commands.txt`; version output is in
`evidence/environment.json`. Raw build output is retained in `build.log` and
`build-final.log` under evidence. No remote CI result is claimed.

```sh
python3 -B /tmp/T81-CPA-003/verify_packet.py
python3 -B /tmp/T81-CPA-003/reproduce_oracle.py
cmake -S /tmp/t81-cpa-004 -B /tmp/cpa004-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DT81_BUILD_TESTS=OFF -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=ON -DT81_ENABLE_DPE=OFF -DT81_BUILD_CPA004_EXPERIMENT=ON
cmake --build /tmp/cpa004-build --target t81_ternaryos_qemu_slice6_efi t81_cpa004_admission_test -j 4
/tmp/cpa004-build/t81_cpa004_admission_test
/opt/homebrew/opt/llvm/bin/clang++ -std=c++17 -O0 -g -UNDEBUG -Wall -Wextra -Wpedantic -Werror -Iternaryos/hal -I/tmp/cpa004-build/ternaryos/generated experiments/cpa004/admission_test.cpp ternaryos/hal/cpa004_evaluator.cpp -o /tmp/cpa004-results/admission-debug-final
/tmp/cpa004-results/admission-debug-final
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -O0 -g -UNDEBUG -DT81_DETERMINISTIC -I/tmp/t81-cpa-803075e-fbzvi39l/include experiments/cpa004/hosted_observer.cpp -L/tmp/t81-cpa-build -L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++ -lt81_vm -lt81_isa -lt81_axion -lt81_ffi -lt81_experimental -lt81_core -o /tmp/cpa004-results/hosted_observer
python3 -B experiments/cpa004/evidence/acceptance/check.py --hosted-observer /tmp/cpa004-results/hosted_observer --output /tmp/cpa004-results/recheck
python3 -B experiments/cpa004/run_qemu.py --build /tmp/cpa004-build --output /tmp/cpa004-results/boots-ternos --qemu /opt/homebrew/bin/qemu-system-aarch64 --firmware-code /opt/homebrew/share/qemu/edk2-aarch64-code.fd --firmware-vars /opt/homebrew/share/qemu/edk2-arm-vars.fd
/tmp/t81-h04-enforcement-venv/bin/python -B experiments/cpa004/verify_evidence.py
git diff --exit-code 803075e9f7e36ed4b5318a8d2ddaacf62ae0a7e4 -- include/t81/isa isa vm spec kernel/axion lang
git diff --check
```

The original CPA-003 oracle packet is external, pinned by the copied
`evidence/acceptance/packet-manifest.json`; the self-contained CPA-004 replay
checks require only this directory, Python, jsonschema 4.23.0, and (for live
hosted replay) a newly compiled hosted observer. `verify_evidence.py` validates
retained observations without QEMU; `check.py` independently re-derives all states
and launches three hosted processes. Use new output paths when re-running live
QEMU boots, since each run creates fresh ESP staging and firmware variables.

Hosted libraries were reused from the clean pinned baseline build, not rebuilt
from the user's dirty checkout. Their exact original configure/build commands,
hashes and the earlier Ninja recovery message are retained in
`evidence/acceptance/hosted-build.json` and `CPA003-HOSTED_REPORT.md`. The observer
was freshly recompiled from the formatted source in this implementation.
`T81_CANONFS_ROOT` is removed from each hosted process environment. CMake's
repository-wide optimization/NDEBUG flags apply even to some Debug library
builds; the observation/admission checks use explicit failure returns.
The native admission executable was run directly, not reported as a CTest run
with `T81_BUILD_TESTS=OFF`.

The first QEMU invocation used the same command with output `/tmp/cpa004-results/boots`
before the runner staged `TERNOS`; all three 30-second deadlines were inconclusive.
The corrected run reused the identical final EFI artifact. The supervisor records
every actual QEMU argument and firmware/input hash in its summary.
Default-off object comparisons and the temporary wrong-expectation Release
compile/run are recorded with exact arguments/stdout/stderr/status in
`option-off-check.json` and `ndebug-failure-demonstration.json` under evidence.
The wrong expectation exists only in a temporary test copy, not the submitted
test. The final `verify_evidence.py` replay passed after correcting an initial
comparison of raw hosted diagnostic fields against the normalized projection;
neither raw observations nor expected states changed.
