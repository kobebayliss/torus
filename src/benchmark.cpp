#include <atomic>
#include <cstdint>
#include "SPSCQueue.hpp"
#include <mutex>
#include <thread>
#include <iostream>
#include <chrono>
#include <deque>
#include <boost/lockfree/spsc_queue.hpp>

struct OrderInfo {
	uint64_t timestamp;
	uint64_t symbol;
	int price;
	int quantity;
};

constexpr uint64_t N = 100'000'000;

template<typename PushFn, typename PopFn>
void run_test(const char* title, PushFn try_push, PopFn try_pop) {
	std::atomic<bool> start_flag = false;
	
	std::thread producer([&]() {
		while (!start_flag.load(std::memory_order_acquire)) {}

		for (uint64_t i = 0; i < N; i++) {
			OrderInfo o{ i, 1, 1, 1 };
			while (!try_push(o)) {}
		}
	});

	std::thread consumer([&]() {
		while (!start_flag.load(std::memory_order_acquire)) {}

		uint64_t expected = 0;
		while (expected < N) {
			auto o = try_pop();
			if (!o) continue;
			if (o->timestamp != expected) {
				std::cout << "MISMATCH at expected = " << expected << ", got = " << o->timestamp << std::endl;
				std::abort();
			}
			expected++;
		}
	});
	auto start = std::chrono::high_resolution_clock::now();
	start_flag.store(true, std::memory_order_release);
	producer.join();
	consumer.join();
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	std::cout << title << " PASSED: " << N << " items verified in order, no drops, no duplicates." << std::endl;
	std::cout << "TIME TAKEN: " << elapsed << '\n' << std::endl;
}

int main() {
	{
		std::deque<OrderInfo> dq;
		std::mutex m;
		run_test("DEQUE (MUTEX)", 
				[&](const OrderInfo& o) { 
					std::lock_guard<std::mutex> lock(m);
					dq.push_back(o);
					return true;
				},
				[&]() -> std::optional<OrderInfo> {
					std::lock_guard<std::mutex> lock(m);
					if (dq.empty()) return std::nullopt;
					OrderInfo o = dq.front();
					dq.pop_front();
					return o;
				}
		);
	}
	{
		boost::lockfree::spsc_queue<OrderInfo, boost::lockfree::capacity<4096>> bq;
		run_test("BOOST",
				[&](const OrderInfo& o) {
					return bq.push(o); 
				},
				[&]() -> std::optional<OrderInfo> {
					OrderInfo o;
					if (bq.pop(o)) return o;
					return std::nullopt;
				}
		);
	}
	{
		SPSCQueue<OrderInfo, 4096> torus_queue;
		run_test("TORUS",
				[&](const OrderInfo& o) {
					return torus_queue.try_push(o); 
				},
				[&]() { 
					return torus_queue.try_pop(); 
				}
		);
	}
	return 0;
}
