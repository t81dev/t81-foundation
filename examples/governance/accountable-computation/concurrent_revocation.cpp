// Bounded host-ownership experiment; no production Driver concurrency extension.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "dev/block_device.hpp"
#include "t81/canonfs/axion_hook.hpp"
#include "t81/tracing/canonhash.hpp"

namespace fs = std::filesystem;
using namespace t81::canonfs;
using Clock = std::chrono::steady_clock;
constexpr auto kWatchdog = std::chrono::seconds(10);
const std::string kPayload = "25000\n";

void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
std::span<const std::byte> bytes(const std::string& text) {
  return std::as_bytes(std::span(text.data(), text.size()));
}
std::string hex(std::span<const std::byte> data) {
  constexpr char digits[] = "0123456789abcdef";
  std::string out;
  for (auto byte : data) {
    const auto b = std::to_integer<unsigned>(byte);
    out += digits[b >> 4];
    out += digits[b & 15];
  }
  return out;
}
std::vector<std::byte> read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  require(bool(in), "cannot read observed file");
  const std::string content{std::istreambuf_iterator<char>(in), {}};
  return {bytes(content).begin(), bytes(content).end()};
}

// Every wait has a predicate and watchdog. Timeouts detect a broken schedule;
// neither sleeps nor timeout expiry choose a successful experimental outcome.
class Schedule {
public:
  void event(std::string name) {
    std::lock_guard lock(mutex_);
    events.push_back(std::move(name));
  }
  void signal(const std::string& name) {
    std::lock_guard lock(mutex_);
    flags_[name] = true;
    cv_.notify_all();
  }
  void wait(const std::string& name) {
    std::unique_lock lock(mutex_);
    if (!cv_.wait_for(lock, kWatchdog, [&] { return flags_[name]; })) {
      std::cerr << "schedule watchdog: " << name << '\n';
      std::abort();
    }
  }
  void pause(const std::string& name) {
    event("pause." + name);
    signal(name);
    wait("release." + name);
  }
  void release(const std::string& name) {
    event("release." + name);
    signal("release." + name);
  }
  // Read only after both actors have joined.
  std::vector<std::string> events;

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::map<std::string, bool> flags_;
};

// This live authority is deliberately host-owned, not a claimed Axion text-policy
// revocation API. No callback replacement or concurrent Driver mutation occurs.
struct Authority {
  std::atomic<bool> active{true};
  std::mutex owner;
};

class ObservedDevice final : public t81::ternaryos::dev::IBlockDevice {
public:
  explicit ObservedDevice(Schedule& schedule) : schedule_(schedule), blocks_(32) {}
  bool pause_first_mutation{false};
  std::size_t writes{0};
  std::size_t flushes{0};
  t81::ternaryos::dev::BlockDeviceInfo info() const noexcept override { return {}; }
  uint64_t block_count() const noexcept override { return blocks_.size(); }
  bool read_block(uint64_t lba, t81::ternaryos::dev::BlockData& out) const override {
    if (lba >= blocks_.size()) return false;
    out = blocks_[lba];
    return true;
  }
  bool write_block(uint64_t lba, const t81::ternaryos::dev::BlockData& data) override {
    if (lba >= blocks_.size()) return false;
    if (writes == 0) {
      schedule_.event("storage.before_first_mutation");
      if (pause_first_mutation) schedule_.pause("mutation");
    }
    blocks_[lba] = data;
    if (++writes == 1) schedule_.event("storage.first_mutation");
    return true;
  }
  bool flush() override {
    ++flushes;
    return true;
  }
  const auto& blocks() const { return blocks_; }

private:
  Schedule& schedule_;
  std::vector<t81::ternaryos::dev::BlockData> blocks_;
};

