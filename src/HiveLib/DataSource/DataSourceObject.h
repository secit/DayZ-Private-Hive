#pragma once

#include "DataSource.h"

class ObjDataSource
{
public:
	virtual ~ObjDataSource() {}

	typedef std::queue<Sqf::Parameters> ServerObjectsQueue;
	virtual void populateObjects( int serverId, ServerObjectsQueue& queue ) = 0;

	virtual bool updateObjectInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory ) = 0;
	virtual bool deleteObject( int serverId, Int64 objectIdent, bool byUID ) = 0;
	virtual bool updateVehicleMovement( int serverId, Int64 objectIdent, const Sqf::Value& worldspace, double fuel ) = 0;
	virtual bool updateVehicleStatus( int serverId, Int64 objectIdent, const Sqf::Value& hitPoints, double damage ) = 0;
	virtual bool createObject( int serverId, const string& className, int characterId, const Sqf::Value& worldSpace, Int64 uniqueId ) = 0;
};
