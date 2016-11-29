#pragma once
#define NULL 0
#include "Queue.h"
#include "ArrayList.h"

#include <BWAPI.h>
#include <BWTA.h>

using namespace std;
using namespace BWAPI;
using namespace BWTA;

class StarcraftBot
{
private:
	struct Box;
	class Command;
	class BuildCommand;
	class TrainCommand;
	class AddonCommand;
	class ResearchCommand;

	class Army;
	class WorkerGroup;


	Unit* focusGeyser;

	ArrayList<Box*> buildBoxes;

	Queue<Command*> stepQueue;
	ArrayList<Command*> activeCommands;

	Army &army;
	WorkerGroup &workers;
	
	int reservedGas;
	int reservedMinerals;
	int supplyDepots;

	bool haveBarracks;
	bool refineryDone;

public:
	// Constructor and Deconstructor //
	StarcraftBot();
	~StarcraftBot();

	// Event Handlers //
	void onStart();
	void onFrame();
	void onEnd();

	void onUnitCreate(BWAPI::Unit* unit);
	void onUnitComplete(BWAPI::Unit *unit);
	void onUnitDestroy(BWAPI::Unit* unit);

	// Get Functions //
	Unit* getWorker();
	Unit* stealWorker();
	Unit* getIdleUnit(UnitType type);
	static Unit* getClosestGeyser(Unit* unit);
	static Unit* getClosestMineral(Unit* unit);

	Position	 getGuardPoint();
	TilePosition getBuildTile(Unit* worker, UnitType type);

	// Help Functions //
	bool canBuildHere(const Unit* worker, TilePosition tile, UnitType type);
	bool buildingTooClose(TilePosition tile, UnitType type);

	bool canAfford(UnitType unit);
	bool haveRequirements(UnitType unit);
	void reserveUnitCost(UnitType unit);
	void releaseUnitCost(UnitType unit);

	// Action Functions //
	void runCommands();
	void supplyCheck();
	void idleWorkers();

	void trainUnit(UnitType type, int amount = 1);
	void researchTech(TechType tech, int amount = 1);
	void constructAddon(UnitType addon, int amount = 1);
	void constructBuilding(UnitType building, int amount = 1);

	// Debug Functions //
	void drawBuildSites();
	void drawBuildTiles();
	void drawStatusText();
	void drawAllUnits();
};
