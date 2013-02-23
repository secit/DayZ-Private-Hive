#pragma once

#include "SqlDataSource.h"
#include "DataSourceCharacter.h"
#include "Database/SqlStatement.h"

class SqlCharDataSource : public SqlDataSource, public CharDataSource
{
public:
	SqlCharDataSource(Poco::Logger& logger, shared_ptr<Database> db, const string& idFieldName, const string& wsFieldName);
	~SqlCharDataSource();

	Sqf::Value fetchCharacterInitial( string playerId, int serverId, const string& playerName ) override;
	Sqf::Value fetchCharacterDetails( int characterId ) override;
	bool updateCharacter( int characterId, const FieldsType& fields ) override;
	bool killCharacter( int characterId, int duration ) override;
	bool recordLogEntry( string playerId, int characterId, int serverId, int action ) override;

private:
	string _idFieldName;
	string _wsFieldName;

	// Statement ids
	SqlStatementID _stmtChangePlayerName;
	SqlStatementID _stmtInsertPlayer;
	SqlStatementID _stmtUpdateCharacterLastLogin;
	SqlStatementID _stmtInsertNewCharacter;
	SqlStatementID _stmtInitCharacter;
	SqlStatementID _stmtKillStatCharacter;
	SqlStatementID _stmtKillCharacter;
	SqlStatementID _stmtRecordLogin;
};
