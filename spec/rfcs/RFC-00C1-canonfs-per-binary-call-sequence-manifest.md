# RFC-00C1: CanonFS Per-Binary Call Sequence Manifest

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81X binary format, T81M manifest format, TernaryOS EL0 loader
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00C0 (CanonFS Executable Identity), RFC-00BF (Freestanding KernelCall Observability)
**Blocks:** RFC-00C2 (Hardware-Interrupt-Driven WaitForDevice Wake)

---

## Summary

This RFC introduces the **T81M manifest format** — a 512-byte CanonFS sector that
records the expected KernelCall sequence for a T81X v2 binary, keyed by its
`code_hash` (FNV-1a-64, from RFC-00C0).

After executing a binary, the EL1 loader loads the binary's manifest sector,
verifies the `code_hash` matches, then walks the manifest entries against the
observability ring (RFC-00BF) to confirm every expected call was recorded.

The Phase 12 CI gate confirms that the el0_wait_test binary (tid=4,
WaitForDevice) produces exactly the manifest-specified call sequence.

---

## Motivation

RFC-00C0 established executable identity via `code_hash`: a binary is uniquely
identified by the FNV-1a-64 hash of its code section.  RFC-00BF established the
observability ring as the ground-truth record of every KernelCall dispatched.

These two mechanisms are connected here: a binary identified by `code_hash` H
has an associated expected call sequence manifest M(H) stored in CanonFS.
After execution, if the obs ring does not match M(H), the binary is either
corrupted or executing unexpectedly — both are security-relevant events in a
governance-enforced system.

This closes the RFC-00C0 acceptance criterion:
> RFC-00C1 stores per-binary expected call sequences in CanonFS, keyed by `code_hash`.

---

## Scope

This RFC governs:

1. **T81M manifest format** — fixed 512-byte sector: 32-byte header +
   up to 40 entries of 12 bytes each.
2. **Manifest sector addressing** — one manifest sector per binary; LBA
   placement is by convention (binary LBA + 1 for Phase 12; a future CanonFS
   index RFC will generalise this).
3. **Manifest verification** — `canon_manifest_verify(code_hash, manifest_lba)`:
   loads the manifest, checks `code_hash` match, walks entries against the obs
   ring, emits Phase 12 gate on full match.
4. **Phase 12 CI gate** — `[axion] el0: manifest OK (code_hash=verified, seq=1/1 matched)`
5. **CI toolchain** — Python `write_t81m()` function computes `code_hash` at
   image-build time and embeds the manifest.

This RFC does **not** govern:

- Per-manifest signing or MAC (deferred).
- Manifest index in CanonFS superblock (deferred to a future CanonFS index RFC).
- Negative-match (reject on unexpected extra calls): the ring may contain more
  records than the manifest; only the manifest entries are checked.
- Hardware-interrupt-driven WaitForDevice waker (RFC-00C2).

---

## T81M Manifest Sector Layout

```
T81M sector (512 bytes, one CanonFS LBA):

Header (32 bytes):
  [0:4]   magic        b'T81M'  (0x54, 0x38, 0x31, 0x4D)
  [4]     version      1        (uint8_t)
  [5:8]   reserved0    0        (3 bytes)
  [8:16]  code_hash    uint64_t (FNV-1a-64 of binary's code section, little-endian)
  [16]    entry_count  uint8_t  (number of expected manifest entries; max 40)
  [17:32] reserved1    0        (15 bytes)

Entries (12 bytes each, packed after header):
  [0:4]   tid      uint32_t  (thread id that must issue the call)
  [4:8]   kind     uint32_t  (KernelCallKind ordinal, RFC-00BD frozen)
  [8:12]  peer_tid uint32_t  (expected peer_tid in obs record; 0 if N/A)
```

Maximum entries per sector: `(512 − 32) / 12 = 40`.

The manifest loader verifies entries in declaration order. Each entry is
checked via `fs_obs_find(tid, kind, peer_tid)` against the obs ring; all
must match for the manifest to pass.

---

## Manifest Verification Algorithm

```
canon_manifest_verify(expected_hash, manifest_lba):
  1. Read manifest_lba → reject on LBA error.
  2. Verify b'T81M' magic → reject on mismatch.
  3. Verify version == 1 → reject on unknown version.
  4. Read code_hash from [8:16] → reject if ≠ expected_hash.
  5. Read entry_count from [16] → reject if 0 or > 40.
  6. For each entry i in [0 .. entry_count):
       read (tid, kind, peer_tid) from header + 32 + i*12
       if fs_obs_find(tid, kind, peer_tid) → matched++
  7. If matched == entry_count:
       emit "[axion] el0: manifest OK (code_hash=verified, seq=M/N matched)"
     else:
       emit "[axion] el0: manifest FAIL (seq mismatch)"
```

`fs_obs_find` returns true if any obs ring record has matching `(tid, kind,
peer_tid)` fields regardless of `seq_id` (which is runtime-monotonic and not
part of the manifest contract).

---

## Phase 12 CI Sequence

