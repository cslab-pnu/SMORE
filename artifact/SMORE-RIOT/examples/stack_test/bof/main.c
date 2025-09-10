#include <stdio.h>
#include <string.h>

#include "test.h"

__attribute__((optnone))
int main()
{
    char x[10];
    memset(x, 0, 10);

	int temp = DUMMY_LEN;
    x[DUMMY_LEN * 10] = temp;

	int res = x[DUMMY_LEN * 10];

	if(res == 0){
		printf("Hello World\n");
	}

    return res;
}

