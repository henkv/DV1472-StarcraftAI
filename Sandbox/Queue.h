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

public:

	Queue(void)
	{
		length = 0;
		root = NULL;
		tail = NULL;
	}

	virtual ~Queue(void)
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
};
