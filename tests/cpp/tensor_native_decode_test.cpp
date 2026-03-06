#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <vector>

#include "test_runtime_check.hpp"

#include "t81/tensor/native.hpp"

namespace {

t81::weights::NativeTensor balanced_zero_square() {
  t81::weights::NativeTensor tensor;
  tensor.format = t81::weights::NativeFormat::BalancedTernary;
  tensor.shape = {2, 2};
  tensor.trits = 4;
  tensor.data = {40};  // [-1,-1,-1,-1]
  return tensor;
}

t81::weights::NativeTensor canonical_fixed_native(const t81::T729DynamicTensor& tensor) {
  T81_TEST_CHECK(tensor.has_canonical_fixed_data());

  std::ostringstream payload(std::ios::binary);
  for (const auto& value : tensor.canonical_fixed_data()) {
    value.v.serialize(payload);
  }
  const std::string bytes = payload.str();
  T81_TEST_CHECK((bytes.size() % sizeof(uint64_t)) == 0U);

  t81::weights::NativeTensor native;
  native.format = t81::weights::NativeFormat::CanonicalFixed;
  native.shape.reserve(tensor.shape().size());
  for (int dim : tensor.shape()) {
    native.shape.push_back(static_cast<uint64_t>(dim));
  }
  native.trits = static_cast<uint64_t>(tensor.size()) *
                 static_cast<uint64_t>(t81::core::detail::DFixed::Storage::kNumTrits);
  native.data.resize(bytes.size() / sizeof(uint64_t));
  std::memcpy(native.data.data(), bytes.data(), bytes.size());
  return native;
}

}  // namespace

int main() {
  {
    auto decoded =
        t81::tensor_native::decode(balanced_zero_square(), t81::tensor_native::DecodeMode::StrictCanonical);
    T81_TEST_CHECK(decoded.has_value());
    const auto& tensor = *decoded;
    T81_TEST_CHECK(tensor.shape() == std::vector<int>({2, 2}));
    T81_TEST_CHECK(tensor.numeric_class() == t81::TensorNumericClass::ExactTrit);
    T81_TEST_CHECK(tensor.canonical_fixed_authoritative());
    T81_TEST_CHECK(tensor.data() == std::vector<float>({-1.0f, -1.0f, -1.0f, -1.0f}));
  }

  {
    const t81::T729DynamicTensor exact({3}, {-1.0f, 0.0f, 1.0f});
    auto decoded = t81::tensor_native::decode(canonical_fixed_native(exact),
                                              t81::tensor_native::DecodeMode::Lenient);
    T81_TEST_CHECK(decoded.has_value());
    const auto& tensor = *decoded;
    T81_TEST_CHECK(tensor.numeric_class() == t81::TensorNumericClass::ExactTrit);
    T81_TEST_CHECK(tensor.canonical_fixed_authoritative());
    T81_TEST_CHECK(tensor.data() == exact.data());
  }

  {
    const t81::T729DynamicTensor fractional({2}, {0.5f, -1.25f});
    auto decoded = t81::tensor_native::decode(canonical_fixed_native(fractional),
                                              t81::tensor_native::DecodeMode::Lenient);
    T81_TEST_CHECK(decoded.has_value());
    const auto& tensor = *decoded;
    T81_TEST_CHECK(tensor.numeric_class() == t81::TensorNumericClass::HostFloat);
    T81_TEST_CHECK(tensor.canonical_fixed_authoritative());
    T81_TEST_CHECK(std::fabs(tensor.data()[0] - 0.5f) < 1e-4f);
    T81_TEST_CHECK(std::fabs(tensor.data()[1] + 1.25f) < 1e-4f);
  }

  return 0;
}
