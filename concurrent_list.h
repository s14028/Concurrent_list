#pragma once
#include <iostream>
#include <set>
#include <functional>
#include <list>
#include <memory>
#include <iterator>
#include <tuple>
#include "partition.h"
#include "semaphore.h"
#include "atomic.h"
#include "priority_queue.h"
#include "lock.h"

template <typename T>
void* garbage_collector(void* arguments);



template <typename T>
class concurrent_list
{
	std::list<Partition> partitions;

	pthread_t pthread_id;
	bool is_destroyed;
	atomic<unsigned int> destroyed_elements_count;

	Semaphore collector_semaphore;
	Semaphore insert_semaphore;

	ReadWriteLock shrinker_lock;

	std::list<T> elements;

public:
	concurrent_list() :
		is_destroyed(false),
		destroyed_elements_count(0),
		collector_semaphore(0)
	{
		init_new_partition(0);
		
		pthread_create(&pthread_id, 0, garbage_collector<T>, (void*) this);
	}

	void close()
	{
		is_destroyed = true;
		collector_semaphore.release();
		pthread_join(pthread_id, (void**) 0);
	}


	void append(T element)
	{
		insert_semaphore.acquire();
		shrinker_lock.read_acquire();

		elements.push_back(element);

		auto last_partition = std::prev(partitions.end());
		(*last_partition).lock().write_acquire();

		if(could_fit_new_element(*last_partition))
		{
			unsigned int index = (*last_partition).begin() + (*last_partition).size();
			auto position = elements.begin();
			std::advance(position, index);
			std::iter_swap(position, std::prev(elements.end()));
			
			(*last_partition).add_element();
			(*last_partition).lock().write_release();
		}
		else
		{
			(*last_partition).lock().write_release();
			Partition& npartition = init_new_partition(elements.size() - 1);

			npartition.lock().write_acquire();
			npartition.add_element();
			npartition.lock().write_release();
		}

		shrinker_lock.read_release();
		insert_semaphore.release();
	}

	bool remove_element(T element)
	{
		shrinker_lock.read_acquire();

		auto begin = elements.begin();
		unsigned int last_index = 0;
		for(Partition& partition : partitions)
		{
			partition.lock().read_acquire();

			std::advance(begin, partition.begin() - last_index);
			auto end = begin;
			std::advance(end, partition.size());
			last_index = partition.begin();

			auto position = std::find(begin, end, element);

			partition.lock().read_release();

			if(position != end)
			{
				partition.lock().write_acquire();

				end = begin;
				std::advance(end, partition.size());

				position = std::find(begin, end, element);

				if(position != end)
				{
					push_to_end(position, end);
					partition.remove_element();

					partition.lock().write_release();

					destroyed_elements_count++;

					trigger();
					shrinker_lock.read_release();
					return true;
				}
				partition.lock().write_release();
			}
		}

		shrinker_lock.read_release();
		return false;
	}

	bool search(T element)
	{
		shrinker_lock.read_acquire();

		auto begin = elements.begin();
		unsigned int last_index = 0;
		for(Partition& partition : partitions)
		{
			partition.lock().read_acquire();

			std::advance(begin, partition.begin() - last_index);
			auto end = begin;
			std::advance(end, partition.size());
			last_index = partition.begin();

			auto position = std::find(begin, end, element);

			partition.lock().read_release();

			if(position != end)
			{
				shrinker_lock.read_release();
				return true;
			}
		}

		shrinker_lock.read_release();
		return false;
	}

	template <typename R>
	friend std::ostream& operator<<(std::ostream& stream, concurrent_list<R>& list)
	{
		list.shrinker_lock.write_acquire();
		stream << "Size of list is " << list.elements.size() << "\n"
		<< "Number of partitions is " << list.partitions.size() << "\n";
		
		auto begin = list.elements.begin();
		unsigned int last_index = 0;
		for(Partition& partition : list.partitions)
		{
			std::advance(begin, partition.begin() - last_index);
			auto end = begin;
			std::advance(end, partition.size());
			last_index = partition.begin();

			while(begin != end)
			{
				std::cout << *begin << " ";
				last_index++;
				begin++;
			}

			for(unsigned int i = partition.size(); i < Partition::max_length; i++)
			{
				std::cout << "* ";
			}
		}
		list.shrinker_lock.write_release();

		return stream;
	}

private:

	bool could_fit_new_element(const Partition& partition)
	{
		const unsigned int begin = partition.begin();
		const unsigned int size = Partition::max_length;

		return begin + size >= elements.size();
	}

	void trigger()
	{
		double ratio = static_cast<double>((*destroyed_elements_count)) / elements.size();
		if(ratio > 0.15)
		{
			collector_semaphore.release();
		}
	}


	template <typename R>
	friend void* garbage_collector(void* arguments);

	void shrink_to_fit()
	{
		auto first = first_partition_to_merge();
		auto last = remove(first, partitions.end());

		auto pair = elements_to_remove(last);

		elements.erase(std::get<0>(pair), elements.end());
		partitions.erase(std::get<1>(pair), partitions.end());

		destroyed_elements_count = 0;
	}

	auto elements_to_remove(auto last)
	{
		if(last == partitions.begin())
		{
			return std::make_pair(elements.begin(), ++last);
		}

		auto prev = std::prev(last);
		if((*last).size() > 0)
		{
			prev++;
			last++;
		}

		auto begin = elements.begin();
		std::advance(begin, (*prev).begin() + (*prev).size());
		return std::make_pair(begin, last);
	}

	void push_to_end(auto begin, auto end)
	{
		for(auto current = std::next(begin); current != end; begin++, current++)
		{
			std::iter_swap(begin, current);
		}
	}

	auto first_partition_to_merge()
	{
		return std::find_if(partitions.begin(), partitions.end(), [](const Partition& partition)
		{
			return partition.size() < Partition::max_length;
		});
	}

	auto remove(auto left, auto right)
	{
		unsigned int last_size = (*left).size();
		unsigned int last_left_size = last_size;

		auto current = std::next(left);

		auto to = elements.begin();
		auto from = elements.begin();
		auto count = elements.begin();

		std::advance(to, (*left).begin() + (*left).size());
		std::advance(from, (*left).begin() + (*left).size());

		for(; current != right; current++)
		{
			last_left_size = last_size;
			std::advance(from, Partition::max_length - last_size);
			count = from;
			std::advance(count, (*current).size());
			last_size = (*current).size();

			(*current).clear();

			while(from != count)
			{
				std::iter_swap(to, from);
				from++;
				to++;
				(*left).add_element();

				if((*left).size() >= Partition::max_length)
				{
					left++;
				}
			}
		}

		return left;
	}

	Partition& init_new_partition(unsigned int index)
	{
		partitions.emplace_back(index);
		auto partition = std::prev(partitions.end());

		return *partition;
	}
};

template <typename T>
void* garbage_collector(void* arguments)
{
	concurrent_list<T>* list = static_cast<concurrent_list<T>*>(arguments);
	while(!list->is_destroyed)
	{
		list->collector_semaphore.acquire();
		if(list->is_destroyed)
		{
			return (void*) 0;
		}
		list->shrinker_lock.write_acquire();

		list->shrink_to_fit();
		list->collector_semaphore.set_permits(0);

		list->shrinker_lock.write_release();
	}
	return (void*) 0;
}
