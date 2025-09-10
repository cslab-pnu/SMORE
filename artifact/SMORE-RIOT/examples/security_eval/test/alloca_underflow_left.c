// RUN: %clangxx_asan -O0 -mllvm -asan-instrument-dynamic-allocas %s -o %t
#pragma clang optimize off

// RUN: not %run %t 2>&1 | FileCheck %s
//
#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline)) void foo(int index, int len) {
  volatile char str[len] __attribute__((aligned(32)));
  str[index] = '1'; // BOOM
// CHECK: ERROR: AddressSanitizer: dynamic-stack-buffer-overflow on address [[ADDR:0x[0-9a-f]+]]
// CHECK: WRITE of size 1 at [[ADDR]] thread T0
}

int main() {
  foo(-1, 10);
  return 0;
}
#pragma clang optimize on
