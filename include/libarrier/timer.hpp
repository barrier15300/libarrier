#ifndef LIBARRIER_TIMER_HPP
#define LIBARRIER_TIMER_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace libarrier {

class Timer {
protected:

	using clock_type = std::chrono::steady_clock;
	using time_point = clock_type::time_point;
	using tick_type = time_point::duration;

	template<typename T>
	static constexpr bool is_number = std::is_integral_v<T> || std::is_floating_point_v<T>;

	time_point m_tp{};
	time_point m_stopped{};
	bool is_running : 1 = false;
	bool is_stopped : 1 = false;

	static time_point GetNowTimePoint() {
		return clock_type::now();
	}

public:

	void Start() {
		m_tp = is_stopped ? (m_tp + (GetNowTimePoint() - m_stopped)) : (GetNowTimePoint());
		is_running = true;
	}
	void Reset() {
		*this = {};
	}
	void Stop() {
		m_stopped = GetNowTimePoint();
		is_stopped = true;
	}

	bool IsRunning() const {
		return is_running;
	}
	bool IsStopped() const {
		return is_stopped;
	}

	struct Elapsed {
		using ret_type = double;

		tick_type m_elapsed{};

		constexpr Elapsed(tick_type tick) noexcept : m_elapsed(tick) {}

		template<typename D, typename T>
		T GetTime() const
			requires(is_number<T>)
		{
			return std::chrono::duration_cast<std::chrono::duration<T, typename D::period>>(m_elapsed).count();
		}

		template<typename T = ret_type>
		T Second() const
			requires(is_number<T>)
		{
			return GetTime<std::chrono::seconds, T>();
		}
		template<typename T = ret_type>
		T MilliSecond() const
			requires(is_number<T>)
		{
			return GetTime<std::chrono::milliseconds, T>();
		}
		template<typename T = ret_type>
		T MicroSecond() const
			requires(is_number<T>)
		{
			return GetTime<std::chrono::microseconds, T>();
		}
		template<typename T = ret_type>
		T NanoSecond() const
			requires(is_number<T>)
		{
			return GetTime<std::chrono::nanoseconds, T>();
		}

		template<typename T = ret_type, typename arg_type = ret_type>
		T CustomRatio(arg_type den) const
			requires(is_number<T> && is_number<arg_type>)
		{
			return static_cast<T>(Second() * den);
		}
	};
	struct ElapsedTick {
		using ret_type = uint64_t;

		tick_type m_elapsed{};

		constexpr ElapsedTick(tick_type tick) noexcept : m_elapsed(tick) {}

		template<typename D, typename T>
		T GetTick() const
			requires(is_number<T>)
		{
			return std::chrono::duration_cast<std::chrono::duration<T, typename D::period>>(m_elapsed).count();
		}

		template<typename T = ret_type>
		T Second() const
			requires(is_number<T>)
		{
			return GetTick<std::chrono::seconds, T>();
		}
		template<typename T = ret_type>
		T MilliSecond() const
			requires(is_number<T>)
		{
			return GetTick<std::chrono::milliseconds, T>();
		}
		template<typename T = ret_type>
		T MicroSecond() const
			requires(is_number<T>)
		{
			return GetTick<std::chrono::microseconds, T>();
		}
		template<typename T = ret_type>
		T NanoSecond() const
			requires(is_number<T>)
		{
			return GetTick<std::chrono::nanoseconds, T>();
		}

		template<typename T = ret_type, typename arg_type = ret_type>
		T CustomRatio(arg_type den) const
			requires(is_number<T> && is_number<arg_type>)
		{
			return static_cast<T>(Elapsed(m_elapsed).Second() * den);
		}
	};

	static inline const Elapsed NonElapsed = Elapsed(tick_type::min());
	static inline const ElapsedTick NonElapsedTick = ElapsedTick(tick_type::min());

	tick_type GetTicks() const {
		return is_stopped ? (m_stopped - m_tp) : (GetNowTimePoint() - m_tp);
	}

	Elapsed GetElapsed() const {
		return Elapsed(GetTicks());
	}

	ElapsedTick GetElapsedTick() const {
		return ElapsedTick(GetTicks());
	}
};
} // namespace libarrier

#endif // LIBARRIER_TIMER_HPP
