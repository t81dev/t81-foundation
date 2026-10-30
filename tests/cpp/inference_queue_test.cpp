#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "t81/inference/batch_inference_engine.hpp"

using namespace t81::inference;

void test_fifo_preservation() {
    std::cout << "[Test] FIFO preservation..." << std::endl;
    InferenceQueue queue(10);
    assert(queue.empty());
    assert(queue.size() == 0);

    for (int i = 0; i < 5; ++i) {
        EnhancedInferenceRequest req;
        req.request_id = "req_" + std::to_string(i);
        req.priority = InferencePriority::NORMAL;
        assert(queue.enqueue(req));
    }
    assert(queue.size() == 5);

    for (int i = 0; i < 5; ++i) {
        auto opt = queue.dequeue();
        assert(opt.has_value());
        assert(opt->request_id == "req_" + std::to_string(i));
    }
    assert(queue.empty());
    assert(queue.size() == 0);
    std::cout << "  -> PASSED" << std::endl;
}

void test_zero_capacity() {
    std::cout << "[Test] Zero capacity behavior..." << std::endl;
    InferenceQueue queue(0);
    assert(queue.empty());
    assert(queue.full());

    EnhancedInferenceRequest req;
    req.request_id = "blocked_req";
    req.priority = InferencePriority::NORMAL;
    // Enqueue must fail immediately when max_size == 0
    assert(!queue.enqueue(req));
    assert(queue.size() == 0);
    assert(queue.get_stats().max_size_reached == 1);
    std::cout << "  -> PASSED" << std::endl;
}

void test_downsizing_below_occupancy() {
    std::cout << "[Test] Downsizing below occupancy..." << std::endl;
    InferenceQueue queue(10);
    for (int i = 0; i < 6; ++i) {
        EnhancedInferenceRequest req;
        req.request_id = "item_" + std::to_string(i);
        req.priority = InferencePriority::NORMAL;
        assert(queue.enqueue(req));
    }
    assert(queue.size() == 6);

    // Downsize capacity to 3 (below current occupancy 6)
    queue.set_max_size(3);
    assert(queue.size() == 6); // existing items must NOT be dropped
    assert(queue.full());      // current_size (6) >= max_size (3)

    // New enqueue should be rejected
    EnhancedInferenceRequest extra;
    extra.request_id = "extra";
    extra.priority = InferencePriority::NORMAL;
    assert(!queue.enqueue(extra));

    // Dequeue all items; FIFO preserved
    for (int i = 0; i < 6; ++i) {
        auto opt = queue.dequeue();
        assert(opt.has_value());
        assert(opt->request_id == "item_" + std::to_string(i));
    }
    assert(queue.empty());

    // Now under capacity 3, enqueue should succeed up to 3
    assert(queue.enqueue(extra));
    assert(queue.size() == 1);
    std::cout << "  -> PASSED" << std::endl;
}

void test_concurrent_stress() {
    std::cout << "[Test] Concurrent enqueue/dequeue/resize stress..." << std::endl;
    InferenceQueue queue(50);
    std::atomic<bool> running{true};
    std::atomic<size_t> enqueued_count{0};
    std::atomic<size_t> dequeued_count{0};

    // Producers
    std::vector<std::thread> producers;
    for (int p = 0; p < 4; ++p) {
        producers.emplace_back([&, p]() {
            size_t idx = 0;
            while (running.load(std::memory_order_relaxed)) {
                EnhancedInferenceRequest req;
                req.request_id = "p_" + std::to_string(p) + "_" + std::to_string(idx++);
                req.priority = (idx % 2 == 0) ? InferencePriority::HIGH : InferencePriority::NORMAL;
                if (queue.enqueue(req)) {
                    enqueued_count.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::yield();
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c) {
        consumers.emplace_back([&]() {
            while (running.load(std::memory_order_relaxed)) {
                auto opt = queue.dequeue();
                if (opt.has_value()) {
                    dequeued_count.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::yield();
            }
        });
    }

    // Resizers
    std::vector<std::thread> resizers;
    for (int r = 0; r < 2; ++r) {
        resizers.emplace_back([&]() {
            size_t caps[] = {0, 1, 5, 20, 50, 100, 10, 0};
            size_t i = 0;
            while (running.load(std::memory_order_relaxed)) {
                queue.set_max_size(caps[i % 8]);
                queue.full();
                queue.empty();
                queue.size();
                queue.get_stats();
                i++;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running.store(false, std::memory_order_relaxed);

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    for (auto& t : resizers) t.join();

    // Drain remaining
    while (auto opt = queue.dequeue()) {
        dequeued_count.fetch_add(1, std::memory_order_relaxed);
    }

    assert(enqueued_count.load() == dequeued_count.load());
    assert(queue.empty());
    std::cout << "  -> PASSED (Processed " << enqueued_count.load() << " requests concurrently)" << std::endl;
}

void test_boundary_capacities() {
    std::cout << "[Test] Boundary capacities..." << std::endl;
    InferenceQueue queue(1);
    EnhancedInferenceRequest r1, r2;
    r1.request_id = "r1";
    r2.request_id = "r2";
    r1.priority = InferencePriority::NORMAL;
    r2.priority = InferencePriority::NORMAL;

    assert(queue.enqueue(r1));
    assert(queue.full());
    assert(!queue.enqueue(r2));

    queue.set_max_size(2);
    assert(!queue.full());
    assert(queue.enqueue(r2));
    assert(queue.full());

    queue.clear();
    assert(queue.empty());
    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Starting InferenceQueue Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    test_fifo_preservation();
    test_zero_capacity();
    test_downsizing_below_occupancy();
    test_boundary_capacities();
    test_concurrent_stress();

    std::cout << "========================================" << std::endl;
    std::cout << "All InferenceQueue Tests PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
