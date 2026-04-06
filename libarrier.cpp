#include "libarrier.h"

#include "include/libarrier/match.hpp"

int main() {
	using namespace libarrier;

	union_t<int, char, std::string> data;

	type_match match{[](int i) {
		return i;
	}, [](char c) {
		return (int)c;
	}, [](const std::string& s) {
		return (int)s.size();
	}};
	auto ret = data | match;

	return 0;
}
