#include "kernels.hpp"
#include "t81/ternary/arith.hpp"
void base01_encode_batch(const std::vector<int64_t>& inputs,
                         std::vector<std::vector<t81::Trit>>& outputs) {
  for(size_t i=0;i<inputs.size();++i) outputs[i]=t81::ternary::encode_i64(inputs[i]);
}
void base01_decode_batch(const std::vector<std::vector<t81::Trit>>& inputs,
                         std::vector<int64_t>& outputs) {
  for(size_t i=0;i<inputs.size();++i) outputs[i]=t81::ternary::decode_i64(inputs[i]);
}
