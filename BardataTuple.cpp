#include "bardatatuple.h"

#include "bardatadefinition.h"
#include "boost/algorithm/string.hpp"

#include <iostream>

// enable for safe-access map element
#define CHECKING_ELEMENT_EXISTS

using namespace bardata;
using namespace std;

BardataTuple::BardataTuple() {}

double BardataTuple::get_data_double(const std::string &column_name) const
{
	std::string _column = boost::algorithm::to_lower_copy(column_name);
	double _value = 0.0;

#ifdef CHECKING_ELEMENT_EXISTS
	if (_data.count(_column) > 0)
	{
#endif

		std::string data_value = _data[_column];

		if (data_value.length() > 0)
		{
			try
			{
				_value = std::stod(data_value);
			}
			catch (const std::invalid_argument &e1)
			{
				std::cout << "invalid argument:" << e1.what() << "\n";
			 _value = 0.0;
			}
			catch (const std::out_of_range &e2)
			{
				std::cout << "out of range:" << e2.what() << "\n";
				_value = 0.0;
			}
		}

#ifdef CHECKING_ELEMENT_EXISTS
	}
#endif

	return _value;
}

int BardataTuple::get_data_int(const std::string &column_name) const
{
	if (!column_name.empty())
	{
#ifdef CHECKING_ELEMENT_EXISTS
		std::string _column = boost::algorithm::to_lower_copy(column_name);
		if (_data.count(_column) > 0)
		{
#endif

		std::string data_value = _data[_column];
		return data_value.length() > 0 ? std::stol(data_value) : 0;

#ifdef CHECKING_ELEMENT_EXISTS
		}
#endif
	}

	return 0;
}

std::string BardataTuple::get_data_string(const std::string &column_name) const
{
	if (!column_name.empty())
	{
		std::string _column = boost::algorithm::to_lower_copy(column_name);

#ifdef CHECKING_ELEMENT_EXISTS
		if (_data.count(_column) > 0)
		{
#endif

		return _data[_column];

 #ifdef CHECKING_ELEMENT_EXISTS
		}
#endif
	}

	return "";
}

std::string BardataTuple::get_datetime() const
{
	return (date + " " + time);
}

void BardataTuple::set_data(const std::string &column_name, const std::string &value)
{
	if (!column_name.empty())
	{
		// to maintain consistency, we uniform all key inserted in lower case

		_data[boost::algorithm::to_lower_copy(column_name)] = value;
	}
}

bool BardataTuple::is_exists(const std::string &column_name)
{
	return (_data.count(column_name) > 0);
}
