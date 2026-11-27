#include "t81/isa/binary_io.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv) {
 if(argc!=3)return 2;
 const std::string mode=argv[1],stem=argv[2];
 if(mode=="write") {
  t81::tisc::Program p;p.insns.push_back({t81::tisc::Opcode::VLoad,1,2,3});
  t81::tisc::save_program(p,stem+".tisc");
  auto raw=t81::tisc::encode(p);std::ofstream f(stem+".raw13",std::ios::binary);
  f.write(reinterpret_cast<const char*>(raw.data()),raw.size());
  std::cout<<"writer="<<t81::tisc::opcode_name(p.insns[0].opcode)<<" byte="<<int(p.insns[0].opcode)<<"\n";
  return f?0:3;
 }
 if(mode=="read") {
  auto p=t81::tisc::load_program(stem+".tisc");
  if(p.insns.size()!=1)return 4;
  auto &i=p.insns[0];std::cout<<"load_program="<<t81::tisc::opcode_name(i.opcode)<<" byte="<<int(i.opcode)<<" a="<<i.a<<" b="<<i.b<<" c="<<i.c<<"\n";
  std::ifstream f(stem+".raw13",std::ios::binary);std::vector<char> c((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte>b;for(unsigned char v:c)b.push_back(std::byte(v));
  auto r=t81::tisc::decode(b);if(!r||r->insns.size()!=1)return 5;
  std::cout<<"decode_raw13="<<t81::tisc::opcode_name(r->insns[0].opcode)<<" byte="<<int(r->insns[0].opcode)<<"\n";return 0;
 }
 return 6;
}
