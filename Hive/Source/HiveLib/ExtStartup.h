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

#pragma once

#include "HiveExtApp.h"
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