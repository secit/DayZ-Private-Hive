#pragma once

#include "HiveExt.h"
#include <boost/function.hpp>

namespace ExtStartup
{
	typedef boost::function<HiveExtApp*(string profileFolder)> MakeAppFunction;

	void InitModule(MakeAppFunction makeAppFunc);
	void ProcessShutdown();
};

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

extern "C"
{
	__declspec(dllexport) void CALLBACK RVExtension(char* output, int outputSize, const char* function); 
};