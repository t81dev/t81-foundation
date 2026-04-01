# CanonFS v1 Contract Example

This directory is the canonical RFC-00D1 v1 contract fixture set.

The JSON files here are meant to be read as frozen examples of the current
CanonFS interchange contract, not as a new interchange request format.

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

## Inputs

- `model.t81w`
  the stable single-file payload used by the success cases
- `invalid-schema.json`
  JSON input used for the `invalid_schema` failure case
- `policy-deny-all.apl`
  Axion policy input used for the policy-denial case
