#include "libarrier.h"

#include "include/libarrier/textfile_reader.hpp"
//#include <map>
#include <random>
#include <format>

using namespace libarrier;

int main(int argc, char** argv) {


	for (auto&& entry : std::filesystem::directory_iterator("S:\\MyMakingProject\\NowCoding\\libarrier\\out\\build\\x64-debug\\test")) {
		auto&& p = entry.path();
		TextfileReader reader(p);
	}


	return 0;
}
