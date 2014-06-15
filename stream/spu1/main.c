
#include "common/types.h"
#include "common/compiler.h"

#include "stream/spu1/args.h"

#include <stdlib.h>
#include <stdio.h>

#include <libspe2.h>
#include <pthread.h>

#define FLT 	float
/*
 * It is best if LEN is divisible by the number
 * of SPUs.
 */
#define LEN 	(6*128*64*1024/sizeof(FLT))
#define NTIMES 	10

struct ppu_pthread_data {
	spe_context_ptr_t	id;
	pthread_t 		pthread;
	void 			*argp;
};

void *ppu_pthread_function(void *arg)
{
	struct ppu_pthread_data *data = (struct ppu_pthread_data *)arg;
	int retval;
	unsigned int entry = SPE_DEFAULT_ENTRY;

	retval = spe_context_run(data->id, &entry, 0,
	                         data->argp, NULL, NULL);
	if (unlikely(retval < 0)) {
		perror("spe_context_run");
		exit(128);
	}

	pthread_exit(NULL);
}

#ifndef MAX_SPUS
# define MAX_SPUS 16
#endif
struct ppu_pthread_data data[MAX_SPUS];
struct spu1_kernel_args args[MAX_SPUS];

static int exec_spu_program(spe_program_handle_t *handle, int N,
                            float *restrict a, float *restrict b,
                            float *restrict c, float scalar)
{
	int i, nspus;
	int retval;
	char *tailptr;

	if (getenv("NSPUS")) {
		nspus = (int )strtol(getenv("NSPUS"), &tailptr, 0);
	} else {
		nspus = spe_cpu_info_get(SPE_COUNT_USABLE_SPES, 0);
	}

	for (i = 0; i < nspus; ++i) {
		args[i].N      = N / nspus;
		args[i].a      = a + i * args[i].N;
		args[i].b      = b + i * args[i].N;
		args[i].c      = c + i * args[i].N;
		args[i].scalar = scalar;
	}

	/* The last spu has to do more work if N is not divisible by nspus.
	 */	
	args[nspus-1].N = N - (nspus - 1)*((int )(N / nspus));

	for (i = 0; i < nspus; ++i) {
		if (NULL == (data[i].id = spe_context_create(0, NULL))) {
			perror("spe_context_create");
			return retval;
		}

		retval = spe_program_load(data[i].id, handle);
		if (unlikely(retval)) {
			perror("spe_program_load");
			return retval;
		}

		data[i].argp = &args[i];

		retval = pthread_create(&data[i].pthread, NULL,
		                        ppu_pthread_function, &data[i]);
		if (unlikely(retval)) {
			perror("pthread_create");
			return retval;
		}
	}

	for (i = 0; i < nspus; ++i) {
		retval = pthread_join(data[i].pthread, NULL);
		if (unlikely(retval)) {
			perror("pthread_join");
			return retval;
		}

		retval = spe_context_destroy(data[i].id);
		if (unlikely(retval)) {
			perror("spe_context_destroy");
			return retval;
		}
	}

	return 0;
}
	
extern spe_program_handle_t stream_copy_spu;
extern spe_program_handle_t stream_scale_spu;
extern spe_program_handle_t stream_add_spu;
extern spe_program_handle_t stream_triad_spu;

static inline void stream_copy(int N, float *restrict a, float *restrict c)
{
	int retval;

	retval = exec_spu_program(&stream_copy_spu, N, a, NULL, c, 0);
	if (unlikely(retval))
		fprintf(stderr, " exec_spu_program failed with error %d.\n", retval);
}

static inline void stream_scale(int N, float *restrict b, float *restrict c, 
                                float scalar)
{
	int retval;

	retval = exec_spu_program(&stream_scale_spu, N, NULL, b, c, scalar);
	if (unlikely(retval))
		fprintf(stderr, " exec_spu_program failed with error %d.\n", retval);
}

static inline void stream_add(int N, float *restrict a, float *restrict b, 
                              float *restrict c)
{
	int retval;

	retval = exec_spu_program(&stream_add_spu, N, a, b, c, 0);
	if (unlikely(retval))
		fprintf(stderr, " exec_spu_program failed with error %d.\n", retval);
}

static inline void stream_triad(int N, float *restrict a, float *restrict b, 
                                float *restrict c, float scalar)
{
	int retval;

	retval = exec_spu_program(&stream_triad_spu, N, a, b, c, scalar);
	if (unlikely(retval))
		fprintf(stderr, " exec_spu_program failed with error %d.\n", retval);
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
