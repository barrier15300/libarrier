#ifndef LIBARRIER_BINARYDATA_HPP
#define LIBARRIER_BINARYDATA_HPP

#include "meta.hpp"
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <ranges>
#include <unordered_map>

namespace libarrier {

	using bytearray = std::vector<std::byte>;
	using byteview = std::span<const std::byte>;
	using byteref = std::span<std::byte>;

	template<class T>
	concept to_byteable = requires (T x) {
		{ x.ToBytes() } -> std::convertible_to<bytearray>;
	};
	template<class T>
	concept from_byteable = requires (T x) {
		{ T() } -> std::convertible_to<T>;
		{ x.FromBytes(std::declval<bytearray>()) } -> std::convertible_to<T>;
	};
	template<class T>
	concept cross_convertible = from_byteable<T> && to_byteable<T>;

	template<class T>
	concept memcopyable = std::is_trivially_copyable_v<T> && !(from_byteable<T> || to_byteable<T>);

class BinaryData {
	bytearray m_data {};

public:
	BinaryData(const void* src, size_t len) {
		m_data.resize(len);
		memcpy(m_data.data(), src, len);
	}

private:

	template<to_byteable T>
	static bytearray Convert(const T& from) {
		return from.ToBytes();
	}
	template<from_byteable T>
	static std::pair<T, byteview> Convert(byteview from) {
		T ret;
		byteview view = ret.FromBytes(from);
		return {ret, view};
	}

	// raw memcpy
	static void StoreBytes(bytearray& dest, const void* src, size_t len) {
		auto begin = static_cast<const std::byte*>(src);
		dest.insert(dest.end(), begin, begin + len);
	}
	static void LoadBytes(byteview& view, void* dest, size_t len) {
		std::ranges::copy(view.begin(), view.begin() + len, static_cast<std::byte*>(dest));
		view = view.subspan(len);
	}

	// memcpyable type
	template<memcopyable T>
	static void StoreBytes(bytearray& dest, const T& src) {
		StoreBytes(dest, std::addressof(src), sizeof(src));
	}
	template<memcopyable T>
	static void LoadBytes(byteview& view, T& dest) {
		LoadBytes(view, std::addressof(dest), sizeof(dest));
	}

	// cross convertible type
	template<cross_convertible T>
	static void StoreBytes(bytearray& dest, const T& src) {
		StoreBytes(dest, Convert<T>(src));
	}
	template<cross_convertible T>
	static void LoadBytes(byteview& view, T& dest) {
		auto&& [ret, last] = Convert<T>(view);
		dest = std::move(ret);
		view = last;
	}

	// cross convertible tuple
	template<cross_convertible ...T>
	static void StoreBytes(bytearray& dest, const std::tuple<T...>& src) {
		std::apply([&](auto&& ...args) {
			(StoreBytes(dest, Convert<T>(std::forward<T>(args))), ...);
		}, src);
	}
	template<cross_convertible ...T>
	static void LoadBytes(byteview& view, std::tuple<T...>& dest) {
		std::apply([&](auto&& ...args) {
			auto conv = [&](auto&& arg) {
				auto&& [ret, last] = Convert<decltype(arg)>(view);
				arg = std::move(ret);
				view = last;
			};
			(conv(std::forward<decltype(args)>(args)), ...);
		}, dest);
	}

	// memcpyable container
	template<std::ranges::contiguous_range R, memcopyable T = std::ranges::range_value_t<R>>
	static void StoreBytes(bytearray& dest, const R& src) {
		uint32_t size = std::ranges::size(src);
		StoreBytes(dest, size);
		StoreBytes(dest, std::ranges::size(src), sizeof(T) * size);
	}
	template<std::ranges::contiguous_range R, memcopyable T = std::ranges::range_value_t<R>>
	static void LoadBytes(byteview& view, R& dest) {
		uint32_t size = 0;
		LoadBytes(view, size);
		dest.resize(size);
		LoadBytes(view, std::data(dest), size);
	}

	// general cross convertible container
	template<std::ranges::range R, cross_convertible T = std::ranges::range_value_t<R>>
	static void StoreBytes(bytearray& dest, const R& src) {
		uint32_t size = src.size();
		StoreBytes(dest, size);
		for (auto&& elem : src) {
			StoreBytes(dest, elem);
		}
	}
	template<std::ranges::range R, cross_convertible T = std::ranges::range_value_t<R>>
	static void LoadBytes(byteview& view, R& dest) {
		uint32_t size = 0;
		LoadBytes(view, dest);
		auto back = std::inserter(dest, std::ranges::end(dest));
		for (size_t i = 0; i < size; ++i) {
			T ret;
			LoadBytes(view, ret);
			*(back++) = std::move(ret);
		}
	}

	// partial specialization
	template<memcopyable T, size_t N>
	static void StoreBytes(bytearray& dest, const std::array<T, N>& src) {
		StoreBytes(dest, std::apply([](auto ...args) { return std::make_tuple(args...); }, src));
	}
	template<memcopyable T, size_t N>
	static void LoadBytes(byteview& view, std::array<T, N>& dest) {
		LoadBytes(view, std::apply([](auto&& ...args) { return std::tie(std::forward<decltype(args)>(args)...); }, dest));
	}
};

template <typename... Types>
class BinaryLayouts {

	using types = std::variant<std::monostate, Types...>;
	types m_data;

	template<typename Ty>
	static constexpr bool is_abletype = metatuple::type_find_v<metatuple::args<Types...>, Ty>;

	template <size_t... Is>
	static constexpr types from_index_impl(size_t idx, std::index_sequence<Is...>) {
		constexpr types table[] = {
			[]() {
				return types(std::in_place_index<Is>);
			}...
		};
		return table[idx];
	}

public:
	template <typename Ty, std::enable_if_t<is_abletype<Ty>, int> = 0>
	explicit BinaryLayouts(Ty&& from) : m_data(std::forward<Ty>(from)) {
	}

	BinaryLayouts(const BinaryData& from) {
		
	}

	bool valid() const {
		return !(m_data.index() == 0 || m_data.index() == std::variant_npos);
	}
	operator bool() const {
		return valid();
	}

	auto get() -> decltype(std::get(m_data)) {
		return std::get(m_data);
	}
};

} // namespace libarrier

#endif // LIBARRIER_BINARYDATA_HPP