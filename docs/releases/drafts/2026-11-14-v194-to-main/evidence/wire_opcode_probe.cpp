#include "t81/isa/encoding.hpp"
#include <iostream>
int main() {
 t81::tisc::Program p;
 t81::tisc::Insn instruction;
 instruction.opcode = t81::tisc::Opcode::VLoad;
 p.insns.push_back(instruction);
 auto bytes = t81::tisc::encode(p);
 std::cout << "VLoad record size=" << bytes.size() << " first-byte=" << std::to_integer<unsigned>(bytes.at(0)) << "\n";
}
