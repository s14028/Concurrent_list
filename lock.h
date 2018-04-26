#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>


class ReadWriteLock
{
	int active_readers_count;
	pthread_mutex_t reader_mutex;
	pthread_mutex_t reader_count_mutex;
	pthread_cond_t count_cv;

	unsigned int counter;

	static unsigned int i;

public:

	ReadWriteLock(bool d = true);

	~ReadWriteLock();

	void read_acquire();
	void read_release();

	void write_acquire();
	void write_release();

private:
	void wait_till_readers_finish();
};
