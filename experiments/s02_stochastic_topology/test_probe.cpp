
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
// Intrusive experiment-only fixture: alter graph/state directly to isolate energy
// semantics without depending on random trajectories. No production T81 API.
#include "stochastic_topology.hpp"
namespace t81::s02 {
struct IntegrityAccess {
 static auto& states(NetworkSimulator& n) { return n.states_; }
 static auto& adjacency(NetworkSimulator& n) { return n.active_adj_; }
};
}
using namespace t81::s02;
int main(int argc,char** argv) {
 std::cout << std::setprecision(17);
 std::string mode=argc>1?argv[1]:"";
 if(mode=="cache") {
  ProblemInstance p;p.num_nodes=1;p.h_fields={1.0};p.J_couplings={{0.0}};
  auto first=compute_exact_ground_state(p,ElementType::Binary);
  p.h_fields[0]=2.0;
  std::cout<<"["<<first<<","<<compute_exact_ground_state(p,ElementType::Binary)<<"]";
 } else if(mode=="objective") {
  ProblemInstance p;p.num_nodes=3;p.h_fields={0,0,0};p.J_couplings={{0,1,0},{1,0,0},{0,0,0}};
  SimulationConfig cfg;cfg.num_nodes=3;cfg.coupling_scale=2;
  NetworkSimulator sim(p,cfg);IntegrityAccess::states(sim)={1,1,1};
  auto before=sim.calculate_current_energy();IntegrityAccess::adjacency(sim).assign(3,std::vector<int>(3,0));
  auto removed=sim.calculate_current_energy();IntegrityAccess::adjacency(sim)[0][2]=IntegrityAccess::adjacency(sim)[2][0]=1;
  std::cout<<"["<<before<<","<<removed<<","<<sim.calculate_current_energy()<<"]";
 } else if(mode=="sampling") {
  SimplePRNG p(1);std::cout<<"[";
  for(int i=0;i<64;i++){if(i)std::cout<<",";std::cout<<p.uniform_int(0,1);}std::cout<<"]";
 }
}
