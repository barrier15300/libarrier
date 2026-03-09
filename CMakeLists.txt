# CMakeList.txt : libarrier の CMake プロジェクト。ソースを含めて、次を定義します:
# プロジェクト専用ロジックはこちらです。
#
cmake_minimum_required (VERSION 3.10)

# サポートされている場合は、MSVC コンパイラのホット リロードを有効にします。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("libarrier")

# ソースをこのプロジェクトの実行可能ファイルに追加します。
add_executable (libarrier
	"libarrier.cpp"
	"libarrier.h"

	"include/libarrier/arrier.hpp"
	"include/libarrier/ringbuffer.hpp"
	"include/libarrier/timer.hpp"
	"include/libarrier/textfile_reader.hpp"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET libarrier PROPERTY CXX_STANDARD 20)
endif()

# TODO: テストを追加し、必要な場合は、ターゲットをインストールします。
