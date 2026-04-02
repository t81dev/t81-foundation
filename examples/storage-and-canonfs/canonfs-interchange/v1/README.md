# CanonFS v1 Contract Example

This directory is the canonical RFC-00D1 v1 contract fixture set.

The JSON files here are meant to be read as frozen examples of the current
CanonFS interchange contract, not as a new interchange request format.

This fixture set is the current build-against seed for RFC-00D1, not proof that
the entire RFC has been promoted out of `draft`.

The current seed boundary here is intentionally narrow:

- `canonfs import` / `canonfs export`
- `t81.canonfs-import.v1` / `t81.canonfs-export.v1`
- `t81.canonfs-import-provenance.v1` /
  `t81.canonfs-export-provenance.v1`
- `t81.canonfs-interchange-manifest.v1`
- `host-file`
- the built-in `permissive` policy profile recorded in the result
- structured error entries with `kind`, `message`, `code`, and `reason`

This fixture set demonstrates the current seed contract. It does not settle
deferred RFC-00D1 questions such as symlink posture, archive/bundle export, or
text output as a co-equal contract.

For each case:

- `*.request.json` describes the CLI invocation in a stable, machine-readable
  form
- `*.output.json` is the exact JSON emitted by the current CLI/core contract

These files are compared byte-for-byte in the contract tests.

## Cases

- `import-success.*`
  import a single `model.t81w` file successfully
- `export-success.*`
  export that same CanonFS object successfully
- `export-missing-object.*`
  export a missing CanonFS object and emit a structured error
- `export-invalid-schema.*`
  export a JSON object with a non-manifest `schema` field and emit
  `invalid_schema`
- `import-policy-denied.*`
  import through a checked-in policy file that denies the request
- `export-policy-denied.*`
  export through that same checked-in policy file and emit a structured
  `policy_denied` error while the recorded built-in profile remains
  `permissive`

## Inputs

- `model.t81w`
  the stable single-file payload used by the success cases
- `invalid-schema.json`
  JSON input used for the `invalid_schema` failure case
- `policy-deny-all.apl`
  Axion policy input used for the policy-denial cases
