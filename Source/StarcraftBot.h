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
	class TechCommand;


	Unit* focusWorker;
	ArrayList<Box*> buildBoxes;

	Queue<Command*> stepQueue;
	ArrayList<Command*> activeCommands;
	
	int reservedGas;
	int reservedMinerals;

public:
	// Event Handlers //
	void onStart();
	void onFrame();
	void onUnitCreate(BWAPI::Unit* unit);
	void onUnitComplete(BWAPI::Unit *unit);
	void onUnitDestroy(BWAPI::Unit* unit);
	// --- //

	// Get Functions //
	// Units
	Unit* getWorker();
	Unit* getTrainer(UnitType type);	
	Unit* getClosestGeyser(Unit* unit);
	Unit* getClosestMineral(Unit* unit);
	// Positions
	Position	 getGuardPoint();
	TilePosition getBuildTile(Unit* worker, UnitType type);
	// --- //

	// Help Functions //
	bool canBuildHere(Unit* worker, TilePosition tile, UnitType type);
	bool buildingTooClose(TilePosition tile, UnitType type);

	bool canAfford(UnitType unit);
	void reserveUnitCost(UnitType unit);
	void releaseUnitCost(UnitType unit);
	// --- //

	// Action Functions //
	void trainUnit(UnitType type, int amount = 1);
	void researchTech(UnitType addon, int amount = 1);
	void constructAddon(UnitType addon, int amount = 1);
	void constructBuilding(UnitType building, int amount = 1);

	void idleWorkers();
	// --- //
};
