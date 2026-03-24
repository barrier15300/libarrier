#include "libarrier.h"

#include "include/libarrier/timer.hpp"

#include <ostream>

int main() {
	using namespace libarrier;

	std::atomic<double> progress = 0;
	Task task = Task<int>([&]() {
		Timer timer;
		timer.Start();

		unsigned long long mem = -1, now = 0, target = 10000000;
		while ((now = timer.GetElapsedTick().MicroSecond()) < target) {
			if (mem != now) { progress = (double)now / target; }
		}

		return 100;
	});

	while (!task.IsFinished()) {
		std::cout << "\r" << std::format("progress: {0:>3.5}%   ", progress.load() * 100) << std::flush;
	}

	std::cout << "\r" << "Completed!            " << std::flush << "\n";

	return 0;
}
