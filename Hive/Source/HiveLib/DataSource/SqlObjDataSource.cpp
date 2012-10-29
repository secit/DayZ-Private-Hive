/*
* Copyright (C) 2009-2012 Rajko Stojadinovic <http://github.com/rajkosto/hive>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "SqlObjDataSource.h"
#include "Database/Database.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

namespace
{
	typedef boost::optional<Sqf::Value> PositionInfo;
	class PositionFixerVisitor : public boost::static_visitor<PositionInfo>
	{
	public:
		PositionInfo operator()(Sqf::Parameters& pos) const 
		{ 
			if (pos.size() != 3)
				return PositionInfo();

			try
			{
				double x = Sqf::GetDouble(pos[0]);
				double y = Sqf::GetDouble(pos[1]);
				double z = Sqf::GetDouble(pos[2]);

				/*scoped_ptr<QueryResult> limitRes(getDB()->PQuery("select `max_x`, `max_y` from `instance` where `id` = '%s'", getServerId()));
				int max_x = 0;
				int max_y = 15360;

				if (limitRes) { // override defaults if results exist
					limitRes->NextRow();
					Field* fields = limitRes->Fetch();

					max_x = fields[0].GetInt32();
					max_y = fields[1].GetInt32();
				}*/
				int max_x = 0;
				int max_y = 15360;

				if (x < max_x || y > max_y)
				{
					PositionInfo fixed(pos);
					pos.clear();
					return fixed;
				}
			}
			catch(const boost::bad_get&) {}

			return PositionInfo();
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	class WorldspaceFixerVisitor : public boost::static_visitor<PositionInfo>
	{
	public:
		PositionInfo operator()(Sqf::Parameters& ws) const 
		{
			if (ws.size() != 2)
				return PositionInfo();

			return boost::apply_visitor(PositionFixerVisitor(),ws[1]);
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	PositionInfo FixOOBWorldspace(Sqf::Value& v) { return boost::apply_visitor(WorldspaceFixerVisitor(),v); }
};

#include <Poco/Util/AbstractConfiguration.h>
SqlObjDataSource::SqlObjDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	_depTableName = getDB()->escape_string(conf->getString("Table","instance_deployable"));
	_vehTableName = getDB()->escape_string(conf->getString("Table","instance_vehicle"));
	_objectOOBReset = conf->getBool("ResetOOBObjects",false);
}

