set(MYCHECKS_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MYCHECKS_PLUGIN "${CMAKE_BINARY_DIR}/src/${CMAKE_SHARED_MODULE_PREFIX}mychecks${CMAKE_SHARED_MODULE_SUFFIX}")

find_program(CLANG_TIDY_BIN clang-tidy REQUIRED)
cmake_path(GET CLANG_TIDY_BIN PARENT_PATH CLANG_TIDY_DIR)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

add_lit_testsuite(check-mychecks "Running mychecks regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS mychecks
)
