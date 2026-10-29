// Opt-in research instrumentation of existing production compiler/VM operations.
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include "t81/cli/driver.hpp"
#include "t81/vm/decoder.hpp"
#include "t81/weights.hpp"
void array(const auto& values) {
  std::cout << '[';
  bool first = true;
  for (auto value : values) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << value;
  }
  std::cout << ']';
}
int main(int argc, char** argv) {
  try {
    if (argc != 4) throw std::runtime_error("usage: probe model tokenizer prompt");
    auto model = std::make_shared<t81::weights::ModelFile>(t81::weights::load_t81w(argv[1]));
    auto probe = t81::vm::run_native_vm_probe(t81::vm::make_initial_probe_request(
        model, t81::vm::detect_architecture_profile(*model), argv[3], argv[2]));
    if (!probe.ok) throw std::runtime_error("probe failed: " + probe.trap);
    std::ostringstream source;
    source << "@ternary_inference\n@tier(2)\nfn main() -> i32 {\n"
           << "let table: i32 = std.tensor.load(\"model.embed_tokens.weight\");\n"
           << "let emb0: Tensor = std.tnn.embed(table, " << probe.token_index << ");\n";
    std::string input = "emb0";
    int i = 0;
    for (int token : probe.context_token_indices) {
      source << "let ctx" << i << ": Tensor = std.tnn.embed(table, " << token << ");\n"
             << "let emb" << i + 1 << ": Tensor = std.tensor.vec_add(" << input << ", ctx" << i
             << ");\n";
      input = "emb" + std::to_string(++i);
    }

    std::vector<std::string> names{"embedding"};
    for (int layer = 0; layer < 2; ++layer) {
      for (const std::string name : {"q", "k", "v"}) {
        const auto id = name + std::to_string(layer);
        source << "let " << id << "w: i32 = std.tensor.load(\"model.layers." << layer
               << ".self_attn." << name << "_proj.weight\");\n"
               << "let " << id << ": Tensor = std.tensor.matmul(" << input << ", " << id << "w);\n";
        names.push_back(id);
      }
      input = "attn" + std::to_string(layer);
      source << "let " << input << ": Tensor = std.tensor.attention(q" << layer << ", k" << layer
             << ", v" << layer << ");\n";
      names.push_back(input);
    }
    source << "return 0;\n}\n";
    auto program =
        t81::cli::build_program_from_source(source.str(), "<investigation08 stages>", model);
    if (!program) throw std::runtime_error("stage compile failed");
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(*program);
    std::map<std::string, std::vector<float>> stages;
    std::size_t captured = 1;
    for (int steps = 0; steps < 10000 && !vm->state().halted; ++steps) {
      const auto pc = vm->state().contexts[0].pc;
      if (pc >= program->insns.size()) break;
      const auto insn = program->insns[pc];
      if (!vm->step()) throw std::runtime_error("stage execution trapped");
      using t81::tisc::Opcode;
      const auto& state = vm->state();
      if (insn.opcode == Opcode::TWEMBED || insn.opcode == Opcode::TVecAdd ||
          insn.opcode == Opcode::TWMATMUL || insn.opcode == Opcode::TATTN) {
        const auto handle = state.contexts[0].registers[insn.a];
        const auto values = state.tensors.at(handle - 1).value().snapshot_values();
        if (insn.opcode == Opcode::TWEMBED || insn.opcode == Opcode::TVecAdd)
          stages["embedding"] = values;
        else {
          if (captured >= names.size()) throw std::runtime_error("unexpected stage instruction");
          stages[names[captured++]] = values;
        }
      }
    }
    if (!vm->state().halted || captured != names.size())
      throw std::runtime_error("incomplete stage trace");
    std::cout << "{\"stages\":{";
    bool first = true;
    for (const auto& [name, values] : stages) {
      if (!first) std::cout << ',';
      first = false;
      std::cout << '"' << name << "\":";
      array(values);
    }
    std::cout << "},\"sampled_token_ids\":";
    array(probe.sampled_token_ids);
    std::cout << ",\"sampled_token_scores\":";
    array(probe.sampled_token_scores);
    std::cout << ",\"prompt_token_history\":";
    array(probe.prompt_token_ids);
    std::cout << ",\"selected_token_id\":" << *probe.selected_token_id << "}\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
