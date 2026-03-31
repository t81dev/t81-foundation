# CanonFS Interchange Example

This is the smallest golden RFC-00D1 example in the repo.

It shows a real host directory import into CanonFS, a real export back out to a
host directory, and one explicit policy-profile denial.

## Files

- `input/alpha.txt`
- `input/nested/beta.txt`

## Run

From the repo root:

```bash
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"
export_root="$tmp_root/exported"

build/t81 canonfs import \
  examples/canonfs-interchange/input \
  --canonfs-root "$canon_root" \
  --json
```

Expected shape:

- schema: `t81.canonfs-import.v1`
- source kind: `host-directory`
- status: `ok`
- policy profile: `permissive`
- `manifest_ref` populated

Export the returned `manifest_ref` back to a host directory:

```bash
manifest_ref="<manifest_ref from previous step>"

build/t81 canonfs export \
  "$manifest_ref" \
  --canonfs-root "$canon_root" \
  --out "$export_root" \
  --json
```

Expected shape:

- schema: `t81.canonfs-export.v1`
- target kind: `host-directory`
- status: `ok`
- `materialized_paths` includes `alpha.txt` and `nested/beta.txt`

You can verify the exported payloads directly:

```bash
cat "$export_root/alpha.txt"
cat "$export_root/nested/beta.txt"
```

## Negative Path

This lane also supports explicit policy-profile denial without needing an extra
policy file:

```bash
build/t81 canonfs import \
  examples/canonfs-interchange/input \
  --canonfs-root "$canon_root" \
  --policy-profile export-only \
  --json
```

Expected shape:

- status: `error`
- policy result: `denied`
- error kind: `policy-failure`
- error code: `canonfs-policy-denied`

## Notes

- This example is intentionally file-based and small.
- It is meant to match the current RFC-00D1 CLI seed and contract tests, not to
  demonstrate every future interchange feature.
