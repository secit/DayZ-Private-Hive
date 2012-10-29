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

#include "SqlCharDataSource.h"
#include "Database/Database.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

SqlCharDataSource::SqlCharDataSource( Poco::Logger& logger, shared_ptr<Database> db, const string& idFieldName, const string& wsFieldName ) : SqlDataSource(logger,db)
{
	_idFieldName = getDB()->escape_string(idFieldName);
	_wsFieldName = getDB()->escape_string(wsFieldName);
}

SqlCharDataSource::~SqlCharDataSource() {}

Sqf::Value SqlCharDataSource::fetchCharacterInitial( string playerId, int serverId, const string& playerName )
{
	bool newPlayer = false;
	//make sure player exists in db
	{
		scoped_ptr<QueryResult> playerRes(getDB()->PQuery("select `name` from `profile` WHERE `unique_id` = '%s'", getDB()->escape_string(playerId).c_str()));
		if (playerRes)
		{
			newPlayer = false;
			Field* fields = playerRes->Fetch();
			//update player name if not current
			if (fields[0].GetCppString() != playerName)
			{
				static SqlStatementID stmtId;
				scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, "update `profile` set `name` = ? where `unique_id` = ?"));
				stmt->addString(playerName);
				stmt->addString(playerId);
				bool exRes = stmt->Execute();
				poco_assert(exRes == true);
				_logger.information("Changed name of player " + playerId + " from '" + fields[0].GetCppString() + "' to '" + playerName + "'");
			}
		}
		else
		{
			newPlayer = true;
			//insert new player into db
			static SqlStatementID stmtId;
			scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, "insert into profile (`unique_id`, `name`) values (?, ?)"));
			stmt->addString(playerId);
			stmt->addString(playerName);
			bool exRes = stmt->Execute();
			poco_assert(exRes == true);
			_logger.information("Created a new player " + playerId + " named '" + playerName + "'");
		}
	}

	//get characters from db
	scoped_ptr<QueryResult> charsRes(getDB()->PQuery(
		"select s.`id`, s.`worldspace`, s.`inventory`, s.`backpack`, "
		"timestampdiff(minute, s.`start_time`, s.`last_updated`) as `SurvivalTime`, "
		"timestampdiff(minute, s.`last_ate`, NOW()) as `MinsLastAte`, "
		"timestampdiff(minute, s.`last_drank`, NOW()) as `MinsLastDrank`, "
		"s.`model` from `survivor` s join `instance` i on s.`world_id` = i.`world_id` and i.`id` = %d where s.`unique_id` = '%s' and s.`is_dead` = 0", serverId, getDB()->escape_string(playerId).c_str()));

	bool newChar = false; //not a new char
	int characterId = -1; //invalid charid
	Sqf::Value worldSpace = Sqf::Parameters(); //empty worldspace
	Sqf::Value inventory = lexical_cast<Sqf::Value>("[]"); //empty inventory
	Sqf::Value backpack = lexical_cast<Sqf::Value>("[]"); //empty backpack
	Sqf::Value survival = lexical_cast<Sqf::Value>("[0,0,0]"); //0 mins alive, 0 mins since last ate, 0 mins since last drank
	string model = ""; //empty models will be defaulted by scripts
	if (charsRes)
	{
		Field* fields = charsRes->Fetch();
		newChar = false;
		characterId = fields[0].GetInt32();
		try
		{
			worldSpace = lexical_cast<Sqf::Value>(fields[1].GetCppString());
		}
		catch(bad_lexical_cast)
		{
			_logger.warning("Invalid Worldspace for CharacterID("+lexical_cast<string>(characterId)+"): "+fields[1].GetCppString());
		}
		if (!fields[2].IsNULL()) //inventory can be null
		{
			try
			{
				inventory = lexical_cast<Sqf::Value>(fields[2].GetCppString());
				try { SanitiseInv(boost::get<Sqf::Parameters>(inventory)); } catch (const boost::bad_get&) {}
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Inventory for CharacterID("+lexical_cast<string>(characterId)+"): "+fields[2].GetCppString());
			}
		}		
		if (!fields[3].IsNULL()) //backpack can be null
		{
			try
			{
				backpack = lexical_cast<Sqf::Value>(fields[3].GetCppString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Backpack for CharacterID("+lexical_cast<string>(characterId)+"): "+fields[3].GetCppString());
			}
		}
		//set survival info
		{
			Sqf::Parameters& survivalArr = boost::get<Sqf::Parameters>(survival);
			survivalArr[0] = fields[4].GetInt32();
			survivalArr[1] = fields[5].GetInt32();
			survivalArr[2] = fields[6].GetInt32();
		}
		try
		{
			model = boost::get<string>(lexical_cast<Sqf::Value>(fields[7].GetCppString()));
		}
		catch(...)
		{
			model = fields[7].GetCppString();
		}

		//update last login
		{
			//update last character login
			static SqlStatementID stmtId;
			scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, "update `survivor` set `last_updated` = CURRENT_TIMESTAMP where `id` = ?"));
			stmt->addInt32(characterId);
			bool exRes = stmt->Execute();
			poco_assert(exRes == true);
		}
	}
	else //inserting new character
	{
		newChar = true;

		//try getting custom inventory info
		{
			scoped_ptr<QueryResult> invRes(getDB()->PQuery(
				"select `inventory`, `backpack` from `instance` where `id` = %d", serverId));
			if (invRes)
			{
				Field* fields = invRes->Fetch();
				inventory = lexical_cast<Sqf::Value>(fields[0].GetCppString());
				backpack = lexical_cast<Sqf::Value>(fields[1].GetCppString());
			}
		}
		//insert new char into db
		{
			//update last character login
			static SqlStatementID stmtId;
			scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, 
				"insert into `survivor` (`unique_id`, `start_time`, `world_id`, `worldspace`, `inventory`, `backpack`, `medical`) "
				"select ?, now(), i.`world_id`, ?, i.`inventory`, i.`backpack`, ? from `instance` i where i.`id` = ?"));
			stmt->addString(playerId);
			stmt->addString(lexical_cast<string>(worldSpace));
			stmt->addString("[false,false,false,false,false,false,false,12000,[],[0,0],0]");
			stmt->addInt32(serverId);
			bool exRes = stmt->DirectExecute(); //need sync as we will be getting the CharacterID right after this
			if (exRes == false)
			{
				_logger.error("Error creating character for playerId " + playerId);
				Sqf::Parameters retVal;
				retVal.push_back(string("ERROR"));
				return retVal;
			}
		}
		//get the new character's id
		{
			scoped_ptr<QueryResult> newCharRes(getDB()->PQuery(
				"select s.`id` from `survivor` s join `instance` i on s.world_id = i.world_id and i.id = %d where s.`unique_id` = '%s' and s.`is_dead` = 0", serverId, getDB()->escape_string(playerId).c_str()));
			if (!newCharRes)
			{
				_logger.error("Error fetching created character for playerId " + playerId);
				Sqf::Parameters retVal;
				retVal.push_back(string("ERROR"));
				return retVal;
			}
			Field* fields = newCharRes->Fetch();
			characterId = fields[0].GetInt32();
		}
		_logger.information("Created a new character " + lexical_cast<string>(characterId) + " for player '" + playerName + "' (" + playerId + ")" );
	}

	//always push back full login details
	Sqf::Parameters retVal;
	retVal.push_back(string("PASS"));
	retVal.push_back(newPlayer);
	retVal.push_back(lexical_cast<string>(characterId));
	retVal.push_back(worldSpace);
	retVal.push_back(inventory);
	retVal.push_back(backpack);
	retVal.push_back(survival);
	retVal.push_back(model);
	//hive interface version
	retVal.push_back(0.96f);

	return retVal;
}

