//===-- aeabi_dcmp.S - EABI dcmp* implementation ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

// int __aeabi_dcmp{eq,lt,le,ge,gt}(double a, double b) {
//   int result = __{eq,lt,le,ge,gt}df2(a, b);
//   if (result {==,<,<=,>=,>} 0) {
//     return 1;
//   } else {
//     return 0;
//   }
// }

#if defined(COMPILER_RT_ARMHF_TARGET)
#  define CONVERT_DCMP_ARGS_TO_DF2_ARGS                    \
        vmov      d0, r0, r1                     SEPARATOR \
        vmov      d1, r2, r3
#else
#  define CONVERT_DCMP_ARGS_TO_DF2_ARGS
#endif

#define DEFINE_AEABI_DCMP(cond)                            \
        .syntax unified                          SEPARATOR \
        .p2align 2                               SEPARATOR \
DEFINE_COMPILERRT_FUNCTION(__aeabi_dcmp ## cond)           \
        push      { r4, lr }                     SEPARATOR \
        sub       sp, #56                        SEPARATOR \
        tt        r9, sp                         SEPARATOR \
        cmp.w     r9, #0                         SEPARATOR \
        beq       2f                             SEPARATOR \
        bl        placeRedzone                   SEPARATOR \
2:                                               SEPARATOR \
        sub       sp, #64                        SEPARATOR \
        CONVERT_DCMP_ARGS_TO_DF2_ARGS            SEPARATOR \
        bl        SYMBOL_NAME(__ ## cond ## df2) SEPARATOR \
        cmp       r0, #0                         SEPARATOR \
        b ## cond 1f                             SEPARATOR \
        movs      r0, #0                         SEPARATOR \
        add       sp, #64                        SEPARATOR \
        add.w     r9, sp, #65                    SEPARATOR \
        tt        r9, r9                         SEPARATOR \
        and       r9, r9, #255                   SEPARATOR \
        cmp.w     r9, #5                         SEPARATOR \
        bne       3f                             SEPARATOR \
        bl        restoreRedzone                 SEPARATOR \
3:                                               SEPARATOR \
        add       sp, #56                        SEPARATOR \
        pop       { r4, pc }                     SEPARATOR \
1:                                               SEPARATOR \
        movs      r0, #1                         SEPARATOR \
        pop       { r4, pc }                     SEPARATOR \
END_COMPILERRT_FUNCTION(__aeabi_dcmp ## cond)

DEFINE_AEABI_DCMP(eq)
DEFINE_AEABI_DCMP(lt)
DEFINE_AEABI_DCMP(le)
DEFINE_AEABI_DCMP(ge)
DEFINE_AEABI_DCMP(gt)

NO_EXEC_STACK_DIRECTIVE

