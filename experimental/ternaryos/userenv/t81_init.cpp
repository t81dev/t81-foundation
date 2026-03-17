// experimental/ternaryos/userenv/t81_init.cpp
//
// RFC-00B9 §4 — t81-init: First userland process.
//
// t81-init is the statically linked TISC binary that the Axion kernel
// hands control to after completing boot. It is responsible for:
//   - Reading t81-services.json and parsing the service graph
//   - Computing topological spawn order for required services
//   - Recording the boot hash to CanonFS
//   - Spawning required services in dependency order
//   - Halting the system on any service spawn failure

#include "service_registry.hpp"
#include "session_manager.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace t81::ternaryos::userenv {

// ─── Boot hash computation (AC-2) ─────────────────────────────────────────────

static uint64_t compute_boot_hash(const ServiceRegistry& registry) {
  // FNV-1a hash of all service entries in spawn order
  uint64_t h = 14695981039346656037ULL;
  
  auto required_order = registry.required_order();
  for (const auto* service : required_order) {
    // Hash service id + binary + canon_hash
    std::string combined = service->id + "|" + service->binary + "|" + service->canon_hash;
    for (unsigned char c : combined) {
      h ^= c;
      h *= 1099511628211ULL;
    }
  }
  
  return h;
}

static std::string hex64(uint64_t v) {
  char buf[17];
  std::snprintf(buf, sizeof(buf), "%016" PRIx64, v);
  return buf;
}

// ─── Service spawning simulation (AC-1, AC-3) ─────────────────────────────────────

static bool spawn_service(const ServiceEntry& service) {
  std::printf("[t81-init] Spawning service: %s (binary: %s)\n", 
              service.id.c_str(), service.binary.c_str());
  
  // AC-3: Binary integrity check simulation
  // In real implementation, this would:
  // 1. Load binary from CanonFS
  // 2. Compute CanonHash81
  // 3. Compare with service.canon_hash
  // 4. Fire Axion IntegrityViolation on mismatch
  
  if (service.canon_hash.empty()) {
    std::fprintf(stderr, "[t81-init] ERROR: Service %s has no canon_hash\n", 
                service.id.c_str());
    return false;
  }
  
  // Simulate successful spawn
  std::printf("[t81-init] Service %s spawned successfully (hash: %s)\n",
              service.id.c_str(), service.canon_hash.substr(0, 8).c_str());
  
  return true;
}

// ─── Main init sequence (AC-1, AC-2) ─────────────────────────────────────────────

static int run_init(const std::string& services_file) {
  std::printf("[t81-init] Starting TernaryOS user environment initialization\n");
  
  // Load service registry
  ServiceRegistry registry;
  if (!registry.load_from_file(services_file)) {
    std::fprintf(stderr, "[t81-init] ERROR: Failed to load services from %s\n", 
                services_file.c_str());
    return 1;
  }
  
  std::printf("[t81-init] Loaded %zu services from %s\n", 
              registry.services.size(), services_file.c_str());
  
  // Compute and record boot hash (AC-2)
  uint64_t boot_hash = compute_boot_hash(registry);
  std::string boot_hash_hex = hex64(boot_hash);
  
  std::printf("[t81-init] Boot hash: %s\n", boot_hash_hex.c_str());
  std::printf("[t81-init] Recording boot hash to CanonFS: canonfs://var/log/boot-%" PRIx64 ".canonhash\n", 
              boot_hash);
  
  // In real implementation, this would write to CanonFS via Axion kernel
  // For now, we just simulate the recording
  
  // Spawn required services in topological order (AC-1)
  auto required_order = registry.required_order();
  std::printf("[t81-init] Spawning %zu required services\n", required_order.size());
  
  for (const auto* service : required_order) {
    if (!spawn_service(*service)) {
      std::fprintf(stderr, "[t81-init] FATAL: Failed to spawn required service %s\n", 
                  service->id.c_str());
      std::fprintf(stderr, "[t81-init] System halt required per RFC-00B9 AC-1\n");
      return 1;
    }
  }
  
  std::printf("[t81-init] All required services spawned successfully\n");
  std::printf("[t81-init] Boot sequence complete, handing off to session manager\n");
  
  return 0;
}

} // namespace t81::ternaryos::userenv

// ─── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
  std::string services_file = "t81-services.json";
  
  if (argc > 1) {
    services_file = argv[1];
  }
  
  return t81::ternaryos::userenv::run_init(services_file);
}
