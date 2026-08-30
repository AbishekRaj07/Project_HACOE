cmake_minimum_required(VERSION 3.20)

project(HACOE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake from ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

add_executable(hacoe-profiler hardware_profiler/profiler.cpp)
target_compile_options(hacoe-profiler PRIVATE -Wall -Wextra -Wpedantic)

add_executable(hacoe-ir-analyzer ir_analyzer/analyzer.cpp)
llvm_map_components_to_libnames(HACOE_LLVM_LIBS Core IRReader Support)
target_link_libraries(hacoe-ir-analyzer PRIVATE ${HACOE_LLVM_LIBS})
target_compile_options(hacoe-ir-analyzer PRIVATE -Wall -Wextra -Wpedantic)

add_subdirectory(llvm_passes)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME profiler-json COMMAND hacoe-profiler --json)
endif()
