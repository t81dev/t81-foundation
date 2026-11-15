# Final B0 environment record

Validated implementation commit: `54c1a92ffe5c19871c3b1a68c71083ac87fc7f0c`.
Audited source: `cda9135d36144d4de2771af174e52ac7b30a83c9`.
The following evidence commit adds this record and raw final outputs without
changing the implementation measured by the validation receipt.

Final command (exit 0):

```sh
python3 -B experiments/base01_lane_a/lane_a.py --validate-only \
  --source /private/tmp/t81-base01-audited \
  --source-build /private/tmp/t81-base01-source-build \
  --native-build /private/tmp/t81-base01-native-build \
  --output /private/tmp/t81-base01-final-validation
```

Actual host: Apple M2, Mac14,2, ARM64, 8 cores, 8 GiB RAM; macOS 26.5.2,
Darwin kernel 25.5.0. Compiler: Apple clang 21.0.0 (clang-2100.1.1.101),
`/usr/bin/c++`; source and runner Release build flags are retained in `final/`.
Python 3.14.3. Full sysctl observations, cache values and errors are in the receipt.
No CPU-affinity or frequency/boost controls were applied. macOS affinity is
unsupported. Observed 1/5/15-minute load averages were approximately
4.08/5.53/6.17, exceeding Phase A's <=0.10 readiness criterion. These are host
observations, not workload performance results.

Disposition: **B0 validated; B1 not authorized and this host not qualified.**
Remaining work is the reviewed measurement dispatcher/activation patch,
VM batch/timer-overhead qualification and a controlled/approved measurement
host. Linux execution remains unverified; Windows observer execution unsupported.
No baseline performance results exist in this packet.

Four audited correctness executables, all assigned native validation cases and
seven Python contract tests passed. Commons proposal shape/reference validation
passed. Both measurement entry points reject execution. Final stdout and exit
code are recorded here because the validator's own success output is emitted
after writing its receipt:

```json
{"status":"validation passed","receipt":"/private/tmp/t81-base01-final-validation/receipt.json","measurement_authorized":false}
```

Additional precommit validation of build-path/compiler/configuration checks passed
and is retained as `06-build-configuration-check` in the development archive.
All development and final artifact hashes were checked before the evidence commit.
No frozen artifacts, audited source bytes, governance state or main checkout
user files were changed. The implementation branch and audited tree were clean
before evidence was copied. No Lane B runner was accessed or coordinated.
