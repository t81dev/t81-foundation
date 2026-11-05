// Always-active false-equivalence controls. No production or frozen inputs change.
#include <cstdio>
#include <cstring>
#include <string>
#include "cpa006_evaluator.hpp"
#include "cpa006_program.hpp"
static std::string output;
static void emit(const char* s) { output+=s; }
static bool check(bool ok,const char* message) {
  if(!ok) std::fprintf(stderr,"CPA006 negative check failed: %s\n",message);
  return ok;
}
int main(int argc,char**) {
  unsigned char bytes[sizeof(kCpa006Program)];
  std::memcpy(bytes,kCpa006Program,sizeof(bytes));
  bytes[5]^=1;
  if(!check(cpa006_evaluate(bytes,sizeof(bytes),emit)==Cpa006ArtifactMismatch,"modified instruction byte") ||
     !check(output.empty(),"no state after artifact rejection")) return 1;
  bytes[0]=255;
  if(!check(cpa006_evaluate(bytes,sizeof(bytes),emit)==Cpa006Unsupported,"undefined opcode")) return 1;
  bytes[0]=36;
  if(!check(cpa006_evaluate(bytes,sizeof(bytes),emit)==Cpa006Unsupported,"excluded valid opcode")) return 1;
  if(!check(cpa006_evaluate(kCpa006Program,sizeof(bytes)-1,emit)==Cpa006Malformed,"truncated input") ||
     !check(cpa006_evaluate(nullptr,sizeof(bytes),emit)==Cpa006Malformed,"null buffer") ||
     !check(cpa006_evaluate(kCpa006Program,sizeof(bytes),nullptr)==Cpa006Malformed,"null sink") ||
     !check(cpa006_evaluate(kCpa006Program,sizeof(bytes),emit,65)==Cpa006OutsideProfile,"excessive requested budget") ||
     !check(output.empty(),"rejections emitted no machine state")) return 1;
  const int limited=cpa006_evaluate(kCpa006Program,sizeof(bytes),emit,1);
  if(argc>1) return check(limited==Cpa006Halted,"deliberately wrong Halt expectation") ? 0:1;
  if(!check(limited==Cpa006InstructionLimit,"budget exhaustion distinct from Halt") ||
     !check(output.find("\"sequence\":1")!=std::string::npos,"actual final attempted step")) return 1;
  std::puts("PASS CPA006 admission negatives and instruction limit, active under NDEBUG");
}
