#ifndef LIBARRIER_BINARYDATA_HPP
#define LIBARRIER_BINARYDATA_HPP

#include <span>
#include <type_traits>
#include <variant>
#include <vector>

namespace libarrier {

template<typename... Types>
class BinaryLayout {
	std::variant<std::monostate, Types...> m_data;

public:
};

class BinaryData {
	std::vector<std::byte> m_data{};

public:

	BinaryData() {}
};

} // namespace libarrier

#endif // LIBARRIER_BINARYDATA_HPP