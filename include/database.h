#pragma once
#include <popcntintrin.h>

static T* database;
static uint32_t* bitmasks;
unsigned int n_elements;

static void create_db()
{
	for (unsigned int i = 0; i < n_elements; i++) {
		database[i] = i + 1;
	}
}

static void host_realloc(unsigned int n_elem)
{
	database = realloc(database, n_elem * sizeof(T));
	assert(database != NULL);

	bitmasks = realloc(bitmasks, n_elem / 32 * sizeof(uint32_t) + sizeof(uint32_t));
	assert(bitmasks != NULL);
}

static unsigned int host_count(enum predicates pred, unsigned int pred_arg)
{
	unsigned int count = 0;
	bool (*_pred_f)(uint64_t const, uint64_t const) = get_pred(pred);

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(database[i], pred_arg)) {
			count++;
		}
	}
	return count;
}

static void host_insert(unsigned int n_insert)
{
	for (unsigned int i = n_elements; i < n_elements + n_insert; i++) {
		database[i] = i + 1;
	}
	n_elements += n_insert;
}

static unsigned int host_update(enum predicates pred, unsigned int pred_arg)
{
	unsigned int count = 0;
	bool (*_pred_f)(uint64_t const, uint64_t const) = get_pred(pred);

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(database[i], pred_arg)) {
			database[i] = pred_arg;
			count++;
		}
	}
	return count;
}

static unsigned int host_delete(enum predicates pred, unsigned int pred_arg)
{
	bool (*_pred_f)(uint64_t const, uint64_t const) = get_pred(pred);
	unsigned int n_delete = 0;
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(database[i], pred_arg)) {
			n_delete += 1;
		} else if (n_delete) {
			database[i-n_delete] = database[i];
		}
	}
	n_elements -= n_delete;
	return n_delete;
}

static void host_select(uint32_t* out, enum predicates pred, unsigned int pred_arg)
{
	bool (*_pred_f)(uint64_t const, uint64_t const) = get_pred(pred);
	memset(bitmasks, 0, n_elements * sizeof(uint32_t) / 32 + sizeof(uint32_t));

	#pragma omp parallel for
	for (unsigned int i = 0; i < n_elements/32; i++) {
		for (unsigned int j = 0; j < 32; j++) {
			out[i] |= (_pred_f(database[i*32+j], pred_arg) * 1) << j;
		}
	}

	for (unsigned int j = 0; j < n_elements % 32; j++) {
		out[n_elements/32] |= (_pred_f(database[(n_elements/32)*32 + j], pred_arg) * 1) << j;
	}
}

static unsigned int count_bits(uint32_t* masks)
{
	unsigned int count = 0;

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements/32; i++) {
		count += _mm_popcnt_u32(masks[i]);
	}

	for (unsigned int j = 0; j < n_elements % 32; j++) {
		if (masks[n_elements/32] & (1<<j)) {
			count += 1;
		}
	}

	return count;
}

