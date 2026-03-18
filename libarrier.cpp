// libarrier.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "libarrier.h"

void func(int a) {
	printf("func: %d\n", a);
}

int main() {
	using namespace libarrier;
	
	int b = 16;

	function_ref<void(int)> f1 = func;
	function_ref<void(int)> f2 = [](int a) {
		printf("lambda: %d\n", a);
	};
	function_ref<void(int)> f3 = [b](int a) {
		printf("captured lambda: %d\n", b * a);
	};

	auto l = [](int a) {
		printf("lambda: %d\n", a);
	};

	f1(1);
	f2(2);
	f3(3);

	return 0;
}
