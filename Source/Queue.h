#pragma once
#define NULL 0

template<typename T>
class Queue
{
private:
	struct Node
	{
		T data;
		Node* next;

		Node(T const& _data, Node* _next = NULL) 
			: data(_data), next(_next) {};
	};

	size_t length;
	Node* root;
	Node* tail;

	void freeMemory()
	{
		Node* walker = root;
		Node* trash;

		while (walker != NULL)
		{
			trash = walker;
			walker = walker->next;
			delete trash;
		}
	}
	void copyFrom(Queue const& other)
	{
		length = 0;
		Node* walker = other.root;

		while (walker != NULL)
		{
			queue(walker->data);
			walker = walker->next;
		}
	}
public:
	Queue()
	{
		length = 0;
		root = NULL;
		tail = NULL;
	}

	Queue(Queue const& other)
	{
		copyFrom(other);
	}

	Queue& operator=(Queue const& other)
	{
		if (this != &other)
		{
			freeMemory();
			copyFrom(other);
		}

		return *this;
	}

	virtual ~Queue()
	{
		freeMemory();
	}

	void clear()
	{
		freeMemory();
		length = 0;
	}

	void queue(T const& data)
	{
		if (length > 0)
		{
			tail->next = new Node(data);
			tail = tail->next;
		}
		else
		{
			root = new Node(data);
			tail = root;
		}

		length++;
	}

	bool dequeue(T& data)
	{
		Node* trash;
		bool output = false;

		if (length > 0)
		{
			trash = root;
			data = trash->data;
			root = trash->next;
			delete trash;
			length--;

			output = true;
		}

		return output;
	}

	bool dequeue()
	{
		Node* trash;
		bool output = false;

		if (length > 0)
		{
			trash = root;
			root = trash->next;
			delete trash;
			length--;

			output = true;
		}

		return output;
	}

	size_t size()
	{
		return length;
	}

	T& first()
	{
		return root->data;
	}

	void forEach(void (*fun)(T &data))
	{
		Node* walker = root;

		while (walker != NULL)
		{
			fun(walker->data);
			walker = walker->next;
		}
	}
};