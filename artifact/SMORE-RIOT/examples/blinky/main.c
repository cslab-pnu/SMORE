/*
 * Copyright (C) 2021 Otto-von-Guericke-Universität Magdeburg
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
 * @brief       Blinky application
 *
 * @author      Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 *
 * @}
 */

#include <stdio.h>

#include "clk.h"
#include "board.h"
#include "periph_conf.h"
#include "timex.h"
#include "ztimer.h"

#define DWT_CYCCNT (*(uint32_t *) 0xE0001004)

static void delay(void)
{
    /*if (IS_USED(MODULE_ZTIMER)) {
        ztimer_sleep(ZTIMER_USEC, 1 * US_PER_SEC);
    }
    else {*/
        /*
         * As fallback for freshly ported boards with no timer drivers written
         * yet, we just use the CPU to delay execution and assume that roughly
         * 20 CPU cycles are spend per loop iteration.
         *
         * Note that the volatile qualifier disables compiler optimizations for
         * all accesses to the counter variable. Without volatile, modern
         * compilers would detect that the loop is only wasting CPU cycles and
         * optimize it out - but here the wasting of CPU cycles is desired.
         */
        uint32_t loops = coreclk() / 20;
        for (volatile uint32_t i = 0; i < loops; i++) { }
    //}
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



    //while (1) {
	for (int i=0; i<1; i++) {
        delay();
#ifdef LED0_TOGGLE
        LED0_TOGGLE;
#else
        puts("Blink! (No LED present or configured...)");
#endif
    }

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
	printf("blinky clock cycle: %u\n", clock);

    return 0;
}
