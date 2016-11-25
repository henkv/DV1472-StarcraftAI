#include <iostream>
using namespace std;

#include "ArrayList.h"
#include "Queue.h"

class Command
{
	virtual bool run() = 0;
};

int main()
{




	struct:Command
	{
		bool run()
		{
			
			cout << "Jump!" << endl;
			return true;
		}
	} jumpCommand;





	ArrayList<Command*> activeCommands;
	
	activeCommands.add(new (struct:Command {
		
	}));

	bool run = true;
	size_t i, size;
	while (run)
	{

		size = activeCommands.size();
		if (size > 0)
		{
			for (i = 0; i < size; i++)
			{
				if (activeCommands[i]->run())
				{
					delete activeCommands[i];
					activeCommands.remove(i);
					break;
				}
			}
		}
		else
		{
			Command* nextStep = NULL;
			if (stepQueue.dequeue(nextStep))
			{
				game->sendText("Run nextStep: %d", nextStep);
				nextStep->run(this);
				delete nextStep;
			}
		}
	}

	cin.get();
	return 0;
}