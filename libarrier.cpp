#include "libarrier.h"

#include "include/libarrier/functional.hpp"

int main() {
	using namespace libarrier;

	int a = 3;
	function<void(int)> f = [a](int b) {
		printf("v: %d", a * b);
	};

	f(5);

	return 0;
}
