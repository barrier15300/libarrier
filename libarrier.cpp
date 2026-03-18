// libarrier.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "libarrier.h"

class Hoge {
public:

	void mut(int a) {
		printf("Hoge::mut: %d\n", a);
	}
	void imut(int a) const {
		printf("Hoge::imut: %d\n", a);
	}
};

void func(int a) {
	printf("func: %d\n", a);
}

int main() {
	using namespace libarrier;
	
	int b = 16;

	function f1 = func;
	function f2 = [](int a) {
		printf("lambda: %d\n", a);
	};
	function f3 = [b](int a) {
		printf("captured lambda: %d\n", b * a);
	};

	Hoge obj;

	function f4 = {obj, &Hoge::mut};
	function f5 = {obj, &Hoge::imut};

	f1(1);
	f2(2);
	f3(3);
	f4(4);
	f5(5);

	return 0;
}
