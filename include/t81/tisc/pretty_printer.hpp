#ifndef T81_TISC_PRETTY_PRINTER_HPP
#define T81_TISC_PRETTY_PRINTER_HPP

#include "t81/tisc/ir.hpp"
#include <string>

namespace t81 {
namespace tisc {

std::string pretty_print(const Program& program);

} // namespace tisc
} // namespace t81

#endif // T81_TISC_PRETTY_PRINTER_HPP
