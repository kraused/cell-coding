
#ifndef CELL_CODING_STREAM_STREAM_H
#define CELL_CODING_STREAM_STREAM_H 1

#include "common/compiler.h"
#include "common/ppu/walltime.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <tgmath.h>

/*
 * Infrastructure for the implementation of
 * different variants of a STREAM-like benchmark
 * for the memory bandwidth.
 *
 * Implemented as a header online library to
 * allow for defining the floating point type as
 * a preprocessor macro.
 */

/*
 * Allocate the arrays for the STREAM-like benchmark
 * from the PPU. The arrays aligned according the specified
 * boundary.
 * The function returns zero in case of success and 
 * otherwise the negative errno.
 */
static int stream_alloc_arrays_on_ppu(int N,
                                      int align,
                                      FLT *restrict * a,
                                      FLT *restrict * b,
                                      FLT *restrict * c)
{
	*a = NULL;
	*b = NULL;
	*c = NULL;

	*a = memalign(align, N * sizeof(FLT));
	if (unlikely(!*a))
		goto fail;
	*b = memalign(align, N * sizeof(FLT));
	if (unlikely(!*b))
		goto fail;
	*c = memalign(align, N * sizeof(FLT));
	if (unlikely(!*c))
		goto fail;

	return 0;

fail:
	if (*a)
		free(*a);
	if (*b)
		free(*b);
	if (*c)
		free(*c);

	return -errno;
}

/*
 * Deallocate the arrays allocated in the function
 * stream_alloc_arrays_on_ppu().
 */
static int stream_free_arrays_on_ppu(FLT *restrict * a,
                                     FLT *restrict * b,
                                     FLT *restrict * c)
{
	free(*a); 
	*a = NULL;
	free(*b); 
	*b = NULL;
	free(*c); 
	*c = NULL;

	return 0;
}

/*
 * Initialize the arrays.
 */
static int stream_fill_arrays_on_ppu(int N,
                                     FLT *restrict a,
                                     FLT *restrict b,
                                     FLT *restrict c)
{
	int i;

	for (i = 0; i < N; ++i) {
        	a[i] = 1.0;
        	b[i] = 2.0;
        	c[i] = 0.0;
	}

	return 0;
}

/*
 * Validate the results. Do not call this function if you
 * did not use stream_fill_arrays_on_ppu() for the initialization
 * of the array.
 * The function returns zero on success and -1 else.
 */
static int stream_validate_arrays_on_ppu(int N,
                                         int ntimes,
                                         const double eps,
                                         FLT *restrict a,
                                         FLT *restrict b,
                                         FLT *restrict c,
                                         FLT scalar)
{
	FLT a0, b0, c0;
	int i;
	int fails;

	/* as in fill_stream_arrays() */
	a0 = 1.0;
	b0 = 2.0;
	c0 = 0.0;

	for (i = 0; i < ntimes; ++i) {
		c0 = a0;
		b0 = scalar * c0;
		c0 = a0 + b0;
		a0 = b0 + scalar *c0;
	}

	fails = 0;
	for (i = 0; i < N; ++i) {
		if (unlikely(fabs(a0 - a[i]) > eps*a0)) {
			fprintf(stderr, " Validation failure: a[%d] is %g "
			                "but should be %g\n", i, a[0], a0);
			++fails;
		}
		if (unlikely(fabs(b0 - b[i]) > eps*b0)) {
			fprintf(stderr, " Validation failure: b[%d] is %g "
			                "but should be %g\n", i, b[0], b0);
			++fails;
		}
		if (unlikely(fabs(c0 - c[i]) > eps*c0)) {
			fprintf(stderr, " Validation failure: c[%d] is %g "
			                "but should be %g\n", i, c[0], c0);
			++fails;
		}
		
		if (unlikely(fails >= 10))
			break;
	}

	if (likely(0 == fails))
		printf(" Arrays are ok.\n");

	return -1;
}

/*
 * Helper function for stream_print_results(): Compute minimal, maximal and
 * average execution times for the four kernels.
 */
static int stream_analyze_timings_on_ppu(int ntimes, double times[ntimes-1][4],
                                         double min[static 4],
                                         double max[static 4],
                                         double avg[static 4])
{
	int i, j;

	memcpy(min, times[0], 4*sizeof(double));
	memcpy(max, times[0], 4*sizeof(double));

	for (i = 0; i < ntimes-1; ++i) {
		for (j = 0; j < 4; ++j) {
			min[j] = (min[j] > times[i][j]) ? times[i][j] : min[j];
			max[j] = (max[j] > times[i][j]) ? max[j] : times[i][j];
			avg[j] = avg[j] + times[i][j];
		}
	}
	for (j = 0; j < 4; ++j)
		avg[j] /= (double )(ntimes - 1);

	return 0;
}

/*
 * Report the measured performance results to stdout.
 */
int stream_print_results_on_ppu(int N, int ntimes,
                                double times[ntimes-1][4])
{
	double min[4], max[4];
	double avg[4] = { 0 };
	int i;
	const char* const names[] = { "Copy ", "Scale", "Add  ", "Triad" };
	const double bytes[] = {
		(double )(2 * N * sizeof(FLT)),
		(double )(2 * N * sizeof(FLT)),
		(double )(3 * N * sizeof(FLT)),
		(double )(3 * N * sizeof(FLT))
	};

	stream_analyze_timings_on_ppu(ntimes, times, min, max, avg);

	printf("\n");
	printf(" Kernel | Bandwidth (MiBps) | Min time (ms) | Avg time (ms) | Max time (ms)\n");
	printf(" -------+-------------------+---------------+---------------+--------------\n");
	for (i = 0; i < 4; ++i) {
		printf(" %-6s |    %11.4f    |   %11.4f |   %11.4f |   %11.4f\n",
		       names[i], bytes[i]/((double )(1 << 20)*min[i]),
		       100*min[i], 100*avg[i], 100*max[i]);
	}
	printf(" -------+-------------------+---------------+---------------+--------------\n");
	printf("\n");

	return 0;
}

/*
 * Call the four kernels. The functions stream_copy(), stream_scale(),
 * stream_add() and stream_triad() are to be provided by the application.
 */
int stream_call_kernels_from_ppu(int N,
                                 FLT *restrict a, FLT *restrict b,
                                 FLT *restrict c, FLT scalar,
                                 double times[static 4])
{
	double tmp[4];

	tmp[0] = walltime();
	stream_copy(N, a, c);
	tmp[0] = walltime() - tmp[0];
	
	tmp[1] = walltime();
	stream_scale(N, b, c, scalar);
	tmp[1] = walltime() - tmp[1];
	
	tmp[2] = walltime();
	stream_add(N, a, b, c);
	tmp[2] = walltime() - tmp[2];
	
	tmp[3] = walltime();
	stream_triad(N, a, b, c, scalar);
	tmp[3] = walltime() - tmp[3];

	if (times)
		memcpy(times, tmp, 4*sizeof(double));

	return 0;
}

int stream_execute_main_loop_on_ppu(int N, int ntimes,
                                    FLT *restrict a, FLT *restrict b,
                                    FLT *restrict c, FLT scalar,
                                    double times[ntimes-1][4])
{
	int i;

	/* One warmup run as done by the original
	 * STREAM benchmark */
	stream_call_kernels_from_ppu(N, a, b, c, scalar, NULL);

	for (i = 0; i < ntimes-1; ++i) {
		stream_call_kernels_from_ppu(N, a, b, c, scalar, times[i]);
	}

	return 0;
}

#endif
