#include "stochastic_topology.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>

using namespace t81::s02;

int main(int argc, char* argv[]) {
    std::string output_filepath = "results.json";
    if (argc > 1) {
        output_filepath = argv[1];
    }

    std::vector<uint64_t> seeds = {1, 2, 3, 4, 5};
    std::vector<std::size_t> node_counts = {8, 12, 16};
    std::vector<double> temperatures = {0.1, 0.5, 1.0};
    std::vector<double> topo_costs = {0.0, 0.5, 1.0, 5.0};

    std::vector<ExecutionMetrics> all_metrics;

    // Run sweep over configurations
    for (std::size_t N : node_counts) {
        for (double temp : temperatures) {
            for (uint64_t seed : seeds) {
                ProblemInstance prob = generate_problem_instance(N, 1.0, seed);

                for (double c_top : topo_costs) {
                    // Model A: Binary Fixed Stochastic
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Binary;
                        cfg.topology_type = TopologyType::Fixed;
                        cfg.dynamics_type = DynamicsType::Stochastic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }

                    // Model B: Ternary Fixed Stochastic
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Ternary;
                        cfg.topology_type = TopologyType::Fixed;
                        cfg.dynamics_type = DynamicsType::Stochastic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }

                    // Model C: Binary Adaptive Stochastic
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Binary;
                        cfg.topology_type = TopologyType::AdaptiveLocal;
                        cfg.dynamics_type = DynamicsType::Stochastic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }

                    // Model D: Ternary Adaptive Stochastic
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Ternary;
                        cfg.topology_type = TopologyType::AdaptiveLocal;
                        cfg.dynamics_type = DynamicsType::Stochastic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }

                    // Baseline: Centralized Adaptive Baseline
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Binary;
                        cfg.topology_type = TopologyType::CentralizedAdaptive;
                        cfg.dynamics_type = DynamicsType::Stochastic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }

                    // Control 1: Random Adaptation Control
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Binary;
                        cfg.topology_type = TopologyType::RandomAdaptive;
                        cfg.dynamics_type = DynamicsType::Stochastic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }

                    // Control 2: Deterministic Local Descent Control
                    {
                        SimulationConfig cfg;
                        cfg.element_type = ElementType::Binary;
                        cfg.topology_type = TopologyType::AdaptiveLocal;
                        cfg.dynamics_type = DynamicsType::Deterministic;
                        cfg.num_nodes = N;
                        cfg.num_steps = 200;
                        cfg.seed = seed;
                        cfg.temperature = temp;
                        cfg.cost_topology_update = c_top;

                        NetworkSimulator sim(prob, cfg);
                        all_metrics.push_back(sim.run());
                    }
                }
            }
        }
    }

    // Output results as JSON
    std::ofstream out(output_filepath);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file: " << output_filepath << std::endl;
        return 1;
    }

    out << "{\n";
    out << "  \"experiment\": \"T81 S02 Stochastic State-Topology Computation\",\n";
    out << "  \"total_runs\": " << all_metrics.size() << ",\n";
    out << "  \"runs\": [\n";

    for (std::size_t i = 0; i < all_metrics.size(); ++i) {
        const auto& m = all_metrics[i];
        out << "    {\n";
        out << "      \"element_type\": \"" << element_type_to_string(m.element_type) << "\",\n";
        out << "      \"topology_type\": \"" << topology_type_to_string(m.topology_type) << "\",\n";
        out << "      \"dynamics_type\": \"" << dynamics_type_to_string(m.dynamics_type) << "\",\n";
        out << "      \"num_nodes\": " << m.num_nodes << ",\n";
        out << "      \"seed\": " << m.seed << ",\n";
        out << "      \"temperature\": " << m.temperature << ",\n";
        out << "      \"cost_topology_update\": " << m.cost_topology_update << ",\n";
        out << "      \"final_energy\": " << m.final_energy << ",\n";
        out << "      \"ground_state_energy\": " << m.ground_state_energy << ",\n";
        out << "      \"energy_error\": " << m.energy_error << ",\n";
        out << "      \"mean_energy\": " << m.mean_energy << ",\n";
        out << "      \"spin_update_ops\": " << m.spin_update_ops << ",\n";
        out << "      \"comm_ops\": " << m.comm_ops << ",\n";
        out << "      \"topology_update_ops\": " << m.topology_update_ops << ",\n";
        out << "      \"total_cost\": " << m.total_cost << ",\n";
        out << "      \"steps_to_ground_state\": " << m.steps_to_ground_state << ",\n";
        out << "      \"reached_ground_state\": " << (m.reached_ground_state ? "true" : "false") << ",\n";
        out << "      \"effective_sampling_efficiency\": " << m.effective_sampling_efficiency << "\n";
        out << "    }" << (i + 1 < all_metrics.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    std::cout << "Experiment S02 complete. Executed " << all_metrics.size()
              << " configurations. Results written to " << output_filepath << std::endl;

    return 0;
}
