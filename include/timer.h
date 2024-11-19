#pragma once

#include <sys/time.h>

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
