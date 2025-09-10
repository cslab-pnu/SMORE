/* Common main.c for the benchmarks

   Copyright (C) 2014 Embecosm Limited and University of Bristol
   Copyright (C) 2018 Embecosm Limited

   Contributor: James Pallister <james.pallister@bristol.ac.uk>
   Contributor: Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of the Bristol/Embecosm Embedded Benchmark Suite.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: GPL-3.0-or-later */

#include "support.h"
#include "xtimer.h"

#define DWT_CYCCNT (*(uint32_t *) 0xE0001004)

extern int initialise_benchmark (void);
extern int verify_benchmark (int unused);

int
main (int   argc __attribute__ ((unused)),
      char *argv[] __attribute__ ((unused)) )
{
  int i;
  volatile int result;
  int correct;

  //initialise_board ();
  initialise_benchmark ();
  //start_trigger ();
  //uint32_t start = xtimer_now_usec();
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

  for (i = 0; i < 1; i++)
    {
      initialise_benchmark ();
      result = benchmark ();
    }

  //stop_trigger ();
  //uint32_t end = xtimer_now_usec();

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
  /* bmarks that use arrays will check a global array rather than int result */
  //printf("ud timer value : %dms\n", end-start);
  printf("ud clock cycle : %d\n", clock);
  correct = verify_benchmark (result);

  return (!correct);

}	/* main () */

/*
   Local Variables:
   mode: C++
   c-file-style: "gnu"
   End:
*/
