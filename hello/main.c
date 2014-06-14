
#define _SVID_SOURCE

#include "common/types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libspe2.h>
#include <pthread.h>

#include <dirent.h>

struct ppu_pthread_data {
	spe_context_ptr_t id;
	pthread_t pthread;
	void *argp;
};

void *ppu_pthread_function(void *arg)
{
	struct ppu_pthread_data *data = (struct ppu_pthread_data *)arg;
	int retval;
	unsigned int entry = SPE_DEFAULT_ENTRY;

	if ((retval = spe_context_run(data->id, &entry, 0,
				      data->argp, NULL, NULL)) < 0) {
		perror("spe_context_run");
		exit(128);
	}

	pthread_exit(NULL);
}

/* SPU program handle */
extern spe_program_handle_t spu_program;

#ifndef MAX_SPUS
# define MAX_SPUS 16
#endif
struct ppu_pthread_data data[MAX_SPUS];

int main(int argc, char **argv)
{
	int i, n, retval, nspus;
	char temp[256];
	struct dirent **spu_files;
	FILE *fh;

	nspus = spe_cpu_info_get(SPE_COUNT_USABLE_SPES, 0);
	printf(" [PPU]: # usable synergistic processing units = %d\n", nspus);

	for (i = 0; i < nspus; ++i) {
		if (NULL == (data[i].id = spe_context_create(0, NULL))) {
			perror("spe_context_create");
			exit(128);
		}

		if (0 != (retval = spe_program_load(data[i].id, &spu_program))) {
			perror("spe_program_load");
			exit(128);
		}

		data[i].argp = (void *)(ull )i;

		if (0 != (retval = pthread_create(&data[i].pthread, NULL,
						  ppu_pthread_function,
						  &data[i]))) {
			perror("pthread_create");
			exit(128);
		}
	}

	n = scandir("/spu", &spu_files, NULL, alphasort);
	while (n--) {
		if (!strncmp(spu_files[n]->d_name, "spethread", 9)) {
			snprintf(temp, sizeof(temp), "/spu/%s/phys-id",
				 spu_files[n]->d_name);
			if (NULL == (fh = fopen(temp, "r"))) {
				perror("fopen");
				exit(128);
			}

			fgets(temp, 128, fh);
			fclose(fh);

			printf(" [PPU]: context = %s: physical id = %s",
			       spu_files[n]->d_name, temp);
		}
	}
	free(spu_files);

	for (i = 0; i < nspus; ++i) {
		if (0 != (retval = pthread_join(data[i].pthread, NULL))) {
			perror("pthread_join");
			exit(128);
		}

		if (0 != (retval = spe_context_destroy(data[i].id))) {
			perror("spe_context_destroy");
			exit(128);
		}
	}

	return 0;
}

