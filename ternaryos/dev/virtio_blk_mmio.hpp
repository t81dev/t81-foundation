#pragma once

// ternaryos/dev/virtio_blk_mmio.hpp
//
// VirtioBlkMmioDevice: IBlockDevice backed by a virtio-blk MMIO transport.
//
// Implements the virtio v1.0 (modern) MMIO transport for the QEMU virt
// machine.  Supports synchronous (polling) I/O — no interrupt handler
// required.  Static ring buffers; no heap allocation.
//
// On bare-metal AArch64 / x86_64: drives MMIO registers directly.
// On hosted (macOS / Linux) builds: probe() always returns false and no
// MMIO is touched.
//
// Usage (bare-metal):
//   VirtioBlkMmioDevice dev;
//   if (VirtioBlkMmioDevice::probe(0x0A000000, dev)) {
//     BlockData buf;
//     dev.read_block(0, buf);
//   }
//
// RFC-00B2 §2.4 (MMIO block device).

#include "block_device.hpp"

#include <cstdint>
#include <string>

namespace t81::ternaryos::dev {

// ─── Virtio-blk MMIO register offsets ────────────────────────────────────────

inline constexpr uint32_t kVirtioMagic           = 0x000u;
inline constexpr uint32_t kVirtioVersion         = 0x004u;
inline constexpr uint32_t kVirtioDeviceId        = 0x008u;
inline constexpr uint32_t kVirtioDeviceFeatures  = 0x010u;
inline constexpr uint32_t kVirtioDevFeatSel      = 0x014u;
inline constexpr uint32_t kVirtioDrvFeatures     = 0x020u;
inline constexpr uint32_t kVirtioDrvFeatSel      = 0x024u;
inline constexpr uint32_t kVirtioQueueSel        = 0x030u;
inline constexpr uint32_t kVirtioQueueNumMax     = 0x034u;
inline constexpr uint32_t kVirtioQueueNum        = 0x038u;
inline constexpr uint32_t kVirtioQueueReady      = 0x044u;
inline constexpr uint32_t kVirtioQueueNotify     = 0x050u;
inline constexpr uint32_t kVirtioIntStatus       = 0x060u;
inline constexpr uint32_t kVirtioIntACK          = 0x064u;
inline constexpr uint32_t kVirtioStatus          = 0x070u;
inline constexpr uint32_t kVirtioQueueDescLow    = 0x080u;
inline constexpr uint32_t kVirtioQueueDescHigh   = 0x084u;
inline constexpr uint32_t kVirtioQueueDrvLow     = 0x090u;
inline constexpr uint32_t kVirtioQueueDrvHigh    = 0x094u;
inline constexpr uint32_t kVirtioQueueDevLow     = 0x0A0u;
inline constexpr uint32_t kVirtioQueueDevHigh    = 0x0A4u;
inline constexpr uint32_t kVirtioConfigCapacity  = 0x100u; // 64-bit, 512-byte sectors

// Virtio device status flags
inline constexpr uint32_t kVirtioStatusACK       = 1u;
inline constexpr uint32_t kVirtioStatusDriver    = 2u;
inline constexpr uint32_t kVirtioStatusFeatOK    = 8u;
inline constexpr uint32_t kVirtioStatusDriverOK  = 4u;
inline constexpr uint32_t kVirtioStatusFailed    = 128u;

// Virtio descriptor flags
inline constexpr uint16_t kVirtqDescFNext        = 1u;
inline constexpr uint16_t kVirtqDescFWrite       = 2u;  // device writes

// Virtio-blk request types
inline constexpr uint32_t kVirtioBlkRead         = 0u;
inline constexpr uint32_t kVirtioBlkWrite        = 1u;
inline constexpr uint32_t kVirtioBlkFlush        = 4u;

// Virtio-blk request status
inline constexpr uint8_t  kVirtioBlkOK           = 0u;

// Physical sectors per logical CanonBlock (729 bytes < 2 × 512 = 1024)
inline constexpr uint64_t kSectorsPerBlock       = 2u;
inline constexpr uint64_t kPhysSectorBytes       = 512u;

// ─── VirtioBlkMmioDevice ──────────────────────────────────────────────────────

class VirtioBlkMmioDevice final : public IBlockDevice {
public:
  /// QEMU virt CanonFS virtio MMIO base (second device slot, 0x200 stride).
  /// Slot 0 (0x0A000000) is reserved for the FAT32 boot disk; slot 1
  /// (0x0A000200) is the dedicated raw CanonFS block store.
  static constexpr uint64_t kDefaultMmioBase = UINT64_C(0x0A000200);

  VirtioBlkMmioDevice() = default;

  /// Probe and initialise a virtio-blk device at `mmio_base`.
  /// Returns true if the device is present and ready.
  /// On hosted (non-bare-metal) builds, always returns false.
  static bool probe(uint64_t mmio_base, VirtioBlkMmioDevice& out) noexcept;

  // ── IBlockDevice ────────────────────────────────────────────────────────────
  BlockDeviceInfo info() const noexcept override;
  uint64_t        block_count() const noexcept override;
  bool            read_block(uint64_t lba, BlockData& out) const override;
  bool            write_block(uint64_t lba, const BlockData& data) override;
  bool            flush() override;

private:
  // Virtio ring entry structures (must be exact virtio layout).

  struct alignas(16) Desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
  };

  struct Avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[4];
    uint16_t used_event;
  };

  struct UsedElem { uint32_t id; uint32_t len; };
  struct Used {
    uint16_t  flags;
    uint16_t  idx;
    UsedElem  ring[4];
    uint16_t  avail_event;
  };

  struct ReqHdr {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
  };

  static constexpr int kQueueSize = 4;

  // ── State set by probe() ───────────────────────────────────────────────────
  uint64_t mmio_base_     = 0;
  uint64_t total_sectors_ = 0;  // from device config
  bool     ready_         = false;

  // ── Static ring buffers ────────────────────────────────────────────────────
  // Declared non-static so each VirtioBlkMmioDevice instance has its own
  // ring (supports up to one device instance per translation unit).
  alignas(16)  Desc    desc_[kQueueSize]   = {};
  alignas(2)   Avail   avail_             = {};
  alignas(4)   Used    used_              = {};

  // Per-request scratch (reused; single in-flight request only).
  mutable      ReqHdr  req_hdr_           = {};
  mutable      uint8_t req_status_        = 0xFF;
  alignas(512) mutable uint8_t data_buf_[kSectorsPerBlock * kPhysSectorBytes] = {};

  uint16_t next_avail_ = 0;  // shadow of avail_.idx

  // ── Internal helpers ───────────────────────────────────────────────────────
  bool submit_and_wait(uint32_t type, uint64_t lba) const noexcept;
};

}  // namespace t81::ternaryos::dev
