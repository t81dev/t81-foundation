// src/dpe/thread_pool.cpp
//
// RFC-DPE-0006: DpeThreadPool implementation.

#include "t81/dpe/thread_pool.hpp"

namespace t81::dpe {

DpeThreadPool::DpeThreadPool(std::size_t worker_count) noexcept {
  if (worker_count == 0) worker_count = 1;
  workers_.reserve(worker_count);
  for (std::size_t i = 0; i < worker_count; ++i) {
    try {
      workers_.emplace_back([this]() noexcept { worker_loop(); });
    } catch (...) {
      // Thread creation failed — pool continues with fewer workers.
    }
  }
}

DpeThreadPool::~DpeThreadPool() {
  shutdown();
}

void DpeThreadPool::worker_loop() noexcept {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(mu_);
      cv_.wait(lock, [this]() { return stopped_ || !queue_.empty(); });
      if (stopped_ && queue_.empty()) return;
      task = std::move(queue_.front());
      queue_.pop();
    }  // lock released before executing task

    try { task(); } catch (...) {}

    {
      std::lock_guard<std::mutex> lock(mu_);
      if (pending_ > 0) --pending_;
    }
    idle_cv_.notify_all();
  }
}

bool DpeThreadPool::submit(std::function<void()> task) noexcept {
  try {
    std::lock_guard<std::mutex> lock(mu_);
    if (stopped_) return false;
    ++pending_;
    queue_.push(std::move(task));
    cv_.notify_one();
    return true;
  } catch (...) {
    return false;
  }
}

void DpeThreadPool::wait_idle() noexcept {
  try {
    std::unique_lock<std::mutex> lock(mu_);
    idle_cv_.wait(lock, [this]() { return pending_ == 0; });
  } catch (...) {}
}

void DpeThreadPool::shutdown() noexcept {
  try {
    {
      std::lock_guard<std::mutex> lock(mu_);
      stopped_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) {
      if (w.joinable()) w.join();
    }
    workers_.clear();
  } catch (...) {}
}

}  // namespace t81::dpe
