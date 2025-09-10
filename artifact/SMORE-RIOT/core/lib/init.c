/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *               2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     core_internal
 * @{
 *
 * @file
 * @brief       Platform-independent kernel initialization
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "auto_init.h"
#include "irq.h"
#include "kernel_init.h"
#include "log.h"
#include "periph/pm.h"
#include "thread.h"
#include "stdio_base.h"

#if IS_USED(MODULE_VFS)
#include "vfs.h"
#endif

#define ENABLE_DEBUG 0
#include "debug.h"

#ifndef CONFIG_BOOT_MSG_STRING
#define CONFIG_BOOT_MSG_STRING "main(): This is RIOT! (Version: " \
    RIOT_VERSION ")"
#endif

#define DEMCR (*(volatile uint32_t *) 0xE000EDFC)
#define DWT_CTRL (*(volatile uint32_t *) 0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *) 0xE0001004)
#define DWT_CPICNT (*(volatile uint32_t *) 0xE0001008)
#define DWT_EXCCNT (*(volatile uint32_t *) 0xE000100C)
#define DWT_SLEEPCNT (*(volatile uint32_t *) 0xE0001010)
#define DWT_LSUCNT (*(volatile uint32_t *) 0xE0001014)
#define DWT_FOLDCNT (*(volatile uint32_t *) 0xE0001018)

#define DWT_COMP0 (*(volatile uint32_t *) 0xE0001020)
#define DWT_FUNCTION0 (*(volatile uint32_t *) 0xE0001028)
#define DWT_COMP1 (*(volatile uint32_t *) 0xE0001030)
#define DWT_FUNCTION1 (*(volatile uint32_t *) 0xE0001038)

void enable_dwt(void){
 if(DWT_CTRL != 0){
    DEMCR |= (1 << 24);
    DWT_CYCCNT = 0;
    DWT_CTRL |= 1;
  }
}

void calc_mem(void) {
	__asm__ volatile(
		"mrs	r9, psplim	\n"
		"cmp	sp, r9		\n"
		"it		lt			\n"
		"movlt	r9, sp		\n"
		"msr	psplim, r9	\n"
	);
}

#ifdef SMORE_EXT
// SMORE-Ext
void placeRedzone() {
	__asm__ volatile(
		"cpsid	i				\n"
		"mov	r9, #8			\n"
		
		"push	{r4-r6, r8}		\n"
	
		// disable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		// set redzone for current frame
		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldm	r4, {r5, r6}	\n" // r5, r6: rnr, rz for last frame
		"add	r7, sp, #19		\n" // 4*3+3
		"str	r7, [r4, #4]	\n"
		"str	r7, [r4, #8]	\n"
		"mov	r7, #4			\n"
		"str	r7, [r4, #0]	\n"

          // store poison value in previous redzone
        "movw   r7, #0x0        \n"
        "movt   r7, #0x2000     \n"
        "ldr    r8, [r7]        \n"
        "str    r8, [r6, #-3]   \n"
        "ldr    r8, [r7, #4]    \n"
        "str    r8, [r6, #1]    \n"
        "ldr    r8, [r7, #8]    \n"
        "str    r8, [r6, #5]    \n"
        "ldr    r8, [r7, #12]   \n"
        "str    r8, [r6, #9]    \n"
        "ldr    r8, [r7, #16]   \n"
        "str    r8, [r6, #13]   \n"
        "ldr    r8, [r7, #20]   \n"
        "str    r8, [r6, #17]   \n"
        "ldr    r8, [r7, #24]   \n"
        "str    r8, [r6, #21]   \n"
        "ldr    r8, [r7, #28]   \n"
        "str    r8, [r6, #25]   \n"

		// enable bytemap write
		"add	r7, #3			\n" // r7 = 0x20000003
		"str	r7, [r4, #28]	\n"
		// mask previous redzone
		"lsrs	r6, r6, #5		\n"
		"mov.w	r7, #0x40000	\n"
		"bfi	r6, r7, #11, #21\n"
		"mov	r7, #1			\n"
		"strb	r7, [r6]		\n"
		// disable bytemap write
		"movw	r7, #0x7		\n"
		"movt	r7, #0x2000		\n"
		"str	r7, [r4, #28]	\n"

		// enlarge poison value region
		"add	r5, #1			\n"
		"cmp	r5, #5			\n"
		"it		eq				\n"
		"moveq	r5, #0			\n"
		"str	r5, [r4, #0]	\n"
		"ldr	r7, [r4, #4]	\n"
		"add	r7, #32			\n"
		"mov	r6, #5			\n"
		"stm	r4, {r6, r7}	\n"
		// reduce stack region
		"sub	r7, #4			\n"		
		"str	r7, [r4, #24]	\n"

		// restore rnr
		"str	r5, [r4]		\n"

		// enable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n"	
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"pop	{r4-r6, r8}		\n"

		"cmp	r9, #8			\n"
		"bne	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);
}

void restoreRedzone() {
	__asm__ volatile(
		"cpsid	i				\n"
		"mov	r9, #8			\n"

		"push	{r4-r6}			\n"

		// disable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"

		// enable bytemap write
		"mov	r6, #4			\n"
		"str	r6, [r4]		\n"
		"movw	r7, #0x3		\n"
		"movt	r7, #0x2000		\n"
		"str	r7, [r4, #28]	\n"
		// mask redzone
		"add	r6, sp, #111	\n" // 96 + push regs*4+3
		"lsrs	r4, r6, #5		\n"
		"mov.w	r7, #0x40000	\n"
		"bfi	r4, r7, #11, #21\n"
		"check_redzone:			\n"
		"ldrb	r7, [r4]		\n"
		"cmp	r7, #1			\n"
		"beq	redzone			\n"
		"add	r6, r6, #32		\n" // r6 has redzone addr
		"add	r4, #1			\n"
		"b		check_redzone	\n"
		"redzone:				\n"
		"movw	r7, #0			\n"
		"strb	r7, [r4]		\n"
		// disable bytemap write
		"movw	r7, #0x7		\n"
		"movt	r7, #0x2000		\n"
		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"str	r7, [r4, #28]	\n"

		// reduce poison value region
		"add	r7, r6, #32		\n"
		"str	r7, [r4, #12]	\n"
		// enlarge stack region
		"sub	r7, #4			\n"
		"str	r7, [r4, #24]	\n"

		// set redzone in return frame
		"sub	r5, #1			\n"
		"cmp	r5, #-1			\n"
		"it		eq				\n"
		"moveq	r5, #4			\n"
		"stm	r4, {r5, r6}	\n"
		"str	r6, [r4, #8]	\n"

		// enable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"pop	{r4-r6}			\n"

		"cmp	r9, #8			\n"
		"bne	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);
}

#else
// SMORE
void placeRedzone() {
	__asm__ volatile(
		"mrs	r9, msplim		\n"
		"add	r9, #8			\n"
		"msr	msplim, r9		\n"

		"cpsid	i				\n"
		"mov	r9, #8			\n"
		
		"push	{r4-r6}			\n"
	
		// disable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		// set redzone for current frame
		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldm	r4, {r5, r6}	\n" // r5, r6: rnr, rz for last frame
		"add	r7, sp, #15		\n" // 4*3+3
		"str	r7, [r4, #4]	\n"
		"str	r7, [r4, #8]	\n"
		"mov	r7, #4			\n"
		"str	r7, [r4, #0]	\n"

		// enable bytemap write
		"mov	r7, #0x3		\n"
		"movt	r7, #0x2000		\n"
		"str	r7, [r4, #28]	\n"
		// mask previous redzone
		"lsrs	r6, r6, #5		\n"
		"mov.w	r7, #0x40000	\n"
		"bfi	r6, r7, #11, #21\n"
		"mov	r7, #1			\n"
		"strb	r7, [r6]		\n"
		// disable bytemap write
		"movw	r7, #0x7		\n"
		"movt	r7, #0x2000		\n"
		"str	r7, [r4, #28]	\n"

		// enlarge poison value region
		"add	r5, #1			\n"
		"cmp	r5, #5			\n"
		"it		eq				\n"
		"moveq	r5, #0			\n"
		"str	r5, [r4, #0]	\n"
		"ldr	r7, [r4, #4]	\n"
		"add	r7, #32			\n"
		"mov	r6, #5			\n"
		"stm	r4, {r6, r7}	\n"
		// reduce stack region
		"sub	r7, #4			\n"		
		"str	r7, [r4, #24]	\n"

		// restore rnr
		"str	r5, [r4]		\n"

		// enable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n"	
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"pop	{r4-r6}			\n"

		"cmp	r9, #8			\n"
		"bne	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);
}

void restoreRedzone() {
	__asm__ volatile(
		"cpsid	i				\n"
		"mov	r9, #8			\n"

		"push	{r4-r6}			\n"

		// disable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"

		// enable bytemap write
		"mov	r6, #4			\n"
		"str	r6, [r4]		\n"
		"movw	r7, #0x3		\n"
		"movt	r7, #0x2000		\n"
		"str	r7, [r4, #28]	\n"
		// mask redzone
		"add	r6, sp, #111	\n" // 96 +push regs*4+3
		"lsrs	r4, r6, #5		\n"
		"mov.w	r7, #0x40000	\n"
		"bfi	r4, r7, #11, #21\n"
		"check_redzone:			\n"
		"ldrb	r7, [r4]		\n"
		"cmp	r7, #1			\n"
		"beq	redzone			\n"
		"add	r6, r6, #32		\n" // r6 has redzone addr
		"add	r4, #1			\n"
		"b		check_redzone	\n"
		"redzone:				\n"
		"movw	r7, #0			\n"
		"strb	r7, [r4]		\n"
		// disable bytemap write
		"movw	r7, #0x7		\n"
		"movt	r7, #0x2000		\n"
		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"str	r7, [r4, #28]	\n"

		// reduce poison value region
		"add	r7, r6, #32		\n"
		"str	r7, [r4, #12]	\n"
		// enlarge stack region
		"sub	r7, #4			\n"
		"str	r7, [r4, #24]	\n"

		// set redzone in return frame
		"sub	r5, #1			\n"
		"cmp	r5, #-1			\n"
		"it		eq				\n"
		"moveq	r5, #4			\n"
		"stm	r4, {r5, r6}	\n"
		"str	r6, [r4, #8]	\n"

		// enable debug event
		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"pop	{r4-r6}			\n"

		"cmp	r9, #8			\n"
		"bne	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);
}
#endif

void shiftRedzone() {
	__asm__ volatile(
		"mrs	r9, psp			\n"
		"sub	r9, #8			\n"
		"msr	psp, r9			\n"

		"cpsid	i				\n"
		"mov	r9, #8			\n" // save magic value

		"push	{r4-r6}			\n" 
		"add	sp, #12			\n"

		"movw	r4, #0x1028		\n" 
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n" // disable debug event
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4, #0]	\n" // r5 has rnr
		"mov	r6, #0			\n"
		"str	r6, [r4, #0]	\n"

		"ldr	r6, [r4, #4]	\n"
		"cmp	r6, sp			\n"
		"bgt	s1				\n"
		"sub	r6, r6, r7		\n"
		"str	r6, [r4, #4]	\n"
		"str	r6, [r4, #8]	\n"

		"s1:					\n"
		"ldr	r6, [r4, #12]	\n"
		"cmp	r6, sp			\n"
		"bgt	s2				\n"
		"sub	r6, r6, r7		\n"
		"str	r6, [r4, #12]	\n"
		"str	r6, [r4, #16]	\n"

		"s2:					\n"
		"ldr	r6, [r4, #20]	\n"
		"cmp	r6, sp			\n"
		"bgt	s3				\n"
		"sub	r6, r6, r7		\n"
		"str	r6, [r4, #20]	\n"
		"str	r6, [r4, #24]	\n"

		"s3:					\n"
		"ldr	r6, [r4, #28]	\n"
		"cmp	r6, sp			\n"
		"bgt	s4				\n"
		"sub	r6, r6, r7		\n"
		"str	r6, [r4, #28]	\n"
		"str	r6, [r4, #32]	\n"

		"s4:					\n"
		"mov	r6, #4			\n"
		"str	r6, [r4]		\n"
		"ldr	r6, [r4, #4]	\n"
		"cmp	r6, sp			\n"
		"bgt	shift_end		\n"
		"sub	r6, r6, r7		\n"
		"str	r6, [r4, #4]	\n"
		"str	r6, [r4, #8]	\n"

		"shift_end:				\n"
		"str	r5, [r4, #0]	\n" // restore rnr

		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n" // enable debug event
		"str.w	r5, [r4]		\n"	
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"sub	sp, #12			\n"
		"pop	{r4-r6}			\n"

		"cmp	r9, #8			\n" // check magic value
		"bne	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);
} 


void unshiftRedzone() {
	__asm__ volatile(
		"cpsid	i				\n"
		"mov	r9, #8			\n" // save magic value
		
		"push	{r4-r6}			\n" 
		"add	sp, #12			\n"

		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n" // disable debug event
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4, #0]	\n" // r5 has rnr
		"mov	r6, #0			\n"
		"str	r6, [r4, #0]	\n"

		"ldr	r6, [r4, #4]	\n"
		"cmp	r6, sp			\n"
		"bgt	u1				\n"
		"add	r6, r6, r7		\n"
		"str	r6, [r4, #4]	\n"
		"str	r6, [r4, #8]	\n"

		"u1:					\n"
		"ldr	r6, [r4, #12]	\n"
		"cmp	r6, sp			\n"
		"bgt	u2				\n"
		"add	r6, r6, r7		\n"
		"str	r6, [r4, #12]	\n"
		"str	r6, [r4, #16]	\n"

		"u2:					\n"
		"ldr	r6, [r4, #20]	\n"
		"cmp	r6, sp			\n"
		"bgt	u3				\n"
		"add	r6, r6, r7		\n"
		"str	r6, [r4, #20]	\n"
		"str	r6, [r4, #24]	\n"

		"u3:					\n"
		"ldr	r6, [r4, #28]	\n"
		"cmp	r6, sp			\n"
		"bgt	u4				\n"
		"add	r6, r6, r7		\n"
		"str	r6, [r4, #28]	\n"
		"str	r6, [r4, #32]	\n"

		"u4:					\n"
		"mov	r6, #4			\n"
		"str	r6, [r4]		\n"
		"ldr	r6, [r4, #4]	\n"
		"cmp	r6, sp			\n"
		"bgt	unshift_end		\n"
		"add	r6, r6, r7		\n"
		"str	r6, [r4, #4]	\n"
		"str	r6, [r4, #8]	\n"

		"unshift_end:			\n"
		"str	r5, [r4, #0]	\n" // restore rnr

		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n"
		"str.w	r5, [r4]		\n" // enable debug event
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"sub	sp, #12			\n"
		"pop	{r4-r6}			\n" 

		"cmp	r9, #8			\n" // check magic value
		"bne 	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);	
}

void resetRedzone() {
	__asm__ volatile(
		"mrs	r9, psplim		\n"
		"cmp	sp, r9			\n"
		"it		lt				\n"
		"movlt	r9, sp			\n"
		"msr	psplim, r9		\n"

		"cpsid	i				\n"
		"mov	r9, #8			\n"

		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"movw	r5, #0x05		\n"
		"str.w	r5, [r4]		\n"
		"movw	r5, #0x07		\n"
		"str.w	r5, [r4, #16]	\n"

		"movw	r4, #0xed98		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4, #0]	\n"
		"sub	r5, #1			\n"
		"cmp	r5, #-1			\n"
		"it		eq				\n"
		"moveq	r5, #4			\n" // end

		"sub	r7, r7, #64		\n"
		"str	r5, [r4, #0]	\n"
		"ldr	r6, [r4, #4]	\n"
		"cmp	r6, r7			\n"
		"bgt	reset_end		\n"
		"mov	r6, r5			\n"

		"find_start:			\n"
		"sub	r6, #1			\n"
		"cmp	r6, #-1			\n"
		"it		eq				\n"
		"moveq	r6, #4			\n"
		"str	r6, [r4, #0]	\n"
		"ldr	r8, [r4, #4]	\n"
		"cmp	r7, r8			\n"
		"bgt	find_start		\n"

		"add	r6, #1			\n"
		"cmp	r6, #5			\n"
		"it		eq				\n"
		"moveq	r6, #0			\n"
		"sub	r8, sp, #61		\n"
		"str	r6, [r4, #0]	\n"
		"str	r8, [r4, #4]	\n"
		"str	r8, [r4, #8]	\n"	

		"reset:					\n"
		"cmp	r6, r5			\n"
		"beq	reset_end		\n"
		"sub	r8, #96			\n"
		"add	r6, #1			\n"
		"cmp	r6, #5			\n"
		"it		eq				\n"
		"moveq	r6, #0			\n"
		"str	r6, [r4, #0]	\n"
		"str	r8, [r4, #4]	\n"
		"str	r8, [r4, #8]	\n"
		"b		reset			\n"

		"reset_end:				\n"
		"add	r5, #1			\n"
		"cmp	r5, #5			\n"
		"it		eq				\n"
		"moveq	r5, #0			\n"
		"str	r5, [r4, #0]	\n"

		"movw	r4, #0x1028		\n"
		"movt	r4, #0xe000		\n"
		"ldr	r5, [r4]		\n"
		"ldr	r5, [r4, #16]	\n"
		"movw	r5, #0x15		\n"
		"str.w	r5, [r4]		\n" // enable debug event
		"movw	r5, #0x17		\n"
		"str.w	r5, [r4, #16]	\n"

		"cmp	r9, #8			\n"
		"bne 	dummy_handler_default	\n"
		"mov	r9, #0			\n"
		"cpsie	i				\n"
	);
}

extern int main(void);

static char main_stack[36]; //[THREAD_STACKSIZE_MAIN];
static char idle_stack[THREAD_STACKSIZE_IDLE];

static void *main_trampoline(void *arg)
{
    (void)arg;

    if (IS_USED(MODULE_AUTO_INIT)) {
        auto_init();
    }

	if (!IS_ACTIVE(CONFIG_SKIP_BOOT_MSG) && !IS_USED(MODULE_STDIO_NULL)) {
		LOG_INFO(CONFIG_BOOT_MSG_STRING "\n");
	}

    int res = main();

    if (IS_USED(MODULE_TEST_UTILS_MAIN_EXIT_CB)) {
        void test_utils_main_exit_cb(int res);
        test_utils_main_exit_cb(res);
    }

#ifdef MODULE_TEST_UTILS_PRINT_STACK_USAGE
    void print_stack_usage_metric(const char *name, void *stack, unsigned max_size);
    if (IS_USED(MODULE_CORE_IDLE_THREAD)) {
        print_stack_usage_metric("idle", idle_stack, THREAD_STACKSIZE_IDLE);
    }
#endif

    return NULL;
}

static void *idle_thread(void *arg)
{
    (void)arg;

    while (1) {
        pm_set_lowest();
    }

    return NULL;
}

void kernel_init(void)
{
        main_trampoline(NULL);
        while (1) {}
        return;

    cpu_switch_context_exit();
}

void early_init(void)
{
	/* initialize leds */
	if (IS_USED(MODULE_PERIPH_INIT_LEDS)) {
		extern void led_init(void);
		led_init();
	}

    stdio_init();

#if MODULE_VFS
    vfs_bind_stdio();
#endif
}
