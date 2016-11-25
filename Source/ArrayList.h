#pragma once

template<typename T>
class ArrayList
{
private:
	T** elements;
	size_t nrOfElements;
	size_t elementCapacity;

	void expand()
	{
		elementCapacity += 10;
		T** newElements = new T*[elementCapacity];

		for (size_t i = 0; i < nrOfElements; i++)
		{
			newElements[i] = elements[i];
		}

		delete elements;
		elements = newElements;
	}
	void freeMemory()
	{
		for (size_t i = 0; i < nrOfElements; i++)
		{
			delete elements[i];
		}
		delete[] elements;
	}

	void copyFrom(ArrayList const& other)
	{
		freeMemory();
		nrOfElements = other.nrOfElements;
		elementCapacity = other.elementCapacity;
		elements = new T*[elementCapacity];

		for (size_t i = 0; i < other.nrOfElements; i++)
		{
			elements[i] = new T(*other.elements[i]);
		}
	}

public:
	ArrayList(size_t capacity = 10)
	{
		nrOfElements = 0;
		elementCapacity = capacity;
		elements = new T*[elementCapacity];
	}

	ArrayList(ArrayList const& other)
	{
		copyFrom(other);		
	}

	~ArrayList()
	{
		freeMemory();	
	}

	
	ArrayList& operator=(ArrayList const& other)
	{
		if (this != &other)
		{
			copyFrom(other);
		}

		return *this;
	}

	T& operator[](size_t index)
	{
		if (index >= nrOfElements)
			throw "out of range";

		return *elements[index];
	}
	
	void clear()
	{
		for (size_t i = 0; i < nrOfElements; i++)
		{
			delete elements[i];
		}

		nrOfElements = 0;
	}

	void add(T const& element)
	{
		if (nrOfElements == elementCapacity)
			expand();

		elements[nrOfElements++] = new T(element);
	}

	void remove(size_t index)
	{
		if (index >= nrOfElements)
			throw "out of range";

		delete elements[index];
		elements[index] = elements[--nrOfElements];
	}

	void remove(T const& element)
	{
		for	(size_t i = 0; i < nrOfElements; i++)
		{
			if (element == *elements[i])
			{
				remove(i);
				break;
			}
		}
	}

	size_t size()
	{
		return nrOfElements;
	}


};
