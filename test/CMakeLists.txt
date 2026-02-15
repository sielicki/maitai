set(MAITAI_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(LLVM_PLUGIN_EXT ".dylib")
else()
  set(LLVM_PLUGIN_EXT ".so")
endif()

set(MAITAI_PLUGIN "${CMAKE_BINARY_DIR}/src/libmaitai${LLVM_PLUGIN_EXT}")

find_program(CLANG_TIDY_BIN clang-tidy REQUIRED)
get_filename_component(CLANG_TIDY_DIR "${CLANG_TIDY_BIN}" DIRECTORY)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

add_lit_testsuite(check-maitai "Running maitai regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS maitai
)
