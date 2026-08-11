#include <atomic>
#include <cstdint>
#include <iostream>
#include <optional>
#include <bit>

template<typename T, size_t capacity>
class SPSCQueue {
	alignas(64) T* buffer;  // align to 64 byte segments for cache friendliness
	std::atomic<uint64_t> head;
	std::atomic<uint64_t> tail;
	std::atomic<size_t> size;
	static constexpr size_t rounded_capacity = std::bit_ceil(capacity);  // round to power of two for one cycle position calculation

public:
	SPSCQueue() {
		buffer = static_cast<T*>(std::aligned_alloc(64, rounded_capacity * sizeof(T)));
	}
	bool try_push(const T& item) {
		if (size == rounded_capacity) {
			std::cout << "FAILED TO PUSH." << std::endl;
			return false;
		}
		uint64_t pos = head++ & (rounded_capacity - 1);
		buffer[pos] = item;
		size++;
		std::cout << "PUSHED. HEAD: " << pos << std::endl;
		return true;
	}
	std::optional<T> try_pop() {
		if (size == 0) return std::nullopt;
		uint64_t pos = tail++ & (rounded_capacity - 1);
		size--;
		std::cout << "POPPED. TAIL: " << pos << std::endl;
		return buffer[pos];
	}
};

struct OrderInfo {
	uint64_t timestamp;
	uint64_t symbol;
	int price;
	int quantity;
};

int main() {
	OrderInfo o1{1, 1, 1, 1};
	OrderInfo o2{1, 1, 1, 1};
	OrderInfo o3{1, 1, 1, 1};
	OrderInfo o4{1, 1, 1, 1};
	OrderInfo o5{1, 1, 1, 1};
	OrderInfo o6{1, 1, 1, 1};
	OrderInfo o7{1, 1, 1, 1};
	OrderInfo o8{1, 1, 1, 1};
	OrderInfo o9{1, 1, 1, 1};
	OrderInfo o10{1, 1, 1, 1};
	OrderInfo o11{1, 1, 1, 1};
	OrderInfo o12{1, 1, 1, 1};
	SPSCQueue<OrderInfo, 8> TorusQueue;
	TorusQueue.try_push(o1);
	TorusQueue.try_push(o2);
	TorusQueue.try_push(o3);
	TorusQueue.try_push(o4);
	TorusQueue.try_push(o5);
	TorusQueue.try_push(o6);
	TorusQueue.try_pop();
	TorusQueue.try_pop();
	TorusQueue.try_push(o7);
	TorusQueue.try_push(o8);
	TorusQueue.try_push(o9);
	TorusQueue.try_push(o10);
	TorusQueue.try_push(o11);
	TorusQueue.try_push(o12);
	return 0;
}
