#pragma once
// experimental/dpe/thread_pool.hpp
//
// RFC-DPE-0006 §2: DpeThreadPool — bounded worker pool for epoch-level
// parallel dispatch.
//
// A pool with N workers accepts tasks via submit(), executes them on worker
// threads, and allows the caller to synchronise via wait_idle().  Designed
// for use by axion_kernel_submit_epoch() as an alternative to the
// RFC-DPE-0005 unbounded one-thread-per-task dispatch.

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace t81::dpe {

class DpeThreadPool {
public:
  /// Construct a pool with `worker_count` worker threads.
  /// worker_count == 0 is treated as 1 (minimum one worker).
  explicit DpeThreadPool(std::size_t worker_count) noexcept;

  /// Shuts down the pool (calls shutdown()) and joins all workers.
  ~DpeThreadPool();

  // Non-copyable, non-movable.
  DpeThreadPool(const DpeThreadPool&)            = delete;
  DpeThreadPool& operator=(const DpeThreadPool&) = delete;

  /// Submit a callable to the pool queue.
  /// Returns true if accepted; false if the pool is stopped.
  /// Never throws — pool submission failure falls back to inline execution
  /// at the call site (RFC-DPE-0006 §5).
  bool submit(std::function<void()> task) noexcept;

  /// Block until all previously submitted tasks have completed.
  /// Returns immediately if no tasks are pending.
  void wait_idle() noexcept;

  /// Stop accepting new tasks and join all worker threads.  Idempotent.
  void shutdown() noexcept;

  /// Number of worker threads successfully created at construction.
  [[nodiscard]] std::size_t worker_count() const noexcept {
    return workers_.size();
  }

private:
  void worker_loop() noexcept;

  std::vector<std::thread>          workers_;
  std::queue<std::function<void()>> queue_;
  std::mutex                        mu_;
  std::condition_variable           cv_;       ///< wakes workers on new task / shutdown
  std::condition_variable           idle_cv_;  ///< wakes wait_idle() callers
  std::size_t                       pending_{0};
  bool                              stopped_{false};
};

}  // namespace t81::dpe
