#ifndef LIBARRIER_BINARYDATA_HPP
#define LIBARRIER_BINARYDATA_HPP

#include "meta.hpp"
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace libarrier {

class BinaryData {
public:
    using bytearray = std::vector<std::byte>;

private:
    bytearray m_data {};

public:
    BinaryData(const void* src, size_t len) {
        m_data.resize(len);
        memcpy(m_data.data(), src, len);
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