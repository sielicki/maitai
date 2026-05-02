// RUN: %clang_tidy -checks='-*,mychecks-example' -load %mychecks_plugin %s -- > %t 2>&1 || true
// RUN: %FileCheck %s < %t

void DeleteMe() {}
// CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: function 'DeleteMe' should not exist [mychecks-example]
