// Lane A implementation by Codex. Existing audited test builders/oracle are reused,
// not another baseline runner. No performance clock or measurement path in B0.
#define main base01_audited_vm_correctness_main
#include "vm_workload_determinism_tiers_test.cpp"
#undef main
#include "t81/ternary/arith.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/tracing/canonhash.hpp"
#include <limits>
#include "kernels.hpp"
#include "measurement.hpp"
#include <stdexcept>

static void require(bool yes, const char* message) {
  if (!yes) throw std::runtime_error(message);
}

// Same audited signature contract, applied to the actual runner-owned machine.
static uint64_t state_signature(const t81::vm::State& state) {
  uint64_t sig=1469598103934665603ULL;
  for(size_t i=0;i<48;++i) {
    sig=mix(sig,static_cast<uint64_t>(state.contexts[0].registers[i]));
    sig=mix(sig,static_cast<uint64_t>(state.contexts[0].register_tags[i]));
  }
  sig=mix(sig,state.contexts[0].pc);
  sig=mix(sig,state.contexts[0].sp);
  sig=mix(sig,state.trace.size());
  sig=mix(sig,state.axion_log.size());
  for(const auto& entry : state.trace) {
    sig=mix(sig,entry.pc);
    sig=mix(sig,static_cast<uint64_t>(entry.opcode));
    sig=mix(sig,entry.trap.has_value()?static_cast<uint64_t>(*entry.trap):0ULL);
  }
  for(const auto& line : state.printed_output)
    for(unsigned char c : line) sig=mix(sig,c);
  for(const auto& event : state.axion_log) {
    sig=mix(sig,static_cast<uint64_t>(event.opcode));
    sig=mix(sig,static_cast<uint64_t>(event.tag));
    sig=mix(sig,static_cast<uint64_t>(event.value));
    for(unsigned char c : event.verdict.reason) sig=mix(sig,c);
  }
  return sig;
}

int main(int argc, char** argv) {
  if (argc != 2 || std::string(argv[1]) != "--validate-only") {
    std::cerr << "BASE-01 Phase B1 not authorized; only --validate-only is available\n";
    return 2;
  }
  try {
    using namespace t81::ternary;
    // Exact portable input generation: unsigned LCG; map low 63 bits to magnitude.
    std::vector<int64_t> values{INT64_MIN, INT64_MIN+1, INT64_MAX-1, INT64_MAX, 0, 1, -1};
    uint64_t seed = 0x424153453031ULL;
    for (unsigned i=0; i<1024; ++i) {
      seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
      auto magnitude = static_cast<int64_t>(seed & 0x7fffffffffffffffULL);
      values.push_back((seed >> 63) ? -magnitude : magnitude);
    }
    // Exercise prepared retention kernels once for correctness; never read a clock.
    std::vector<std::vector<t81::Trit>> encoded(values.size());
    std::vector<int64_t> decoded(values.size());
    base01_encode_batch(values,encoded);
    base01_decode_batch(encoded,decoded);
    require(values==decoded,"batch codec roundtrip failed");
    std::cout << "{\"codec\":[";
    bool first = true;
    for (auto x : values) {
      auto digits = encode_i64(x);
      auto back = decode_i64(digits);
      require(back == x, "codec roundtrip failed");
      require(digits.size()==1 || digits.back()!=t81::Trit::Zero, "noncanonical codec output");
      if (!first) std::cout << ',';
      first=false;
      std::cout << "[" << x << "," << back << ",[";
      for (size_t j=0; j<digits.size(); ++j) {
        if (j) std::cout << ',';
        std::cout << t81::trit_to_int(digits[j]);
      }
      std::cout << "]]";
    }
    std::cout << "],\"vm\":[";
    const std::vector<std::pair<std::string,t81::tisc::Program>> programs{
      {"workload",workload_program()}, {"micro",micro_program()}, {"meso",meso_program()},
      {"mixed",mixed_program()}, {"policy-heavy",policy_heavy_program()},
      {"tensor-access",tensor_access_program()}};
    first=true;
    for (const auto& [name,p] : programs) {
      bool ok=false;
      auto reference=signature_for_program(p,&ok);
      require(ok,"audited VM oracle failed");
      auto machine=t81::vm::make_interpreter_vm();
      machine->load_program(p); // setup outside any future timing interval
      auto result=machine->run_to_halt(5000);
      require(result.has_value(),"VM failed");
      const auto& state=machine->state();
      require(state_signature(state)==reference,"runner state differs from audited oracle");
      // Repeat using audited signature oracle, not a new golden learned as truth.
      bool repeated_ok=false;
      auto repeated=signature_for_program(p,&repeated_ok);
      require(repeated_ok && reference==repeated,"VM signature drift");
      if (!first) std::cout << ',';
      first=false;
      std::cout << "{\"tier\":\"" << name << "\",\"signature\":" << reference
                << ",\"trace_entries\":" << state.trace.size()
                << ",\"instruction_ticks_r75\":" << state.contexts[0].registers[75]
                << ",\"inputs\":[";
      bool insn_first=true;
      for (const auto& insn : p.insns) {
        if (!insn_first) std::cout << ',';
        insn_first=false;
        std::cout << '[' << static_cast<int>(insn.opcode) << ',' << insn.a << ',' << insn.b
                  << ',' << insn.c << ',' << static_cast<int>(insn.literal_kind) << ']';
      }
      std::cout << "],\"policy\":\"" << p.axion_policy_text << "\",\"registers\":[";
      for (size_t i=0;i<48;++i) {
        if(i) std::cout << ',';
        std::cout << state.contexts[0].registers[i];
      }
      std::cout << "],\"tags\":[";
      for (size_t i=0;i<48;++i) {
        if(i) std::cout << ',';
        std::cout << static_cast<int>(state.contexts[0].register_tags[i]);
      }
      std::cout << "],\"floats\":[";
      for(size_t i=0;i<state.floats.size();++i) {
        if(i) std::cout << ',';
        std::cout << state.floats[i];
      }
      std::cout << "]}";
    }
    std::cout << "],\"canonfs\":[";
    first=true;
    for (size_t size : {4096U,65536U,1048576U}) {
      using namespace t81::canonfs;
      std::vector<std::byte> payload(size,std::byte{0x42});
      auto driver=make_in_memory_driver();
      auto ref=driver->write_object(ObjectType::RawBlock,payload);
      require(ref.has_value(),"CanonFS write failed");
      auto expected=t81::hash::hash_bytes(payload);
      require(ref->hash.h==expected,"CanonFS content identity mismatch");
      auto again=driver->write_object(ObjectType::RawBlock,payload);
      require(again.has_value() && again->hash==ref->hash,"CanonFS identity drift");
      auto back=driver->read_object_bytes(*ref);
      require(back.has_value() && *back==payload,"CanonFS read mismatch");
      if(!first) std::cout << ',';
      first=false;
      std::cout << "{\"bytes\":" << size << ",\"fill\":66,\"verified\":true}";
    }
    std::cout << "]}\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
