#ifndef LIBARRIER_MATCH_HPP
#define LIBARRIER_MATCH_HPP

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace libarrier {

#if 0 // TODO: checked type_match
	namespace metatuple {
		
		template<class ArgsTy>
static constexpr size_t args_size = std::tuple_size_v<ArgsTy>;

static constexpr size_t args_npos = -1;

template<typename... Ty>
using args = std::tuple<Ty...>;

template<size_t I, typename ArgsTy>
using args_indexer = std::tuple_element_t<I, ArgsTy>;

template<size_t B, typename ArgsTy, size_t... Is>
[[noreturn]] static constexpr auto sub_args_impl(std::index_sequence<Is...>)
	-> args<std::decay_t<decltype(std::get<Is + B>(std::declval<ArgsTy>()))>...> {}

template<typename ArgsTy, size_t B, size_t C = args_size<ArgsTy> - B>
using sub_args = decltype(sub_args_impl<B, ArgsTy>(std::make_index_sequence<C>()));

template<size_t Idx, size_t Size, typename ArgsTy, typename T>
struct type_find_impl
	: std::conditional_t<
		  Idx == Size, type_find_impl<Idx, Size, std::false_type, T>,
		  std::conditional_t<std::is_same_v<args_indexer<Idx, ArgsTy>, T>, type_find_impl<Idx, Size, std::true_type, T>,
                             type_find_impl<Idx + 1, Size, ArgsTy, T>>> {};

template<size_t Idx, size_t Size, typename T>
struct type_find_impl<Idx, Size, std::true_type, T> {
	static constexpr size_t pos = Idx;
};

template<size_t Idx, size_t Size, typename T>
struct type_find_impl<Idx, Size, std::false_type, T> {
	static constexpr size_t pos = args_npos;
};

template<typename ArgsTy, typename T>
struct type_find : type_find_impl<0, args_size<ArgsTy>, ArgsTy, T> {};

template<typename ArgsTy, typename T>
static constexpr size_t type_find_v = type_find<ArgsTy, T>::pos;

static constexpr size_t test = type_find_v<args<int, char, short>, int>;
using test_t = std::decay_t<int(int)>;

} // namespace metatuple

namespace type_match_detail {

using namespace metatuple;

template<typename Rt, typename Fn, typename TyArg>
struct fallback_impl : Fn {
	using Fn::operator();
};

template<typename Rt, typename TyArg>
struct fallback_impl<Rt, void, TyArg> {
	Rt operator()(const TyArg&) {
		return Rt();
	}
};

template<typename Rt, typename Fn, typename TyArg,
         typename CondTy = std::conditional_t<std::is_invocable_r_v<Rt, Fn, TyArg>, fallback_impl<Rt, Fn, TyArg>,
                                              fallback_impl<Rt, void, TyArg>>>
struct fallback : CondTy {
	using CondTy::operator();
};

template<typename Rt, typename TyArg>
struct fallback<Rt, void, TyArg, void> {};

template<typename Rt, typename ArgsTy, typename TailsTy>
struct check_impl
	: fallback<Rt, std::conditional_t<args_size<TailsTy> == 0, void, sub_args<TailsTy, 0, 1>>, ArgsTy>,
	  std::conditional_t<args_size<TailsTy> == 0, check_impl<Rt, void, void>, check_impl<Rt, ArgsTy, sub_args<TailsTy, 1>>> {};

// end of 'type_match_check_impl<Rt, void, void>'

template<typename Rt>
struct check_impl<Rt, void, void> {};

} // namespace type_match_detail
#endif

template<typename... Ty>
using union_t = std::variant<Ty...>;

template<typename... Fn>
struct type_match : Fn... {
	using Fn::operator()...;
};
template<typename... Fn>
type_match(Fn...) -> type_match<Fn...>;

template<typename... Ty, typename... Ty2>
constexpr auto operator|(const union_t<Ty...>& u, type_match<Ty2...> match) {
	return std::visit(match, u);
}

} // namespace libarrier

#endif // LIBARRIER_MATCH_HPP
