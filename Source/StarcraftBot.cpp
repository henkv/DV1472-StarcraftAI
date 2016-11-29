#include "StarcraftBot.h"

// Local Classes
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

	void drawMap()
	{
		Broodwar->drawBoxMap(
			topLeft.x(),
			topLeft.y(),
			bottomRight.x(),
			bottomRight.y(),
			color
		);
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
	Unit*	 geyser;
	
	bool done;
	bool isRefinery;
	bool costReserved;

	Position lastPos;
	Position movePos;
	TilePosition buildPos;

	Box box;

public:
	BuildCommand(StarcraftBot* bot, UnitType _type)
		: Command(bot)
	{
		type = _type;
		worker = NULL;
		geyser = NULL;

		done = false;
		isRefinery = type.isRefinery();

		costReserved = false;

		lastPos = Positions::None;
		movePos = Positions::None;
		buildPos = TilePositions::None;

		box.color = Colors::Purple;
	};

	void findBuildPos()
	{
		if (isRefinery) 
		{
			geyser = getClosestGeyser(worker);
			buildPos = geyser->getTilePosition();
			bot->focusGeyser = geyser;
		}
		else
		{
			buildPos = bot->getBuildTile(worker, type);
		}

		movePos = Position(buildPos);
		movePos.x() += type.tileWidth() * 16;
		movePos.y() += type.tileHeight() * 16;

		Position topLeft = Position(buildPos);
		Position bottomRight = topLeft + Position(type.tileWidth() * 32, type.tileHeight() * 32);

		box.topLeft = topLeft;
		box.bottomRight = bottomRight;
	}

	bool refineryCheck()
	{
		bool output = false;

		Position geyserPos = geyser->getPosition();

		set<Unit*> onGeyser = Broodwar->getUnitsInRectangle(
			geyserPos.x()-200,
			geyserPos.y()-100,
			geyserPos.x()+200,
			geyserPos.y()+100
		);

		set<Unit*>::iterator i;
		for (i = onGeyser.begin(); i != onGeyser.end(); i++)
		{
			Broodwar->sendText("%s on geyser", (*onGeyser.begin())->getType().getName().c_str());
		}


		return true;
	}

	bool run()
	{ 
		 if (!costReserved)
		{
			if (bot->canAfford(type) &&
				bot->haveRequirements(type))
			{
				bot->reserveUnitCost(type);
				costReserved = true;

				if (isRefinery)
				{
					worker = bot->stealWorker();
				}
				else
				{
					worker = bot->getWorker();
				}
				Broodwar->sendText("Worker[%d] ready to work",
							worker->getID());

				findBuildPos();				
				bot->buildBoxes.add(&box);
			}
		}
		else if (worker->isCarryingMinerals() || worker->isCarryingGas())
		{
			worker->returnCargo();
		}
		else if (!done)
		{
			Position workerPos = worker->getPosition();

			if (workerPos != movePos && !isRefinery)
			{
				if (lastPos != workerPos)
				{
					worker->move(movePos);
					lastPos = workerPos;
				}
				else
				{
					Broodwar->sendText("Worker[%d] got stuck",
						worker->getID());
					
					findBuildPos();
					worker->move(movePos);
				}
			}
			else
			{
				if (worker->build(buildPos, type))
				{	
					done = true;

					bot->buildBoxes.remove(&box);
					Broodwar->sendText("Worker[%d] started to build",
						worker->getID()
					);
				}
				else
				{
					Broodwar->sendText("Worker[%d] failed to build",
						worker->getID()
					);
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
	UnitType unitType;
	UnitType sourceType;

	bool done;

public:
	TrainCommand(StarcraftBot* bot, UnitType _unitType) 
		: Command(bot)
	{
		unitType = _unitType;
		sourceType = unitType.whatBuilds().first;

		done = false;
	};

	bool run()
	{ 
		if (bot->canAfford(unitType))
		{
			Unit* source = bot->getIdleUnit(sourceType);

			if (source != NULL && source->train(unitType))
			{
				done = true;
			}
		}

		return done;
	}
};

class StarcraftBot::AddonCommand : public Command
{ 
private:
	UnitType addonType;
	UnitType sourceType;

	Unit* source;
	bool done;

public:
	AddonCommand(StarcraftBot* bot, UnitType _addonType)
		: Command(bot)
	{
		addonType = _addonType;
		sourceType = addonType.whatBuilds().first;

		source = NULL;
		done = false;
	};


	bool run()
	{ 
		if (bot->canAfford(addonType))
		{
			if (source == NULL)
			{
				source = bot->getIdleUnit(sourceType);
				//Broodwar->sendText("%s[%d] wants addon", sourceType.getName().c_str(), source->getID());
			}
			else
			{
				source->buildAddon(addonType);

				if (source->getAddon() != NULL)
				{
					Broodwar->sendText("Building[%d] has addon", source->getID());
					done = true;
				}
			}			
		}

		return done;
	}
};
class StarcraftBot::ResearchCommand : public Command
{
private:
	TechType techType;
	UnitType sourceType;

	bool done;

public:
	ResearchCommand(StarcraftBot *bot, TechType _techType) 
		: Command(bot) 
	{
		techType = _techType;
		sourceType = techType.whatResearches();

		done = false;
	}

	bool run()
	{
		if (bot->canAfford(UnitType(techType)))
		{
			Unit* source = bot->getIdleUnit(sourceType);

			if (source != NULL && source->research(techType))
			{
				done = true;
			}
		}
		
		return done;
	}
};




class StarcraftBot::Army 
	: private ArrayList<Unit*>
{
private:
	Position position;
	int maxDistance;

	bool inArcliteRange(Unit* siegeTank)
	{
		static int minRange = WeaponTypes::Arclite_Shock_Cannon.minRange();
		static int maxRange = WeaponTypes::Arclite_Shock_Cannon.maxRange();

		bool foundEnemy = false;
		Player* self = Broodwar->self();
		Player* enemy = Broodwar->enemy();
		
		set<Unit*>::iterator i;
		set<Unit*> units = siegeTank->getUnitsInRadius(maxRange);
		for(i = units.begin(); i != units.end() && !foundEnemy; i++)
		{
			if ((*i)->getPlayer() == enemy && 
				(*i)->isVisible() &&
				!(*i)->isBurrowed() &&
				!(*i)->getType().isFlyer())
			{
				if (siegeTank->getDistance(*i) > minRange)
				{
					foundEnemy = true;
				}
			}
		}

		return foundEnemy;
	}

	Position getCenter()
	{
		Position center;
		Unit* unit;

		size_t s = size();

		if (s > 0)
		{
			for (size_t i = 0; i < s; i++)
			{
				unit = operator[](i);
				if (!unit->exists())
				{
					remove(i--);
					s--;
				}
				else
				{
					center += unit->getPosition();
				}
			}

			center.x() /= s;
			center.y() /= s;
		}
		else
		{
			center = position;
		}

		return center;
	}

public:
	Army() : ArrayList()
	{
		maxDistance = 200;
	}
	
	void clear() 
	{
		ArrayList::clear();
	}

	size_t size() 
	{
		return ArrayList::size();
	}

	void add(Unit* unit)
	{
		ArrayList::add(unit);
	}
	
	void setAttackPosition(Position pos)
	{
		position = pos;
	}
	void setAttackPosition(TilePosition pos)
	{
		setAttackPosition(Position(pos));
	}

	void attackPosition()
	{
		Position center = getCenter();
		Unit* unit;

		for (size_t i = 0, s = size(); i < s; i++)
		{
			unit = operator[](i);

			if (unit->getType().isFlyer())
			{
				unit->attack(center);
			}
			else
			{
				unit->attack(position);
			}
		}
	}

	void siegeCheck()
	{
		Unit* tank;
		for (size_t i = 0, s = size(); i < s; i++)
		{
			tank = operator[](i);
			if (tank->getType() == UnitTypes::Terran_Siege_Tank_Tank_Mode ||
				tank->getType() == UnitTypes::Terran_Siege_Tank_Siege_Mode)
			{
				if (inArcliteRange(tank))
				{
					tank->siege();
				}
				else
				{
					tank->unsiege();
				}
			}
		}	
	}


	void regroup()
	{
		Position center = getCenter();
		Unit* unit;

		for (size_t i = 0, s = size(); i < s; i++)
		{
			unit = operator[](i);
			if (unit->getDistance(center) > maxDistance)
			{
				unit->attack(center);
			}			
		}
	}

	void debugDraw()
	{
		if (size() > 0)
		{
			Position center = getCenter();
			Position unitPos;

			for (size_t i = 0, s = size(); i < s; i++)
			{
				unitPos = operator[](i)->getPosition();
				Broodwar->drawLineMap(
					unitPos.x(),
					unitPos.y(),
					center.x(),
					center.y(), 
					Colors::Green);
			}

			Broodwar->drawLineMap(
				center.x(), 
				center.y(), 
				position.x(), 
				position.y(), 
				Colors::Red);
			Broodwar->drawCircleMap(
				center.x(),
				center.y(),
				maxDistance,
				Colors::Orange);
			}
	}
};

class StarcraftBot::WorkerGroup 
	: private Queue<Unit*>
{
private:
	static void idleCheck(Unit* &worker)
	{
		if (worker->isIdle())
		{
			worker->gather(getClosestMineral(worker));
		}
	}

public:
	void add(Unit *worker)
	{
		Queue::queue(worker);
	}

	void clear() 
	{
		Queue::clear();
	}

	size_t size()
	{
		return Queue::size();
	}

	
	Unit* getWorker()
	{
		Unit* worker = NULL;
		size_t tries = size();

		while (worker == NULL && tries > 0)
		{
			Queue::dequeue(worker);
			Queue::queue(worker);
			tries--;

			if (!worker->isGatheringMinerals())
				worker == NULL;
		}		

		return worker;
	}

	Unit* stealWorker()
	{
		Unit* worker = NULL;
		size_t tries = size();

		while (tries > 0)
		{
			Queue::dequeue(worker);

			if (worker->isGatheringMinerals())
				return worker;

			Queue::queue(worker);
			tries--;
		}		

		return NULL;
	}

	void idleWorkers()
	{
		forEach(idleCheck);
	}
};

// Constructor and Deconstructor //
StarcraftBot::StarcraftBot()
	: army(*new Army()), workers(*new WorkerGroup())
{
}

StarcraftBot::~StarcraftBot()
{
	delete &army;
	delete &workers;
}

// Event Handlers //
void StarcraftBot::onStart()
{
	BWTA::analyze();

	focusGeyser = NULL;

	buildBoxes.clear();

	stepQueue.clear();
	activeCommands.clear();

	army.clear();
	workers.clear();

	reservedGas = 0;
	reservedMinerals = 0;

	supplyDepots = 0;
	haveBarracks = false;
	refineryDone = false;


	army.setAttackPosition(getGuardPoint());

	struct BuildStep : Command
	{
		BuildStep(StarcraftBot *bot) : Command(bot) {}
		bool run()
		{
			
			bot->constructBuilding(UnitTypes::Terran_Factory);
			bot->constructAddon(UnitTypes::Terran_Machine_Shop);
			bot->trainUnit(UnitTypes::Terran_Siege_Tank_Tank_Mode, 3);
			bot->researchTech(TechTypes::Tank_Siege_Mode);
			
			bot->constructBuilding(UnitTypes::Terran_Barracks, 2);
			bot->trainUnit(UnitTypes::Terran_Marine, 30);
			
			bot->constructBuilding(UnitTypes::Terran_Academy);
			bot->constructBuilding(UnitTypes::Terran_Refinery);
			bot->trainUnit(UnitTypes::Terran_Medic, 6);
			bot->trainUnit(UnitTypes::Terran_SCV, 10);

			return true;
		}
	};


	struct AttackStep : Command
	{
		AttackStep(StarcraftBot *bot) : Command(bot) {}

		bool run()
		{
			Broodwar->sendText("EXTERMINATE! Frame %d", Broodwar->getFrameCount());
			bot->army.setAttackPosition(Broodwar->enemy()->getStartLocation());
	

			bot->constructBuilding(UnitTypes::Terran_Starport);
			bot->constructAddon(UnitTypes::Terran_Control_Tower);
			bot->constructBuilding(UnitTypes::Terran_Science_Facility);

			bot->trainUnit(UnitTypes::Terran_Science_Vessel);


			return true;
		}
	};

	stepQueue.queue(new BuildStep(this));
	stepQueue.queue(new AttackStep(this));
}

void StarcraftBot::onFrame()
{
	size_t frame = Broodwar->getFrameCount();

	if (frame % 25 == 0)
	{
		runCommands();
		supplyCheck();
	}
	if (frame % 50 == 0)
	{
		workers.idleWorkers();
		army.siegeCheck();
	}
	if (frame % 100 == 0)
	{
		army.attackPosition();
		army.regroup();
	}

	drawBuildTiles();
	drawBuildSites();
	army.debugDraw();
	drawAllUnits();

	drawStatusText();
}


void StarcraftBot::onEnd()
{

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
	if (unit->getPlayer() == Broodwar->self())
	{
		UnitType type = unit->getType();

		if (type == UnitTypes::Terran_SCV)
		{
			workers.add(unit);
			unit->gather(getClosestMineral(unit));
		}
		else if (type == UnitTypes::Terran_Barracks)
		{
			haveBarracks = true;
		}
		else if (!type.isBuilding() && !type.isAddon())
		{
			army.add(unit);
		}
	}
}

void StarcraftBot::onUnitDestroy(BWAPI::Unit* unit)
{
	if(unit->getPlayer() == Broodwar->self())
	{
		UnitType type = unit->getType();

		if (type.isBuilding())
		{
			if (type.isAddon())
			{
				constructAddon(type);
			}
			else
			{
				constructBuilding(type);
			}
		}
		else
		{
			if (type == UnitTypes::Terran_Siege_Tank_Siege_Mode)
				type = UnitTypes::Terran_Siege_Tank_Tank_Mode;

			trainUnit(type);
		}
	}
}
// Get Functions //
Unit* StarcraftBot::getWorker()
{
	return workers.getWorker();
}
Unit* StarcraftBot::stealWorker()
{
	return workers.stealWorker();
}

Unit* StarcraftBot::getIdleUnit(UnitType type)
{
	Unit* idleUnit = NULL;

	set<Unit*>::iterator i;
	set<Unit*> units = Broodwar->self()->getUnits();
	for (i = units.begin(); i != units.end() && idleUnit == NULL; i++)
	{
		if ((*i)->getType() == type && (*i)->isIdle())
		{
			idleUnit = *i;
		}
	}

	return idleUnit;
}




Unit* StarcraftBot::getClosestGeyser(Unit* unit)
{
	Unit* closestGeyser = NULL;
	set<Unit*> geysers = Broodwar->getGeysers();

	set<Unit*>::iterator i;
	for (i = geysers.begin(); i != geysers.end(); i++)
	{
		if ((closestGeyser == NULL) ||
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

	Position initPos = Position(Broodwar->self()->getStartLocation());

	set<Unit*>::iterator i;
	for (i = minerals.begin(); i != minerals.end(); i++)
	{
		if (closestMineral == NULL ||
			initPos.getDistance((*i)->getPosition()) < initPos.getDistance(closestMineral->getPosition()))
		{	
			closestMineral = *i;
		}
	}

	return closestMineral;
}

Position	 StarcraftBot::getGuardPoint()
{
	double length;
	double min_length = 10000;
	Chokepoint* choke = NULL;

	//Get the chokepoints linked to our home region
	BWTA::Region* home = getStartLocation(Broodwar->self())->getRegion();
	set<Chokepoint*> chokepoints = home->getChokepoints();

	//Iterate through all chokepoints and look for the one with the smallest gap (least width)
	set<Chokepoint*>::iterator c;
	for(c = chokepoints.begin(); c != chokepoints.end(); c++)
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

	if (canBuildHere(worker, startPosition, type))
		return startPosition;

	size_t r, d1, d2, d3, d4, i;
	for (r = 1; r < 64; r++)
	{
		d1 = r + r;
		d2 = d1 + d1;
		d3 = d2 + d1;
		d4 = d3 + d1;

		testPosition = startPosition;
		testPosition.x() -= r;
		testPosition.y() -= r;

		for (i = 0; i < d4; i++)
		{
			if (i < d1)
				testPosition.x()++;
			else if (i < d2)
				testPosition.y()++;
			else if (i < d3)
				testPosition.x()--;
			else
				testPosition.y()--;

			if (canBuildHere(worker, testPosition, type))
				return testPosition;
		}
	}

	return TilePositions::None;
}

// Help Functions //
bool StarcraftBot::canBuildHere(const Unit* worker, TilePosition tile, UnitType type)
{
	return (Broodwar->canBuildHere(worker, tile, type, true) &&
			Broodwar->canBuildHere(worker, tile + TilePosition(2,1), type, true) &&
			!buildingTooClose(tile, type));
			
}

bool StarcraftBot::buildingTooClose(TilePosition tile, UnitType type)
{
	static const Position margin = Position(64, 32);
	bool tooClose = false;

	Position topLeft = Position(tile);
	Position bottomRight = topLeft + Position(type.tileWidth() * 32, type.tileHeight() * 32) ;

	set<Unit*> units = Broodwar->getUnitsInRectangle(
		topLeft - margin, 
		bottomRight + margin 
	);

	std::set<Unit*>::iterator b;
	for(b = units.begin(); b != units.end() && !tooClose; b++)
	{
		if ((*b)->getType().isBuilding())
		{	
			tooClose = true;
		}
	}

	return tooClose;
}

bool StarcraftBot::canAfford(UnitType unit)
{
	Player* self = Broodwar->self();
	return (self->gas() - reservedGas) >= unit.gasPrice() &&
		   (self->minerals() - reservedMinerals) >= unit.mineralPrice();
}
bool StarcraftBot::haveRequirements(UnitType unit)
{
	bool output = true;

	if (unit == UnitTypes::Terran_Academy)
	{
		output = haveBarracks;
	}

	return output;
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
// Action Functions //
void StarcraftBot::runCommands()
{
	size_t i, size = activeCommands.size();
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
			nextStep->run();
			delete nextStep;
		}
	}
}

void StarcraftBot::trainUnit(UnitType type, int amount)
{
	Broodwar->sendText("Unit %s added to queue", type.getName().c_str());
	for (int i = 0; i < amount; i++)
		activeCommands.add(new TrainCommand(this, type));
}

void StarcraftBot::researchTech(TechType tech, int amount)
{
	Broodwar->sendText("Tech %s added to queue", tech.getName().c_str());
	for (int i = 0; i < amount; i++)
		activeCommands.add(new ResearchCommand(this, tech));
}

void StarcraftBot::constructAddon(UnitType addon, int amount)
{
	Broodwar->sendText("Addon %s added to queue", addon.getName().c_str());
	for (int i = 0; i < amount; i++)
		activeCommands.add(new AddonCommand(this, addon));
}

void StarcraftBot::constructBuilding(UnitType type, int amount)
{
	Broodwar->sendText("Building %s added to queue", type.getName().c_str());
	for (int i = 0; i < amount; i++)
		activeCommands.add(new BuildCommand(this, type));
}


void StarcraftBot::supplyCheck()
{
	int supplyDepotNeed = ((Broodwar->self()->supplyUsed() / 2) + 4) / 8;

	if (supplyDepots < supplyDepotNeed)
	{
		constructBuilding(UnitTypes::Terran_Supply_Depot);
		supplyDepots++;
	}
}

void StarcraftBot::idleWorkers()
{
	workers.idleWorkers();
}
// Debug Functions //
void StarcraftBot::drawBuildSites()
{
	for (size_t i = 0, size = buildBoxes.size(); i < size; i++)
	{
		buildBoxes[i]->drawMap();
	}
}

void StarcraftBot::drawBuildTiles()
{
	TilePosition startPosition = Broodwar->self()->getStartLocation();
	UnitType type = UnitTypes::Terran_SCV;
	Unit* worker = getWorker();

	TilePosition testPosition;
	Position drawPos;

	size_t r, d1, d2, d3, d4, i;
	for (r = 1; r < 64; r++)
	{
		d1 = r + r;
		d2 = d1 + d1;
		d3 = d2 + d1;
		d4 = d3 + d1;

		testPosition = startPosition;
		testPosition.x() -= r;
		testPosition.y() -= r;

		for (i = 0; i < d4; i++)
		{
			if (i < d1)
				testPosition.x()++;
			else if (i < d2)
				testPosition.y()++;
			else if (i < d3)
				testPosition.x()--;
			else
				testPosition.y()--;

			if (canBuildHere(worker, testPosition, type))
			{
				drawPos = Position(testPosition);
				Broodwar->drawBoxMap(
					drawPos.x() + 2,
					drawPos.y() + 2,
					drawPos.x() + 30,
					drawPos.y() + 30,
					Colors::Grey
				);
			}
		}
				
	}
}

void StarcraftBot::drawStatusText()
{
	Broodwar->drawTextScreen(
		-32, 4,
		"\
		%d Active commands\n\
		%d Minerals reserved\n\
		%d Gas reserved\n\
		%d Buildings pending\n\
		%d Units in army",
		activeCommands.size(),
		reservedMinerals,
		reservedGas,
		buildBoxes.size(),
		army.size());
}
void StarcraftBot::drawAllUnits()
{
	Unit* unit;
	Position unitPos;

	set<Unit*> units = Broodwar->self()->getUnits();

	set<Unit*>::iterator i;
	for (i = units.begin(); i != units.end(); i++)
	{
		unit = *i;
		unitPos = unit->getPosition();

		Broodwar->drawTextMap(
			unitPos.x(), 
			unitPos.y(), 
			"[%d]%s",
			unit->getID(),
			unit->getType().getName().c_str());
		
	}
}
