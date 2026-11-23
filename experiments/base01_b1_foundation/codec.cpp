#include "codec.hpp"
// External, non-LTO definitions. All outputs escape and remain live for verification.
void encode_all(const std::vector<int64_t>& inputs, std::vector<Digits>& outputs) {
  for (size_t i = 0; i < inputs.size(); ++i) outputs[i] = t81::ternary::encode_i64(inputs[i]);
}
void decode_all(const std::vector<Digits>& inputs, std::vector<int64_t>& outputs) {
  for (size_t i = 0; i < inputs.size(); ++i) outputs[i] = t81::ternary::decode_i64(inputs[i]);
}
