#include "semaphore.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

Semaphore::Semaphore(int permits) : permits(permits)
{
	mutex = PTHREAD_MUTEX_INITIALIZER;
	cv = PTHREAD_COND_INITIALIZER;
}

Semaphore::Semaphore() : Semaphore(1)
{
}

Semaphore::~Semaphore()
{
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);
}


void Semaphore::acquire()
{
	pthread_mutex_lock(&mutex);

	while(permits == 0)
	{
		pthread_cond_wait(&cv, &mutex);
	}

	permits--;

	pthread_mutex_unlock(&mutex);
}


void Semaphore::release()
{
	pthread_mutex_lock(&mutex);

	permits++;
	pthread_cond_signal(&cv);

	pthread_mutex_unlock(&mutex);
}

void Semaphore::set_permits(int permits)
{
	this->permits = permits;
}
