#pragma clang optimize off 
// RUN: %clangxx_asan -O0 %s -o %t
// RUN: not %run %t 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

struct A {
  int a[8];
};

int bar(struct A *a) {
  int *volatile ptr = &a->a[0];
  return *(ptr - 1);
}

void foo(struct A a) {
  bar(&a);
}

int main() {
  foo((struct A){0});
  return 0;
}

// CHECK: ERROR: AddressSanitizer: stack-buffer-underflow
// CHECK: READ of size 4 at
// CHECK: is located in stack of thread
#pragma clang optimize off 
