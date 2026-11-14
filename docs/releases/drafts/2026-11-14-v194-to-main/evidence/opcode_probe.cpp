#include "t81/isa/opcodes.hpp"
#include <iostream>
int main() {
 using t81::tisc::Opcode;
#define SHOW(x) std::cout << #x << "=" << static_cast<unsigned>(Opcode::x) << "\n"
 SHOW(Int2BigInt); SHOW(VLoad); SHOW(VStore); SHOW(VAdd); SHOW(VFma);
 SHOW(ReadIsaVersion); SHOW(GcSafepoint); SHOW(AgentInvoke);
 SHOW(TWMATMUL); SHOW(FFICall); SHOW(TOR_SWAR);
}
