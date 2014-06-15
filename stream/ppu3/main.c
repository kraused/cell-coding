
#include "common/types.h"

#include <altivec.h>
#include <stdio.h>

#define FLT 	float
#define LEN 	(32*1024 * 1024/sizeof(float))
#define NTIMES 	10

static inline void stream_copy(int N, float *restrict a, float *restrict c)
{
	int i;

	for (i = 0; (i + 32) < N; i += 32) {
		vec_st(vec_ld(i * sizeof(float), &a[   0]), i *sizeof(float), &c[   0]);
		vec_st(vec_ld(i * sizeof(float), &a[   4]), i *sizeof(float), &c[   4]);
		vec_st(vec_ld(i * sizeof(float), &a[   8]), i *sizeof(float), &c[   8]);
		vec_st(vec_ld(i * sizeof(float), &a[  12]), i *sizeof(float), &c[  12]);
		vec_st(vec_ld(i * sizeof(float), &a[  16]), i *sizeof(float), &c[  16]);
		vec_st(vec_ld(i * sizeof(float), &a[  20]), i *sizeof(float), &c[  20]);
		vec_st(vec_ld(i * sizeof(float), &a[  24]), i *sizeof(float), &c[  24]);
		vec_st(vec_ld(i * sizeof(float), &a[  28]), i *sizeof(float), &c[  28]);
	}
	
	for(; i < N; ++i) {
		c[i] = a[i];
	}
}

static inline void stream_scale(int N, float *restrict b, float *restrict c, 
                                float scalar)
{
	int i;
	vector float v;
	vector float s = (vector float){ 
		scalar, scalar, scalar, scalar
	};
	const vector float zero = (vector float){ 0, 0, 0, 0 };

	for (i = 0; (i + 32) < N; i += 32) {
		v = vec_ld(i * sizeof(float), &c[   0]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[   0]);
		v = vec_ld(i * sizeof(float), &c[   4]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[   4]);
		v = vec_ld(i * sizeof(float), &c[   8]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[   8]);
		v = vec_ld(i * sizeof(float), &c[  12]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[  12]);
		v = vec_ld(i * sizeof(float), &c[  16]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[  16]);
		v = vec_ld(i * sizeof(float), &c[  20]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[  20]);
		v = vec_ld(i * sizeof(float), &c[  24]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[  24]);
		v = vec_ld(i * sizeof(float), &c[  28]);
		vec_st(vec_madd(s, v, zero), i * sizeof(float), &b[  28]);
	}

	for (; i < N; ++i) {
		b[i] = scalar*c[i];
	}
}

static inline void stream_add(int N, float *restrict a, float *restrict b, 
                              float *restrict c)
{
	int i;
	vector float v, w;

	for (i = 0; (i + 32) < N; i += 32) {
		v = vec_ld(i * sizeof(float), &a[   0]);
		w = vec_ld(i * sizeof(float), &b[   0]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[   0]);
		v = vec_ld(i * sizeof(float), &a[   4]);
		w = vec_ld(i * sizeof(float), &b[   4]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[   4]);
		v = vec_ld(i * sizeof(float), &a[   8]);
		w = vec_ld(i * sizeof(float), &b[   8]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[   8]);
		v = vec_ld(i * sizeof(float), &a[  12]);
		w = vec_ld(i * sizeof(float), &b[  12]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[  12]);
		v = vec_ld(i * sizeof(float), &a[  16]);
		w = vec_ld(i * sizeof(float), &b[  16]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[  16]);
		v = vec_ld(i * sizeof(float), &a[  20]);
		w = vec_ld(i * sizeof(float), &b[  20]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[  20]);
		v = vec_ld(i * sizeof(float), &a[  24]);
		w = vec_ld(i * sizeof(float), &b[  24]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[  24]);
		v = vec_ld(i * sizeof(float), &a[  28]);
		w = vec_ld(i * sizeof(float), &b[  28]);
		vec_st(vec_add(v, w), i * sizeof(float), &c[  28]);
	}

	for (; i < N; ++i) {
		c[i] = a[i] + b[i];
	}
}

static inline void stream_triad(int N, float *restrict a, float *restrict b, 
                                float *restrict c, float scalar)
{
	int i;
	vector float v, w;
	vector float s = (vector float){ 
		scalar, scalar, scalar, scalar
	};

	for (i = 0; (i + 32) < N; i += 32) {
		v = vec_ld(i * sizeof(float), &b[   0]);
		w = vec_ld(i * sizeof(float), &c[   0]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[   0]);
		v = vec_ld(i * sizeof(float), &b[   4]);
		w = vec_ld(i * sizeof(float), &c[   4]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[   4]);
		v = vec_ld(i * sizeof(float), &b[   8]);
		w = vec_ld(i * sizeof(float), &c[   8]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[   8]);
		v = vec_ld(i * sizeof(float), &b[  12]);
		w = vec_ld(i * sizeof(float), &c[  12]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[  12]);
		v = vec_ld(i * sizeof(float), &b[  16]);
		w = vec_ld(i * sizeof(float), &c[  16]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[  16]);
		v = vec_ld(i * sizeof(float), &b[  20]);
		w = vec_ld(i * sizeof(float), &c[  20]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[  20]);
		v = vec_ld(i * sizeof(float), &b[  24]);
		w = vec_ld(i * sizeof(float), &c[  24]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[  24]);
		v = vec_ld(i * sizeof(float), &b[  28]);
		w = vec_ld(i * sizeof(float), &c[  28]);
		vec_st(vec_madd(s, w, v), i * sizeof(float), &a[  28]);
	}

	for (; i < N; ++i) {
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
		fprintf(stderr, "stream_alloc_arrays_on_ppu failed with error %d.\n", retval);
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

