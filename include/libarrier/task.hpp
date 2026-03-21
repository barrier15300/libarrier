#ifndef LIBARRIER_TASK_HPP
#define LIBARRIER_TASK_HPP

#include <chrono>
#include <future>
#include <optional>
#include <thread>

namespace libarrier {

template<typename T>
class Task {
	std::future<T> m_future;

public:

	template<typename F, typename... Args>
	Task(F&& proc, Args&&... args) {
		m_future = std::async(std::launch::async, proc, std::forward<Args>(args)...);
	}

	bool IsFinished() const {
		return m_future.wait_for(std::chrono::seconds::zero()) == std::future_status::ready;
	}

	void Wait() {
		while (!IsFinished()) {
			std::this_thread::yield();
		}
	}

	std::optional<T> ResultAsync() const {
		return IsFinished() ? m_future.get() : std::nullopt;
	}
	T Result() {
		Wait();
		return m_future.get();
	}
};

} // namespace libarrier

#endif // LIBARRIER_TASK_HPP