// Six endpoints, three matched storage pairs. Conventional endpoints use C++
// containers/files/device writes; T81 endpoints use the actual in-tree drivers.
class Endpoint {
public:
  Endpoint(std::string name, fs::path root, Schedule& schedule)
      : name_(std::move(name)), root_(std::move(root)), schedule_(schedule) {
    fs::create_directories(root_);
    if (name_ == "t81-file") driver = make_persistent_driver(root_);
    if (name_ == "t81-memory") driver = make_in_memory_driver();
    if (name_.ends_with("block")) {
      auto device = std::make_unique<ObservedDevice>(schedule_);
      counter = device.get();
      if (name_ == "t81-block")
        driver = make_block_backed_driver(std::move(device), false);
      else
        conventional_device_ = std::move(device);
    }
  }
  void configure(std::function<AxionVerdict(OpKind, const CanonRef&)> hook) {
    if (driver)
      driver->set_axion_hook(std::move(hook));
    else
      conventional_hook_ = std::move(hook);
  }
  bool write(const std::string& payload) {
    if (driver) {
      auto result = driver->write_object(ObjectType::RawBlock, bytes(payload));
      if (result)
        ref_ = *result;
      else
        require(result.error() == Error::CapabilityError, "unexpected T81 write failure");
      return result.has_value();
    }
    if (!conventional_hook_(OpKind::Write, {}).allow) return false;
    if (name_ == "conventional-file") {
      // One writer, a fresh store, and no competing filesystem actor.
      std::ofstream out(root_ / "result", std::ios::binary);
      out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
      out.close();
      require(bool(out), "conventional file write failed");
    } else if (name_ == "conventional-memory") {
      memory_ = std::vector<std::byte>(bytes(payload).begin(), bytes(payload).end());
    } else {
      t81::ternaryos::dev::BlockData block{};
      std::copy(payload.begin(), payload.end(), block.begin());
      require(counter->write_block(1, block), "conventional device write failed");
      require(counter->flush(), "conventional device flush failed");
    }
    conventional_written_ = true;
    return true;
  }
  std::vector<std::byte> observe() {
    // All actors joined. Inspection is trusted and outside the authority under
    // test; do not record inspection as another protected execution request.
    if (driver) {
      driver->set_axion_hook({});
      if (name_ != "t81-block") {
        const auto future =
            driver->read_object_bytes(CanonRef{CanonHash{t81::hash::hash_string("25001\n")}});
        require(!future && future.error() == Error::NotFound, "future object unexpectedly present");
      }
      CanonRef ref = ref_.value_or(CanonRef{CanonHash{t81::hash::hash_string(kPayload)}});
      if (name_ == "t81-block" && !ref_) return {};
      auto result = driver->read_object_bytes(ref);
      if (!result) {
        require(result.error() == Error::NotFound, "unexpected inspection failure");
        return {};
      }
      return *result;
    }
    if (!conventional_written_) return {};
    if (name_ == "conventional-file") return read_file(root_ / "result");
    if (name_ == "conventional-memory") return memory_;
    const auto& block = counter->blocks()[1];
    return {reinterpret_cast<const std::byte*>(block.data()),
            reinterpret_cast<const std::byte*>(block.data() + kPayload.size())};
  }
  std::map<std::string, std::string> file_snapshot() const {
    std::map<std::string, std::string> out;
    for (const auto& entry : fs::recursive_directory_iterator(root_)) {
      if (entry.is_regular_file())
        out.emplace(fs::relative(entry.path(), root_).generic_string(),
                    hex(read_file(entry.path())));
    }
    return out;
  }
  std::unique_ptr<Driver> driver;
  ObservedDevice* counter{nullptr};

private:
  std::string name_;
  fs::path root_;
  Schedule& schedule_;
  std::function<AxionVerdict(OpKind, const CanonRef&)> conventional_hook_;
  std::unique_ptr<ObservedDevice> conventional_device_;
  std::vector<std::byte> memory_;
  bool conventional_written_{false};
  std::optional<CanonRef> ref_;
};

struct Observation {
  std::vector<std::string> trace;
  bool committed{false};
  bool payload_present{false};
  bool followup_denied{false};
  std::size_t device_writes{0};
  std::size_t device_flushes{0};
  std::size_t policy_calls{0};
  std::map<std::string, std::string> files;
  std::string observed_hex;
  std::string device_data_hex;
  double writer_us{0};
};

