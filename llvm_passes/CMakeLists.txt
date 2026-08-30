add_llvm_pass_plugin(
  FeatureExtractor
  FeatureExtractorPass.cpp
  PARTIAL_SOURCES_INTENDED
)
add_llvm_pass_plugin(
  HardwareAware
  HardwareAwarePass.cpp
  PARTIAL_SOURCES_INTENDED
)

target_compile_features(FeatureExtractor PRIVATE cxx_std_20)
target_compile_features(HardwareAware PRIVATE cxx_std_20)
target_link_libraries(FeatureExtractor PRIVATE hacoe-ir-features)
