
#include "common/types.h"
#include "common/compiler.h"

#include "ring/args.h"

#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <spu_timer.h>

#include <libsync.h>

#include <stdio.h>

/*
 * 16KiB is the maximum size of a single DMA
 * transfer.
 */
#define SIZE		(16*1024)
#define TAG		3

/*
 * FIXME This is system dependent. From /proc/cpuinfo
 */
#define TIMEBASE	79800000

volatile struct spu_program_args args __attribute__((aligned(128)));

volatile char ls1[SIZE] __attribute__((aligned(128)));
volatile char ls2[SIZE] __attribute__((aligned(128)));
volatile char ls3[SIZE] __attribute__((aligned(128)));
volatile char ls4[SIZE] __attribute__((aligned(128)));

void ring(int left, int right, int size)
{
	mfc_put((void *)&ls1[0],
	        args.ls[left] + (ull )&ls2[0],
	        size,
	        TAG, 0, 0);
	mfc_put((void *)&ls3[0],
	        args.ls[right] + (ull )&ls4[0],
	        size,
	        TAG, 0, 0);

	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	while((!ls2[size-1]) || (!ls4[size-1])); 

	ls2[size-1] = 0;
	ls4[size-1] = 0;
}

void barrier()
{
	static volatile int index = 0;

	index ^= 1;
	atomic_inc(args.barrier[index]);

	if (0 == args.rank) {
		while (atomic_read(args.barrier[index]) != args.nspus);
		atomic_set(args.barrier[index], 0);		
	} else {
		while (atomic_read(args.barrier[index]));
	}
}

int main(ull id, ull argp, ull envp)
{
	int left, right;
	int i, stride;
	ull size;
	ull start;

	mfc_get(&args, argp, sizeof(args), TAG, 0, 0);
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	for (i = 0; i < SIZE; ++i) {
		ls1[i] = 1;
		ls3[i] = 1;
		ls2[i] = 0;
		ls4[i] = 0;
	}

	barrier();	

	size = 8;
	while (size <= SIZE) {
		if (0 == args.rank)
			printf("%6llu\t", size);

		for (stride = 1; stride < args.nspus; ++stride) {
			left  = (args.rank - stride + args.nspus) % args.nspus;
			right = (args.rank + stride) % args.nspus;

			spu_clock_start();

			start = spu_clock_read();
			barrier();

			for (i = 0; i < 100000; ++i) {
				ring(left, right, size);
				barrier();
			}

			start = spu_clock_read() - start;
			if (0 == args.rank) {
				double tmp = ((double )start)/79800000;

				printf("%12.6f\t", size*2*100000/tmp/1024/1024);
			}

			spu_clock_stop();

			barrier();
		}
		
		if (0 == args.rank)
			printf("\n");
		size <<= 1;
	}

	return 0;
}

