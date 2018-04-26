#pragma once

#include "semaphore.h"

template <typename T>
class atomic
{
	Semaphore semaphore;
	T element;
public:
	atomic(T element) : element(element)
	{

	}

	T operator*()
	{
		semaphore.acquire();
		T element = this->element;
		semaphore.release();
		return element;
	}

	T operator=(T element)
	{
		semaphore.acquire();
		this->element = element;
		semaphore.release();
	}

	T operator++()
	{
		semaphore.acquire();
		T value = ++element;
		semaphore.release();
		return value;
	}

	T operator++(int)
	{
		semaphore.acquire();
		T value = element++;
		semaphore.release();
		return value;
	}

	T operator--()
	{
		semaphore.acquire();
		T previous = element--;
		semaphore.release();
		return previous;
	}

};
