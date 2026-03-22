#include "libarrier.h"

#include "include/libarrier/functional.hpp"
#include "include/libarrier/task.hpp"

int main() {
	using namespace libarrier;

	Task task = Task<int>([&]() {
		return 0;
	});

	return 0;
}
