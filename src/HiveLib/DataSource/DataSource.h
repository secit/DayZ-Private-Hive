#pragma once

#include "../Sqf.h"

namespace Poco { class Logger; };
class DataSource
{
public:
	DataSource(Poco::Logger& logger) : _logger(logger) {}
	virtual ~DataSource() {}

protected:
	Poco::Logger& _logger;
};