#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/storage_mapping.hpp"
namespace fs = std::filesystem;
using namespace t81::canonfs;
void require(bool ok, const char* why) {
  if (!ok) throw std::runtime_error(why);
}
std::span<const std::byte> bytes(const std::string& s) {
  return std::as_bytes(std::span(s.data(), s.size()));
}
void put(const fs::path& path, const std::string& data) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  f.write(data.data(), static_cast<std::streamsize>(data.size()));
  f.close();
  require(bool(f), "fixture write failed");
}
std::string get(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), {}};
}
template <class F>
void rejects(F action) {
  bool rejected = false;
  try {
    action();
  } catch (const std::exception&) {
    rejected = true;
  }
  require(rejected, "layout unexpectedly accepted");
}
int main(int argc, char** argv) {
  try {
    require(argc == 2, "usage: contract_test fresh-output-root");
    const fs::path base = argv[1];
    require(!fs::exists(base), "fresh output required");
    fs::create_directories(base);
    // Measure filesystem behavior rather than infer it from the OS name.
    const auto namespace_probe = base / "namespace-probe";
    fs::create_directory(namespace_probe);
    put(namespace_probe / "A", "upper");
    put(namespace_probe / fs::path(u8"\u00e9"), "composed");
    put(namespace_probe / "trailing.", "dot");
    put(namespace_probe / "space ", "space");
    std::cout << "{\"case_fold_alias\":" << fs::exists(namespace_probe / "a")
              << ",\"normalization_alias\":" << fs::exists(namespace_probe / fs::path(u8"e\u0301"))
              << ",\"trailing_dot_alias\":" << fs::exists(namespace_probe / "trailing")
              << ",\"trailing_space_alias\":" << fs::exists(namespace_probe / "space") << "}\n";
    const CanonHash upper_hash{t81::hash::CanonHash81::from_string("A")};
    const CanonHash lower_hash{t81::hash::CanonHash81::from_string("a")};
    require(portable_storage_key(upper_hash) != portable_storage_key(lower_hash),
            "case fold lost digest identity");
    // Exercise every byte value in every position, including leading zeros.
    for (std::size_t pos = 0; pos < 32; ++pos)
      for (unsigned value = 0; value < 256; ++value) {
        CanonHash hash{};
        hash.h.bytes[pos] = static_cast<std::uint8_t>(value);
        auto key = portable_storage_key(hash);
        auto restored = parse_portable_storage_key(key);
        require(key.size() == 64 && restored && *restored == hash, "mapping inverse failed");
      }
    for (const std::string& bad : std::vector<std::string>{
             "", "CON", "nul.blk", "../objects", "A", std::string(63, '0'), std::string(65, '0'),
             std::string(64, 'A'), std::string(63, '0') + "g", std::string(64, '0') + " ",
             std::string(64, '0') + ".", std::string(64, '0') + std::string(1, '\0'),
             std::string(63, '0') + std::string(1, '\0'), std::string(63, '0') + "/",
             std::string(63, '0') + "\\"})
      require(!parse_portable_storage_key(bad), "malformed key accepted");
    const std::string payload = "8491 25000\n";
    const CanonRef expected{CanonHash{t81::hash::hash_string(payload)}};
    const auto key = portable_storage_key(expected.hash);
    // Native Unicode root uses filesystem's UTF-8-to-native path conversion.
    const auto root = base / fs::path(u8"portable-\u00e9-\u03bb");
    auto d = make_portable_persistent_driver(root);
    auto ref = d->write_object(ObjectType::RawBlock, bytes(payload));
    require(ref && ref->hash == expected.hash, "write identity");
    const auto path = root / "v2" / "objects" / (key + ".blk");
    require(get(path) == payload, "disk bytes");
    require(!fs::exists(root / "objects"), "legacy directory created");
    require(!fs::exists(root / "v2" / "staging" / (key + ".partial")),
            "successful staging not cleared");
    auto again = d->write_object(ObjectType::CanonTensor, bytes(payload));
    require(again && again->hash == ref->hash, "duplicate/type identity");
    d.reset();
    d = make_portable_persistent_driver(root);
    require(d->read_object_bytes(*ref).value() ==
                std::vector<std::byte>(bytes(payload).begin(), bytes(payload).end()),
            "cold read");
    rejects([&] { make_persistent_driver(root); });
    require(!fs::exists(root / "objects"), "legacy factory mutated v2");
    // Real disk corruption is detected even with a previously successful read.
    put(path, payload.substr(0, 3));
    require(!d->read_object_bytes(*ref), "warm read hid corruption");
    require(!d->write_object(ObjectType::RawBlock, bytes(payload)),
            "duplicate repaired/overwrote corrupt bytes");
    require(get(path) == payload.substr(0, 3), "corruption evidence overwritten");
    put(path, payload + "extra");
    require(!d->read_object_bytes(*ref), "appended corruption accepted");
    put(path, payload);
    require(d->read_object_bytes(*ref).has_value(), "restored fixture");
    // Interrupted private staging is retained and never used as a final object.
    const std::string later = "next-payload";
    const CanonRef later_ref{CanonHash{t81::hash::hash_string(later)}};
    auto stage = root / "v2" / "staging" / (portable_storage_key(later_ref.hash) + ".partial");
    put(stage, "next-");
    require(!d->write_object(ObjectType::RawBlock, bytes(later)), "stale partial overwritten");
    require(get(stage) == "next-" && !d->read_object_bytes(later_ref), "partial became readable");
    d.reset();
    d = make_portable_persistent_driver(root);
    require(!d->write_object(ObjectType::RawBlock, bytes(later)), "partial reused after restart");
    // Denial cannot create a staged or final object.
    d->set_axion_hook([](auto, const auto&) { return AxionVerdict{false, "contract deny"}; });
    require(!d->write_object(ObjectType::RawBlock, bytes("denied")), "policy bypass");
    const auto denied_key = portable_storage_key(CanonHash{t81::hash::hash_string("denied")});
    require(!fs::exists(root / "v2" / "objects" / (denied_key + ".blk")) &&
                !fs::exists(root / "v2" / "staging" / (denied_key + ".partial")),
            "denial mutated storage");
    d->set_axion_hook({});
    CapabilityGrant grant{};
    grant.target = *ref;
    grant.perms = CANON_PERM_READ;
    require(d->publish_capability(grant).has_value(), "publish mask");
    const auto cap = root / "v2" / "caps" / (key + ".cap");
    require(get(cap) == "1\n", "cap file bytes");
    d.reset();
    d = make_portable_persistent_driver(root);
    require(d->read_object_bytes(*ref).has_value(), "cold cap read");
    require(!d->write_object(ObjectType::RawBlock, bytes(payload)), "read-only cap allowed write");
    put(cap, "65536\n");
    require(!d->read_object_bytes(*ref), "overflow cap accepted");
    put(cap, "1junk\n");
    require(!d->read_object_bytes(*ref), "malformed cap accepted");
    grant.perms = CANON_PERM_READ | CANON_PERM_WRITE;
    require(d->publish_capability(grant).has_value() && get(cap) == "3\n", "atomic cap update");
    require(d->write_object(ObjectType::RawBlock, bytes(payload)).has_value(),
            "restored cap write");
    require(d->revoke_capability(*ref).has_value() && !fs::exists(cap), "cap revoke path");
    // No implicit migration or dual-namespace lookup in either direction.
    const auto legacy = base / "legacy";
    auto l = make_persistent_driver(legacy);
    const std::string legacy_payload = "legacy-safe-0";
    const auto legacy_ref = l->write_object(ObjectType::RawBlock, bytes(legacy_payload));
    require(legacy_ref.has_value(), "safe legacy fixture write failed");
    l.reset();
    l = make_persistent_driver(legacy);
    auto legacy_read = l->read_object_bytes(*legacy_ref);
    require(legacy_read && *legacy_read == std::vector<std::byte>(bytes(legacy_payload).begin(),
                                                                  bytes(legacy_payload).end()),
            "legacy store no longer readable");
    // Platform-independent legacy fixture uses the all-zero typed digest's '0' path.
    const auto legacy_fixture = legacy / "objects" / "0.blk";
    put(legacy_fixture, "");
    rejects([&] { make_portable_persistent_driver(legacy); });
    require(get(legacy_fixture).empty(), "legacy fixture overwritten");
    require(!fs::exists(legacy / "canonfs-layout"), "legacy store converted");
    // Marker corruption, incomplete init, mixed stores and filename aliases fail closed.
    const auto marker = root / "canonfs-layout";
    const auto marker_text = get(marker);
    put(marker, "unknown-version\n");
    rejects([&] { make_portable_persistent_driver(root); });
    put(marker, marker_text);
    fs::create_directory(root / "objects");
    rejects([&] { make_portable_persistent_driver(root); });
    fs::remove(root / "objects");
    const auto malformed = root / "v2" / "objects" / "CON.blk";
    std::ofstream invalid_name(malformed, std::ios::binary);
    const bool opened_reserved = bool(invalid_name);
    invalid_name.close();
    bool reserved_entry = false;
    for (const auto& entry : fs::directory_iterator(malformed.parent_path()))
      if (entry.path().filename() == malformed.filename()) reserved_entry = true;
    std::cout << "{\"reserved_name_opened\":" << opened_reserved
              << ",\"reserved_directory_entry\":" << reserved_entry << "}\n";
    if (reserved_entry) {
      rejects([&] { make_portable_persistent_driver(root); });
      fs::remove(malformed);
    } else {
      // Win32 may open a reserved device without creating any directory entry.
      // The format validator must judge the actual directory, not stream state.
      require(!parse_portable_storage_key("CON"), "reserved key accepted");
      require(make_portable_persistent_driver(root) != nullptr, "device probe damaged the store");
    }
    const auto alias = root / "v2" / "objects" / (std::string(64, 'A') + ".blk");
    put(alias, "x");
    rejects([&] { make_portable_persistent_driver(root); });
    fs::remove(alias);
    const auto incomplete = base / "incomplete";
    fs::create_directory(incomplete);
    put(incomplete / "canonfs-layout", marker_text);
    rejects([&] { make_portable_persistent_driver(incomplete); });
    rejects([&] { make_portable_persistent_driver(base / std::string(300, 'z')); });
    // A link/reparse point is never accepted as a format/object boundary.
    std::error_code link_error;
    const auto link_root = base / "linked-root";
    fs::create_directory_symlink(root, link_root, link_error);
    if (!link_error) {
      rejects([&] { make_portable_persistent_driver(link_root); });
      fs::remove(link_root);
    } else {
      std::cout << "{\"symlink_creation\":\"unavailable\",\"native_error\":" << link_error.value()
                << "}\n";
    }
    const auto device_root = base / "device-display-identities";
    auto device_store = make_portable_persistent_driver(device_root);
    for (const std::string name : {"CON", "PRN", "AUX", "NUL", "COM1", "COM9", "LPT1", "LPT9"}) {
      CapabilityGrant device_grant{};
      device_grant.target = CanonRef{CanonHash{t81::hash::CanonHash81::from_string(name)}};
      device_grant.perms = CANON_PERM_READ;
      const auto device_key = portable_storage_key(device_grant.target.hash);
      require(device_store->publish_capability(device_grant).has_value(),
              "device-display capability create");
      require(get(device_root / "v2" / "caps" / (device_key + ".cap")) == "1\n",
              "device-display native path");
      auto missing = device_store->read_object_bytes(device_grant.target);
      require(!missing && missing.error() == Error::NotFound,
              "device-display lookup escaped storage");
    }
    // The key stays constant even when root paths differ in space/Unicode/case.
    const auto second = base / "Other Root";
    auto s = make_portable_persistent_driver(second);
    auto sr = s->write_object(ObjectType::RawBlock, bytes(payload));
    require(sr && sr->hash == expected.hash, "root changed identity");
    require(get(second / "v2" / "objects" / (key + ".blk")) == payload, "second root bytes");
    std::cout << "{\"status\":\"pass\",\"mapping_byte_position_cases\":8192,\"key\":\"" << key
              << "\"}\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
