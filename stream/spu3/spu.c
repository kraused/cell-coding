
#include "common/types.h"
#include "common/compiler.h"

#include "stream/spu3/args.h"
#include "stream/spu2/msg.h"

#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include <stdio.h>

/* Should be a multiple of 128 / sizeof(float) = 32 */
#define SIZE	512
#define TAG	3

volatile struct spu3_kernel_args args __attribute__((aligned(128)));

volatile vector float ls1[SIZE / 4] __attribute__((aligned(128)));
volatile vector float ls2[SIZE / 4] __attribute__((aligned(128)));
volatile vector float ls3[SIZE / 4] __attribute__((aligned(128)));

void copy()
{
	int i, j, n;

	n = SIZE * sizeof(float);

	for (i = 0; (i + SIZE) < args.N; i += SIZE) {
		mfc_get((void *)&ls1[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();
		
		for (j = 0; j < (SIZE / 4); ++j)
			ls2[j] = ls1[j];

		mfc_put((void *)&ls2[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
	}

	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	if (unlikely(i < args.N)) {
		/* 
		 * args.N - i will be smaller than SIZE at this point so
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

	/* 
	 * At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */
}

void scale()
{
	int i, j, n;

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
	}
		
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	if (unlikely(i < args.N)) {
		/* 
		 * args.N - i will be smaller than SIZE at this point so
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

	/* 
	 * At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */
}

void add()
{
	int i, j, n;
	
	n = SIZE * sizeof(float);

	for (i = 0; (i + SIZE) < args.N; i += SIZE) {
		mfc_get((void *)&ls1[0], (unsigned int )&args.a[i], n, TAG, 0, 0);
		mfc_get((void *)&ls2[0], (unsigned int )&args.b[i], n, TAG, 0, 0);
		mfc_write_tag_mask(1 << TAG);
		mfc_read_tag_status_all();

		for (j = 0; j < (SIZE / 4); ++j)
			ls3[j] = spu_add(ls1[j], ls2[j]);

		mfc_put((void *)&ls3[0], (unsigned int )&args.c[i], n, TAG, 0, 0);
	}
		
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	if (unlikely(i < args.N)) {
		/* 
		 * args.N - i will be smaller than SIZE at this point so
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

	/* 
	 * At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */
}

void triad()
{
	int i, j, n;
	
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
	}
		
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	if (unlikely(i < args.N)) {
		/* 
		 * args.N - i will be smaller than SIZE at this point so
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

	/* 
	 * At this point it may be that i is still smaller than args.N if the length
	 * was not divisible by the number of SPUs times 16.
	 */
}

int main(ull id, ull argp, ull envp)
{
	unsigned int cmd;

	mfc_get(&args, argp, sizeof(args), TAG, 0, 0);
	mfc_write_tag_mask(1 << TAG);
	mfc_read_tag_status_all();

	while (1) {
		cmd = spu_read_in_mbox();

		if (unlikely(SPU2_MSG_PPU_TO_SPU_EXIT == cmd))
			break;

		switch (cmd) {
		case SPU2_MSG_PPU_TO_SPU_DO_COPY:
			copy();
			break;
		case SPU2_MSG_PPU_TO_SPU_DO_SCALE:
			scale();
			break;
		case SPU2_MSG_PPU_TO_SPU_DO_ADD:
			add();
			break;
		case SPU2_MSG_PPU_TO_SPU_DO_TRIAD:
			triad();
			break;
		default:
			fprintf(stderr, " [SPU]: Invalid command received in mailbox\n");
		}

		spu_write_out_mbox(SPU2_MSG_SPU_TO_PPU_DONE);
	}

	return 0;
}
