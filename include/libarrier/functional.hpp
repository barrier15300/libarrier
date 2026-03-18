#ifndef LIBARRIER_FUNCTIONAL_HPP
#define LIBARRIER_FUNCTIONAL_HPP

#include <type_traits>
#include <functional>
#include <utility>
#include <memory>
#include <mutex>

namespace libarrier {

	class _unique_lambda_impl {
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
	class function_ref;

	template<typename R, typename ...Args>
	class function_ref<R(Args...)> : _unique_lambda_impl {

		using Object = void*;
		using Callback = R(*)(Object, Args&&...);
		
		Object m_obj = nullptr;
		Callback m_callback = nullptr;

	public:

		using func_ptr = R(*)(Args...);
		template<typename C>
		using member_func = R(C::*)(Args...);
		template<typename C>
		using const_member_func = R(C::*)(Args...) const;

		template<typename F>
		static constexpr bool func_object = requires(F obj) {
			{ obj.operator()(std::declval<Args>()...) } -> std::convertible_to<R>;
		};
		template<typename F>
		static constexpr bool convertible_to_func_pointer = requires(F obj) {
			{ obj } -> std::convertible_to<func_ptr>;
		}

		function_ref() = delete;
		function_ref(const function_ref&) = default;
		function_ref(function_ref&&) = default;
		function_ref& operator=(const function_ref&) = default;
		function_ref& operator=(function_ref&&) = default;

		function_ref(func_ptr func) {
			m_obj = reinterpret_cast<Object>(func);
			m_callback = [](Object obj, Args&&... args) -> R {
				return reinterpret_cast<R(*)(Args...)>(obj)(std::forward<Args>(args)...);
			};
		}
		template<typename F>
		function_ref(F&& fn_obj) requires (convertible_to_func_pointer<F>)
		{
			m_obj = static_cast<Object>(store_lambda(std::forward<F>(fn_obj)));
			m_callback = [](Object obj, Args&&... args) -> R {
				return std::invoke(*static_cast<F*>(obj), std::forward<Args>(args)...);
			};
		}
		template<typename F>
		function_ref(F& fn_obj) requires (func_object<F>) {
			m_obj = static_cast<Object>(std::addressof(fn_obj));
			m_callback = [](Object obj, Args&&... args) -> R {
				return std::invoke(*static_cast<F*>(obj), std::forward<Args>(args)...);
			};
		}
		template<typename F>
		function_ref(F&& fn_obj) requires (func_object<F> && !convertible_to_func_pointer<F>)
		{
			m_obj = static_cast<Object>(store_lambda(std::forward<F>(fn_obj)));
			m_callback = [](Object obj, Args&&... args) -> R {
				return std::invoke(*static_cast<F*>(obj), std::forward<Args>(args)...);
			};
		}
		template<typename C>
		function_ref(C& obj, member_func<C> mem_fn) {
			auto pobj = std::addressof(obj);
			*this = function_ref([pobj, mem_fn](Args&&... args) -> R {
				return (pobj->*mem_fn)(std::forward<Args>(args)...);
			});
		}
		template<typename C>
		function_ref(const C& obj, const_member_func<C> mem_fn) {
			auto pobj = std::addressof(obj);
			*this = function_ref([pobj, mem_fn](Args&&... args) -> R {
				return (pobj->*mem_fn)(std::forward<Args>(args)...);
			});
		}

		R operator()(Args&&... args) const {
			return m_callback(m_obj, std::forward<Args>(args)...);
		}
	};
}

#endif // LIBARRIER_FUNCTIONAL_HPP
