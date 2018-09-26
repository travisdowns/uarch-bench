/* 
 * Output stats on interrupt frequency, seqlock update frequency.
 * 
 * Run it with an output interval in ms, default 1000.
 *
 * Based on rtest.c from pmu-tools
 */

#include "../pmu-tools/jevents/rdpmc.h"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>


#define HW_INTERRUPTS 0x1cb

typedef unsigned long long u64;
typedef unsigned long u32;

/* microseconds */
u64 get_time(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (u64)tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int ac, char **av)
{
	struct rdpmc_ctx ctx = {};

	int update_us;
	if (av[1]) {
		update_us = atoi(av[1]) * 1000;
	} else {
		update_us = 1000 * 1000; // 1 second
	}

	struct perf_event_attr event = {
		.type = PERF_TYPE_RAW,
		// .type = PERF_TYPE_HARDWARE,
		.size = PERF_ATTR_SIZE_VER0,
		.config = HW_INTERRUPTS,
		// .config = PERF_COUNT_HW_CPU_CYCLES,
		.sample_type = 0, // PERF_SAMPLE_READ,
		.exclude_kernel = 0,
		.sample_period = 0
	};
	
	if (rdpmc_open_attr(&event, &ctx, NULL) < 0) {
		err(errno, "rdpmc_open");
	}

#define COL_WIDTH 14
#define CW "14"
#define CWS "%" CW "s"

	printf(CWS CWS CWS,      "<-----------", "Delta",   "----------->");
	printf(CWS CWS CWS "\n", "<-----------", "Average", "----------->");
	for (int i = 0; i < 2; i++) {
		printf(CWS CWS CWS, "interrupts", "lock chg", "lock delta");
	}
	printf("\n");

	u32 lock_orig = ctx.buf->lock, lock_upd_total = 0;
	u64 time_orig = get_time(), next_print = time_orig;
	u64 irgs_orig = rdpmc_read(&ctx);
	while (1) {
		next_print += update_us;
		u64 time_start = get_time(), time_end;
		u64 irqs_start = rdpmc_read(&ctx);
		u32 lock_start = ctx.buf->lock;

		u64 lock_upd = 0;
		u32 lock_last = lock_start;
		while ((time_end = get_time()) < next_print) {
			// for (int i = 0; i < 100; i++) {
				u32 lock_now = ctx.buf->lock;
				if (lock_now != lock_last) {
					lock_upd++;
				}
				lock_last = lock_now;
			// }
		}

		u64 irqs_end = rdpmc_read(&ctx);

		u64 time_delta = time_end  - time_start;
		u64 irqs_delta = irqs_end  - irqs_start;
		u32 lock_delta = lock_last - lock_start;
		lock_upd_total += lock_upd;
		
		// interval deltas
		printf("%14.1f", 1000000.0 * irqs_delta / time_delta);
		printf("%14.1f", 1000000.0 * lock_upd   / time_delta);
		printf("%14.1f", 1000000.0 * lock_delta / time_delta);

		u64 time_orig_delta = time_end - time_orig;
		// full-run average
		printf("%14.1f", 1000000.0 * (irqs_end  - irgs_orig) / time_orig_delta);
		printf("%14.1f", 1000000.0 * lock_upd_total          / time_orig_delta);
		printf("%14.1f", 1000000.0 * (lock_last - lock_orig) / time_orig_delta);

		printf("\n");
	}
			
	rdpmc_close(&ctx);
	return 0;
}
