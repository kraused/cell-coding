
#include "common/types.h"
#include "common/compiler.h"

#include "stream/spu2/args.h"
#include "stream/spu2/msg.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
struct spu3_kernel_args args[MAX_SPUS];

static int num_usable_spus()
{
	int nspus;
	char *tailptr;

	if (getenv("NSPUS")) {
		nspus = (int )strtol(getenv("NSPUS"), &tailptr, 0);
	} else {
		nspus = spe_cpu_info_get(SPE_COUNT_USABLE_SPES, 0);
	}

	return nspus;
}

static int exec_spu_program(spe_program_handle_t *handle, int N,
                            float *restrict a, float *restrict b,
                            float *restrict c, float scalar)
{
	int i, nspus;
	int retval;

	nspus = num_usable_spus();

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

	return 0;
}

/*
 * Stop the execution of all SPU threads.
 */
static int stop_spus()
{
	int i, nspus;
	int retval;
	unsigned int msg = SPU2_MSG_PPU_TO_SPU_EXIT;

	nspus = num_usable_spus();

	for (i = 0; i < nspus; ++i) {
		retval = spe_in_mbox_write(data[i].id, &msg, 1,
		                           SPE_MBOX_ALL_BLOCKING);
		if (unlikely(1 != retval)) {
			perror("spe_in_mbox_write");
			return retval;
		}

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

/*
 * Broadcast msg to all SPUs via the mailbox. 
 */
static int bcast_to_spus(unsigned int msg)
{
	int i, nspus;
	int retval;

	nspus = num_usable_spus();

	for (i = 0; i < nspus; ++i) {
		retval = spe_in_mbox_write(data[i].id, &msg, 1,
		                           SPE_MBOX_ALL_BLOCKING);
		if (unlikely(1 != retval)) {
			perror("spe_in_mbox_write");
			return retval;
		}
	}

	return 0;
}

/*
 * Wait for all SPUs to send the DONE signal.
 */
static int wait_for_spus_done()
{
	int i, nspus, done;
	int retval;
	unsigned int stats[MAX_SPUS];

	nspus = num_usable_spus();

	memset(stats, 0, sizeof(stats));
	done = 0;

	while (nspus != done) {
		for (i = 0; i < nspus; ++i) {
			if(stats[i])
				continue;

			retval = spe_out_mbox_status(data[i].id);
			if (retval > 0) {
				retval = spe_out_mbox_read(data[i].id, &stats[i], 1);
				if (unlikely(1 != retval)) {
					perror("spe_out_mbox_read");
					return -1;
				}
				if (unlikely(SPU2_MSG_SPU_TO_PPU_DONE != stats[i])) {
					fprintf(stderr, " dropping unexpected message"
					                " in wait_for_spus_done.\n");
					stats[i] = 0;
				}
				
				++done;				
			}
		}
	}

	return 0;
}

static inline void stream_copy(int N, float *restrict a, float *restrict c)
{
	bcast_to_spus(SPU2_MSG_PPU_TO_SPU_DO_COPY);
	wait_for_spus_done();
}

static inline void stream_scale(int N, float *restrict b, float *restrict c, 
                                float scalar)
{
	bcast_to_spus(SPU2_MSG_PPU_TO_SPU_DO_SCALE);
	wait_for_spus_done();
}

static inline void stream_add(int N, float *restrict a, float *restrict b, 
                              float *restrict c)
{
	bcast_to_spus(SPU2_MSG_PPU_TO_SPU_DO_ADD);
	wait_for_spus_done();
}

static inline void stream_triad(int N, float *restrict a, float *restrict b, 
                                float *restrict c, float scalar)
{
	bcast_to_spus(SPU2_MSG_PPU_TO_SPU_DO_TRIAD);
	wait_for_spus_done();
}

#include "stream/stream.h"

extern spe_program_handle_t stream_spu;

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

	retval = exec_spu_program(&stream_spu, LEN, a, b, c, scalar);
	if (unlikely(retval)) {
		fprintf( stderr, " exec_spu_program failed with error %d.\n", retval);
		return 1;
	}
	
	printf(" Starting kernel.\n");
	stream_execute_main_loop_on_ppu(LEN, NTIMES, a, b, c, scalar, times);
	
	stream_print_results_on_ppu(LEN, NTIMES, times);
	stream_validate_arrays_on_ppu(LEN, NTIMES, 1e-5, a, b, c, scalar);

	stop_spus();

	stream_free_arrays_on_ppu(&a, &b, &c);

	return 0;
}
