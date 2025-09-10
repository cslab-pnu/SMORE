/*
 * Copyright (C) 2017 Inria
 *               2017 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example of how to use javascript on RIOT
 *
 * @author      Emmanuel Baccelli <emmanuel.baccelli@inria.fr>
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

/* include header generated from main.js */
#include "blob/main.js.h"

#define DWT_CYCCNT (*(uint32_t *) 0xe0001004)

int js_run(const jerry_char_t *script, size_t script_size)
{

    jerry_value_t parsed_code, ret_value;
    int res = 0;

    /* Initialize engine, no flags, default configuration */

    jerry_init(JERRY_INIT_EMPTY);

    /* Register the print function in the global object. */

    jerryx_handler_register_global((const jerry_char_t *) "print",
                                   jerryx_handler_print);

    /* Setup Global scope code */

    parsed_code = jerry_parse(NULL, 0, script, script_size, JERRY_PARSE_NO_OPTS);

    if (!jerry_value_is_error(parsed_code)) {
        /* Execute the parsed source code in the Global scope */
        ret_value = jerry_run(parsed_code);
        if (jerry_value_is_error(ret_value)) {
            printf("js_run(): Script Error!");
            res = -1;
        }
        jerry_release_value(ret_value);
    }

    jerry_release_value(parsed_code);

    /* Cleanup engine */
    jerry_cleanup();

    return res;
}

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


    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    printf("Executing main.js:\n");

    js_run(main_js, main_js_len);

  unsigned psplim, msplim, psp;
  __asm__ volatile(
	"mrs	%0, psplim	\n"
	"mrs	%1, msplim	\n"
	"mrs	%2, psp		\n"
	:"=r"(psplim), "=r"(msplim), "=r"(psp)::
  );
  printf("psplim: 0x%lx\tmsplim: %u\tpsp: %u\n", psplim, msplim, psp);
  printf("stack peak: %u\tplaceRedzone: %u\tshiftRedzone: %u\n", 0x20008000-psplim, msplim/8, (0x20030000-psp)/8);



	unsigned int clock = DWT_CYCCNT;
	printf("javascript clock cycle: %u\n", clock);

    return 0;
}
