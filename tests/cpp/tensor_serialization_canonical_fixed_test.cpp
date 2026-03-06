#include <cmath>
#include <cstdint>
#include <sstream>
#include <vector>

#include "t81/tensor.hpp"
#include "test_runtime_check.hpp"

namespace {

std::string serialize_tensor(const t81::T729DynamicTensor& tensor) {
  std::ostringstream out(std::ios::binary);
  tensor.serialize(out);
  return out.str();
}

t81::T729DynamicTensor deserialize_tensor(const std::string& bytes) {
  std::istringstream in(bytes, std::ios::binary);
  t81::T729DynamicTensor tensor;
  tensor.deserialize(in);
  return tensor;
}

std::string serialize_legacy_raw_tensor(const std::vector<int>& shape, const std::vector<float>& data) {
  std::ostringstream out(std::ios::binary);
  uint64_t shape_size = static_cast<uint64_t>(shape.size());
  out.write(reinterpret_cast<const char*>(&shape_size), sizeof(shape_size));
  out.write(reinterpret_cast<const char*>(shape.data()),
            static_cast<std::streamsize>(shape.size() * sizeof(int)));
  uint64_t data_size = static_cast<uint64_t>(data.size());
  out.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
  out.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size() * sizeof(float)));
  return out.str();
}

}  // namespace

int main() {
  const t81::T729DynamicTensor finite({3}, {0.5f, -1.25f, 2.0f});
  T81_TEST_CHECK(finite.has_canonical_fixed_data());

  const auto encoded = serialize_tensor(finite);
  const auto roundtrip = deserialize_tensor(encoded);
  T81_TEST_CHECK(roundtrip.shape() == finite.shape());
  T81_TEST_CHECK(roundtrip.numeric_class() == t81::TensorNumericClass::HostFloat);
  T81_TEST_CHECK(roundtrip.has_canonical_fixed_data());
  T81_TEST_CHECK(roundtrip.canonical_fixed_authoritative());
  T81_TEST_CHECK(roundtrip.data().size() == finite.data().size());
  for (std::size_t i = 0; i < finite.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(roundtrip.data()[i] - finite.data()[i]) < 1e-4f);
  }

  const auto reencoded = serialize_tensor(roundtrip);
  T81_TEST_CHECK(reencoded == encoded);

  const t81::T729DynamicTensor exact({3}, {-1.0f, 0.0f, 1.0f});
  const auto exact_encoded = serialize_tensor(exact);
  const auto exact_roundtrip = deserialize_tensor(exact_encoded);
  T81_TEST_CHECK(exact_roundtrip.numeric_class() == t81::TensorNumericClass::ExactTrit);
  T81_TEST_CHECK(exact_roundtrip.canonical_fixed_authoritative());
  T81_TEST_CHECK(exact_roundtrip.data() == exact.data());

  const auto legacy_bytes = serialize_legacy_raw_tensor({2}, {1.0f, 2.5f});
  const auto legacy = deserialize_tensor(legacy_bytes);
  T81_TEST_CHECK(legacy.shape() == std::vector<int>({2}));
  T81_TEST_CHECK(legacy.data().size() == 2);
  T81_TEST_CHECK(legacy.data()[0] == 1.0f);
  T81_TEST_CHECK(legacy.data()[1] == 2.5f);
  T81_TEST_CHECK(legacy.has_canonical_fixed_data());
  T81_TEST_CHECK(legacy.canonical_fixed_authoritative());

  return 0;
}
