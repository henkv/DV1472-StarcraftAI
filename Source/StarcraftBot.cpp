#include "StarcraftBot.h"

// Local classes //
struct StarcraftBot::Box
{
	Position topLeft;
	Position bottomRight;
	Color color;

	Box()
	{
		topLeft = Positions::None;
		bottomRight = Positions::None;
		color = Colors::Black;
	}

	Box(Position _topLeft, Position _bottomRight, Color _color)
	{
		topLeft = _topLeft;
		bottomRight = _bottomRight;
		color = _color;
	}
};

class StarcraftBot::Command 
{
public:
	StarcraftBot* bot;
	virtual bool run() = 0;

	Command(StarcraftBot* _bot)
		: bot(_bot){}
};


class StarcraftBot::BuildCommand : public Command
{ 
private:
	UnitType type;
	Unit*    worker;
	
	bool done;
	bool refinery;
	bool reserved;

	Position lastPos;
	Position movePos;
	TilePosition buildPos;

	Box box;
	Box clearBox;

public:
	BuildCommand(StarcraftBot* bot, UnitType _type)
		: Command(bot)
	{
		type = _type;
		worker = NULL;

		done = false;
		refinery = type.isRefinery();
		reserved = false;

		lastPos = Positions::None;
		movePos = Positions::None;
		buildPos = TilePositions::None;

		box.color = Colors::Purple;
		clearBox.color = Colors::Red;
	};

	void findBuildPos()
	{
		if (refinery) buildPos = bot->getClosestGeyser(worker)->getTilePosition();
		else		  buildPos = bot->getBuildTile(worker, type);

		movePos = Position(buildPos) + Position(type.tileWidth() * 16 - 1, type.tileHeight() * 16 - 1);

		Position topLeft = Position(buildPos);
		Position bottomRight = topLeft + Position(type.tileWidth() * 32, type.tileHeight() * 32);

		box.topLeft = topLeft;
		box.bottomRight = bottomRight;

		clearBox.topLeft	 = box.topLeft	   - Position(16, 16);
		clearBox.bottomRight = box.bottomRight + Position(16, 16);
	}

	bool run()
	{ 
		if (!reserved)
		{
			if (bot->canAfford(type))
			{
				bot->reserveUnitCost(type);
				reserved = true;

				worker = bot->getWorker();
				findBuildPos();
				
				bot->buildBoxes.add(&box);
				bot->buildBoxes.add(&clearBox);
			}
		}
		else
		{
			bot->focusWorker = worker;
			if (worker->isCarryingMinerals() || worker->isCarryingGas())
			{
				worker->returnCargo();
			}
			else if (!refinery && worker->getPosition() != movePos)
			{
				if (worker->getPosition() != lastPos && !worker->isStuck())
				{
					worker->move(movePos);
					lastPos = worker->getPosition();
				}
				else
				{
					findBuildPos();
				}
			}
			else 
			{
				if (worker->build(buildPos, type))
				{
					done = true;
					bot->buildBoxes.remove(&box);
					bot->buildBoxes.remove(&clearBox);
				}
				else
				{
					findBuildPos();
				}
			}				
		}

		return done;
	}
};


class StarcraftBot::TrainCommand : public Command
{ 
private:
	UnitType type;
	bool done;

public:
	TrainCommand(StarcraftBot* bot, UnitType _type) 
		: Command(bot)
	{
		type = _type;
		done = false;
	};

	bool run()
	{ 
		if (bot->canAfford(type))
		{
			Unit* trainer = bot->getTrainer(type);

			if (trainer != NULL && trainer->train(type))
			{
				done = true;
			}
		}

		return done;
	}
};
// --- //

// Event Handlers //
void StarcraftBot::onStart()
{
	focusWorker = NULL;
	buildBoxes.clear();

	stepQueue.clear();
	activeCommands.clear();

	reservedGas = 0;
	reservedMinerals = 0;



	constructBuilding(UnitTypes::Terran_Supply_Depot);
	
}

