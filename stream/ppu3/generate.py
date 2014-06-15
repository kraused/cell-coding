
import sys

def copy(b):
	str = "\tfor (i = 0; (i + %d) < N; i += %d) {\n" % (b, b)
	for i in range(b / 4):
		str += "\t\tvec_st(vec_ld(i * sizeof(float), &a[%4d]), " \
		       "i *sizeof(float), &c[%4d]);\n" % (4 * i, 4 * i)
	str += "\t}"
	
	return str

def scale(b):
	str = "\tfor (i = 0; (i + %d) < N; i += %d) {\n" % (b, b)
	for i in range(b / 4):
		str += "\t\tv = vec_ld(i * sizeof(float), &c[%4d]);\n" % (4 * i)
		str += "\t\tvec_st(vec_madd(s, v, zero), "\
		       "i * sizeof(float), &b[%4d]);\n" % (4 * i)
	str += "\t}"

	return str

def add(b):
	str = "\tfor (i = 0; (i + %d) < N; i += %d) {\n" % (b, b)
	for i in range(b / 4):
		str += "\t\tv = vec_ld(i * sizeof(float), &a[%4d]);\n" % (4 * i)
		str += "\t\tw = vec_ld(i * sizeof(float), &b[%4d]);\n" % (4 * i)
		str += "\t\tvec_st(vec_add(v, w), "\
		       "i * sizeof(float), &c[%4d]);\n" % (4 * i)
	str += "\t}"

	return str

def triad(b):
	str = "\tfor (i = 0; (i + %d) < N; i += %d) {\n" % (b, b)
	for i in range(b / 4):
		str += "\t\tv = vec_ld(i * sizeof(float), &b[%4d]);\n" % (4 * i)
		str += "\t\tw = vec_ld(i * sizeof(float), &c[%4d]);\n" % (4 * i)
		str += "\t\tvec_st(vec_madd(s, w, v), "\
		       "i * sizeof(float), &a[%4d]);\n" % (4 * i)
	str += "\t}"

	return str

b = int(sys.argv[1])

assert(b >= 4)
assert(0 == b % 4)

print """
#include "common/types.h"

#include <altivec.h>
#include <stdio.h>

#define FLT 	float
#define LEN 	(32*1024 * 1024/sizeof(float))
#define NTIMES 	10

static inline void stream_copy(int N, float *restrict a, float *restrict c)
{
	int i;

%s
	
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

%s

	for (; i < N; ++i) {
		b[i] = scalar*c[i];
	}
}

static inline void stream_add(int N, float *restrict a, float *restrict b, 
                              float *restrict c)
{
	int i;
	vector float v, w;

%s

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

%s

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
		fprintf(stderr, "stream_alloc_arrays_on_ppu failed with error %%d.\\n", retval);
		return 1;
	}

	stream_fill_arrays_on_ppu(LEN, a, b, c);
	
	printf(" Starting kernel.\\n");
	stream_execute_main_loop_on_ppu(LEN, NTIMES, a, b, c, scalar, times);
	
	stream_print_results_on_ppu(LEN, NTIMES, times);
	stream_validate_arrays_on_ppu(LEN, NTIMES, 1e-5, a, b, c, scalar);

	stream_free_arrays_on_ppu(&a, &b, &c);

	return 0;
}
""" % (copy(b), scale(b), add(b), triad(b))

