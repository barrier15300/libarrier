#ifndef LIBARRIER_FUNCTIONAL_HPP
#define LIBARRIER_FUNCTIONAL_HPP

#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

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
		virtual void* get_functor() = 0;
		virtual const void* get_functor() const = 0;
	};

	template<typename F>
	struct generic_functor : generic_functor_base {
		generic_functor(F&& prfunctor) : functor(std::move(prfunctor)) {}
		void* get_functor() {
			return std::addressof(functor);
		}
		const void* get_functor() const {
			return std::addressof(functor);
		}
		F functor;
	};
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
	using prfunc_ptr = std::shared_ptr<generic_functor_base>;

	using Object = std::variant<std::nullptr_t, func_ptr, void*, const void*, prfunc_ptr>;
	using Callback = R (*)(Object, Args...);
	Object m_obj = nullptr;
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

	function(func_ptr func) :
		m_obj(func),
		m_callback([](Object obj, Args... args) -> R {
			return std::get<func_ptr>(obj)(std::forward<Args>(args)...);
		}) {}
	template<typename F>
	function(F&& fn_obj)
		requires(func_object<F> && convertible_to_func_pointer<F>)
		: function(static_cast<func_ptr>(fn_obj)) {}
	template<typename F>
	function(F& fn_obj)
		requires(func_object<F>)
		:
		m_obj(static_cast<void*>(std::addressof(fn_obj))),
		m_callback([](Object obj, Args... args) -> R {
			return (*static_cast<F*>(std::get<void*>(obj)))(std::forward<Args>(args)...);
		}) {}
	template<typename F>
	function(const F& fn_obj)
		requires(func_object<F>)
		:
		m_obj(static_cast<const void*>(std::addressof(fn_obj))),
		m_callback([](Object obj, Args... args) -> R {
			return (*static_cast<const F*>(std::get<const void*>(obj)))(std::forward<Args>(args)...);
		}) {}
	template<typename C>
	function(C& obj, member_func<C> fn) :
		function([pobj = std::addressof(obj), fn](Args... args) {
			return (pobj->*fn)(std::forward<Args>(args)...);
		}) {}
	template<typename C>
	function(const C& obj, const_member_func<C> fn) :
		function([pobj = std::addressof(obj), fn](Args... args) {
			return (pobj->*fn)(std::forward<Args>(args)...);
		}) {}
	template<typename F>
	function(F&& fn_obj)
		requires(func_object<F> && !convertible_to_func_pointer<F>)
		:
		m_obj(std::make_shared<generic_functor<F>>(std::move(fn_obj))),
		m_callback([](Object obj, Args... args) -> R {
			return (*static_cast<F*>(std::get<prfunc_ptr>(obj)->get_functor()))(std::forward<Args>(args)...);
		}) {}

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
