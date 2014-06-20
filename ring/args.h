
#ifndef CELL_CODING_STREAM_SPU1_ARGS_H
#define CELL_CODING_STREAM_SPU1_ARGS_H 1

#include <libsync.h>

#ifndef MAX_SPUS
# define MAX_SPUS 16
#endif

struct spu_program_args {
	int	nspus;
	int	rank;
	/*
	 * Array of addresses of the memory mapped
	 * local stores.
	 */
	ull	ls[MAX_SPUS];

	ull	barrier[2];

} __attribute__((aligned(128)));

#endif

