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
		m_future = std::async(std::launch::async, std::forward<F>(proc), std::forward<Args>(args)...);
	}
	bool IsFinished() const {
		if (!m_future.valid()) { return false; }
		return m_future.wait_for(std::chrono::seconds::zero()) == std::future_status::ready;
	}

	void Wait() const {
		if (m_future.valid()) { m_future.wait(); }
	}

	std::optional<T> ResultAsync()
		requires(!std::is_void_v<T>)
	{
		return IsFinished() ? m_future.get() : std::nullopt;
	}
	std::optional<T> Result()
		requires(!std::is_void_v<T>)
	{
		if (!m_future.valid()) { return std::nullopt; }
		Wait();
		return m_future.get();
	}
};

} // namespace libarrier

#endif // LIBARRIER_TASK_HPP