
#include "common/types.h"

#include <stdio.h>

#define FLT 	float
#define LEN 	(32*1024*1024/sizeof(float))
#define NTIMES 	10

static inline void stream_copy(int N, float *restrict a, float *restrict c)
{
	int i;

	for (i = 0; i < N; ++i) {
		c[i] = a[i];
	}
}

static inline void stream_scale(int N, float *restrict b, float *restrict c, 
                                float scalar)
{
	int i;

	for (i = 0; i < N; ++i) {
		b[i] = scalar*c[i];
	}
}

static inline void stream_add(int N, float *restrict a, float *restrict b, 
                              float *restrict c)
{
	int i;

	for (i = 0; i < N; ++i) {
		c[i] = a[i] + b[i];
	}
}

static inline void stream_triad(int N, float *restrict a, float *restrict b, 
                                float *restrict c, float scalar)
{
	int i;

	for (i = 0; i < N; ++i) {
		a[i] = b[i] + scalar*c[i];
	}
}

#include "stream/stream.h"

int main(int argc, char **argv)
{
	double times[NTIMES-1][4];
	float *restrict a;
	float *restrict b;
	float *restrict c;
	const float scalar = 3;
	int retval;

	retval = stream_alloc_arrays_on_ppu(LEN, 128, &a, &b, &c);
	if (unlikely(retval)) {
		fprintf(stderr, " stream_alloc_arrays_on_ppu failed with error %d.\n", retval);
		return 1;
	}

	stream_fill_arrays_on_ppu(LEN, a, b, c);
	
	printf(" Starting kernel.\n");
	stream_execute_main_loop_on_ppu(LEN, NTIMES, a, b, c, scalar, times);
	
	stream_print_results_on_ppu(LEN, NTIMES, times);
	stream_validate_arrays_on_ppu(LEN, NTIMES, 1e-5, a, b, c, scalar);

	stream_free_arrays_on_ppu(&a, &b, &c);

	return 0;
}
