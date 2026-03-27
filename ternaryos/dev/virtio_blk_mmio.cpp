// ternaryos/dev/virtio_blk_mmio.cpp
//
// VirtioBlkMmioDevice — virtio v1.0 MMIO transport, synchronous polling.
//
// Initialisation sequence (virtio spec 1.2 §3.1.1):
//   1. Reset device (Status = 0).
//   2. Set ACKNOWLEDGE + DRIVER.
//   3. Read DeviceFeatures, negotiate (we take no optional features).
//   4. Set FEATURES_OK; re-read to confirm.
//   5. Configure queue 0 (descriptor table, available ring, used ring).
//   6. Set DRIVER_OK.
//
// I/O (synchronous, queue depth = 1):
//   Submit a 3-descriptor chain (header | data | status) to the available
//   ring, write QueueNotify, spin-poll the used ring idx.
//
// On non-bare-metal builds every MMIO read returns 0 and every write is
//  a no-op, so probe() returns false and no side effects occur.

#include "virtio_blk_mmio.hpp"

#include <atomic>
#include <cstring>

namespace t81::ternaryos::dev {

// ── MMIO helpers ──────────────────────────────────────────────────────────────
//
// Bare-metal guard: AArch64 (non-Apple) or native Linux x86_64.
// All other builds (macOS, cross-compile for test) return 0 / no-op.

namespace {

inline void mmio_wr32(uint64_t base, uint32_t off, uint32_t v) noexcept {
#if !defined(T81_TERNARYOS_HOSTED_BUILD) &&           \
    ((defined(__aarch64__) && !defined(__APPLE__)) || \
     (defined(__x86_64__) && !defined(_WIN32) && !defined(__APPLE__)))
  *reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(base + off)) = v;
#else
  (void)base;
  (void)off;
  (void)v;
#endif
}

inline uint32_t mmio_rd32(uint64_t base, uint32_t off) noexcept {
#if !defined(T81_TERNARYOS_HOSTED_BUILD) &&           \
    ((defined(__aarch64__) && !defined(__APPLE__)) || \
     (defined(__x86_64__) && !defined(_WIN32) && !defined(__APPLE__)))
  return *reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(base + off));
#else
  (void)base;
  (void)off;
  return 0u;
#endif
}

inline void mem_barrier() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile("dsb sy" ::: "memory");
#elif defined(__x86_64__) && !defined(_WIN32)
  __asm__ volatile("mfence" ::: "memory");
#else
  std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

// Cast a pointer to its physical address for DMA programming.
// On a bare-metal identity-mapped system, virtual == physical.
inline uint64_t phys(const void* p) noexcept {
  return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p));
}

}  // namespace

// ── probe() ───────────────────────────────────────────────────────────────────

bool VirtioBlkMmioDevice::probe(uint64_t base, VirtioBlkMmioDevice& dev) noexcept {
  // 1. Verify magic and device type.
  if (mmio_rd32(base, kVirtioMagic) != 0x74726976u) return false;  // 'virt'
  if (mmio_rd32(base, kVirtioVersion) != 2u) return false;         // modern
  if (mmio_rd32(base, kVirtioDeviceId) != 2u) return false;        // block

  dev.mmio_base_ = base;

  // 2. Reset.
  mmio_wr32(base, kVirtioStatus, 0u);
  mem_barrier();

  // 3. ACK + DRIVER.
  mmio_wr32(base, kVirtioStatus, kVirtioStatusACK | kVirtioStatusDriver);

  // 4. Feature negotiation — accept no optional features.
  mmio_wr32(base, kVirtioDevFeatSel, 0u);
  mmio_wr32(base, kVirtioDrvFeatSel, 0u);
  mmio_wr32(base, kVirtioDrvFeatures, 0u);

  mmio_wr32(base, kVirtioStatus, kVirtioStatusACK | kVirtioStatusDriver | kVirtioStatusFeatOK);
  mem_barrier();

  if (!(mmio_rd32(base, kVirtioStatus) & kVirtioStatusFeatOK)) {
    mmio_wr32(base, kVirtioStatus, kVirtioStatusFailed);
    return false;
  }

  // 5. Read device capacity (in 512-byte sectors).
  const uint32_t cap_lo = mmio_rd32(base, kVirtioConfigCapacity);
  const uint32_t cap_hi = mmio_rd32(base, kVirtioConfigCapacity + 4u);
  dev.total_sectors_ = (static_cast<uint64_t>(cap_hi) << 32) | cap_lo;

  // 6. Configure queue 0.
  mmio_wr32(base, kVirtioQueueSel, 0u);

  const uint32_t qmax = mmio_rd32(base, kVirtioQueueNumMax);
  if (qmax == 0u) {
    mmio_wr32(base, kVirtioStatus, kVirtioStatusFailed);
    return false;
  }

  const uint32_t qsize =
      (kQueueSize < static_cast<int>(qmax)) ? static_cast<uint32_t>(kQueueSize) : qmax;
  mmio_wr32(base, kVirtioQueueNum, qsize);

  // Zero-initialise rings before telling device about them.
  std::memset(dev.desc_, 0, sizeof(dev.desc_));
  std::memset(&dev.avail_, 0, sizeof(dev.avail_));
  std::memset(&dev.used_, 0, sizeof(dev.used_));
  dev.next_avail_ = 0;

  mmio_wr32(base, kVirtioQueueDescLow, static_cast<uint32_t>(phys(dev.desc_)));
  mmio_wr32(base, kVirtioQueueDescHigh, static_cast<uint32_t>(phys(dev.desc_) >> 32));
  mmio_wr32(base, kVirtioQueueDrvLow, static_cast<uint32_t>(phys(&dev.avail_)));
  mmio_wr32(base, kVirtioQueueDrvHigh, static_cast<uint32_t>(phys(&dev.avail_) >> 32));
  mmio_wr32(base, kVirtioQueueDevLow, static_cast<uint32_t>(phys(&dev.used_)));
  mmio_wr32(base, kVirtioQueueDevHigh, static_cast<uint32_t>(phys(&dev.used_) >> 32));
  mmio_wr32(base, kVirtioQueueReady, 1u);

  // 7. DRIVER_OK — device is live.
  mmio_wr32(base, kVirtioStatus,
            kVirtioStatusACK | kVirtioStatusDriver | kVirtioStatusFeatOK | kVirtioStatusDriverOK);
  mem_barrier();

  dev.ready_ = true;
  return true;
}

