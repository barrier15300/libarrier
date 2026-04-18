#include "libarrier.h"

#include "include/libarrier/match.hpp"
#include <map>

using namespace libarrier;

int main(int argc, char** argv) {

    union_t<int, char, std::string> data = '8';
    auto ret = data | type_match {
        [](int i) {
            return i;
        },
        [](char c) {
            return (int)c;
        },
        [](const std::string& s) {
            return (int)s.size();
        }
    };

    return 0;
}
