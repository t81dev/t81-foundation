#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include <iostream>
int main(){
 for(unsigned i=0;i<256;++i){auto n=t81::tisc::opcode_name(static_cast<t81::tisc::Opcode>(i));if(n!="Unknown")std::cout<<i<<"\t"<<n<<"\n";}
 std::cerr<<"sizeof Insn="<<sizeof(t81::tisc::Insn)<<" Program="<<sizeof(t81::tisc::Program)<<" string="<<sizeof(std::string)<<"\n";
}
