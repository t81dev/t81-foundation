#include "t81/dpe/epoch_commit.hpp"
#include "t81/dpe/task_graph.hpp"
#include "t81/dpe/task_runner.hpp"
#include "t81/dpe/thread_pool.hpp"

int main() {
  t81::dpe::TaskDescriptor task;
  t81::dpe::EpochGraph epoch;
  t81::dpe::DpeTaskRunner runner;
  t81::dpe::DpeThreadPool pool(1);
  (void)task;
  (void)epoch;
  (void)runner;
  (void)pool;
  return 0;
}
