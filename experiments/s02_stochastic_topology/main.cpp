#include "stochastic_topology.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>
using namespace t81::s02;

struct Arm { ElementType element; TopologyType topology; DynamicsType dynamics; };
struct Record { ExecutionMetrics metrics; std::size_t problem_id; std::size_t trajectory_id; };

template <class T> void write_vector(std::ostream& out, const std::vector<T>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << values[i]; }
    out << ']';
}

int main(int argc, char* argv[]) {
    try {
        const std::string output = argc > 1 ? argv[1] : "results.json";
        const std::string trace_path = argc > 2 ? argv[2] : output + ".traces.jsonl";
        std::ofstream out(output), traces(trace_path);
        if (!out || !traces) throw std::runtime_error("cannot open result/trace outputs");
        out << std::setprecision(17); traces << std::setprecision(17);
        const std::vector<uint64_t> seeds{1,2,3,4,5};
        const std::vector<std::size_t> sizes{8,12,16};
        const std::vector<double> temperatures{0.1,0.5,1.0}, costs{0.0,0.5,1.0,5.0};
        const std::vector<Arm> arms{
            {ElementType::Binary,TopologyType::Fixed,DynamicsType::Stochastic},
            {ElementType::Ternary,TopologyType::Fixed,DynamicsType::Stochastic},
            {ElementType::Binary,TopologyType::AdaptiveLocal,DynamicsType::Stochastic},
            {ElementType::Ternary,TopologyType::AdaptiveLocal,DynamicsType::Stochastic},
            {ElementType::Binary,TopologyType::CentralizedAdaptive,DynamicsType::Stochastic},
            {ElementType::Binary,TopologyType::RandomAdaptive,DynamicsType::Stochastic},
            {ElementType::Binary,TopologyType::AdaptiveLocal,DynamicsType::Deterministic}
        };
        std::vector<ProblemInstance> problems;
        std::map<std::pair<std::size_t,uint64_t>,std::size_t> ids;
        for (auto n:sizes) for (auto seed:seeds) {
            ids[{n,seed}]=problems.size();problems.push_back(generate_problem_instance(n,1.0,seed));
        }
        std::vector<Record> records;
        std::size_t trajectory_id=0;
        for (auto n:sizes) for (auto temp:temperatures) for (auto seed:seeds) for (const auto& arm:arms) {
            const auto id=ids.at({n,seed});
            SimulationConfig cfg; cfg.num_nodes=n;cfg.num_steps=200;cfg.seed=seed;cfg.temperature=temp;
            cfg.element_type=arm.element;cfg.topology_type=arm.topology;cfg.dynamics_type=arm.dynamics;
            cfg.cost_topology_update=0.0;
            NetworkSimulator simulator(problems[id],cfg);
            auto metrics=simulator.run();
            traces << "{\"trajectory_id\":" << trajectory_id << ",\"problem_id\":" << id << ",\"states\":[";
            const auto& states=simulator.trajectory();
            for (std::size_t i=0;i<states.size();++i) {if(i) traces << ',';write_vector(traces,states[i]);}
            traces << "]}\n";
            // c_top does not enter the dynamics: these four rows reprice ONE
            // trajectory, never four independent observations.
            for (auto cost:costs) {
                auto priced=metrics;priced.cost_topology_update=cost;
                priced.total_cost=priced.spin_update_ops+priced.comm_ops+cost*priced.topology_update_ops;
                records.push_back({priced,id,trajectory_id});
            }
            ++trajectory_id;
        }
        out << "{\n  \"schema_version\":2,\n  \"experiment\":\"S02 scientific-integrity correction\",\n"
            << "  \"scope\":\"PARTIAL ORIGINAL GRID / CORRECTED PILOT\",\n"
            << "  \"capacity_comparison\":\"VERTEX_MATCHED_ONLY\",\n"
            << "  \"rng\":\"SplitMix64 + rejection; domain-separated task/proposal/acceptance/topology/initialization\",\n"
            << "  \"cost_semantics\":\"Legacy three-term software proxy; auxiliary scan/sort/RNG counts separate; not total physical cost\",\n"
            << "  \"cost_repricing\":\"Four penalties reprice each trajectory; not independent trials\",\n"
            << "  \"simulated_trajectories\":" << trajectory_id << ",\n  \"total_runs\":" << records.size() << ",\n"
            << "  \"problems\":[\n";
        for (std::size_t k=0;k<problems.size();++k) {
            const auto& p=problems[k];
            out << "    {\"problem_id\":" << k << ",\"num_nodes\":" << p.num_nodes << ",\"h_fields\":";
            write_vector(out,p.h_fields);out << ",\"J_couplings\":[";
            for (std::size_t i=0;i<p.num_nodes;++i) {if(i)out<<',';write_vector(out,p.J_couplings[i]);}
            out << "]}" << (k+1<problems.size()?",":"") << '\n';
        }
        out << "  ],\n  \"runs\":[\n";
        for (std::size_t k=0;k<records.size();++k) {
            const auto& r=records[k];const auto& m=r.metrics;
            out << "    {\"problem_id\":" << r.problem_id << ",\"trajectory_id\":" << r.trajectory_id
                << ",\"element_type\":\"" << element_type_to_string(m.element_type)
                << "\",\"topology_type\":\"" << topology_type_to_string(m.topology_type)
                << "\",\"dynamics_type\":\"" << dynamics_type_to_string(m.dynamics_type) << '"'
                << ",\"num_nodes\":" << m.num_nodes << ",\"seed\":" << m.seed
                << ",\"temperature\":" << m.temperature << ",\"coupling_scale\":" << m.coupling_scale
                << ",\"num_steps\":" << m.num_steps << ",\"cost_topology_update\":" << m.cost_topology_update
                << ",\"final_energy\":" << m.final_energy << ",\"final_network_energy\":" << m.final_network_energy
                << ",\"ground_state_energy\":" << m.ground_state_energy << ",\"energy_error\":" << m.energy_error
                << ",\"mean_energy\":" << m.mean_energy << ",\"spin_update_ops\":" << m.spin_update_ops
                << ",\"comm_ops\":" << m.comm_ops << ",\"topology_update_ops\":" << m.topology_update_ops
                << ",\"network_pair_checks\":" << m.network_pair_checks << ",\"topology_pair_checks\":" << m.topology_pair_checks
                << ",\"topology_sort_comparisons\":" << m.topology_sort_comparisons << ",\"random_words\":" << m.random_words
                << ",\"total_cost\":" << m.total_cost << ",\"steps_to_ground_state\":";
            if(m.reached_ground_state)out<<m.steps_to_ground_state;else out<<"null";
            out << ",\"reached_ground_state\":" << (m.reached_ground_state?"true":"false")
                << ",\"ground_state_hits\":" << m.ground_state_hits
                << ",\"ground_state_occupancy_fraction\":" << m.ground_state_occupancy_fraction << '}'
                << (k+1<records.size()?",":"") << '\n';
        }
        out << "  ]\n}\n";
        out.flush();traces.flush();
        if(!out || !traces)throw std::runtime_error("failed writing results/traces");
        std::cout << "S02 corrected pilot: " << trajectory_id << " trajectories, " << records.size() << " priced rows\n";
        return 0;
    } catch(const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
