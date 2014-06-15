
#ifndef CELL_CODING_COMMON_PPU_WALLTIME_H
#define CELL_CODING_COMMON_PPU_WALLTIME_H 1

#include <stdlib.h>
#include <sys/time.h>

static inline double walltime()
{
        struct timeval tv;

        gettimeofday(&tv, NULL);
        return ((double )tv.tv_sec + 1e-6 * (double )tv.tv_usec);
}

#endif

