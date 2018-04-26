#include "concurrent_list.h"
#include "semaphore.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <tuple>
#include <memory>
#include <random>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

int begin = 0;
int end = 100'000;

int random(int begin, int end)
{
	static std::random_device r;
	static std::default_random_engine d(r());
	static std::uniform_int_distribution<int> adistribution(begin, end);
	return adistribution(d);
}

void enter_message(const std::string& test)
{
	printf("%s", test.c_str());
	#ifdef flush
	fflush(stdout);
	#endif
}

class TestInput
{
	unsigned int number_of_iterations;
	unsigned int number_of_threads;
	concurrent_list<int> list;
	std::vector<int> elements;
public:
	TestInput(unsigned int number_of_iterations, unsigned int number_of_threads = 1, unsigned int number_of_elements = 1) :
	number_of_iterations(number_of_iterations), number_of_threads(number_of_threads), elements(number_of_elements)
	{
		
	}

	unsigned int get_iterations() const
	{
		return number_of_iterations;
	}
	
	unsigned int get_thread_number() const
	{
		return number_of_threads;
	}

	void close()
	{
		list.close();
	}

	void insert(int element)
	{
		#ifdef verbose
		printf("Inserting element %d.\n", element);
		#endif
		list.append(element);
		#ifdef verbose
		printf("Done.\n");
		#endif
	}

	void search(int element)
	{
		#ifdef verbose
		printf("Searching element %d.\n", element);
		#endif
		list.search(element);
		#ifdef verbose
		printf("Done.\n");
		#endif
	}

	void remove(int element)
	{
		#ifdef verbose
		printf("Deleting element %d.\n", element);
		#endif
		list.remove_element(element);
		#ifdef verbose
		printf("Done.\n");
		#endif
	}

	void generate_elements()
	{
		std::generate(elements.begin(), elements.end(),
		[]()
		{
			return random(begin, end);
		});
	}

	std::vector<int>& get_elements()
	{
		return elements;
	}
};

inline void print_error_message()
{
	if(errno == -1)
	{
		enter_message("Couldn't create a thread.\n");
	}
}

void run_parallel(void* (*function)(void*), void* input)
{
	TestInput* in = static_cast<TestInput*>(input);
	unsigned int number_of_threads = in->get_thread_number();
	std::vector<pthread_t> ids(number_of_threads);

	#ifdef verbose
	printf("Will create %d.\n", number_of_threads);
	printf("--------------------------------------------\n");
	#endif

	std::for_each(ids.begin(), ids.end(),
	[&function, input](pthread_t& id)
	{
		errno = pthread_create(&id, 0, function, input);
		print_error_message();
	});

	std::for_each(ids.begin(), ids.end(),
	[](const pthread_t& id)
	{
		pthread_join(id, (void**) 0);
	});
	#ifdef verbose
	printf("--------------------------------------------\n");
	#endif
}

void* execute(void* value, const std::function<void(TestInput*, int)>& function)
{
	TestInput* input = static_cast<TestInput*>(value);
	unsigned int number_of_iterations = input->get_iterations();
	std::vector<int>& elements = input->get_elements();

	for(unsigned int i = 0; i < number_of_iterations; i++)
	{
		std::for_each(elements.begin(), elements.end(),
		[&function, &input](int element)
		{
			function(input, element);	
		});
	}
}

void* insert(void* value)
{
	execute(value, [](TestInput* input, int value) { input->insert(value); });
}

void* search(void* value)
{
	execute(value, [](TestInput* input, int value) { input->search(value); });
}

void* remove(void* value)
{
	execute(value, [](TestInput* input, int value) { input->remove(value); });
}

void* inserting(void* input)
{
	run_parallel(insert, input);
}

void* searching(void* input)
{
	run_parallel(search, input);
}

void* removing(void* input)
{
	run_parallel(remove, input);
}

void* inserting_removing(void* input)
{
	pthread_t id;
	void* pointer;
	pthread_create(&id, 0, inserting, input);
	removing(input);
	pthread_join(id, &pointer);
}

void* inserting_searching(void* input)
{
	pthread_t id;
	void* pointer;
	pthread_create(&id, 0, inserting, input);
	searching(input);
	pthread_join(id, &pointer);
}

int main(int argc, char** args)
{
	std::shared_ptr<TestInput> test = std::make_shared<TestInput>(1u, 10u, 100u);

	std::vector<std::pair<std::pair<std::function<void*(void*)>, std::string>, std::shared_ptr<TestInput>>> tests;
	tests.push_back({{inserting, "inserting"}, test});
	tests.push_back({{searching, "searching"}, test});
	tests.push_back({{inserting_removing, "inserting_removing"}, test});
	std::for_each(tests.begin(), tests.end(),
	[](auto& pair)
	{
		const std::string& name = std::get<1>(std::get<0>(pair));
		std::shared_ptr<TestInput> input = std::get<1>(pair);
		input->generate_elements();
		printf("[ Entering test %s ]\n", name.c_str());

		std::get<0>(std::get<0>(pair))((void*) input.get());


		printf("[ OK ]\n\n");
	});
	printf("Done testing.\n");
	test->close();
	return 0;
}
