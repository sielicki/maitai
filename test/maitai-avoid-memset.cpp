// RUN: %clang_tidy -checks='-*,maitai-avoid-memset' -load %maitai_plugin %s -- > %t 2>&1 || true
// RUN: %FileCheck %s < %t

extern "C" void *memset(void *s, int c, unsigned long n);

void test() {
  char buf[16];
  memset(buf, 0, sizeof(buf));
  // CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: use std::fill_n instead of memset [maitai-avoid-memset]
}
