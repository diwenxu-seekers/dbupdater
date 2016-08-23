// DateTimeHelper
// --------------
// wrapper for boost library to handle common datetime operation
// useful for incomplete bar, datetime generator

#ifndef DATETIMEHELPER
#define DATETIMEHELPER

#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <iostream>
#include <string>

class DateTimeHelper {
	public:
		static std::string next_daily_date(const std::string &yyyy_mm_dd)
		{
			if (yyyy_mm_dd.length() < 10) return "";

			try
			{
				boost::gregorian::date d(boost::gregorian::from_string(yyyy_mm_dd));
				d += boost::gregorian::date_duration(1);

				// try to find next business day (Monday to Friday)
				while (d.day_of_week() == boost::gregorian::Saturday ||
							 d.day_of_week() == boost::gregorian::Sunday)
				{
					d += boost::gregorian::date_duration(1);
				}

				return boost::gregorian::to_iso_extended_string(d);
			}
			catch (std::ios_base::failure e0)
			{
				qFatal("next_daily_date() error: %s", e0.what());
			}
			catch (const std::out_of_range &e1)
			{
				qFatal("next_daily_date() out of range error: [%s] %s", yyyy_mm_dd.c_str(), e1.what());
			}
			catch (const std::exception &ex)
			{
				qFatal("next_daily_date() error: %s", ex.what());
			}
			catch (...)
			{
				qFatal("next_daily_date() unhandled error");
			}

			return "";
		}

		static std::string next_weekly_date(const std::string &yyyy_mm_dd, bool start_on_nextday = false)
		{
			if (yyyy_mm_dd.length() < 10) return "";

			try
			{
				boost::gregorian::date d(boost::gregorian::from_string(yyyy_mm_dd));

				if (start_on_nextday)
				{
					d += boost::gregorian::date_duration(1);
				}

				return boost::gregorian::to_iso_extended_string(
					boost::gregorian::next_weekday(d, boost::gregorian::greg_weekday(boost::gregorian::Friday))
				);
			}
			catch (std::ios_base::failure e0)
			{
				qFatal("next_weekly_date() error: %s", e0.what());
			}
			catch (const std::out_of_range &e1)
			{
				qFatal("next_weekly_date() out of range error: [%s] %s", yyyy_mm_dd.c_str(), e1.what());
			}
			catch (const std::exception &ex)
			{
				qFatal("next_weekly_date() error: %s", ex.what());
			}
			catch (...)
			{
				qFatal("next_weekly_date() unhandled error");
			}

			return "";
		}

		static std::string next_monthly_date(const std::string &yyyy_mm_dd, bool start_on_nextday = false)
		{
			if (yyyy_mm_dd.length() < 10) return "";

			try
			{
				boost::gregorian::date d(boost::gregorian::from_string(yyyy_mm_dd));

				if (start_on_nextday)
				{
					d += boost::gregorian::date_duration(1);

					// goto next day (business day)
					while (d.day_of_week() == boost::gregorian::Saturday ||
								 d.day_of_week() == boost::gregorian::Sunday)
					{
						d += boost::gregorian::date_duration(1);
					}
				}

				d = d.end_of_month();

				// try to find previous business day (Monday to Friday)
				while (d.day_of_week() == boost::gregorian::Saturday ||
							 d.day_of_week() == boost::gregorian::Sunday)
				{
					d -= boost::gregorian::date_duration(1);
				}

				return boost::gregorian::to_iso_extended_string(d);
			}
			catch (std::ios_base::failure e0)
			{
				qFatal("next_monthly_date() error: %s", e0.what());
			}
			catch (const std::out_of_range &e1)
			{
				qFatal("next_monthly_date() out of range error: [%s] %s", yyyy_mm_dd.c_str(), e1.what());
			}
			catch (const std::exception &ex)
			{
				qFatal("next_monthly_date() error: %s", ex.what());
			}
			catch (...)
			{
				qFatal("next_monthly_date() unhandled error");
			}

			return "";
		}

		static std::string next_intraday_datetime(const std::string &yyyy_mm_dd_hh_mm, int minutes_interval)
		{
			if (yyyy_mm_dd_hh_mm.length() < 16) return "";

			try
			{
				boost::posix_time::ptime parent_upper_bound(boost::posix_time::time_from_string(yyyy_mm_dd_hh_mm));
				parent_upper_bound += boost::posix_time::minutes(minutes_interval);
				std::string _timestring = boost::posix_time::to_iso_extended_string(parent_upper_bound);

				if (_timestring.length() >= 16)
				{
					return (_timestring.substr(0, 10) + " " + _timestring.substr(11, 5));
				}
			}
			catch (std::ios_base::failure e0)
			{
				qFatal("next_intraday_datetime() error: %s", e0.what());
			}
			catch (const std::out_of_range &e1)
			{
				qFatal("next_intraday_datetime() out of range error: [%s] %s", yyyy_mm_dd_hh_mm.c_str(), e1.what());
			}
			catch (const std::exception &ex)
			{
				qFatal("next_intraday_datetime() error: %s", ex.what());
			}
			catch (...)
			{
				qFatal("next_intraday_datetime() unhandled error");
			}

			return "";
		}
};

#endif // DATETIMEHELPER
