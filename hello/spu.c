
#include "common/types.h"

#include <stdio.h>
#include <unistd.h>

int main(ull id, ull argp, ull envp)
{
	printf(" [SPU]: Hello World! from %llu, argp = %llu\n", id, argp);

	/* Delay the exit() so that the PPU has enough time to
	   read the physical SPU id */
	sleep(1);
	
	return 0;
}

