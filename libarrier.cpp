#include "libarrier.h"

#include "include/libarrier/functional.hpp"

int main() {
	using namespace libarrier;

	double b = 1.5;
	function<void(int)> fn = [b](int a) {
		printf("v: %lf\n", a * b);
	};

	fn(2);
	fn(3);

	return 0;
}
