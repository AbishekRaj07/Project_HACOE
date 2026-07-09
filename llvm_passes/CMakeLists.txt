cmake_minimum_required(VERSION 3.13.4)
project(HACOE_Passes)

# Find LLVM installation on Arch Linux
find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Force C++17/C++20
set(CMAKE_CXX_STANDARD 20)

# Build the Feature Extractor Pass (Shared Library for 'opt' to load)
add_library(FeatureExtractor MODULE FeatureExtractorPass.cpp)
target_link_libraries(FeatureExtractor PRIVATE LLVM)

# Build the Hardware Aware Pass
add_library(HardwareAware MODULE HardwareAwarePass.cpp)
target_link_libraries(HardwareAware PRIVATE LLVM)

# Mac/Linux plugin setup
if(APPLE)
  set_target_properties(FeatureExtractor PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  set_target_properties(HardwareAware PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
