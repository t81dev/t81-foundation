#include "t81/cli/driver.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
class StreamCapture {
public:
  StreamCapture(std::istream& in_target, std::streambuf* in_buf, std::ostream& out_target,
                std::streambuf* out_buf)
      : in_stream_(in_target),
        old_in_buf_(in_stream_.rdbuf(in_buf)),
        out_stream_(out_target),
        old_out_buf_(out_stream_.rdbuf(out_buf)) {}

  ~StreamCapture() {
    in_stream_.rdbuf(old_in_buf_);
    out_stream_.rdbuf(old_out_buf_);
  }

private:
  std::istream& in_stream_;
  std::streambuf* old_in_buf_;
  std::ostream& out_stream_;
  std::streambuf* old_out_buf_;
};

void create_dummy_tisc(const std::filesystem::path& path) {
  t81::tisc::Program prog;
  // Add 1 + 2
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 1, 0});  // R1 = 1
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 2, 0});  // R2 = 2
  prog.insns.push_back({t81::tisc::Opcode::Add, 3, 1, 2});      // R3 = R1 + R2
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  t81::tisc::save_program(prog, path.string());
}

void create_memory_tisc(const std::filesystem::path& path) {
  t81::tisc::Program prog;
  // R1 = 100 (addr)
  // R2 = 42 (val)
  // Store [R1] = R2
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 100, 0});
  prog.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 42, 0});
  prog.insns.push_back({t81::tisc::Opcode::Store, 2, 1, 0});
  prog.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  t81::tisc::save_program(prog, path.string());
}

}  // namespace

namespace fs = std::filesystem;

void test_basic() {
  const fs::path tisc_path = fs::temp_directory_path() / "t81_debug_test.tisc";
  create_dummy_tisc(tisc_path);

  std::stringstream input;
  input << "s\n";  // Step LoadImm (PC=0 -> PC=1)
  input << "s\n";  // Step LoadImm (PC=1 -> PC=2)
  input << "s\n";  // Step Add     (PC=2 -> PC=3)
  input << "t\n";  // Inspect all cognitive tiers
  input << "t 2\n";
  input << "r\n";  // Check registers
  input << "l\n";  // List instructions
  input << "q\n";  // Quit

  std::stringstream output;

  {
    StreamCapture capture(std::cin, input.rdbuf(), std::cout, output.rdbuf());
    t81::cli::debug_tisc(tisc_path);
  }

  std::string out_str = output.str();

  assert(out_str.find("T81VM Debugger active") != std::string::npos);
  assert(out_str.find("LoadImm 1, 1, 0") != std::string::npos);
  assert(out_str.find("Add 3, 1, 2") != std::string::npos);
  assert(out_str.find("R3: 3") != std::string::npos);
  assert(out_str.find("-> [   3] Halt") != std::string::npos);
  assert(out_str.find("Cognitive tier state:") != std::string::npos);
  assert(out_str.find("Tier1 symbolic:") != std::string::npos);
  assert(out_str.find("Tier2 reflective:") != std::string::npos);
  assert(out_str.find("Tier3 recursive:") != std::string::npos);
  assert(out_str.find("Tier4 distributed:") != std::string::npos);
  assert(out_str.find("Tier5 infinite:") != std::string::npos);

  fs::remove(tisc_path);
}

void test_watchpoint() {
  const fs::path tisc_path = fs::temp_directory_path() / "t81_debug_mem.tisc";
  create_memory_tisc(tisc_path);

  std::stringstream input;
  input << "w 100\n";
  input << "c\n";  // Should stop when memory[100] changes
  input << "q\n";

  std::stringstream output;
  {
    StreamCapture capture(std::cin, input.rdbuf(), std::cout, output.rdbuf());
    t81::cli::debug_tisc(tisc_path);
  }

  std::string out_str = output.str();

  assert(out_str.find("Watchpoint set at address 100") != std::string::npos);
  assert(out_str.find("Watchpoint hit at address 100") != std::string::npos);
  assert(out_str.find("0 -> 42") != std::string::npos);

  fs::remove(tisc_path);
}

int main() {
  test_basic();
  test_watchpoint();
  std::cerr << "Debugger test passed!\n";
  return 0;
}
