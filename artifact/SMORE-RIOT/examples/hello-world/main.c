/*
 * Copyright (C) 2014 Freie Universität Berlin
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
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include "mpu_defs.h"
#include "mpu_prog.h"
#include "cpu.h"

#define DEMCR (*(uint32_t *) 0xE000EDFC)
#define DWT_CTRL (*(uint32_t *) 0xE0001000)
#define DWT_CYCCNT (*(uint32_t *) 0XE0001004)

void decode_instruction(uint32_t instruction) {
	uint32_t cond = (instruction >> 28) & 0xF;
	uint32_t opcode = (instruction >> 24) & 0xF;
	uint32_t Rn = (instruction >> 16) & 0XF;
	uint32_t Rd = (instruction >> 12) & 0xF;
	uint32_t immediate = instruction & 0xFFF;

	printf("Rn: R%d, Rd: R%d, Immediate: %X\n", Rn, Rd, immediate);
}

void handle_interrupt(uint32_t pc, uint8_t thumb_mode) {
	uint32_t actual_pc = pc - 8;
	uint32_t instruction = *((uint32_t*) actual_pc);
	decode_instruction(instruction);
}

uint32_t calculate_bitmap(uint32_t addr) {
	uint32_t bitmap = 0x20000000 + (addr & 0xffff) /0x20;

	printf("bitmap = 0x%lx\n", bitmap);
	return bitmap;
}

int main(void) {
	//printf("Hello world!\n");

	calculate_bitmap(0x20007fc0);
	calculate_bitmap(0x20007f40);
	calculate_bitmap(0x20007ec0);
	calculate_bitmap(0x20007e40);
	calculate_bitmap(0x20007dc0);
	calculate_bitmap(0x20007d40);

	/*uint32_t addr = 0x2000ffff;
	uint32_t addr2 = 0x20007ffe;
	addr2 = calculate_bitmap(addr2);
	char msg[] = "hello world!";
	uint8_t size = sizeof(msg);

	printf("%u, %u, %s, %d, %d\n", addr, addr2, msg, sizeof(msg), size);
	puts("hello world!\n");
	printf("%u, %u, %s", addr, addr2, msg);
	printf("%d, %d\n", sizeof(msg), size);*/

    return 0;
}
