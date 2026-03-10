// RFC-0000 §1: CanonFS Parity 3+2 Reed-Solomon Verification
// Formally verifies GF(3^9) math underlying 3 Data + 2 Parity robust storage.

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "t81/canonfs/rs_repair.hpp"

using t81::canonfs::ReedSolomonRepair;

static bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

int main() {
  bool ok = true;

  // Verify GF(3^9) inversions for a few values
  for (int x = 1; x <= 10; ++x) {
    auto inv_x = t81::canonfs::GF3_9::inv(x);
    auto mult = t81::canonfs::GF3_9::mul(x, inv_x);
    ok &= expect(mult == 1, ("GF3_9 invert failed for " + std::to_string(x)).c_str());
  }

  // 1. Prepare raw payload data (e.g. 15 bytes to elegantly divide by 3 data shards)
  std::vector<std::byte> original_payload;
  std::string secret = "THE_T81_RS_3+2_"; // exactly 15 chars 
  for (char c : secret) {
    original_payload.push_back(static_cast<std::byte>(c));
  }

  // 2. Encode into 5 shards (kDataShards=3, kParityShards=2)
  auto shards = ReedSolomonRepair::encode(original_payload);
  ok &= expect(shards.size() == 5, "Encode should return 5 shards");
  
  // Each data shard should be 5 bytes. Parity shards are 10 bytes.
  for (size_t i = 0; i < shards.size(); ++i) {
    if (i < t81::canonfs::ReedSolomonRepair::kDataShards) {
      ok &= expect(shards[i].size() == 5, "Each data shard should be 5 bytes");
    } else {
      ok &= expect(shards[i].size() == 10, "Each parity shard should be 10 bytes mapping GF(3^9) 16-bit elements");
    }
  }

  // 3. Drop shards 0 and 2 (simulate 2 data shard failures out of 5 total shards)
  // Available: {false, true, false, true, true}
  std::vector<bool> available = {false, true, false, true, true};
  
  // 4. Reconstruct broken shards
  auto recovered = ReedSolomonRepair::repair(shards, available);
  ok &= expect(recovered.size() == 3, "Repair should reconstruct the 3 original data shards");

  // 5. Compare with original text
  if (recovered.size() == 3) {
    std::vector<std::byte> reassembled;
    for (int i = 0; i < 3; ++i) {
      reassembled.insert(reassembled.end(), recovered[i].begin(), recovered[i].end());
    }

    ok &= expect(original_payload.size() == reassembled.size(), "Reassembled size mismatch");
    bool matching = true;
    for (size_t i = 0; i < original_payload.size(); ++i) {
      if (original_payload[i] != reassembled[i]) {
        matching = false;
        break;
      }
    }
    ok &= expect(matching, "Reassembled payload does not mathematically match original input");
  }

  if (ok) {
    std::cout << "t81_canonfs_rs_repair_test: 3+2 Shard Reconstruction PASSED\n";
    return 0;
  }

  std::cerr << "t81_canonfs_rs_repair_test: FAILED\n";
  return 1;
}
