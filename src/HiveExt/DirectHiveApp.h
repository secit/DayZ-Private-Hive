#pragma once

#include "Shared/Common/Types.h"
#include "HiveLib/HiveExt.h"

class DirectHiveApp: public HiveExtApp
{
public:
	DirectHiveApp(string suffixDir);

protected:
	bool initialiseService() override;

private:
	shared_ptr<Database> _database;
};