```
[Phase 11 completes — obs ring holds WaitForDevice from tid=4]

EL1: canon_identity_load_and_run() (end of function)
  └─ canon_manifest_verify(computed_hash, 9u)
       ├─ read LBA 9 → T81M sector
       ├─ verify magic=T81M, version=1
       ├─ verify code_hash == computed_hash (set at image-build time)
       ├─ entry_count = 1: {tid=4, kind=43 (WaitForDevice), peer_tid=0}
       ├─ fs_obs_find(4u, 43u, 0u) → true (recorded in Phase 11b)
       ├─ matched=1 == entry_count=1
       └─ emit "[axion] el0: manifest OK (code_hash=verified, seq=1/1 matched)"
```

CI gate (added to Validate boot sequence step):
```
check "[axion] el0: manifest OK (code_hash=verified, seq=1/1 matched)"
```

---

## CI Toolchain: write_t81m()

The following Python function is added to the Phase 12 image-build step in
`.github/workflows/qemu-boot.yml`.  It runs after `write_t81x_v2()` so that
`el0_wait_test.bin` is already available and the hash can be computed from it.

```python
FNV_OFFSET = 14695981039346656037
FNV_PRIME  = 1099511628211

def fnv1a64(data):
    h = FNV_OFFSET
    for b in data:
        h ^= b
        h = (h * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return h

def write_t81m(bin_path, manifest_entries, lba, img_path='canon_store.img'):
    """
    manifest_entries: list of (tid, kind, peer_tid).
    code_hash is computed from bin_path and embedded in the manifest header.
    """
    with open(bin_path, 'rb') as f:
        code = f.read()
    code_hash = fnv1a64(code)
    entry_count = len(manifest_entries)
    assert entry_count <= 40, f"too many manifest entries: {entry_count}"

    hdr  = b'T81M'
    hdr += struct.pack('B', 1)                # version
    hdr += b'\x00\x00\x00'                   # reserved0
    hdr += struct.pack('<Q', code_hash)       # code_hash [8:16]
    hdr += struct.pack('B', entry_count)      # entry_count [16]
    hdr += bytes(15)                          # reserved1 [17:32]
    assert len(hdr) == 32

    body = b''
    for (tid, kind, peer_tid) in manifest_entries:
        body += struct.pack('<III', tid, kind, peer_tid)

    sector = hdr + body
    sector += bytes(512 - len(sector))
    assert len(sector) == 512

    offset = lba * 512
    with open(img_path, 'r+b') as f:
        f.seek(offset)
        f.write(sector)
    print(f"T81M manifest written at LBA{lba} (offset {offset}): "
          f"code_hash=0x{code_hash:016x}, {entry_count} entries")

# Phase 12 manifest: el0_wait_test.bin → LBA 8; manifest → LBA 9.
write_t81m('el0_wait_test.bin',
           [(4, 43, 0)],   # tid=4, kind=WaitForDevice(43), peer_tid=0
           lba=9)
```

---

## Acceptance Criteria

- [x] T81M sector layout: 32-byte header (magic, version, code_hash, entry_count) +
  12-byte entries (tid, kind, peer_tid); max 40 entries per sector
- [x] `canon_manifest_verify()` implemented in `canon_exec_loader.cpp`; validates
  magic, version, code_hash, entry_count; walks entries via `fs_obs_find()`
- [x] `cel_pl011_putd()` decimal printer added; Phase 12 gate string uses
  `seq=N/N matched` format
- [x] `canon_manifest_verify(computed_hash, 9u)` called at end of
  `canon_identity_load_and_run()`
- [x] `write_t81m()` in CI: computes FNV-1a-64 from binary, writes T81M at LBA 9
  with entry `{tid=4, kind=43, peer_tid=0}`
- [x] Phase 12 CI gate: `[axion] el0: manifest OK (code_hash=verified, seq=1/1 matched)`
- [ ] RFC-00C2: hardware-interrupt-driven WaitForDevice waker (GICv3 timer → `fs_sched_wake_device`)
- [ ] Manifest index in CanonFS superblock (future CanonFS index RFC)

---

## Relationship to Other RFCs

- **RFC-00BD** — `WaitForDevice` ordinal 43 is the `kind` field in the manifest entry.
- **RFC-00BF** — `fs_obs_find(tid, kind, peer_tid)` is the ring query used by `canon_manifest_verify`.
- **RFC-00C0** — `code_hash` (FNV-1a-64) is the manifest key; T81X v2 loader
  computes it and passes it to `canon_manifest_verify`.
- **RFC-00C2** — timer-IRQ-driven WaitForDevice waker; manifest verification
  still applies post-wake.

---

## References

- [RFC-00BD: KernelCall ABI Ordinal Freeze](RFC-00BD-kernelcall-abi-ordinal-freeze.md)
- [RFC-00BE: Freestanding Cooperative Scheduler](RFC-00BE-freestanding-cooperative-scheduler.md)
- [RFC-00BF: Freestanding KernelCall Observability](RFC-00BF-freestanding-kernelcall-observability.md)
- [RFC-00C0: CanonFS Executable Identity](RFC-00C0-canonfs-executable-identity.md)
- `userland/experimental/hal/canon_exec_loader.cpp` — manifest verification impl
- `.github/workflows/qemu-boot.yml` — `write_t81m()` CI toolchain function
