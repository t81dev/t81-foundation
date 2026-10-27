# Investigation 08 execution path and measurement boundaries

Recorded **before instrumentation changes**, October 27, 2026. Main is
`c930b8c2ee7478782bf3c6d231bdd59984595ef2`; unmerged #602 is
`7c61afa3ba0e97c3c800ca5cde567cf374d7a5cc`. This change is stacked on #602.

```text
OS loader / C++ static initialization (outside application boundary)
 tools/cli/core/main.cpp::main -> parse_args -> AI dispatch
 tools/cli/ai/ai_cli_shared.cpp::run -> named argument parsing
  run_fixed_label_task
   validate options / normalize input / read policy
   SHA3-512 model file fingerprint -> parse_policy
   policy_allows_single_model_hash / policy_requires_task_marker (admission)
   cli::load_weights_model -> driver model selection/path resolution
    weights::load_t81w -> file read
     load_t81w_bytes -> checksum validation / tensor decoding / materialization
   detect_architecture_profile / companion file resolution
   vm::run_native_vm_probe(make_initial_probe_request(...))
    build_decode_program
     lookup_tokenizer_prompt_token_ids -> candidate/context construction
     synthesize bounded T81Lang -> build_program_from_source -> bind model
    execute_decode_program
     make_interpreter_vm -> load_program
     run_to_halt -> step
      eval_axion_call -> syscall context -> Engine::evaluate
      TWEMBED / TVecAdd / TWMATMUL / TATTN / TERNACCUM
       native tensor helpers / ops / conversion / allocation
      Axion event capture / dispatch / state updates
    interpret_decode_execution
     parse printed scores / select candidate / export state / hash signatures
   derive_initial_transition -> materialize_assess_fixed_task
   render_task_provenance_artifact
   write_canonfs_raw_block -> persistent driver::write_object
    payload identity -> capability/hook checks -> exclusive file create/write
   CLI JSON serialization / output / local teardown
 application boundary ends -> profiler emission / C++ teardown / OS exit
```

Admission's hash/marker check does not install every task APL constraint into the
probe. The probe constructs an interpreter with its existing default engine.
Instrumentation must preserve this actual path, not silently strengthen it.
Existing trace/profile commands report VM traces/metrics; they do not expose this
CLI-to-artifact phase tree. Timing must remain separate from deterministic traces,
VM state, result/provenance bytes, signatures and CanonFS identities.

## Boundaries fixed before collection

Use `steady_clock` durations and an opt-in private scope recorder. Parent IDs,
relative start/end timestamps and direct-child subtraction permit independent
inclusive/exclusive reconstruction. **Sum exclusive durations only** when
partitioning application time. Parent inclusive values must not be added again.
Repeated names retain individual invocations; aggregate counts are derived later.

| Requested phase       | Actual boundary / qualification                                                                                                                                                              |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Process startup       | Unavailable inside the app. Parent process wall minus application window is a combined boundary residual, not isolated startup.                                                              |
| CLI initialization    | Entry and argument parser scopes; dispatch/runtime setup remains separately visible or in application-exclusive residual.                                                                    |
| Model loading         | `load_t81w` inclusive; child file-read and parser scopes. Model identity file fingerprinting for admission is separate, so repeated reads are not hidden.                                    |
| Model parsing         | `load_t81w_bytes`, inclusive checksum/decoding/materialization; no claim that decoding alone is isolated.                                                                                    |
| Tokenization          | Task input normalization and tokenizer-file encoding scopes; candidate-program construction/compilation separate.                                                                            |
| Policy admission      | Whole task admission including policy file read, file fingerprint and APL checks. Nested fingerprint/parse scopes are not double-counted. Per-instruction VM evaluation is separately named. |
| VM initialization     | Interpreter construction and load_program within decoder setup; new state is still created for each probe.                                                                                   |
| Tensor execution      | Existing embedding/projection/attention/vector-add operations inside VM execution; includes allocations/conversions performed in those calls.                                                |
| Output selection      | Native accumulation/candidate scoring within VM execution and candidate selection after execution are distinct scopes.                                                                       |
| Artifact persistence  | Driver write inclusive; identity hashing and file operations remain combined where no further scope exists.                                                                                  |
| Artifact verification | Persistent-driver read inclusive verification; fresh CLI lookup also has its own process window. Semantic evidence/replay checks are measured by the external harness.                       |
| Replay                | Parent-measured reconstruction and fresh re-execution. It is not relabeled as pure computation; reference CLI still needs original fixture files.                                            |
| Process shutdown      | Unavailable as a separate OS phase. Measured app-local destructors occur inside the window; recorder emission and final process teardown are in the boundary residual.                       |

An uninstrumented build must contain no clock/env/file recording work. A timing-
enabled build also requires an explicit output path; otherwise recording is idle.
Timing records use unique external sidecars, never deterministic artifacts.
Overhead is evaluated with separately built OFF/ON executables, equal build
configuration otherwise, fresh processes/stores and randomized recorded ordering.
Warm-process native probe repetitions may reuse the loaded model in the research
adapter; this is not a warmed full CLI and still initializes a VM per invocation.
