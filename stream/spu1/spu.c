
#include "common/types.h"
#include "common/compiler.h"

#include "stream/spu1/args.h"

#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include <stdio.h>

/* Should be a multiple of 128 / sizeof(float) = 32 */
#define SIZE	256
#define TAG	3

volatile struct spu1_kernel_args args __attribute__((aligned(128)));

volatile vector float ls1[SIZE / 4] __attribute__((aligned(128)));
volatile vector float ls2[SIZE / 4] __attribute__((aligned(128)));
volatile vector float ls3[SIZE / 4] __attribute__((aligned(128)));

/*
 * This is an implementation that tries to be as close as possible to
 * the PPU version. Instead of going from RAM -> LS and then directly
 * from LS -> RAM we first copy from LS buffer to another LS buffer.
 * This reduces the measured bandwidth from about 8200 to about 4300 MiB
 * per second.
 */
int copy(ull id, ull argp, ull envp)
{
	int i, j, n;

	mfc_get(&args, argp, sizeof(args), TAG, 0, 0);
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	n = SIZE * sizeof(float);

	for (i = 0; (i + SIZE) < args.N; i += SIZE) {
		mfc_get((void *)&ls1[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
		
		for (j = 0; j < (SIZE / 4); ++j)
			ls2[j] = ls1[j];

		mfc_put((void *)&ls2[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}

	if (unlikely(i < args.N)) {
		/* args.N - i will be smaller than SIZE at this point so
		 * it is safe to do a DMA transfer.
		 * We need to make sure that size is a multiple of 16.
		 */
		n = ((args.N - i) * sizeof(float)) & (~127);

		mfc_get((void *)&ls1[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
		
		/* n must be divisible by 4. */
		for (j = 0; j < ((args.N - i) / 4); ++j)
			ls2[j] = ls1[j];
		
		mfc_put((void *)&ls2[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}
	/* At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */

	return 0;
}

int scale(ull id, ull argp, ull envp)
{
	int i, j, n;

	mfc_get(&args, argp, sizeof(args), TAG, 0, 0);
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();
	
	vector float s = spu_splats(args.scalar);
	vector float z = spu_splats(0.0f);

	n = SIZE * sizeof(float);

	for (i = 0; (i + SIZE) < args.N; i += SIZE) {
		mfc_get((void *)&ls1[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();

		for (j = 0; j < (SIZE / 4); ++j)
			ls2[j] = spu_madd(s, ls1[j], z);

		mfc_put((void *)&ls2[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}

	if (unlikely(i < args.N)) {
		/* args.N - i will be smaller than SIZE at this point so
		 * it is safe to do a DMA transfer.
		 * We need to make sure that size is a multiple of 16.
		 */
		n = ((args.N - i) * sizeof(float)) & (~127);

		mfc_get((void *)&ls1[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
		
		/* n must be divisible by 4. */
		for (j = 0; j < ((args.N - i) / 4); ++j)
			ls2[j] = spu_madd(s, ls1[j], z);
		
		mfc_put((void *)&ls2[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}
	/* At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */

	return 0;
}

int add(ull id, ull argp, ull envp)
{
	int i, j, n;

	mfc_get(&args, argp, sizeof(args), TAG, 0, 0);
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();
	
	n = SIZE * sizeof(float);

	for (i = 0; (i + SIZE) < args.N; i += SIZE) {
		mfc_get((void *)&ls1[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_get((void *)&ls2[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();

		for (j = 0; j < (SIZE / 4); ++j)
			ls3[j] = spu_add(ls1[j], ls2[j]);

		mfc_put((void *)&ls3[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}

	if (unlikely(i < args.N)) {
		/* args.N - i will be smaller than SIZE at this point so
		 * it is safe to do a DMA transfer.
		 * We need to make sure that size is a multiple of 16.
		 */
		n = ((args.N - i) * sizeof(float)) & (~127);

		mfc_get((void *)&ls1[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_get((void *)&ls2[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
		
		/* n must be divisible by 4. */
		for (j = 0; j < ((args.N - i) / 4); ++j)
			ls3[j] = spu_add(ls1[j], ls2[j]);
		
		mfc_put((void *)&ls3[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}
	/* At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */

	return 0;
}

int triad(ull id, ull argp, ull envp)
{
	int i, j, n;

	mfc_get(&args, argp, sizeof(args), TAG, 0, 0);
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();
	
	vector float s = spu_splats(args.scalar);
	
	n = SIZE * sizeof(float);

	for (i = 0; (i + SIZE) < args.N; i += SIZE) {
		mfc_get((void *)&ls1[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_get((void *)&ls2[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();

		for (j = 0; j < (SIZE / 4); ++j)
			ls3[j] = spu_madd(s, ls2[j], ls1[j]);

		mfc_put((void *)&ls3[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}

	if (unlikely(i < args.N)) {
		/* args.N - i will be smaller than SIZE at this point so
		 * it is safe to do a DMA transfer.
		 * We need to make sure that size is a multiple of 16.
		 */
		n = ((args.N - i) * sizeof(float)) & (~127);

		mfc_get((void *)&ls1[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_get((void *)&ls2[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
		
		/* n must be divisible by 4. */
		for (j = 0; j < ((args.N - i) / 4); ++j)
			ls3[j] = spu_madd(s, ls2[j], ls1[j]);
		
		mfc_put((void *)&ls3[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
	}
	/* At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */

	return 0;
}