Sqf::Value SqlCharDataSource::fetchCharacterDetails( int characterId )
{
	Sqf::Parameters retVal;
	//get details from db
	scoped_ptr<QueryResult> charDetRes(getDB()->PQuery(
		"select s.`worldspace`, s.`medical`, s.`zombie_kills`, s.`headshots`, s.`survivor_kills`, s.`bandit_kills`, s.`state`, p.`humanity` "
		"from `survivor` s join `profile` p on s.`unique_id` = p.`unique_id` where s.`id` = %d", characterId));

	if (charDetRes)
	{
		Sqf::Value worldSpace = Sqf::Parameters(); //empty worldspace
		Sqf::Value medical = Sqf::Parameters(); //script will fill this in if empty
		Sqf::Value stats = lexical_cast<Sqf::Value>("[0,0,0,0]"); //killsZ, headZ, killsH, killsB
		Sqf::Value currentState = Sqf::Parameters(); //empty state (aiming, etc)
		int humanity = 2500;
		//get stuff from row
		{
			Field* fields = charDetRes->Fetch();
			try
			{
				worldSpace = lexical_cast<Sqf::Value>(fields[0].GetCppString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Worldspace (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+fields[0].GetCppString());
			}
			try
			{
				medical = lexical_cast<Sqf::Value>(fields[1].GetCppString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Medical (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+fields[1].GetCppString());
			}
			//set stats
			{
				Sqf::Parameters& statsArr = boost::get<Sqf::Parameters>(stats);
				statsArr[0] = fields[2].GetInt32();
				statsArr[1] = fields[3].GetInt32();
				statsArr[2] = fields[4].GetInt32();
				statsArr[3] = fields[5].GetInt32();
			}
			try
			{
				currentState = lexical_cast<Sqf::Value>(fields[6].GetCppString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid CurrentState (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+fields[6].GetCppString());
			}
			humanity = fields[7].GetInt32();
		}

		retVal.push_back(string("PASS"));
		retVal.push_back(medical);
		retVal.push_back(stats);
		retVal.push_back(currentState);
		retVal.push_back(worldSpace);
		retVal.push_back(humanity);
	}
	else
	{
		retVal.push_back(string("ERROR"));
	}

	return retVal;
}

bool SqlCharDataSource::updateCharacter( int characterId, const FieldsType& fields )
{
	map<string,string> sqlFields;

	for (auto it=fields.begin();it!=fields.end();++it)
	{
		const string& name = it->first;
		const Sqf::Value& val = it->second;

		//arrays
		if (name == "worldspace" || name == "inventory" || name == "backpack" || name == "medical" || name == "state")
			sqlFields[name] = "'"+getDB()->escape_string(lexical_cast<string>(val))+"'";
		//booleans
		else if (name == "just_ate" || name == "just_drank")
		{
			if (boost::get<bool>(val))
			{
				string newName = "last_ate";
				if (name == "just_drank")
					newName = "last_drank";

				sqlFields[newName] = "CURRENT_TIMESTAMP";
			}
		}
		//addition integeroids
		else if (name == "zombie_kills" || name == "headshots" || name == "start_time" ||
			name == "survivor_kills" || name == "bandit_kills")
		{
			int integeroid = static_cast<int>(Sqf::GetDouble(val));
			char intSign = '+';
			if (integeroid < 0)
			{
				intSign = '-';
				integeroid = abs(integeroid);
			}

			if (integeroid > 0 && name != "humanity")
				sqlFields[name] = "(s.`"+name+"` "+intSign+" "+lexical_cast<string>(integeroid)+")";
			//humanity references another table
			else if (integeroid > 0)
				sqlFields[name] = "(p.`"+name+"` "+intSign+" "+lexical_cast<string>(integeroid)+")";
		}
		//strings
		else if (name == "model")
		{
			string model = boost::get<string>(val);
			if (model != "") {
				sqlFields[name] = "'"+getDB()->escape_string(model)+"'";
			}
		}
	}

	if (sqlFields.size() > 0)
	{
		string setClause = "";
		bool joinProfile = false;
		for (auto it=sqlFields.begin();it!=sqlFields.end();)
		{
			string fieldName = it->first;
			if (fieldName == "worldspace")
				fieldName = _wsFieldName;

			if (fieldName == "humanity") {
				joinProfile = true;
				setClause += "p.`" + fieldName + "` = " + it->second;
			} else {
				setClause += "s.`" + fieldName + "` = " + it->second;
			}
			++it;
			if (it != sqlFields.end())
				setClause += " , ";
		}

		string query = "update `survivor` s ";
		if (joinProfile)
			query += "join `profile` p on s.`unique_id` = p.`unique_id` ";
		query += "set " + setClause + " where s.`id` = " + lexical_cast<string>(characterId);
		bool exRes = getDB()->Execute(query.c_str());
		poco_assert(exRes == true);

		return exRes;
	}

	return true;
}

bool SqlCharDataSource::killCharacter( int characterId, int duration )
{
	static SqlStatementID statId;
	scoped_ptr<SqlStatement> statStmt(getDB()->CreateStatement(statId, 
		"update `profile` p inner join `survivor` s on s.`unique_id` = p.`unique_id` set p.`survival_attempts` = p.`survival_attempts` + 1, p.`total_survivor_kills` = p.`total_survivor_kills` + s.`survivor_kills`, p.`total_bandit_kills` = p.`total_bandit_kills` + s.`bandit_kills`, p.`total_zombie_kills` = p.`total_zombie_kills` + s.`zombie_kills`, p.`total_headshots` = p.`total_headshots` + s.`headshots`, p.`total_survival_time` = p.`total_survival_time` + s.`survival_time` where s.`id` = ?"));
	statStmt->addInt32(characterId);
	bool statRes = statStmt->Execute();
	poco_assert(statRes == true);

	static SqlStatementID deadId;
	scoped_ptr<SqlStatement> deadStmt(getDB()->CreateStatement(deadId, 
		"update `survivor` set `is_dead` = 1 where `id` = ?"));
	deadStmt->addInt32(characterId);
	bool deadRes = deadStmt->Execute();
	poco_assert(deadRes == true);

	return (statRes && deadRes);
}

bool SqlCharDataSource::recordLogEntry( string playerId, int characterId, int serverId, int action )
{
	static SqlStatementID stmtId;
	scoped_ptr<SqlStatement> stmt(getDB()->CreateStatement(stmtId, 
		"insert into `log_entry` (`unique_id`, `log_code_id`, `instance_id`) select ?, lc.id, ? from log_code lc where lc.name = ?"));
	stmt->addString(playerId);
	stmt->addInt32(serverId);
	switch (action) {
	case 0:
		stmt->addString("Disconnect");
		break;
	case 2:
		stmt->addString("Login");
		break;
	}
	bool exRes = stmt->Execute();
	poco_assert(exRes == true);

	return exRes;
}