void SqlObjDataSource::populateObjects( int serverId, ServerObjectsQueue& queue )
{	
	scoped_ptr<QueryResult> worldObjsRes(getDB()->PQuery("select iv.id, v.class_name, null owner_id, iv.worldspace, iv.inventory, iv.parts, iv.fuel, iv.damage from `%s` iv inner join `vehicle` v on iv.`vehicle_id` = v.`id` where iv.`instance_id` = %d union select id.`id`, d.`class_name`, id.`owner_id`, id.`worldspace`, id.`inventory`, '[]', 0, 0 from `%s` id inner join `deployable` d on id.`deployable_id` = d.`id` where id.`instance_id` = %d", _vehTableName.c_str(), serverId, _depTableName.c_str(), serverId));
	if (worldObjsRes) do
	{
		Field* fields = worldObjsRes->Fetch();
		Sqf::Parameters objParams;
		objParams.push_back(string("OBJ"));

		int objectId = fields[0].GetInt32();
		objParams.push_back(lexical_cast<string>(objectId)); //objectId should be stringified
		try
		{
			objParams.push_back(fields[1].GetCppString()); //classname
			objParams.push_back(lexical_cast<string>(fields[2].GetInt32())); //ownerId should be stringified

			Sqf::Value worldSpace = lexical_cast<Sqf::Value>(fields[3].GetString());
			if (_objectOOBReset)
			{
				PositionInfo posInfo = FixOOBWorldspace(worldSpace);
				if (posInfo.is_initialized())
					_logger.information("Reset ObjectID " + lexical_cast<string>(objectId) + " (" + fields[1].GetCppString() + ") from position " + lexical_cast<string>(*posInfo));

			}
			
			objParams.push_back(worldSpace);

			//Inventory can be NULL
			{
				string invStr = "[]";
				if (!fields[4].IsNULL())
					invStr = fields[4].GetCppString();

				objParams.push_back(lexical_cast<Sqf::Value>(invStr));
			}	
			objParams.push_back(lexical_cast<Sqf::Value>(fields[5].GetString()));
			objParams.push_back(fields[6].GetDouble());
			objParams.push_back(fields[7].GetDouble());
		}
		catch (bad_lexical_cast)
		{
			_logger.error("Skipping ObjectID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(objParams);
	} while(worldObjsRes->NextRow());
}

bool SqlObjDataSource::updateObjectInventory( int serverId, Int64 objectIdent, bool byUID, const Sqf::Value& inventory )
{
	scoped_ptr<SqlStatement> stmt;
	if (byUID) // infer that if byUID, it is a deployable - by id, a vehicle
	{
		static SqlStatementID stmtId;
		stmt.reset(getDB()->CreateStatement(stmtId, "update `"+_depTableName+"` SET `inventory` = ? where `unique_id` = ?"));
	}
	else
	{
		static SqlStatementID stmtId;
		stmt.reset(getDB()->CreateStatement(stmtId, "update `"+_vehTableName+"` SET `inventory` = ? where `id` = ?"));
	}

	stmt->addString(lexical_cast<string>(inventory));
	stmt->addInt64(objectIdent);

	bool exRes = stmt->Execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::deleteObject( int serverId, Int64 objectIdent, bool byUID )
{
	scoped_ptr<SqlStatement> stmt;
	if (byUID) // infer that if byUID, it is a deployable - by id, a vehicle
	{
		static SqlStatementID stmtId;
		stmt.reset(getDB()->CreateStatement(stmtId, "delete from `"+_depTableName+"` where `unique_id` = ?"));
	}
	else
	{
		static SqlStatementID stmtId;
		stmt.reset(getDB()->CreateStatement(stmtId, "delete from `"+_vehTableName+"` where `id` = ?"));
	}
	stmt->addInt64(objectIdent);

	bool exRes = stmt->Execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::updateVehicleMovement( int serverId, Int64 objectIdent, const Sqf::Value& worldSpace, double fuel )
{
	static SqlStatementID stmtId;
	scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, "update `"+_vehTableName+"` set `worldspace` = ? , `fuel` = ? where `id` = ?"));
	stmt->addString(lexical_cast<string>(worldSpace));
	stmt->addDouble(fuel);
	stmt->addInt64(objectIdent);
	bool exRes = stmt->Execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::updateVehicleStatus( int serverId, Int64 objectIdent, const Sqf::Value& hitPoints, double damage )
{
	static SqlStatementID stmtId;
	scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, "update `"+_vehTableName+"` SET `parts` = ? , `damage` = ? where `id` = ?"));
	stmt->addString(lexical_cast<string>(hitPoints));
	stmt->addDouble(damage);
	stmt->addInt64(objectIdent);
	bool exRes = stmt->Execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlObjDataSource::createObject( int serverId, const string& className, int characterId, const Sqf::Value& worldSpace, Int64 objectIdent )
{
	static SqlStatementID stmtId;
	scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, 
		"insert into `"+_depTableName+"` (`unique_id`, `deployable_id`, `owner_id`, `instance_id`, `worldspace`, `created`) "
		"select ?, d.id, ?, ?, ?, CURRENT_TIMESTAMP from deployable d where d.class_name = ?"));

	stmt->addInt64(objectIdent);
	stmt->addInt32(characterId);
	stmt->addInt32(serverId);
	stmt->addString(lexical_cast<string>(worldSpace));
	stmt->addString(lexical_cast<string>(className));
	bool exRes = stmt->Execute();
	poco_assert(exRes == true);

	return exRes;
}

