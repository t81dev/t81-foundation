#ifndef T81_TISC_BINARY_EMITTER_HPP
#define T81_TISC_BINARY_EMITTER_HPP

#include "t81/tisc/ir.hpp"
#include "t81/tisc/program.hpp"
#include <vector>
#include <cstdint>

namespace t81 {
namespace tisc {

class BinaryEmitter {
public:
    Program emit(const ir::IntermediateProgram& program);
};

} // namespace tisc
} // namespace t81

#endif // T81_TISC_BINARY_EMITTER_HPP
