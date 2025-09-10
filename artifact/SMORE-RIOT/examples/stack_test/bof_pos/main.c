#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "test.h"

__attribute__((optnone))
int main()
{
    int idx = atoi(DUMMY_IDX);
    char AAA[10], BBB[10], CCC[10];
    memset(AAA, 0, sizeof(AAA));
    memset(BBB, 0, sizeof(BBB));
    memset(CCC, 0, sizeof(CCC));
    int res = 0;
    char *p = AAA + idx;
    printf("AAA : %p\nBBB: %p\nCCC: %p\np: %p\n", AAA, BBB, CCC, p);

    return *(short *)(p) + BBB[DUMMY_ARGC % 2] + CCC[DUMMY_ARGC % 2];
}

