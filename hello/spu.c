
#include "common/types.h"
#include "common/compiler.h"

#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include <stdio.h>
#include <unistd.h>

int main(ull id, ull argp, ull envp)
{
	unsigned int cmd;

	printf(" [SPU]: Hello World! from %llu, argp = %llu\n", id, argp);

	/* Wait for the PPU to tell us we can exit. This is necessary to
         * ensure that the PPU has enough time to go through /syfs
	 */
	cmd = spu_read_in_mbox();

	if (unlikely(1 != cmd))
		printf(" [SPU]: Invalid command received in mailbox\n");

	return 0;
}

