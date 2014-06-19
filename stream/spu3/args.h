
#ifndef CELL_CODING_STREAM_SPU1_ARGS_H
#define CELL_CODING_STREAM_SPU1_ARGS_H 1

struct spu3_kernel_args {
	int		N;
	float *restrict	a;
	float *restrict	b;
	float *restrict	c;
	float		scalar;
} __attribute__((aligned(128)));

#endif

