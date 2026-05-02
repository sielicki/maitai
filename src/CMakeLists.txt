add_library(maitai MODULE)
target_include_directories(maitai PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/checks
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
)
target_sources(maitai PRIVATE
  module.cc
  checks/maitai_example.cc
  checks/maitai_avoid_memset.cc
)
target_compile_features(maitai PRIVATE cxx_std_23)

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(maitai PRIVATE -fno-rtti -fno-exceptions)
endif()

target_link_libraries(maitai PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangTidy
)

if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(maitai PRIVATE LLVM)
else()
  target_link_libraries(maitai PRIVATE LLVMOption LLVMSupport LLVMTarget)
endif()

include(GNUInstallDirs)
install(
  TARGETS maitai
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/clang-tidy
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
