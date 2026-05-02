add_library(mychecks MODULE)
target_include_directories(mychecks PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/checks
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
)
target_sources(mychecks PRIVATE
  module.cc
  checks/example.cc
)
target_compile_features(mychecks PRIVATE cxx_std_23)

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(mychecks PRIVATE -fno-rtti -fno-exceptions)
endif()

target_link_libraries(mychecks PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangTidy
)

if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(mychecks PRIVATE LLVM)
else()
  target_link_libraries(mychecks PRIVATE LLVMOption LLVMSupport LLVMTarget)
endif()

include(GNUInstallDirs)
install(
  TARGETS mychecks
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/clang-tidy
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
