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

add_library(hacoe-ir-features OBJECT lib/IRFeatures.cpp)
set_target_properties(hacoe-ir-features PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(
  hacoe-ir-features
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(hacoe-ir-features PUBLIC cxx_std_20)
target_compile_options(hacoe-ir-features PRIVATE -Wall -Wextra -Wpedantic)

add_executable(hacoe-ir-analyzer ir_analyzer/analyzer.cpp)
llvm_map_components_to_libnames(
  HACOE_LLVM_LIBS
  Analysis
  Core
  IRReader
  Support
)
target_link_libraries(
  hacoe-ir-analyzer
  PRIVATE hacoe-ir-features ${HACOE_LLVM_LIBS}
)
target_compile_options(hacoe-ir-analyzer PRIVATE -Wall -Wextra -Wpedantic)

add_subdirectory(llvm_passes)

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  find_program(
    HACOE_OPT_EXECUTABLE
    NAMES opt
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED
  )

  add_test(NAME profiler-json COMMAND hacoe-profiler --json)
  add_test(
    NAME ir-feature-contract
    COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_ir_features.py"
      --analyzer "$<TARGET_FILE:hacoe-ir-analyzer>"
      --opt "${HACOE_OPT_EXECUTABLE}"
      --plugin "$<TARGET_FILE:FeatureExtractor>"
      --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/feature_sample.ll"
  )
endif()
