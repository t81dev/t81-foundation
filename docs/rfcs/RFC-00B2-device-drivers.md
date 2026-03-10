# RFC-00B2 — TernOS Device Driver Architecture

**Status:** Draft
**Phase:** 4 (v2.0 gate)
**Gate condition:** CanonFS read/write survives a simulated reboot cycle.
**Depends on:** RFC-00B0 (HAL), RFC-00B1 (Ternary MMU)

---

## 1  Scope

This RFC defines the device driver interfaces for TernOS Phase 4:

| Layer | Interface | Backed by |
| :--- | :--- | :--- |
| Block storage | `IBlockDevice` | NVMe in bare-metal; file-backed in hosted sim |
| CanonFS store | `CanonStore` | `IBlockDevice` + content-addressed index |
| Framebuffer | `TernaryFramebuffer` | Linear `TritPixel` array; stub I/O in Phase 4 |
| Network | `TernaryEthernetPacket` | Binary Ethernet frame + ternary payload |

---

## 2  Block Device Model

### 2.1  Block size

One logical block equals one `CanonBlock` = **729 trytes** (3⁶).
This aligns ternary-block boundaries with the CanonFS fundamental unit.

```
Byte layout: [ tryte[0] .. tryte[728] ]  (1 byte per tryte, values 0..2)
Block size: 729 bytes
```

### 2.2  `IBlockDevice` interface

```cpp
class IBlockDevice {
public:
  virtual BlockDeviceInfo info()  const noexcept = 0;
  virtual bool read_block (uint64_t lba, std::array<uint8_t,729>& out) const = 0;
  virtual bool write_block(uint64_t lba, const std::array<uint8_t,729>& data) = 0;
  virtual bool flush() = 0;
  virtual uint64_t block_count() const noexcept = 0;
};
```

- `lba` is a zero-based Logical Block Address.
- `write_block` succeeds if `lba < block_count()`.
- `flush` durably persists all written blocks (fsync equivalent).

### 2.3  `HostedBlockDev` (Phase 4 simulation)

In-memory `std::vector` of blocks; `save(path)` / `load(path)` for reboot simulation.
All blocks are zero-initialised on construction.

---

## 3  CanonFS Store

`CanonStore` wraps an `IBlockDevice` with a content-addressed write-once store.

```
put(block)  → hash block → allocate next LBA → write_block → record index
get(ref)    → look up LBA in index → read_block → verify hash → return block
rebuild()   → scan all allocated LBAs, re-hash, repopulate index
```

### 3.1  Index

In-memory `std::map<CanonHash, uint64_t>` (hash → LBA).
LBA 0 is reserved for the index header (see §3.2).
Data blocks start at LBA 1.

### 3.2  Index persistence (LBA 0 header block)

```
Bytes  0–3   : magic  "CST1"  (4 bytes)
Bytes  4–7   : entry_count  (uint32_t LE)
Bytes  8–727 : entries; each entry = 32-byte hash + 8-byte LBA (40 bytes)
               → max 17 entries per single-block index (Phase 4 cap)
Byte   728   : 0x00 padding
```

If `entry_count > 17`, additional index blocks are chained (Phase 5; not implemented here).

### 3.3  Durability contract

`CanonStore::flush()` writes the in-memory index to LBA 0, then calls `IBlockDevice::flush()`.
After flush, a new `CanonStore` opened on the same device can `rebuild_index()` and recover all blocks.

---

## 4  Ternary Framebuffer

A 2-D array of `TritPixel { int8_t value; }` with values in `{-1, 0, +1}`.

Default dimensions: **81 × 27** (3⁴ × 3³ tryte-aligned).

```
clear(fill)        — fill all pixels with a given trit
set_pixel(x,y,p)   — set one pixel (bounds-checked)
get_pixel(x,y)     — read one pixel
dump_ascii()       — render to string: '+' for +1, '·' for 0, '-' for -1
```

Phase 4 output target: `hal_log()`.  No hardware MMIO in hosted simulation.

---

## 5  Ternary Ethernet Packet

Compatibility layer bridging binary Ethernet (IEEE 802.3) to ternary payloads.

```cpp
struct TernaryEthernetPacket {
  std::array<uint8_t,6>  dst_mac;
  std::array<uint8_t,6>  src_mac;
  uint16_t               ethertype;     // binary compat field
  std::vector<int8_t>    trit_payload;  // values in {-1,0,+1}
  t81::canonfs::CanonRef content_ref;   // hash of trit_payload for Axion audit
};
```

`trit_payload.size()` must be a multiple of 3 (ternary word alignment).
`content_ref` is computed by the sender and verified by the receiver.

---

## 6  Acceptance Criteria

| ID | Criterion |
| :-- | :--- |
| AC-D1 | `HostedBlockDev` write/read round-trip for all 17 index-cap blocks is bit-exact |
| AC-D2 | `CanonStore::put` assigns distinct LBAs to distinct content; identical content deduplicates |
| AC-D3 | `CanonStore::flush` + reopen + `rebuild_index` recovers all stored blocks by hash |
| AC-D4 | `CanonStore::get` returns `nullopt` for an unknown `CanonRef` |
| AC-D5 | `TernaryFramebuffer::dump_ascii` produces non-empty output after `set_pixel` |
| AC-D6 | `TernaryEthernetPacket` with trit_payload not a multiple of 3 is rejected |
| AC-D7 | `CanonStore::get` verifies hash on read; a block with a corrupted tryte returns `nullopt` |
