#include "test_runtime_check.hpp"

#include "t81/experimental/cog/tier2/reflective.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

t81::cog::v2::ReflectiveFrame make_frame(std::string_view description, std::int64_t delta) {
  t81::cog::v2::ReflectiveFrame frame;
  std::vector<std::int64_t> regs = {1 + delta, 2 + delta, 3 + delta, 4 + delta, 5 + delta};
  frame.capture_state(17, regs, std::string(description));
  frame.add_justification_step("because deterministic", 18, regs);
  frame.trace(19, regs);
  frame.seal();
  return frame;
}

void test_canonical_serialization_stable() {
  auto a = make_frame("reflective-proof", 0);
  auto b = make_frame("reflective-proof", 0);

  const std::string s1 = a.serialize_evidence_canonical();
  const std::string s2 = a.serialize_evidence_canonical();
  const std::string s3 = b.serialize_evidence_canonical();

  T81_TEST_CHECK(s1 == s2);
  T81_TEST_CHECK(s1 == s3);
  T81_TEST_CHECK(s1.find("\"schema\":\"t81.reflective-evidence.v1\"") != std::string::npos);
  T81_TEST_CHECK(s1.find("\"kind\":\"capture\"") != std::string::npos);
  T81_TEST_CHECK(s1.find("\"kind\":\"justify\"") != std::string::npos);
  T81_TEST_CHECK(s1.find("\"kind\":\"trace\"") != std::string::npos);
  T81_TEST_CHECK(s1.find("\"kind\":\"seal\"") != std::string::npos);
}

void test_replay_verification_success() {
  auto expected = make_frame("reflective-proof", 0);
  auto actual = make_frame("reflective-proof", 0);

  std::string reason;
  const bool ok = t81::cog::v2::ReflectiveFrame::verify_evidence_replay(expected, actual, &reason);
  T81_TEST_CHECK(ok);
  T81_TEST_CHECK(reason == "evidence-identical");
}

void test_replay_verification_failure_reason() {
  auto expected = make_frame("reflective-proof", 0);
  auto actual = make_frame("reflective-proof", 7);

  std::string reason;
  const bool ok = t81::cog::v2::ReflectiveFrame::verify_evidence_replay(expected, actual, &reason);
  T81_TEST_CHECK(!ok);
  T81_TEST_CHECK(reason == "hash-mismatch" || reason == "register-snapshot-mismatch" ||
                 reason.find("evidence-entry-mismatch@") == 0);
}

}  // namespace

int main() {
  test_canonical_serialization_stable();
  test_replay_verification_success();
  test_replay_verification_failure_reason();
  return 0;
}
