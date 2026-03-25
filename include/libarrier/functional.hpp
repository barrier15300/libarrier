#ifndef LIBARRIER_FUNCTIONAL_HPP
#define LIBARRIER_FUNCTIONAL_HPP

#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <semaphore>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace libarrier {

// traits
template<typename>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<R(Args...)> {
	using func_type = R(Args...);
	using raw_func_type = func_type;
	using args_tuple = std::tuple<Args...>;
	template<size_t N>
	using arg = std::tuple_element_t<N, args_tuple>;
	static constexpr size_t arity = sizeof...(Args);
};

template<typename R, typename... Args>
struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {};

template<typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {
	using raw_func_type = R (C::*)(Args...);
};

template<typename R, typename C, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
	using raw_func_type = R (C::*)(Args...) const;
};

template<typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

// function impl
class function_base {
protected:

	struct generic_functor_base {
		generic_functor_base() {}
		virtual ~generic_functor_base() {}
	};

	template<typename F>
	struct generic_functor : generic_functor_base {
		generic_functor(F&& prfunctor) : functor(std::move(prfunctor)) {}
		F functor;
	};

	static inline std::unordered_map<void*, generic_functor_base*> storage;
	static inline std::binary_semaphore semaphore{1};

	template<typename F>
	static F* regist_functor(F&& functor) {
		auto gfunctor = new generic_functor(std::move(functor));
		auto pfunctor = std::addressof(gfunctor->functor);
		semaphore.acquire();
		storage[pfunctor] = gfunctor;
		semaphore.release();
		return pfunctor;
	}

	template<typename F>
	static void unregist_functor(F* pfunctor) {
		semaphore.acquire();
		auto target = storage.find(pfunctor);
		if (target == storage.end()) { return; }
		delete target->second;
		storage.erase(target);
		semaphore.release();
	}
};

template<typename F, typename... Args>
class function_impl {};

template<typename>
class function;

template<typename R, typename... Args>
class function<R(Args...)> : function_base {
	template<typename C>
	using member_func = R (C::*)(Args...);
	template<typename C>
	using const_member_func = R (C::*)(Args...) const;
	using func_ptr = R (*)(Args...);

	union Object {
		std::nullptr_t null = nullptr;
		func_ptr func;
		void* obj;
		const void* const_obj;
	} m_obj{};

	using Callback = R (*)(Object, Args...);
	Callback m_callback = nullptr;

public:

	template<typename F>
	static constexpr bool func_object = requires(F obj) {
		{ obj.operator()(std::declval<Args>()...) } -> std::convertible_to<R>;
	};
	template<typename F>
	static constexpr bool convertible_to_func_pointer = requires(F obj) {
		{ obj } -> std::convertible_to<func_ptr>;
	};

	function() = delete;
	function(const function&) = default;
	function(function&&) = default;
	function& operator=(const function&) = default;
	function& operator=(function&&) = default;

	function(func_ptr func) {
		m_obj.func = func;
		m_callback = [](Object obj, Args... args) -> R {
			return obj.func(std::forward<Args>(args)...);
		};
	}
	template<typename F>
	function(F&& fn_obj)
		requires(func_object<F> && convertible_to_func_pointer<F>)
		: function(static_cast<func_ptr>(fn_obj)) {}
	template<typename F>
	function(F& fn_obj)
		requires(func_object<F>)
	{
		m_obj.obj = static_cast<void*>(std::addressof(fn_obj));
		m_callback = [](Object obj, Args... args) -> R {
			return (*static_cast<F*>(obj.obj))(std::forward<Args>(args)...);
		};
	}
	template<typename F>
	function(const F& fn_obj)
		requires(func_object<F>)
	{
		m_obj.const_obj = static_cast<const void*>(std::addressof(fn_obj));
		m_callback = [](Object obj, Args... args) -> R {
			return (*static_cast<const F*>(obj.const_obj))(std::forward<Args>(args)...);
		};
	}
	template<typename C>
	function(C& obj, std::conditional_t<!std::is_const_v<C>, member_func<C>, const_member_func<C>> fn) {
		auto pobj = std::addressof(obj);
		*this = function([pobj, fn](Args... args) {
			return (pobj->*fn)(std::forward<Args>(args)...);
		});
	}

	template<typename F>
	function(F&& fn_obj)
		requires(func_object<F> && !convertible_to_func_pointer<F>)
		: function(*regist_functor(std::move(fn_obj))) {}
	~function() {
		unregist_functor(m_obj.obj);
	}

	R operator()(Args... args) const {
		return m_callback(m_obj, std::forward<Args>(args)...);
	}
};

template<typename R, typename C, typename... Args>
class function<R (C::*)(Args...)> {
	using member_func = R (C::*)(Args...);
	using const_member_func = R (C::*)(Args...) const;

	union Object {
		std::nullptr_t null = nullptr;
		C* obj;
		const C* const_obj;
	} m_obj{};
	union Func {
		std::nullptr_t null = nullptr;
		member_func func;
		const_member_func const_func;
	} m_func{};
	using Callback = R (*)(Object, Func, Args...);
	Callback m_callback = nullptr;

public:

	function() = delete;
	function(const function&) = default;
	function(function&&) = default;
	function& operator=(const function&) = default;
	function& operator=(function&&) = default;

	function(C& obj, member_func mem_fn) {
		m_obj.obj = std::addressof(obj);
		m_func.func = mem_fn;
		m_callback = [](Object obj, Func fn, Args... args) -> R {
			return (obj.obj->*fn.func)(std::forward<Args>(args)...);
		};
	}
	function(const C& obj, const_member_func mem_fn) {
		m_obj.const_obj = std::addressof(obj);
		m_func.const_func = mem_fn;
		m_callback = [](Object obj, Func fn, Args... args) -> R {
			return (obj.const_obj->*fn.const_func)(std::forward<Args>(args)...);
		};
	}
	function(std::pair<C&, member_func> pair) : function(pair.first, pair.second) {}
	function(std::pair<const C&, const_member_func> pair) : function(pair.first, pair.second) {}

	R operator()(Args... args) const {
		return m_callback(m_obj, m_func, std::forward<Args>(args)...);
	}
};

// template helper
template<typename R, typename... Args>
function(R (*)(Args...)) -> function<R(Args...)>;

template<typename R, typename C, typename... Args>
function(C&, R (C::*)(Args...)) -> function<R (C::*)(Args...)>;

template<typename R, typename C, typename... Args>
function(const C&, R (C::*)(Args...) const) -> function<R (C::*)(Args...)>;

template<typename R, typename C, typename... Args>
function(std::pair<C&, R (C::*)(Args...)>) -> function<R (C::*)(Args...)>;

template<typename R, typename C, typename... Args>
function(std::pair<const C&, R (C::*)(Args...) const>) -> function<R (C::*)(Args...)>;

template<typename F>
function(F) -> function<typename function_traits<F>::func_type>;

} // namespace libarrier

#endif // LIBARRIER_FUNCTIONAL_HPP
