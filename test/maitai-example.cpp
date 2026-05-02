// RUN: %clang_tidy -checks='-*,maitai-example' -load %maitai_plugin %s -- > %t 2>&1 || true
// RUN: %FileCheck %s < %t

#define MACRO_IMPLEMENTING_MIN(a, b) ((a) < (b) ? (a) : (b))

void test() {
  int x = 1, y = 2;
  int z = MACRO_IMPLEMENTING_MIN(x, y);
  // CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: use std::min instead of MACRO_IMPLEMENTING_MIN macro [maitai-example]
}