void StarcraftBot::onFrame()
{
	size_t i, size;
	if (Broodwar->getFrameCount() % 25 == 0)
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
				Broodwar->sendText("Run nextStep: %d", nextStep);
				nextStep->run();
				delete nextStep;
			}
		}
	}

	Broodwar->drawTextScreen(30, 300, "%d Active Commands, %d Minerals Reserved, %d Gas Reserved, %d buildBoxes",
		activeCommands.size(),
		reservedMinerals,
		reservedGas,
		buildBoxes.size());

	static Error lastError = Broodwar->getLastError();

	if (lastError.getID() >= 0 && lastError.getID() < Errors::Unknown)
	{
		Broodwar->drawTextScreen(170, 355, "Last Error %d: %s", lastError.getID(), lastError.c_str());
	}


	for (size_t i = 0, size = buildBoxes.size(); i < size; i++)
	{
		Broodwar->drawBoxMap(
			buildBoxes[i]->topLeft.x(),
			buildBoxes[i]->topLeft.y(),
			buildBoxes[i]->bottomRight.x(),
			buildBoxes[i]->bottomRight.y(),
			buildBoxes[i]->color
		);
	}


	if (focusWorker != NULL)
	{
		UnitType type = UnitTypes::Terran_Supply_Depot;
		TilePosition startPosition = focusWorker->getTilePosition();
		TilePosition testPosition;

		//game->setScreenPosition(focusWorker->getPosition() - Position(320, 200));

		for (int i = 0, i2, x, y; i < 8; i++)
		{
			i2 = i * 2;
			for (y = 0; y <= i2; y++)
			{
				for (x = 0; x <= i2; y == 0 || y == i2 ? x++ : x += i2)
				{
					testPosition = startPosition + TilePosition(x - i, y - i);
					if (Broodwar->canBuildHere(focusWorker, testPosition, type, true))
					{
						if (!buildingTooClose(testPosition, type))
						{
							Broodwar->drawCircleMap(testPosition.x() * 32 + 16, testPosition.y() * 32 + 16, 8, Colors::Green);
						}
						else
						{
							Broodwar->drawCircleMap(testPosition.x() * 32 + 16, testPosition.y() * 32 + 16, 8, Colors::Grey);
						}
					}
				}
			}
		}
	}
}

void StarcraftBot::onUnitCreate(BWAPI::Unit* unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		if (unit->getType().isBuilding())
		{
			releaseUnitCost(unit->getType());
		}
	}
}

void StarcraftBot::onUnitComplete(BWAPI::Unit *unit)
{
	UnitType type = unit->getType();


	if (type == UnitTypes::Terran_SCV)
	{
		unit->rightClick(getClosestMineral(unit));
	}
	else if (type == UnitTypes::Terran_Marine)
	{
		Position guardPoint = getGuardPoint();
		unit->rightClick(guardPoint);
	}
	else if (type == UnitTypes::Terran_Refinery)
	{
		getWorker()->rightClick(unit);
	}
	else if (type.isBuilding())
	{
		idleWorkers();
	}
}

void StarcraftBot::onUnitDestroy(BWAPI::Unit* unit)
{
}
// --- //

// Get Functions //
// Units
Unit* StarcraftBot::getWorker()
{
	Unit* worker = NULL;
	set<Unit*> units = Broodwar->self()->getUnits();

	set<Unit*>::iterator i;
	for (i = units.begin(); i != units.end() && worker == NULL; i++)
	{
		if ((*i)->getType().isWorker() && (*i)->isGatheringMinerals())
		{
			worker = *i;
		}
	}

	return worker;
}

Unit* StarcraftBot::getTrainer(UnitType type)
{
	Unit* trainer = NULL;
	UnitType source = type.whatBuilds().first;
	set<Unit*> units = Broodwar->self()->getUnits();

	set<Unit*>::iterator i;
	for (i = units.begin(); i != units.end() && trainer == NULL; i++)
	{
		if ((*i)->getType() == source)
		{
			if (!(*i)->isTraining())
			{
				trainer = *i;
			}
		}
	}

	return trainer;
}




