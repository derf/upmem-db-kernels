#pragma once

#if DFATOOL_TIMING

#include <sys/time.h>

#define dfatool_printf(fmt, ...) do { printf(fmt, __VA_ARGS__); } while (0)

struct timeval starttime;
struct timeval stoptime;

void startTimer()
{
	gettimeofday(&starttime, NULL);
}

double stopTimer()
{
	gettimeofday(&stoptime, NULL);

	return (stoptime.tv_sec - starttime.tv_sec) * 1000000.0 + (stoptime.tv_usec - starttime.tv_usec);
}

#else // DFATOOL_TIMING

#define dfatool_printf(fmt, ...) do {} while (0)

void startTimer() {}

double stopTimer()
{
	return 0.0;
}

#endif
