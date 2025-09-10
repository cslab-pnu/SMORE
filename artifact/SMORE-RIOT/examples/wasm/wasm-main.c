/*
 * Copyright (C) 2020 TU Bergakademie Freiberg Karl Fessel
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*provide some test program*/
#include "blob/test.wasm.h"
#include "blob/hello.wasm.h"

#define DWT_CYCCNT (*(uint32_t *) 0xe0001004)

bool iwasm_runtime_init(void);
void iwasm_runtime_destroy(void);

/* wamr_run is a very direct interpretation of "i like to have a wamr_run" */
int wamr_run(void *bytecode, size_t bytecode_len, int argc, char **argv);

/* wamr_run_cp creates a copy bytecode and argv
 * if argc is 0 it is set to 1 and argv[0] is set to ""
 * to create some space for a return value */
int wamr_run_cp(const void *bytecode, size_t bytecode_len, int argc, const char **argv);

#define telltruth(X) ((X) ? "true" : "false")

int main(void)
{
	enable_dwt();	
  __asm__ volatile(
	"mov	r9, #0x8000	\n"
	"movt	r9, #0x2000	\n"
	"msr	psplim, r9	\n"
	"mov	r9, #0		\n"
	"msr	msplim, r9	\n"
	"movt	r9, #0x2003	\n"
	"msr	psp, r9		\n"
  );



    printf("iwasm_initilised: %s\n", telltruth(iwasm_runtime_init()));

    int app_argc = 2;
    const char *app_argv[] = {"test", "bob"};
    int ret = wamr_run_cp(test_wasm, test_wasm_len, app_argc, app_argv);
    printf("ret = %d\n", ret);
    ret = wamr_run_cp(hello_wasm, hello_wasm_len, app_argc, app_argv);
    printf("ret = %d\n", ret);

    iwasm_runtime_destroy();
  unsigned psplim, msplim, psp;
  __asm__ volatile(
	"mrs	%0, psplim	\n"
	"mrs	%1, msplim	\n"
	"mrs	%2, psp		\n"
	:"=r"(psplim), "=r"(msplim), "=r"(psp)::
  );
  printf("psplim: 0x%lx\tmsplim: %u\tpsp: %u\n", psplim, msplim, psp);
  printf("stack peak: %u\tplaceRedzone: %u\tshiftRedzone: %u\n", 0x20008000-psplim, msplim/8, (0x20030000-psp)/8);


	uint32_t clock = DWT_CYCCNT;
	printf("wasm clock cycle: %u\n", clock);
}