Unit* StarcraftBot::getClosestGeyser(Unit* unit)
{
	Unit* closestGeyser = NULL;
	set<Unit*> geysers = Broodwar->getGeysers();

	set<Unit*>::iterator i;
	for (i = geysers.begin(); i != geysers.end(); i++)
	{
		if ((closestGeyser = NULL) ||
		    (unit->getDistance(*i) < unit->getDistance(closestGeyser)))
		{	
			closestGeyser = *i;
		}
	}

	return closestGeyser;
}
Unit* StarcraftBot::getClosestMineral(Unit* unit)
{
	Unit* closestMineral = NULL;
	set<Unit*> minerals = Broodwar->getMinerals();

	set<Unit*>::iterator i;
	for (i = minerals.begin(); i != minerals.end(); i++)
	{
		if (closestMineral == NULL ||
			unit->getDistance(*i) < unit->getDistance(closestMineral))
		{	
			closestMineral = *i;
		}
	}

	return closestMineral;
}
// Positions
Position StarcraftBot::getGuardPoint()
{
	double length;
	double min_length = 10000;
	Chokepoint* choke = NULL;

	//Get the chokepoints linked to our home region
	BWTA::Region* home = getStartLocation(Broodwar->self())->getRegion();
	set<Chokepoint*> chokepoints = home->getChokepoints();

	//Iterate through all chokepoints and look for the one with the smallest gap (least width)
	set<Chokepoint*>::iterator c;
	for(c = chokepoints.begin(); 
		c != chokepoints.end();
		c++)
	{
		length = (*c)->getWidth();
		if (length < min_length || choke == NULL)
		{
			min_length = length;
			choke = *c;
		}
	}

	return choke->getCenter();
}

TilePosition StarcraftBot::getBuildTile(Unit* worker, UnitType type)
{
	TilePosition startPosition = worker->getTilePosition();
	TilePosition testPosition;

	for (int i = 0, i2, x, y; i < 64; i++)
	{
		i2 = i * 2;
		for (y = 0; y <= i2; y++)
		{
			for (x = 0; x <= i2; y == 0 || y == i2 ? x++ : x += i2)
			{
				testPosition = startPosition + TilePosition(x - i, y - i);
				if (Broodwar->canBuildHere(worker, testPosition, type, true))
				{
					if (!buildingTooClose(testPosition, type))
					{
						return testPosition;
					}
				}
			}
		}
	}

	return TilePositions::None;
}
// --- //

// Help Functions //
bool StarcraftBot::buildingTooClose(TilePosition tile, UnitType type)
{
	static const Position margin = Position(16, 16);
	int buildings = 0;
	Position topLeft = Position(tile);
	Position bottomRight = topLeft + Position(type.tileWidth() * 32, type.tileHeight() * 32) ;

	set<Unit*> units = Broodwar->getUnitsInRectangle(
		topLeft - margin, 
		bottomRight + margin 
	);

	std::set<Unit*>::iterator b;
	for(b = units.begin(); b != units.end(); b++)
	{
		if ((*b)->getType().isBuilding())
		{	
			buildings++;
		}
	}

	return buildings > 0;
}







bool StarcraftBot::canAfford(UnitType unit)
{
	Player* self = Broodwar->self();
	return (self->gas() - reservedGas) >= unit.gasPrice() &&
		   (self->minerals() - reservedMinerals) >= unit.mineralPrice();
}


void StarcraftBot::reserveUnitCost(UnitType unit)
{
	reservedGas += unit.gasPrice();
	reservedMinerals += unit.mineralPrice();
}


void StarcraftBot::releaseUnitCost(UnitType unit)
{
	reservedGas -= unit.gasPrice();
	reservedMinerals -= unit.mineralPrice();

	if (reservedGas < 0) reservedGas = 0;
	if (reservedMinerals < 0) reservedMinerals = 0;
}
// --- //

// Action Functions //
void StarcraftBot::trainUnit(UnitType type, int amount)
{
	for (int i = 0; i < amount; i++)
		activeCommands.add(new TrainCommand(this, type));
}

void StarcraftBot::constructBuilding(UnitType type, int amount)
{
	for (int i = 0; i < amount; i++)
		activeCommands.add(new BuildCommand(this, type));
}

void StarcraftBot::idleWorkers()
{
	set<Unit*> units = Broodwar->self()->getUnits();
	
	set<Unit*>::iterator i;
	for (i = units.begin(); i != units.end(); i++)
	{
		if ((*i)->getType().isWorker() && (*i)->isIdle())
		{
			(*i)->rightClick(getClosestMineral(*i));
		}
	}
}
// --- //