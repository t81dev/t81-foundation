#include "adaptive_experiment.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace t81::research::h04_r02;

struct WorkloadSummary {
  std::string name;
  TraceLedger fb;
  TraceLedger ab;
  TraceLedger ft;
  TraceLedger at;
  double payload_break_even_c3_c2;
  double total_break_even_c3_c2;
};

std::string escape_json(const std::string& s) {
  std::ostringstream o;
  for (char c : s) {
    if (c == '"')
      o << "\\\"";
    else if (c == '\\')
      o << "\\\\";
    else if (c == '\n')
      o << "\\n";
    else
      o << c;
  }
  return o.str();
}

void print_ledger_json(std::ostream& os, const TraceLedger& l, int indent_spaces = 6) {
  std::string ind(indent_spaces, ' ');
  os << "{\n";
  os << ind << "  \"total_elements\": " << l.total_elements << ",\n";
  os << ind << "  \"symbol_slots\": " << l.symbol_slots << ",\n";
  os << ind << "  \"weighted_symbol_cost\": " << l.weighted_symbol_cost << ",\n";
  os << ind << "  \"metadata_cost\": " << l.metadata_cost << ",\n";
  os << ind << "  \"transition_count\": " << l.transition_count << ",\n";
  os << ind << "  \"transition_level_delta\": " << l.transition_level_delta << ",\n";
  os << ind << "  \"transition_cost\": " << l.transition_cost << ",\n";
  os << ind << "  \"selector_checks\": " << l.selector_checks << ",\n";
  os << ind << "  \"tier_sequence\": [";
  for (size_t i = 0; i < l.tier_sequence.size(); ++i) {
    if (i) os << ", ";
    os << l.tier_sequence[i];
  }
  os << "],\n";
  os << ind << "  \"total_cost\": " << l.total_cost << "\n";
  os << ind << "}";
}

