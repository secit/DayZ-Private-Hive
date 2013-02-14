/**
 * Copyright (C) 2009-2013 Rajko Stojadinovic <http://github.com/rajkosto/hive>
 *
 * Overhauled and rewritten by Crosire <http://github.com/Crosire/hive>
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
 **/

#pragma once

#include "Shared/Common/Types.h"
#include <boost/variant.hpp>

namespace Sqf
{
	typedef boost::make_recursive_variant< double, int, Int64, bool, string, void*, vector<boost::recursive_variant_> >::type Value;
	typedef vector<Value> Parameters;

	bool IsNull(const Value& val);
	bool IsAny(const Value& val);
	double GetDouble(const Value& val);
	int GetIntAny(const Value& val);
	Int64 GetBigInt(const Value& val);
	string GetStringAny(const Value& val);
	bool GetBoolAny(const Value& val);

	void runTest();
}

namespace boost
{
	std::istream& operator >> (std::istream& src, Sqf::Value& out);
	std::ostream& operator << (std::ostream& out, const Sqf::Value& val);
};

namespace std
{
	std::istream& operator >> (std::istream& src, Sqf::Parameters& out);
	std::ostream& operator << (std::ostream& out, const Sqf::Parameters& params);
};