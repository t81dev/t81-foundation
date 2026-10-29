// S02-only controls; assertions remain active in Release/NDEBUG builds.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "stochastic_topology.hpp"
namespace t81::s02 {
struct IntegrityAccess {
    static auto& states(NetworkSimulator& n) { return n.states_; }
    static auto& couplings(NetworkSimulator& n) { return n.network_couplings_; }
    static void adapt(NetworkSimulator& n, ExecutionMetrics& m) { n.perform_topology_adaptation(m); }
};
}
using namespace t81::s02;
static int checks=0;
static void check(bool value,const char* why) {if(!value)throw std::runtime_error(why);++checks;}
template<class F> void rejects(F f) {bool threw=false;try{f();}catch(const std::invalid_argument&){threw=true;}check(threw,"invalid input must reject");}
static ProblemInstance pair_problem(double coupling) {return {2,{0,0},{{0,coupling},{coupling,0}}};}
int main() {
 try {
    // Published SplitMix64 reference sequence at initial state zero.
    const std::vector<uint64_t> expected{0xe220a8397b1dcdafULL,0x6e789e6aa1b965f4ULL,0x06c45d188009454fULL,0xf88bb8a8724c81ecULL,0x1b39896a51a8749bULL};
    SimplePRNG random(0);for(auto word:expected)check(random.next()==word,"SplitMix64 vector mismatch");
    std::vector<uint64_t> words{0,5,16};std::size_t draws=0;
    check(sample_uniform_below(10,[&]{return words.at(draws++);})==6 && draws==3,"rejection must discard the incomplete residue interval");
    rejects([&]{sample_uniform_below(0,[&]{return uint64_t{0};});});
    rejects([&]{random.uniform_int(2,1);});
    for(int i=0;i<1000;++i){const auto x=random.uniform_int(-3,3);check(x>=-3 && x<=3,"range overflow");}
    // Same N, changed J and then h, across both alphabets and caller mutation.
    auto p=pair_problem(1);check(compute_exact_ground_state(p,ElementType::Binary)==-1,"J1 ground");
    p.J_couplings={{0,3},{3,0}};check(compute_exact_ground_state(p,ElementType::Binary)==-3,"J3 cache key");
    p.h_fields[0]=2;check(compute_exact_ground_state(p,ElementType::Binary)==-5,"field cache key");
    check(compute_exact_ground_state(p,ElementType::Ternary)==-5,"ternary ground");
    auto bad=p;bad.h_fields[0]=std::numeric_limits<double>::quiet_NaN();rejects([&]{compute_exact_ground_state(bad,ElementType::Binary);});
    bad=p;bad.J_couplings[0][1]=4;rejects([&]{compute_exact_ground_state(bad,ElementType::Binary);});
    bad=p;bad.J_couplings[0][0]=1;rejects([&]{compute_exact_ground_state(bad,ElementType::Binary);});
    // Task coefficients stay immutable while computational couplings change.
    SimulationConfig cfg;cfg.num_nodes=2;NetworkSimulator network(p,cfg);IntegrityAccess::states(network)={1,1};
    const auto task=network.calculate_current_energy();p.h_fields[0]=999;
    IntegrityAccess::couplings(network)[0][1]=IntegrityAccess::couplings(network)[1][0]=-7;
    check(network.calculate_current_energy()==task,"mutating network/caller changed task");
    check(network.calculate_network_energy()!=task,"separate computational Hamiltonian not observed");
    cfg.matched_state_capacity=true;rejects([&]{NetworkSimulator n(pair_problem(1),cfg);});
    cfg.matched_state_capacity=false;cfg.num_steps=0;rejects([&]{NetworkSimulator n(pair_problem(1),cfg);});
    cfg.num_steps=1;cfg.temperature=0;rejects([&]{NetworkSimulator n(pair_problem(1),cfg);});
    // Central rebuild cost is symmetric-difference edge edits, not all top K.
    ProblemInstance triple{3,{0,0,0},{{0,1,0},{1,0,0},{0,0,0}}};cfg=SimulationConfig{};cfg.num_nodes=3;cfg.topology_type=TopologyType::CentralizedAdaptive;
    NetworkSimulator central(triple,cfg);IntegrityAccess::states(central)={1,1,1};ExecutionMetrics metrics;
    IntegrityAccess::adapt(central,metrics);check(metrics.topology_update_ops==2,"first central edit count");
    IntegrityAccess::adapt(central,metrics);check(metrics.topology_update_ops==2,"unchanged central graph charged new edge edits");
    check(metrics.topology_pair_checks==6 && metrics.topology_sort_comparisons>0,"global inspection/sort work must be visible");
    // A known initial optimum has first-hit step zero, with post-update occupancy.
    ProblemInstance one{1,{1},{{0}}};cfg=SimulationConfig{};cfg.num_nodes=1;cfg.num_steps=5;cfg.dynamics_type=DynamicsType::Deterministic;
    NetworkSimulator initial(one,cfg);IntegrityAccess::states(initial)={1};const auto result=initial.run();
    check(result.reached_ground_state && result.steps_to_ground_state==0,"initial ground-state first-hit wrong");
    check(result.ground_state_hits==5 && result.ground_state_occupancy_fraction==1,"occupancy denominator wrong");
    check(initial.trajectory().size()==6,"initial plus post-update trace length");
    std::cout<<"{\"status\":\"PASS\",\"assertions\":"<<checks<<",\"scope\":\"S02 RNG/cache/objective/configuration/cost/trace controls\"}\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
