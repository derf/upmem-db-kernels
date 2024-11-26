#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <handshake.h>
#include <string.h>

#include "common.h"

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS;

uint32_t tasklet_count[NR_TASKLETS];

extern int main_kernel_count(void);
extern int main_kernel_select(void);
extern int main_kernel_update(void);

int (*kernels[nr_kernels])(void) = {main_kernel_count, main_kernel_select, main_kernel_update};

int main(void) {
	return kernels[DPU_INPUT_ARGUMENTS.kernel]();
}

int main_kernel_count()
{
	unsigned int tasklet_id = me();

	if (tasklet_id == 0) {
		mem_reset();
	}

	uint32_t input_size_dpu_bytes = DPU_INPUT_ARGUMENTS.size;
	uint64_t predicate_arg = DPU_INPUT_ARGUMENTS.predicate_arg;

	// Address of the current processing block in MRAM
	uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
	uint32_t mram_base_addr = (uint32_t)DPU_MRAM_HEAP_POINTER;

	T*cache = (T*) mem_alloc(BLOCK_SIZE);
	uint32_t local_count = 0;
	uint32_t total_count = 0;

	bool (*_pred_f)(uint64_t const, uint64_t const) = get_pred(DPU_INPUT_ARGUMENTS.predicate);

	for (unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS) {
		mram_read((__mram_ptr void const*)(mram_base_addr + byte_index), cache, BLOCK_SIZE);
		if (byte_index + (BLOCK_SIZE - 1) >= input_size_dpu_bytes) {
			for (unsigned int i = 0; byte_index + (i * sizeof(T)) < input_size_dpu_bytes; i++) {
				local_count += _pred_f(cache[i], predicate_arg) * 1;
			}
		} else {
			for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++) {
				local_count += _pred_f(cache[i], predicate_arg) * 1;
			}
		}
	}

	if (tasklet_id != 0) {
		handshake_wait_for(tasklet_id - 1);
		total_count = tasklet_count[tasklet_id];
	}

	if (tasklet_id < NR_TASKLETS - 1) {
		tasklet_count[tasklet_id + 1] = total_count + local_count;
		handshake_notify();
	}

	if (tasklet_id == NR_TASKLETS - 1) {
		DPU_RESULTS.count = total_count + local_count;
	}

	return 0;
}

int main_kernel_select()
{
	unsigned int tasklet_id = me();

	if (tasklet_id == 0) {
		mem_reset();
	}

	uint32_t input_size_dpu_bytes = DPU_INPUT_ARGUMENTS.size;
	uint64_t predicate_arg = DPU_INPUT_ARGUMENTS.predicate_arg;

	// Address of the current processing block in MRAM
	uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
	uint32_t mram_base_addr = (uint32_t)DPU_MRAM_HEAP_POINTER;
	uint32_t bitmask_base_addr = mram_base_addr + 65011712;

	T* cache = (T*) mem_alloc(BLOCK_SIZE);
	uint32_t* bitmask_cache = (uint32_t*) mem_alloc(BLOCK_SIZE / sizeof(T) / 32 * sizeof(uint32_t));

	bool (*_pred_f)(uint64_t const, uint64_t const) = get_pred(DPU_INPUT_ARGUMENTS.predicate);

	for (unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS) {
		memset(bitmask_cache, 0, BLOCK_SIZE / sizeof(T) / 32 * sizeof(uint32_t));
		mram_read((__mram_ptr void const*)(mram_base_addr + byte_index), cache, BLOCK_SIZE);
		if (byte_index + (BLOCK_SIZE - 1) >= input_size_dpu_bytes) {
			for (unsigned int i = 0; byte_index + (i * sizeof(T)) < input_size_dpu_bytes; i++) {
				bitmask_cache[i/32] |= (_pred_f(cache[i], predicate_arg) * 1) << (i%32);
			}
		} else {
			for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T) / 32; i++) {
				for (unsigned int j = 0; j < 32; j++) {
					bitmask_cache[i] |= (_pred_f(cache[i*32+j], predicate_arg) * 1) << j;
				}
			}
		}
		mram_write(bitmask_cache, (__mram_ptr void*)(bitmask_base_addr + byte_index / sizeof(T) / 32 * sizeof(uint32_t)), BLOCK_SIZE / sizeof(T) / 32 * sizeof(uint32_t));
	}

	return 0;
}

int main_kernel_update()
{
	unsigned int tasklet_id = me();

	if (tasklet_id == 0) {
		mem_reset();
	}

	uint32_t input_size_dpu_bytes = DPU_INPUT_ARGUMENTS.size;
	uint64_t predicate_arg = DPU_INPUT_ARGUMENTS.predicate_arg;

	// Address of the current processing block in MRAM
	uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
	uint32_t mram_base_addr = (uint32_t)DPU_MRAM_HEAP_POINTER;
	uint32_t bitmask_base_addr = mram_base_addr + 65011712;

	T* cache = (T*) mem_alloc(BLOCK_SIZE);
	uint32_t* bitmask_cache = (uint32_t*) mem_alloc(BLOCK_SIZE / sizeof(T) / 32 * sizeof(uint32_t));

	for (unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS) {
		mram_read((__mram_ptr void*)(mram_base_addr + byte_index), cache, BLOCK_SIZE);
		mram_read((__mram_ptr void*)(bitmask_base_addr + byte_index / sizeof(T) / 32 * sizeof(uint32_t)), bitmask_cache, BLOCK_SIZE / sizeof(T) / 32 * sizeof(uint32_t));
		if (byte_index + (BLOCK_SIZE - 1) >= input_size_dpu_bytes) {
			for (unsigned int i = 0; byte_index + (i * sizeof(T)) < input_size_dpu_bytes; i++) {
				if (bitmask_cache[i/32] & (1<<i)) {
					cache[i] = predicate_arg;
				}
			}
		} else {
			for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T) / 32; i++) {
				for (unsigned int j = 0; j < 32; j++) {
					if (bitmask_cache[i] & (1<<j)) {
						cache[i*32+j] = predicate_arg;
					}
				}
			}
		}
		mram_write(cache, (__mram_ptr void*)(mram_base_addr + byte_index), BLOCK_SIZE);
	}

	return 0;
}