// ── IBlockDevice ──────────────────────────────────────────────────────────────

BlockDeviceInfo VirtioBlkMmioDevice::info() const noexcept {
  return BlockDeviceInfo{
      .total_blocks = total_sectors_ / kSectorsPerBlock,
      .block_size_bytes = kBlockSize,
      .read_only = false,
      .device_id = "virtio-blk-mmio",
  };
}

uint64_t VirtioBlkMmioDevice::block_count() const noexcept {
  return ready_ ? (total_sectors_ / kSectorsPerBlock) : 0u;
}

// ── submit_and_wait() ─────────────────────────────────────────────────────────
//
// Build a 3-descriptor chain:
//   [0] req header (host reads)
//   [1] data buffer (host reads for WRITE, host writes for READ)
//   [2] status byte (host always writes)
//
// Add head descriptor [0] to the available ring, notify device, then
// spin-poll the used ring until a completion appears.

bool VirtioBlkMmioDevice::submit_and_wait(uint32_t type, uint64_t lba) const noexcept {
  if (!ready_) return false;

  // Fill request header.
  req_hdr_.type = type;
  req_hdr_.reserved = 0u;
  req_hdr_.sector = lba * kSectorsPerBlock;
  req_status_ = 0xFFu;  // sentinel — device overwrites on completion

  // Descriptor 0: request header (device reads).
  const int d0 = 0, d1 = 1, d2 = 2;
  auto& dd = const_cast<VirtioBlkMmioDevice*>(this)->desc_;

  dd[d0].addr = phys(&req_hdr_);
  dd[d0].len = sizeof(req_hdr_);
  dd[d0].flags = kVirtqDescFNext;
  dd[d0].next = static_cast<uint16_t>(d1);

  // Descriptor 1: data buffer (device reads for write, writes for read).
  dd[d1].addr = phys(data_buf_);
  dd[d1].len = static_cast<uint32_t>(kSectorsPerBlock * kPhysSectorBytes);
  dd[d1].flags = kVirtqDescFNext | (type == kVirtioBlkRead ? kVirtqDescFWrite : 0u);
  dd[d1].next = static_cast<uint16_t>(d2);

  // Descriptor 2: status byte (device always writes).
  dd[d2].addr = phys(&req_status_);
  dd[d2].len = 1u;
  dd[d2].flags = kVirtqDescFWrite;
  dd[d2].next = 0u;

  mem_barrier();

  // Add to available ring.
  auto& av = const_cast<VirtioBlkMmioDevice*>(this)->avail_;
  auto& nxt = const_cast<VirtioBlkMmioDevice*>(this)->next_avail_;
  av.ring[nxt % kQueueSize] = static_cast<uint16_t>(d0);
  mem_barrier();
  av.idx = static_cast<uint16_t>(++nxt);
  mem_barrier();

  // Notify device (queue 0).
  mmio_wr32(mmio_base_, kVirtioQueueNotify, 0u);

  // Spin-poll used ring.
  const uint16_t expected = nxt;
  auto& us = const_cast<VirtioBlkMmioDevice*>(this)->used_;
  while (static_cast<volatile uint16_t&>(us.idx) != expected) {
    mem_barrier();
  }
  mem_barrier();

  // ACK interrupt.
  mmio_wr32(mmio_base_, kVirtioIntACK, mmio_rd32(mmio_base_, kVirtioIntStatus));

  return req_status_ == kVirtioBlkOK;
}

bool VirtioBlkMmioDevice::read_block(uint64_t lba, BlockData& out) const {
  if (lba >= block_count()) return false;
  if (!submit_and_wait(kVirtioBlkRead, lba)) return false;
  std::memcpy(out.data(), data_buf_, kBlockSize);
  return true;
}

bool VirtioBlkMmioDevice::write_block(uint64_t lba, const BlockData& data) {
  if (lba >= block_count()) return false;
  std::memset(data_buf_, 0, sizeof(data_buf_));
  std::memcpy(data_buf_, data.data(), kBlockSize);
  return submit_and_wait(kVirtioBlkWrite, lba);
}

bool VirtioBlkMmioDevice::flush() {
  if (!ready_) return false;
  return submit_and_wait(kVirtioBlkFlush, 0u);
}

}  // namespace t81::ternaryos::dev
