#include <stdio.h>
#include <string.h>

#include "test.h"

#define NOINLINE __attribute__((noinline))
inline void break_optimization(void *arg) {
  __asm__ __volatile__("" : : "r" (arg) : "memory");
}

NOINLINE static void Frame0(int frame, char *a, char *b, char *c){
    volatile char s[4] = {0};
    volatile char *d = s;
    break_optimization(&d);
	
	volatile char check = s[0];

	__asm__ volatile(
		"movw r9, #0xed98\n"
		"movt r9, #0xe000\n"
		"mov  r7, #4\n"
		"str  r7, [r9]\n"
		);

    switch(frame){
        case 3:
			a[5]++;
			printf("frame3 done\n");
			break;
        case 2:
			b[5]++;
			printf("frame2 done\n");
			break;
        case 1:
			c[5]++;
			printf("frame1 done\n");
			break;
        case 0:
			d[5]++;
			printf("frame0 done\n");
			break;
		default:
			printf("nothing is done\n");
			break;
    }

	printf("%s %s %s %s\n", a, b, c, d);
}

NOINLINE static void Frame1(int frame, char *a, char *b){
    volatile char c[4] = {0};
	Frame0(frame, a, b, c);
	printf("%s\n", a);
	printf("%s\n", b);
	printf("%s\n", c);
    break_optimization(0);
}

NOINLINE static void Frame2(int frame, char *a){
    volatile char b[4] = {0};
	Frame1(frame, a, b);
	printf("%s\n", a);
	printf("%s\n", b);
    break_optimization(0);
}

NOINLINE static void Frame3(int frame) {
    volatile char a[4] = {0};
	Frame2(frame, a);
	printf("%s\n", a);
    break_optimization(0);
}

__attribute__((optnone))
int main()
{
    if(DUMMY_ARGC != 2) return 1;
    Frame3(0);
}

