#pragma once

#include "SqlDataSource.h"
#include "DataSourceObject.h"
#include "Database/SqlStatement.h"

namespace Poco { namespace Util { class AbstractConfiguration; }; };
class SqlObjDataSource : public SqlDataSource, public ObjDataSource
{
public:
	SqlObjDataSource(Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf);
	~SqlObjDataSource() {}

	void populateObjects( int serverId, ServerObjectsQueue& queue ) override;
	bool updateObjectInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory ) override;
	bool deleteObject( int serverId, Int64 objectIdent, bool byUID ) override;
	bool updateVehicleMovement( int serverId, Int64 objectIdent, const Sqf::Value& worldspace, double fuel ) override;
	bool updateVehicleStatus( int serverId, Int64 objectIdent, const Sqf::Value& hitPoints, double damage ) override;
	bool createObject( int serverId, const string& className, int characterId, const Sqf::Value& worldSpace, Int64 uniqueId ) override;

private:
	string _depTableName;
	string _vehTableName;
	bool _objectOOBReset;

	// Statement ids
	SqlStatementID _stmtDeleteOldObject;
	SqlStatementID _stmtUpdateObjectByUID;
	SqlStatementID _stmtUpdateObjectByID;
	SqlStatementID _stmtDeleteObjectByUID;
	SqlStatementID _stmtDeleteObjectByID;
	SqlStatementID _stmtUpdateVehicleMovement;
	SqlStatementID _stmtUpdateVehicleStatus;
	SqlStatementID _stmtCreateObject;
};
