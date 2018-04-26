#include "lock.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

unsigned int ReadWriteLock::i = 0;

ReadWriteLock::ReadWriteLock(bool d) : active_readers_count(0)
{
	counter = i++;
	reader_mutex = PTHREAD_MUTEX_INITIALIZER;
	reader_count_mutex = PTHREAD_MUTEX_INITIALIZER;
	count_cv = PTHREAD_COND_INITIALIZER;
}

ReadWriteLock::~ReadWriteLock()
{
	pthread_mutex_destroy(&reader_mutex);
	pthread_mutex_destroy(&reader_count_mutex);
	pthread_cond_destroy(&count_cv);
}

void ReadWriteLock::read_acquire()
{
	pthread_mutex_lock(&reader_mutex);
	//printf("Zajmuje %d\n", counter);
	pthread_mutex_lock(&reader_count_mutex);

	active_readers_count++;

	pthread_mutex_unlock(&reader_count_mutex);
	//printf("Zwalniam %d\n", counter);
	pthread_mutex_unlock(&reader_mutex);
}


void ReadWriteLock::read_release()
{
	//printf("TU %d\n", counter);
	pthread_mutex_lock(&reader_count_mutex);
	//printf("TU1 %d\n", counter);

	active_readers_count--;

	if(active_readers_count == 0)
	{
		pthread_cond_signal(&count_cv);
	}

	pthread_mutex_unlock(&reader_count_mutex);
}

void ReadWriteLock::wait_till_readers_finish()
{
	pthread_mutex_lock(&reader_count_mutex);

	if(active_readers_count > 0)
	{
		pthread_cond_wait(&count_cv, &reader_count_mutex);
	}

	pthread_mutex_unlock(&reader_count_mutex);
}

void ReadWriteLock::write_acquire()
{
	pthread_mutex_lock(&reader_mutex);

	wait_till_readers_finish();
}

void ReadWriteLock::write_release()
{
	pthread_mutex_unlock(&reader_mutex);
}
