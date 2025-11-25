#ifndef T81_TISC_BINARY_HPP
#define T81_TISC_BINARY_HPP

#include <cstdint>
#include <vector>

namespace t81 {
namespace tisc {

enum class BinaryTag : uint8_t {
    BigInt = 0x01,
    Fraction = 0x02,
    Float = 0x03,
    Matrix = 0x04,
    Vector = 0x05,
    Tensor = 0x06,
    Polynomial = 0x07,
    Graph = 0x08,
    Quaternion = 0x09,
    Opcode = 0x0A,
};

} // namespace tisc
} // namespace t81

#endif // T81_TISC_BINARY_HPP