Observation execute(const std::string& backend, const std::string& mode,
                    const std::string& scenario, const fs::path& root) {
  Schedule schedule;
  Authority authority;
  Endpoint endpoint(backend, root, schedule);
  reset_axion_trace();
  auto policy =
      make_axion_policy_hook("(policy (tier 1) (require-axion-event (reason \"action=Write\")))");
  const bool serialized = mode == "serialized-owner";
  const bool during = scenario == "during_revocation";
  const bool no_revoke = scenario == "no_revocation";
  const bool after_auth = scenario == "after_authorization" || during;
  bool scheduled = true;
  Observation out;
  if (endpoint.counter) endpoint.counter->pause_first_mutation = scenario == "at_mutation";
  endpoint.configure([&](OpKind kind, const CanonRef& ref) {
    ++out.policy_calls;
    const bool current = authority.active.load();
    const bool policy_allowed = !endpoint.driver || policy(kind, ref).allow;
    const bool allowed = current && policy_allowed;
    if (scheduled) {
      schedule.event(allowed ? "authorization.allow" : "authorization.deny");
      if (after_auth) schedule.pause("authorization");
    }
    return AxionVerdict{allowed, "experiment-owned live authority and static policy"};
  });

  std::thread writer([&] {
    schedule.pause("before_authorization");
    const auto begin = Clock::now();
    std::unique_lock owner(authority.owner, std::defer_lock);
    if (serialized) {
      if (!owner.try_lock()) {
        schedule.event("writer.lock_busy");
        schedule.signal("writer.lock_busy");
        owner.lock();
      }
      schedule.event("writer.lock_acquired");
    }
    out.committed = endpoint.write(kPayload);
    schedule.event(out.committed ? "write.commit" : "write.denied");
    if (scenario == "after_commit") schedule.pause("commit");
    out.writer_us = std::chrono::duration<double, std::micro>(Clock::now() - begin).count();
    // Commit is recorded while the owner lock is held. No trace event after
    // unlocking may race with the revoker's next event.
  });
  std::thread revoker([&] {
    schedule.wait("start.revoker");
    if (no_revoke) return;
    std::unique_lock owner(authority.owner, std::defer_lock);
    if (serialized) {
      if (!owner.try_lock()) {
        schedule.event("revoker.lock_busy");
        schedule.signal("revoker.lock_busy");
        owner.lock();
      }
      schedule.event("revoker.lock_acquired");
    }
    schedule.event("revoke.begin");
    authority.active.store(false);
    schedule.event("revoke.linearized");
    if (during) schedule.pause("revocation");
    schedule.event("revoke.complete");
    if (owner.owns_lock()) owner.unlock();
    schedule.signal("revoker.done");
  });

  schedule.wait("before_authorization");
  if (no_revoke) {
    schedule.event("revocation.not_requested");
    schedule.signal("start.revoker");
    schedule.release("before_authorization");
  } else if (scenario == "before_authorization") {
    schedule.signal("start.revoker");
    schedule.wait("revoker.done");
    schedule.release("before_authorization");
  } else if (during) {
    schedule.signal("start.revoker");
    schedule.wait("revocation");
    schedule.release("before_authorization");
    if (serialized) {
      schedule.wait("writer.lock_busy");
      schedule.release("revocation");
      schedule.wait("revoker.done");
      schedule.wait("authorization");
    } else {
      schedule.wait("authorization");
      schedule.release("revocation");
      schedule.wait("revoker.done");
    }
    schedule.release("authorization");
  } else {
    const std::string checkpoint = scenario == "after_authorization" ? "authorization"
                                   : scenario == "after_commit"      ? "commit"
                                                                     : "mutation";
    schedule.release("before_authorization");
    schedule.wait(checkpoint);
    schedule.signal("start.revoker");
    schedule.wait(serialized ? "revoker.lock_busy" : "revoker.done");
    schedule.release(checkpoint);
  }
  writer.join();
  revoker.join();
  // A future new payload must be denied once revoke() has completed, even where
  // an earlier snapshot-authorized operation was permitted to finish.
  scheduled = false;
  if (!no_revoke) {
    const auto before = endpoint.file_snapshot();
    const auto writes = endpoint.counter ? endpoint.counter->writes : 0;
    out.followup_denied = !endpoint.write("25001\n");
    require(endpoint.file_snapshot() == before, "follow-up changed filesystem");
    require(!endpoint.counter || writes == endpoint.counter->writes, "follow-up changed device");
    schedule.event(out.followup_denied ? "followup.denied" : "followup.committed");
  }
  const auto content = endpoint.observe();
  out.payload_present = !content.empty();
  out.observed_hex = hex(content);
  require(!out.payload_present ||
              content == std::vector<std::byte>(bytes(kPayload).begin(), bytes(kPayload).end()),
          "observed wrong result bytes");
  out.files = endpoint.file_snapshot();
  if (endpoint.counter) {
    const auto& block = endpoint.counter->blocks()[1];
    out.device_data_hex = hex(std::as_bytes(std::span(block)));
    out.device_writes = endpoint.counter->writes;
    out.device_flushes = endpoint.counter->flushes;
  }
  out.trace = std::move(schedule.events);
  require(out.committed == out.payload_present, "returned status disagrees with storage");
  require(out.committed || (out.files.empty() && out.device_writes == 0),
          "denial had a storage effect");
  return out;
}

