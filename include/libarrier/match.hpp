#ifndef LIBARRIER_MATCH_HPP
#define LIBARRIER_MATCH_HPP

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "meta.hpp"

namespace libarrier {

#if 0 // TODO: checked type_match

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

template <typename... Ty>
using union_t = std::variant<Ty...>;

template <typename... Fn>
struct type_match : Fn... {
    using Fn::operator()...;
};
template <typename... Fn>
type_match(Fn&&...) -> type_match<Fn...>;

template <typename... Ty, typename... Ty2>
constexpr auto operator|(const union_t<Ty...>& u, type_match<Ty2...> match) {
    return std::visit(match, u);
}

} // namespace libarrier

#endif // LIBARRIER_MATCH_HPP
