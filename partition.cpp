#include "partition.h"

const unsigned int Partition::max_length = 10;

Partition::Partition(int index) : begin_index(index), length(0)
{
}

bool Partition::operator==(const Partition& partition)
{
	return this == &partition;
}

ReadWriteLock& Partition::lock()
{
	return read_write_lock;
}
