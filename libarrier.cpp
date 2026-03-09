// libarrier.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "libarrier.h"

int main() {
	
	libarrier::Timer timer;
	
	timer.Start();
	
	while (timer.GetElapsed().Second() < 1) {}

	timer.Stop();

	std::cout << "Elapsed: " << timer.GetElapsed().CustomRatio(44100) << "\n";

	return 0;
}
