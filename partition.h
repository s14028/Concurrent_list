#pragma once

#include <iostream>
#include "lock.h"

class Partition
{
	unsigned int begin_index;
	int length;

	ReadWriteLock read_write_lock;

public:
	static const unsigned int max_length;

	Partition(int index);

	inline unsigned int begin() const
	{
		return begin_index;
	}

	inline int size() const
	{
		return length;
	}

	inline void clear()
	{
		length = 0;
	}

	inline void add_element()
	{
		length++;
	}

	inline void remove_element()
	{
		length--;
	}

	inline bool operator<(const Partition& partition) const
	{
		return length < partition.length;
	}

	inline bool operator>(const Partition& partition) const
	{
		return length > partition.length;
	}

	inline bool is_destroyed() const
	{
		return length == 0;
	}

	bool operator==(const Partition&);

	ReadWriteLock& lock();
};
