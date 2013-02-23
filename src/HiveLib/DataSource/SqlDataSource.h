#pragma once 

#include "DataSource.h"

class Database;
class SqlDataSource : public DataSource
{
public:
	SqlDataSource(Poco::Logger& logger, shared_ptr<Database> db) : DataSource(logger), _db(db) {}
	~SqlDataSource() {}

protected:
	Database* getDB() const { return _db.get(); }

private:
	shared_ptr<Database> _db;
};