#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include <functional>


class Semaphore
{
public:
	int permits;
	pthread_mutex_t mutex;
	pthread_cond_t cv;

	Semaphore(int permits);
	Semaphore();

	~Semaphore();

	void acquire();
	void release();
	void set_permits(int permits);
};
