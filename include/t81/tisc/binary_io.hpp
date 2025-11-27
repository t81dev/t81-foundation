#ifndef T81_TISC_BINARY_IO_HPP
#define T81_TISC_BINARY_IO_HPP

#include "t81/tisc/program.hpp"
#include <string>

namespace t81 {
namespace tisc {

void save_program(const Program& program, const std::string& path);
Program load_program(const std::string& path);

} // namespace tisc
} // namespace t81

#endif // T81_TISC_BINARY_IO_HPP
