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