
#include "common/types.h"
#include "common/compiler.h"

#include "ring/args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libspe2.h>
#include <pthread.h>

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

/* SPU program handle */
extern spe_program_handle_t spu_program;

struct ppu_pthread_data data[MAX_SPUS];
struct spu_program_args args[MAX_SPUS];

volatile signed int barrier1;
volatile signed int barrier2;

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

int main(int argc, char **argv)
{
	int i, j, retval, nspus;

	nspus = num_usable_spus();

	for (i = 0; i < nspus; ++i) {
		if (NULL == (data[i].id = spe_context_create(0, NULL))) {
			perror("spe_context_create");
			exit(128);
		}

		args[i].nspus = nspus;
		args[i].rank  = i;
		args[i].ls[i] = (ull )spe_ls_area_get(data[i].id);

		args[i].barrier[0] = (ull )&barrier1;
		args[i].barrier[1] = (ull )&barrier2;
	}

	barrier1 = 0;
	barrier2 = 0;

	for (i = 0; i < nspus; ++i)
		for (j = 0; j < nspus; ++j)
			args[j].ls[i] = args[i].ls[i];

	for (i = 0; i < nspus; ++i) {
		retval = spe_program_load(data[i].id, &spu_program);
		if (unlikely(retval)) {
			perror("spe_program_load");
			exit(128);
		}

		data[i].argp = (void *)&args[i];

		retval = pthread_create(&data[i].pthread, NULL,
		                        ppu_pthread_function, &data[i]);
		if (unlikely(retval)) {
			perror("pthread_create");
			exit(128);
		}
	}

	for (i = 0; i < nspus; ++i) {
		retval = pthread_join(data[i].pthread, NULL);
		if (unlikely(retval)) {
			perror("pthread_join");
			exit(128);
		}

		retval = spe_context_destroy(data[i].id);
		if (unlikely(retval)) {
			perror("spe_context_destroy");
			exit(128);
		}
	}

	return 0;
}

