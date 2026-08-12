#include <cstdint>
#include "SPSCQueue.hpp"

struct OrderInfo {
	uint64_t timestamp;
	uint64_t symbol;
	int price;
	int quantity;
};

int main() {
	return 0;
}
