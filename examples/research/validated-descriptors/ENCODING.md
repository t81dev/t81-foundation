# Experiment descriptor encoding

This is a private research encoding, not a new T81 manifest or public schema.
`runner.cpp decode/resolve` and `compare.py decode/conventional` implement it.

Exactly six nonempty ASCII lines, each terminated by one LF byte, maximum 4096
bytes total. Every field byte must be in `[33,126]`. No CRLF, whitespace,
escaping, duplicate fields, optional fields, or trailing lines are accepted.
Fields, in order:

| Line | Field | Validation |
| --- | --- | --- |
| 1 | Schema/version | Exactly `t81.experiment-descriptor.v1` |
| 2 | Payload content reference | Exactly 64 lowercase hex digits; first 32 bytes of SHA3-512, matching CanonFS hash bytes |
| 3 | Representation | `base81-symbolic-tisc` or `safetensors-i8`, paired with the provider below |
| 4 | Provider | `t81vm` or `native-weights`, respectively |
| 5 | Supported operations | Exactly the singleton `run` or `inspect`, respectively; comma-separated/duplicate/extra operations are rejected |
| 6 | Validation constraint profile | Exactly `object-interface-fixtures-v1` |

The constraint profile admits **only the two exact canonical payloads from PR
#540**, not arbitrary programs or SafeTensors files. TISC bytes are the public
renderer output for load 6 into r40, load 7 into r41, multiply into r42, halt.
SafeTensors bytes are the canonical compact JSON header for `weight`, `I8`,
shape `[2,2]`, offsets `[0,4]`, padded to 8 bytes, preceded by its little-endian
64-bit length, followed by bytes `ff 00 01 ff`. The executable constructors and
recorded payload hex in [results.json](results.json) specify the exact bytes.

Example (payload reference shown for the recorded TISC fixture):

```text
t81.experiment-descriptor.v1
c5f5925cf1d666588094757aaf5750741e847cd977bf2023c134d25ba0a2bed1
base81-symbolic-tisc
t81vm
run
object-interface-fixtures-v1
```

Descriptor identity is the same hash algorithm applied to the **descriptor's own
bytes**. It is not the payload reference. The caller independently pins both
expected hashes, the requested operation, and authorization policy. Neither
expected hash is taken on trust from the descriptor being validated.

All six fields are untrusted assertions until validation. Representation is
independently checked by equality with the adapter's known valid fixture;
operations are checked against the adapter's compiled singleton operation set.
A declaration can neither add an adapter nor extend its operations. A descriptor
with a fresh internally consistent hash but false capability still rejects.

Trusted assertions: the compiled catalog correctly describes the two providers,
the known fixtures have the intended semantics, the caller's expected references
are authentic, and the host enforces the final authorization check. There are no
provider signatures, authenticated descriptors, inferred logical identities,
or general format-validation/sandboxing claims.

Storage admission, validated operation discovery, and authorization are separate.
Intake/read occurs under allowed storage policy before descriptor preflight;
invalid metadata may therefore accompany successfully stored bytes. Before a
useful operation, B separately evaluates Axion with `final_authorization=true`
and the requested operation/payload context; C uses an exact local allowlist.
The descriptor contains no permission field. The existing VM also retains its
instruction-level policy checks. The original mutable source is never reopened
for useful provider invocation; a private snapshot is used for the path-only
model loader. Caller and provider preparation run in one trusted process.
