#include "DirectHiveApp.h"

DirectHiveApp::DirectHiveApp(string suffixDir) : HiveExtApp(suffixDir) {}

#include "Shared/Library/Database/DatabaseLoader.h"
#include "HiveLib/DataSource/SqlDataSourceCharacter.h"
#include "HiveLib/DataSource/SqlDataSourceObject.h"

bool DirectHiveApp::initialiseService()
{
	// Load up database
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> globalDBConf(config().createView("Database"));

		try
		{
			Poco::Logger& dbLogger = Poco::Logger::get("Database");
			_database = DatabaseLoader::Create(globalDBConf);

			if (!_database->initialise(dbLogger, DatabaseLoader::MakeConnParams(globalDBConf)))
			{
				return false;
			}
		}
		catch (const DatabaseLoader::CreationError& e) 
		{
			logger().critical(e.displayText());
			return false;
		}
	}

	// Create character datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf(config().createView("Characters"));
		_charData.reset(new SqlCharDataSource(logger(), _database, conf->getString("IDField", "unique_id"), conf->getString("WSField", "worldspace")));	
	}

	// Create object datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> conf(config().createView("Objects"));
		_objData.reset(new SqlObjDataSource(logger(), _database, conf.get()));
	}

	_database->allowAsyncOperations();	
	
	return true;
}