void json_strings(std::ostream& out, const std::vector<std::string>& strings) {
  out << '[';
  bool first = true;
  for (const auto& string : strings) {
    if (!first) out << ',';
    first = false;
    out << '"' << string << '"';  // Fixed event names, not untrusted input.
  }
  out << ']';
}

int main(int argc, char** argv) {
  try {
    if (argc != 3) throw std::runtime_error("usage: runner fresh-output-dir repetitions");
    const fs::path root = fs::absolute(argv[1]);
    const int repetitions = std::stoi(argv[2]);
    require(repetitions >= 1 && repetitions <= 1000, "repetitions out of range");
    require(!fs::exists(root), "output directory must be fresh");
    fs::create_directories(root);
    std::ofstream result(root / "results.json");
    result << "{\"version\":1,\"base_commit\":\"dec74cfa4d7be321e0c28adb276a8dc2d278c502\","
              "\"payload_hex\":\"32353030300a\",\"owner_mutex_bytes\":"
           << sizeof(std::mutex) << ",\"authority_bytes\":" << sizeof(Authority) << ",\"rows\":[";
    bool first_row = true;
    for (const std::string backend : {"conventional-file", "t81-file", "conventional-memory",
                                      "t81-memory", "conventional-block", "t81-block"}) {
      for (const std::string mode : {"snapshot", "serialized-owner"}) {
        for (const std::string scenario :
             {"no_revocation", "before_authorization", "after_authorization", "during_revocation",
              "after_commit", "at_mutation"}) {
          if (scenario == "at_mutation" && !backend.ends_with("block")) continue;
          Observation first;
          std::vector<double> times;
          for (int repetition = 0; repetition < repetitions; ++repetition) {
            auto observed = execute(
                backend, mode, scenario,
                root / (backend + "-" + mode + "-" + scenario + "-" + std::to_string(repetition)));
            if (repetition == 0)
              first = observed;
            else
              require(observed.trace == first.trace && observed.committed == first.committed &&
                          observed.files == first.files &&
                          observed.device_writes == first.device_writes &&
                          observed.device_flushes == first.device_flushes &&
                          observed.policy_calls == first.policy_calls &&
                          observed.observed_hex == first.observed_hex &&
                          observed.device_data_hex == first.device_data_hex &&
                          observed.followup_denied == first.followup_denied,
                      "replayed schedule or state mismatch");
            times.push_back(observed.writer_us);
          }
          if (!first_row) result << ',';
          first_row = false;
          result << "{\"backend\":\"" << backend << "\",\"mode\":\"" << mode << "\",\"scenario\":\""
                 << scenario << "\",\"repetitions\":" << repetitions
                 << ",\"committed\":" << (first.committed ? "true" : "false")
                 << ",\"payload_present\":" << (first.payload_present ? "true" : "false")
                 << ",\"followup_denied\":" << (first.followup_denied ? "true" : "false")
                 << ",\"device_writes\":" << first.device_writes << ",\"observed_hex\":\""
                 << first.observed_hex << "\",\"device_data_hex\":\"" << first.device_data_hex
                 << "\",\"device_flushes\":" << first.device_flushes
                 << ",\"policy_calls\":" << first.policy_calls << ",\"trace\":";
          json_strings(result, first.trace);
          result << ",\"files\":{";
          bool first_file = true;
          for (const auto& [name, content] : first.files) {
            if (!first_file) result << ',';
            first_file = false;
            result << '"' << name << "\":\"" << content << '"';
          }
          result << "},\"writer_us\":[";
          for (std::size_t i = 0; i < times.size(); ++i) {
            if (i) result << ',';
            result << times[i];
          }
          result << "]}";
        }
      }
    }
    result << "]}\n";
    result.close();
    require(bool(result), "evidence write failed");
    std::cout << (root / "results.json").string() << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
