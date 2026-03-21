#ifndef LIBARRIER_FUNCTIONAL_HPP
#define LIBARRIER_FUNCTIONAL_HPP

#include <concepts>
#include <type_traits>
#include <functional>
#include <utility>
#include <memory>
#include <mutex>
#include <vector>
#include <tuple>
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
	struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {

	};

	template<typename R, typename C, typename... Args>
	struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)>{
		using raw_func_type = R(C::*)(Args...);
	};

	template<typename R, typename C, typename... Args>
	struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {
		using raw_func_type = R(C::*)(Args...) const;
	};

	template<typename F>
	struct function_traits : function_traits<decltype(&F::operator())> {};

	// function impl
	class function_base {
	protected:

		struct unique_lambda_base {
			unique_lambda_base() noexcept = default;
			virtual ~unique_lambda_base() noexcept = default;
		};

		template<typename L>
		struct unique_lambda : unique_lambda_base {
			unique_lambda(L&& lambda) { m_lambda = new L(std::forward<L>(lambda)); }
			~unique_lambda() { delete m_lambda; }
			L* m_lambda;
		};
		
		using ptr_t = std::unique_ptr<unique_lambda_base>;
		using lock = std::lock_guard<std::mutex>;

		static inline std::vector<ptr_t> unique_lambdas;
		static inline std::mutex mutex;

		template<typename L>
		static L* store_lambda(L&& lambda) noexcept {
			lock lock(mutex);
			std::unique_ptr p = std::make_unique<unique_lambda<L>>(std::forward<L>(lambda));
			auto ret = p.get()->m_lambda;
			unique_lambdas.push_back(std::move(p));
			return ret;
		}
	};

	template<typename>
	class function;

	template<typename R, typename... Args>
	class function<R(Args...)> : function_base {

		using func_ptr = R(*)(Args...);
		using Object = void*;
		using Storage = std::variant<std::nullptr_t, func_ptr, Object>;
		using Callback = R(*)(Storage, Args...);
		
		Storage m_obj = nullptr;
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
			m_obj = func;
			m_callback = [](Storage obj, Args... args) -> R {
				return std::get<func_ptr>(obj)(std::forward<Args>(args)...);
			};
		}
		template<typename F>
		function(F&& fn_obj) requires (convertible_to_func_pointer<F>) : function(static_cast<func_ptr>(fn_obj)) {}
		template<typename F>
		function(F& fn_obj) requires (func_object<F>) {
			m_obj = static_cast<Object>(std::addressof(fn_obj));
			m_callback = [](Storage obj, Args... args) -> R {
				return std::invoke(*static_cast<F*>(std::get<Object>(obj)), std::forward<Args>(args)...);
			};
		}
		template<typename F>
		function(F&& fn_obj) requires (func_object<F> && !convertible_to_func_pointer<F>) : function(*store_lambda(std::forward<F>(fn_obj))) {}
		
		R operator()(Args&&... args) const {
			return m_callback(m_obj, std::forward<Args>(args)...);
		}
	};
	
	template<typename R, typename C, typename... Args>
	class function<R(C::*)(Args...)> {

		using member_func = R(C::*)(Args...);
		using const_member_func = R(C::*)(Args...) const;

		using Object = std::variant<std::nullptr_t, C*, const C*>;
		using Func = std::variant<std::nullptr_t, member_func, const_member_func>;
		using Callback = R(*)(Object, Func, Args...);

		Object m_obj = nullptr;
		Func m_func = nullptr;
		Callback m_callback = nullptr;

	public:

		function() = delete;
		function(const function&) = default;
		function(function&&) = default;
		function& operator=(const function&) = default;
		function& operator=(function&&) = default;

		function(C& obj, member_func mem_fn) {
			m_obj = std::addressof(obj);
			m_func = mem_fn;
			m_callback = [](Object obj, Func fn, Args... args) -> R {
				return (std::get<C*>(obj)->*std::get<member_func>(fn))(std::forward<Args>(args)...);
			};
		}
		function(const C& obj, const_member_func mem_fn) {
			m_obj = std::addressof(obj);
			m_func = mem_fn;
			m_callback = [](Object obj, Func fn, Args... args) -> R {
				return (std::get<const C*>(obj)->*std::get<const_member_func>(fn))(std::forward<Args>(args)...);
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
	function(R(*)(Args...)) -> function<R(Args...)>;

	template<typename R, typename C, typename... Args>
	function(C, R(C::*)(Args...)) -> function<R(C::*)(Args...)>;

	template<typename R, typename C, typename... Args>
	function(C, R(C::*)(Args...) const) -> function<R(C::*)(Args...)>;

	template<typename R, typename C, typename... Args>
	function(std::pair<C, R(C::*)(Args...)>) -> function<R(C::*)(Args...)>;

	template<typename R, typename C, typename... Args>
	function(std::pair<const C, R(C::*)(Args...) const>) -> function<R(C::*)(Args...)>;

	template<typename F>
	function(F) -> function<typename function_traits<F>::func_type>;
}

#endif // LIBARRIER_FUNCTIONAL_HPP
