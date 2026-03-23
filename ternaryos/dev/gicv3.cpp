// experimental/ternaryos/dev/gicv3.cpp
//
// ARM GICv3 minimal driver.
//
// Bare-metal path: volatile 32-bit MMIO + AArch64 system register MSR/MRS.
// Hosted path: all hardware access compiled out (safe for unit tests on macOS).

#include "gicv3.hpp"

namespace t81::ternaryos::dev {

// ── Hardware access helpers ──────────────────────────────────────────────────

#if defined(__aarch64__) && !defined(__APPLE__)

static inline void gicd_write32(uint64_t base, uint64_t off,
                                uint32_t val) noexcept {
  *reinterpret_cast<volatile uint32_t*>(base + off) = val;
}

static inline uint32_t gicd_read32(uint64_t base, uint64_t off) noexcept {
  return *reinterpret_cast<const volatile uint32_t*>(base + off);
}

static inline void gicd_write64(uint64_t base, uint64_t off,
                                uint64_t val) noexcept {
  *reinterpret_cast<volatile uint64_t*>(base + off) = val;
}

// Spin until GICD_CTLR.RWP clears (register write pending).
static inline void gicd_wait_rwp(uint64_t dist_base) noexcept {
  while (gicd_read32(dist_base, kGicdCtlr) & kGicdCtlrRwp) {
    __asm__ volatile("isb");
  }
}

// ── AArch64 system register accessors (EL1) ──────────────────────────────────

static inline void icc_sre_el1_write(uint64_t val) noexcept {
  __asm__ volatile("msr icc_sre_el1, %0\n\t"
                   "isb"
                   :: "r"(val) : "memory");
}

static inline void icc_pmr_el1_write(uint64_t val) noexcept {
  __asm__ volatile("msr icc_pmr_el1, %0\n\t"
                   "isb"
                   :: "r"(val) : "memory");
}

static inline void icc_igrpen1_el1_write(uint64_t val) noexcept {
  __asm__ volatile("msr icc_igrpen1_el1, %0\n\t"
                   "isb"
                   :: "r"(val) : "memory");
}

static inline uint64_t icc_iar1_el1_read() noexcept {
  uint64_t val;
  __asm__ volatile("mrs %0, icc_iar1_el1" : "=r"(val));
  return val;
}

static inline void icc_eoir1_el1_write(uint64_t val) noexcept {
  __asm__ volatile("msr icc_eoir1_el1, %0" :: "r"(val) : "memory");
}

#else  // hosted build — all hardware access is a no-op

static inline void gicd_write32(uint64_t, uint64_t, uint32_t) noexcept {}
static inline uint32_t gicd_read32(uint64_t, uint64_t) noexcept { return 0; }
static inline void gicd_write64(uint64_t, uint64_t, uint64_t) noexcept {}
static inline void gicd_wait_rwp(uint64_t) noexcept {}

static inline void icc_sre_el1_write(uint64_t) noexcept {}
static inline void icc_pmr_el1_write(uint64_t) noexcept {}
static inline void icc_igrpen1_el1_write(uint64_t) noexcept {}
static inline uint64_t icc_iar1_el1_read() noexcept {
  return kGicSpuriousIntid;
}
static inline void icc_eoir1_el1_write(uint64_t) noexcept {}

#endif

// ── Internal helpers ─────────────────────────────────────────────────────────

/// GICD_IGROUPR[n]: each register covers 32 consecutive INTIDs.
static void set_intid_group1(uint64_t dist_base, uint32_t intid) noexcept {
  const uint32_t reg_idx = intid / 32u;
  const uint32_t bit     = 1u << (intid % 32u);
  const uint64_t off     = kGicdIgrouprN + reg_idx * 4u;
  const uint32_t cur     = gicd_read32(dist_base, off);
  gicd_write32(dist_base, off, cur | bit);
}

/// GICD_ISENABLER[n] / GICD_ICENABLER[n].
static void set_intid_enable(uint64_t dist_base, uint32_t intid,
                              bool enable) noexcept {
  const uint32_t reg_idx = intid / 32u;
  const uint32_t bit     = 1u << (intid % 32u);
  const uint64_t base_off = enable ? kGicdIsenabler0 : kGicdIcenabler0;
  gicd_write32(dist_base, base_off + reg_idx * 4u, bit);
}

/// GICD_IPRIORITYR[n]: one byte per INTID, packed 4-per-word.
static void set_intid_priority(uint64_t dist_base, uint32_t intid,
                                uint8_t priority) noexcept {
  const uint32_t reg_idx  = intid / 4u;
  const uint32_t byte_off = intid % 4u;
  const uint64_t off      = kGicdIpriorityrN + reg_idx * 4u;
  uint32_t cur = gicd_read32(dist_base, off);
  cur &= ~(0xFFu << (byte_off * 8u));
  cur |= (static_cast<uint32_t>(priority) << (byte_off * 8u));
  gicd_write32(dist_base, off, cur);
}

// ── Driver API ───────────────────────────────────────────────────────────────

void gicv3_init(uint64_t dist_base, uint64_t redist_base) noexcept {
  // 1. Enable distributor with Affinity Routing + Group 1 NS.
  //    Write ARE_NS first, then EnableGrp1NS (spec requires ARE before enable).
  gicd_write32(dist_base, kGicdCtlr, kGicdCtlrAreNs);
  gicd_wait_rwp(dist_base);
  gicd_write32(dist_base, kGicdCtlr, kGicdCtlrAreNs | kGicdCtlrEnableGrp1NS);
  gicd_wait_rwp(dist_base);

  // 2. Wake up CPU0 redistributor: clear ProcessorSleep, wait for
  //    ChildrenAsleep to deassert.
  const uint32_t waker = gicd_read32(redist_base, kGicrWaker);
  gicd_write32(redist_base, kGicrWaker, waker & ~kGicrWakerProcessorSleep);
  // Poll ChildrenAsleep until it clears (max ~1000 iterations in simulation).
  for (int i = 0; i < 1000; ++i) {
    if (!(gicd_read32(redist_base, kGicrWaker) & kGicrWakerChildrenAsleep)) {
      break;
    }
    __asm__ volatile("isb");
  }

  // 3. Set all SGIs (0..15) and PPIs (16..31) in the SGI frame to Group 1 NS.
  gicd_write32(redist_base, kGicrIgroupr0, 0xFFFF'FFFFu);

  // 4. Enable the system-register CPU interface (ICC_SRE_EL1.SRE = 1).
  icc_sre_el1_write(1u);

  // 5. Priority mask: accept all (lowest-priority = 0xFF).
  icc_pmr_el1_write(0xFFu);

  // 6. Enable Group 1 interrupts at the CPU interface.
  icc_igrpen1_el1_write(1u);
}

void gicv3_set_spi_group1(uint64_t dist_base, uint32_t intid) noexcept {
  if (intid < 32u || intid > kGicMaxIntid) return;
  set_intid_group1(dist_base, intid);
}

void gicv3_enable_spi(uint64_t dist_base, uint32_t intid) noexcept {
  if (intid < 32u || intid > kGicMaxIntid) return;
  set_intid_enable(dist_base, intid, true);
}

void gicv3_disable_spi(uint64_t dist_base, uint32_t intid) noexcept {
  if (intid < 32u || intid > kGicMaxIntid) return;
  set_intid_enable(dist_base, intid, false);
}

void gicv3_set_priority(uint64_t dist_base, uint32_t intid,
                        uint8_t priority) noexcept {
  if (intid > kGicMaxIntid) return;
  set_intid_priority(dist_base, intid, priority);
}

void gicv3_route_spi(uint64_t dist_base, uint32_t intid,
                     uint8_t aff0) noexcept {
  if (intid < 32u || intid > kGicMaxIntid) return;
  // GICD_IROUTER: 64-bit, Aff0 in bits [7:0], IRM=0 (route to specific CPU).
  const uint64_t off = kGicdIrouterN + (intid - 32u) * 8u;
  gicd_write64(dist_base, off, static_cast<uint64_t>(aff0));
}

uint32_t gicv3_acknowledge() noexcept {
  return static_cast<uint32_t>(icc_iar1_el1_read() & 0xFFFFFFu);
}

void gicv3_eoi(uint32_t intid) noexcept {
  icc_eoir1_el1_write(static_cast<uint64_t>(intid));
}

void gicv3_enable_ppi(uint64_t redist_base, uint32_t intid) noexcept {
  if (intid < 16u || intid > 31u) return;
  // PPIs live in the SGI frame's GICR_ISENABLER0 (same register as SGIs).
  const uint32_t bit = 1u << intid;
  gicd_write32(redist_base, kGicrIsenabler0, bit);
}

}  // namespace t81::ternaryos::dev
