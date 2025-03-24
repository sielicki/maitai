add_library(maitai MODULE)
target_include_directories(maitai PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/checks>
  $<INSTALL_INTERFACE:include>
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
)
target_sources(maitai PUBLIC module.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/checks/maitai_example.cc)
target_compile_features(maitai PUBLIC cxx_std_17)

if(NOT LLVM_ENABLE_RTTI)
  add_compile_options(-fno-rtti -fno-exceptions)
endif()

target_link_libraries(maitai PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangCodeGen
  clangDriver
  clangFrontend
  clangLex
  clangParse
  clangSema
  clangTidy
)

if(LLVM_LINK_LLVM_DYLIB)
  target_link_libraries(maitai PRIVATE LLVM)
else()
  target_link_libraries(maitai PRIVATE LLVMOption LLVMSupport LLVMTarget LLVMX86AsmParser LLVMX86CodeGen LLVMX86Desc LLVMX86Info)
endif()

include(GNUInstallDirs)
install(
  TARGETS maitai
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/clang-tidy
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
