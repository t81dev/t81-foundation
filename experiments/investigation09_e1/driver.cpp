// Experiment adapter only. No compiler or VM implementation changes.
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/vm/vm.hpp"

int main(int argc, char** argv) {
  try {
    if (argc < 3)
      throw std::runtime_error("compile source artifact | run artifact (cases on stdin)");
    if (std::string(argv[1]) == "author") {
      if (argc != 5) throw std::runtime_error("author artifact meet|join native|expanded");
      using O = t81::tisc::Opcode;
      const bool meet = std::string(argv[3]) == "meet";
      if (!meet && std::string(argv[3]) != "join") throw std::runtime_error("bad authored kernel");
      const bool expanded = std::string(argv[4]) == "expanded";
      if (!expanded && std::string(argv[4]) != "native")
        throw std::runtime_error("bad authored variant");
      t81::tisc::Program p;
      auto emit = [&](O op, int a, std::int64_t b = 0, int c = 0) {
        p.insns.push_back({op, a, b, c});
      };
      for (int i = 1; i <= 4; ++i) emit(O::LoadImm, i);
      for (int i = 1; i <= 4; ++i) emit(O::Push, i);
      emit(O::LoadImm, 5, 12);
      emit(O::Call, 0, 5);
      emit(O::Pop, 6);
      emit(O::Halt, 0);
      for (int i = 7; i <= 11; ++i) emit(O::Pop, i);
      if (!expanded)
        emit(meet ? O::TAnd : O::TOr, 12, 10, 9);
      else {
        emit(meet ? O::Less : O::Greater, 13, 10, 9);
        const int branch = static_cast<int>(p.insns.size());
        emit(O::JumpIfZero, 0, 13);
        emit(O::Mov, 12, 10);
        const int jump = static_cast<int>(p.insns.size());
        emit(O::Jump, 0);
        p.insns[branch].a = static_cast<int>(p.insns.size());
        emit(O::Mov, 12, 9);
        p.insns[jump].a = static_cast<int>(p.insns.size());
      }
      emit(O::LoadImm, 14, 1);
      emit(O::Add, 15, 12, 14);
      emit(O::Push, 15);
      emit(O::Push, 7);
      emit(O::Ret, 0);
      p.function_metadata.push_back({"run", false});
      t81::tisc::save_program(p, argv[2]);
      std::cout << "{\"instructions\":" << p.insns.size()
                << ",\"encoded_stream_bytes\":" << t81::tisc::encode(p).size() << "}\n";
      return 0;
    }
    if (std::string(argv[1]) == "compile") {
      if (argc != 4) throw std::runtime_error("compile source artifact");
      std::ifstream in(argv[2]);
      if (!in) throw std::runtime_error("source open failed");
      std::string source((std::istreambuf_iterator<char>(in)), {});
      t81::frontend::Lexer lexer(source);
      t81::frontend::Parser parser(lexer);
      auto ast = parser.parse();
      if (parser.had_error()) throw std::runtime_error("parse failed");
      t81::frontend::SemanticAnalyzer semantic(ast, argv[2]);
      semantic.analyze();
      if (semantic.had_error()) {
        for (const auto& d : semantic.diagnostics())
          std::cerr << d.line << ":" << d.column << ": " << d.message << "\n";
        throw std::runtime_error("semantic analysis failed");
      }
      t81::frontend::IRGenerator generator;
      generator.attach_semantic_analyzer(&semantic);
      auto ir = generator.generate(ast);
      auto program = t81::tisc::BinaryEmitter{}.emit(ir);
      // Same function metadata collection as the production CLI for these
      // function-only sources (no loops, matches, enums or custom policies).
      for (const auto& stmt : ast) {
        if (auto f = dynamic_cast<t81::frontend::FunctionStmt*>(stmt.get()))
          program.function_metadata.push_back({std::string(f->name.lexeme), f->is_axion_verify});
      }
      t81::tisc::save_program(program, argv[3]);
      std::cout << "{\"instructions\":" << program.insns.size()
                << ",\"encoded_stream_bytes\":" << t81::tisc::encode(program).size()
                << ",\"insns\":[";
      for (size_t i = 0; i < program.insns.size(); ++i) {
        if (i) std::cout << ',';
        const auto& x = program.insns[i];
        std::cout << '[' << int(x.opcode) << ',' << x.a << ',' << x.b << ',' << x.c << ']';
      }
      std::cout << "]}\n";
      return 0;
    }
    if (std::string(argv[1]) != "run") throw std::runtime_error("unknown command");
    auto program = t81::tisc::load_program(argv[2]);
    using O = t81::tisc::Opcode;
    // Frozen adapter ABI: four script initializers then host inputs. No bytecode
    // patching: run the emitted initializer instructions, then seed their regs.
    if (program.insns.size() < 6) throw std::runtime_error("bad ABI");
    for (int i = 0; i < 4; ++i)
      if (program.insns[i].opcode != O::LoadImm)
        throw std::runtime_error("initializer ABI mismatch");
    size_t halt = 0;
    while (halt < program.insns.size() && program.insns[halt].opcode != O::Halt) ++halt;
    if (!halt || program.insns[halt - 1].opcode != O::Pop)
      throw std::runtime_error("return ABI mismatch");
    const int result_reg = program.insns[halt - 1].a;
    int k, a, b, c;
    while (std::cin >> k >> a >> b >> c) {
      auto vm = t81::vm::make_interpreter_vm();
      vm->load_program(program);
      for (int i = 0; i < 4; ++i) {
        auto r = vm->step();
        if (!r) throw std::runtime_error("initializer fault");
      }
      const int inputs[] = {k, a, b, c};
      for (int i = 0; i < 4; ++i)
        vm->set_register(program.insns[i].a, inputs[i],
                         vm->state().contexts[0].register_tags[program.insns[i].a]);
      std::cout << "{\"inputs\":[" << k << ',' << a << ',' << b << ',' << c << "],\"steps\":[";
      bool first = true;
      int steps = 4;
      int fault = 0;
      for (; steps < 4096 && !vm->state().halted;) {
        auto pc = vm->state().contexts[0].pc;
        if (pc >= program.insns.size()) throw std::runtime_error("PC outside artifact");
        const auto insn = program.insns[pc];
        auto r = vm->step();
        ++steps;
        if (!first) std::cout << ',';
        first = false;
        const auto& ctx = vm->state().contexts[0];
        std::cout << "{\"pc\":" << pc << ",\"op\":" << int(insn.opcode) << ",\"a\":" << insn.a
                  << ",\"value\":" << (insn.a >= 0 && insn.a < 243 ? ctx.registers[insn.a] : 0)
                  << ",\"flags\":[" << ctx.flags.zero << ',' << ctx.flags.negative << ','
                  << ctx.flags.positive << "]}";
        if (!r) {
          fault = int(r.error()) + 1;
          break;
        }
      }
      const auto& s = vm->state();
      const auto& ctx = s.contexts[0];
      std::cout << "],\"fault\":" << fault << ",\"halted\":" << (s.halted ? "true" : "false")
                << ",\"result\":" << ctx.registers[result_reg]
                << ",\"dynamic_instructions\":" << steps
                << ",\"register_payload_bytes\":" << sizeof(ctx.registers)
                << ",\"register_tag_bytes\":" << sizeof(ctx.register_tags)
                << ",\"memory_payload_bytes\":" << s.memory.size() * sizeof(s.memory[0])
                << ",\"memory_tag_bytes\":" << s.memory_tags.size() * sizeof(s.memory_tags[0])
                << ",\"trace_entries\":" << s.trace.size()
                << ",\"policy_events\":" << s.axion_log.size()
                << ",\"trace_storage_bytes\":" << s.trace.capacity() * sizeof(t81::vm::TraceEntry)
                << ",\"policy_storage_bytes\":"
                << s.axion_log.capacity() * sizeof(t81::vm::AxionEvent)
                << ",\"state_object_bytes\":" << sizeof(s)
                << ",\"context_object_bytes\":" << sizeof(ctx)
                << ",\"flags_bytes\":" << sizeof(ctx.flags)
                << ",\"instruction_object_bytes\":" << sizeof(t81::tisc::Insn)
                << ",\"printed_effects\":" << s.printed_output.size() << "}\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "i09: " << e.what() << '\n';
    return 1;
  }
}
