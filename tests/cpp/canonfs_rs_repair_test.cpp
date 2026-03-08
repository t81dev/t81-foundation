#include <iostream>
#include <vector>
#include "t81/canonfs/rs_repair.hpp"

static bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

int main() {
    bool ok = true;

    std::vector<std::byte> data = {
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
        std::byte{0x04}, std::byte{0x05}, std::byte{0x06},
        std::byte{0x07}, std::byte{0x08}, std::byte{0x09}
    };

    auto encoded = t81::canonfs::ReedSolomonRepair::encode(data);
    ok &= expect(encoded.size() == 5, "Encoded should have 5 shards");

    // Simulate losing shard 1 and 4
    std::vector<bool> avail = {true, false, true, true, false};
    auto repaired = t81::canonfs::ReedSolomonRepair::repair(encoded, avail);

    ok &= expect(repaired.size() == 3, "Repaired should have 3 data shards");

    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            ok &= expect(repaired[i][j] == encoded[i][j], "Data mismatch in repair");
        }
    }

    if (ok) {
        std::cout << "canonfs_rs_repair_test: all checks PASSED\n";
        return 0;
    }
    std::cerr << "canonfs_rs_repair_test: FAILED\n";
    return 1;
}
