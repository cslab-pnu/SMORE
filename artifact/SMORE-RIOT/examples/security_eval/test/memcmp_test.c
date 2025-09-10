// RUN: %clangxx_asan -O0 %s -o %t && not %run %t 2>&1 | FileCheck %s
#pragma clang optimize off
// RUN: %clangxx_asan -O1 %s -o %t && not %run %t 2>&1 | FileCheck %s
// RUN: %clangxx_asan -O2 %s -o %t && not %run %t 2>&1 | FileCheck %s
// RUN: %clangxx_asan -O3 %s -o %t && not %run %t 2>&1 | FileCheck %s

// REQUIRES: compiler-rt-optimized

#include <string.h>
int main() {
	int argc = 22;

  char a1[] = {(char)argc, 2, 3, 4};
  char a2[] = {1, (char)(2*argc), 3, 4};
  int res = memcmp(a1, a2, 4 + argc);  // BOOM
  // CHECK: AddressSanitizer: stack-buffer-overflow
  // CHECK: {{#1.*memcmp}}
  // CHECK: {{#2.*main}}
  printf("Result: %d\n", res);
  return res;
}
#pragma clang optimize off