int main(int argc, char** argv) {
  try {
    std::locale::global(std::locale::classic());
    std::string json_out_path;
    CostParameters params;
    bool custom = false;
    std::vector<int64_t> custom_trace;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (i + 1 >= argc) throw std::invalid_argument("missing option value");
      std::string value = argv[++i];
      auto number = [&]() {
        size_t used = 0;
        double result = std::stod(value, &used);
        if (used != value.size()) throw std::invalid_argument("invalid number");
        return result;
      };
      auto integer = [&]() {
        size_t used = 0;
        int result = std::stoi(value, &used);
        if (used != value.size()) throw std::invalid_argument("invalid integer");
        return result;
      };
      if (arg == "--json-out")
        json_out_path = value;
      else if (arg == "--ratio")
        params.symbol_cost_ratio = number();
      else if (arg == "--metadata-bits")
        params.metadata_bits_per_block = number();
      else if (arg == "--transition-cost")
        params.transition_cost_per_level = number();
      else if (arg == "--conversion-cost")
        params.conversion_cost = number();
      else if (arg == "--block-size")
        params.block_size = integer();
      else if (arg == "--binary-tier0-bits")
        params.binary_tier0_bits = integer();
      else if (arg == "--trace") {
        custom = true;
        std::istringstream input(value);
        std::string item;
        while (std::getline(input, item, ',')) {
          size_t used = 0;
          auto val = std::stoll(item, &used);
          if (used != item.size()) throw std::invalid_argument("invalid trace integer");
          custom_trace.push_back(val);
        }
        if (!value.empty() && value.back() == ',')
          throw std::invalid_argument("trailing trace separator");
      } else
        throw std::invalid_argument("unknown option: " + arg);
    }
    std::map<std::string, std::vector<int64_t>> workloads;
    workloads["constant_small"] = WorkloadGenerator::constant_small(100);
    workloads["constant_large"] = WorkloadGenerator::constant_large(100);
    workloads["phased"] = WorkloadGenerator::phased(100);
    workloads["alternating"] = WorkloadGenerator::alternating(100);
    workloads["boundary_oscillation"] = WorkloadGenerator::boundary_oscillation(100);
    workloads["signed_extremes"] = WorkloadGenerator::signed_extremes(100);
    workloads["sparse_large"] = WorkloadGenerator::sparse_large(100);
    workloads["adversarial_transition"] = WorkloadGenerator::adversarial_transition(100);

    if (custom) workloads = {{"custom", custom_trace}};

    std::vector<WorkloadSummary> summaries;

    std::cout << "=== H04-R02 Adaptive Representation Experiment ===\n\n";

    for (const auto& [name, trace] : workloads) {
      TraceLedger fb = Evaluator::evaluate_trace(Arm::FB, trace, params);
      TraceLedger ab = Evaluator::evaluate_trace(Arm::AB, trace, params);
      TraceLedger ft = Evaluator::evaluate_trace(Arm::FT, trace, params);
      TraceLedger at = Evaluator::evaluate_trace(Arm::AT, trace, params);

      // Payload break-even: AB symbol slots / AT symbol slots
      double payload_break_even = (at.symbol_slots > 0) ? static_cast<double>(ab.symbol_slots) /
                                                              static_cast<double>(at.symbol_slots)
                                                        : 0.0;

      // Reoptimize AT at each ratio: its optimal schedule can change at the crossing.
      double total_break_even = total_crossover(trace, params);

      summaries.push_back({name, fb, ab, ft, at, payload_break_even, total_break_even});

      std::cout << "Workload: " << name << "\n";
      std::cout << "  FB Total Cost: " << fb.total_cost << " (Slots: " << fb.symbol_slots << ")\n";
      std::cout << "  AB Total Cost: " << ab.total_cost << " (Slots: " << ab.symbol_slots
                << ", Trans: " << ab.transition_count << ")\n";
      std::cout << "  FT Total Cost: " << ft.total_cost << " (Slots: " << ft.symbol_slots << ")\n";
      std::cout << "  AT Total Cost: " << at.total_cost << " (Slots: " << at.symbol_slots
                << ", Trans: " << at.transition_count << ")\n";
      std::cout << "  Payload C3/C2 Break-even: " << payload_break_even << "\n";
      std::cout << "  Total Cost C3/C2 Break-even: " << total_break_even << "\n\n";
    }

    if (!json_out_path.empty()) {
      std::ofstream os(json_out_path);
      if (!os.is_open()) throw std::runtime_error("cannot open JSON output");
      {
        os << std::setprecision(17);
        os << "{\n";
        os << "  \"experiment\": \"t81_h04_r02_adaptive_representation\",\n";
        os << "  \"evidence_class\": \"corrected symbolic model; original inference invalid; H04 "
              "unresolved\",\n";
        os << "  \"parameters\": {\"ratio\": " << params.symbol_cost_ratio
           << ", \"metadata_bits\": " << params.metadata_bits_per_block
           << ", \"transition_cost\": " << params.transition_cost_per_level
           << ", \"conversion_cost\": " << params.conversion_cost
           << ", \"block_size\": " << params.block_size
           << ", \"binary_tier0_bits\": " << params.binary_tier0_bits << "},\n";
        os << "  \"workloads\": {\n";

        for (size_t i = 0; i < summaries.size(); ++i) {
          const auto& s = summaries[i];
          os << "    \"" << escape_json(s.name) << "\": {\n";
          os << "      \"FB\": ";
          print_ledger_json(os, s.fb, 8);
          os << ",\n";
          os << "      \"AB\": ";
          print_ledger_json(os, s.ab, 8);
          os << ",\n";
          os << "      \"FT\": ";
          print_ledger_json(os, s.ft, 8);
          os << ",\n";
          os << "      \"AT\": ";
          print_ledger_json(os, s.at, 8);
          os << ",\n";
          os << "      \"payload_break_even_c3_c2\": " << s.payload_break_even_c3_c2 << ",\n";
          os << "      \"total_break_even_c3_c2\": " << s.total_break_even_c3_c2 << "\n";
          os << "    }" << (i + 1 < summaries.size() ? "," : "") << "\n";
        }

        os << "  }\n";
        os << "}\n";
        os.close();
        if (!os) throw std::runtime_error("failed writing JSON output");
        std::cout << "Results written to JSON: " << json_out_path << "\n";
      }
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "h04-r02: " << e.what() << "\n";
    return 1;
  }
}
