/** Globals.h
 * ----------
 *  Various general globals variables used in SearchBardata
 *  Put shared enumeration and static util function in here.
 */
#ifndef GLOBALS
#define GLOBALS

#include "xmlconfighandler.h"

#pragma warning(push,3)
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#pragma warning(pop)

#include "boost/random/random_device.hpp"
#include "boost/random/uniform_int_distribution.hpp"

#include <sys/stat.h>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

// parameter for indicator calculation
// sf = smoothing factor

// MovingAvg
#define FASTAVG_LENGTH      10
#define SLOWAVG_LENGTH      50
#define MA_FASTLENGTH       10
#define MA_SLOWLENGTH       50

// MACD
#define MACD_FASTLENGTH     12
#define MACD_SLOWLENGTH     26
#define MACD_LENGTH         9
#define SF_MACD_FAST        2.0/(MACD_FASTLENGTH + 1)
#define SF_MACD_SLOW        2.0/(MACD_SLOWLENGTH + 1)
#define SF_MACD_LENGTH      2.0/(MACD_LENGTH + 1)

// RSI
#define RSI_LENGTH          14
#define SF_RSI              1.0/RSI_LENGTH

// SlowK/SlowD
#define STOCHASTIC_LENGTH   14
#define SF_STOCHASTIC       3

// max number of Support/Resistance Parameters
#define MAX_THRESHOLD_COUNT   5

// compile option
#define _SEARCHBAR_
#define _DBUPDATER_

#ifdef _SEARCHBAR_
	#define SBAR_LOGFILE "log_sbar.txt"
	#define MAX_SBAR_TAB        10        // normal tab
	#define MAX_SBAR_RTAB       2         // realtime tab
	#define DSPINBOX_MIN_VALUE -10000.00
	#define DSPINBOX_MAX_VALUE  99999.99
	#define INTERSECT_MAX_VALUE 10000
#endif

#ifdef _DBUPDATER_
#pragma warning(push,3)
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#pragma warning(pop)
#endif

enum CompleteStatus {
	INCOMPLETE = 0,
	COMPLETE_SUCCESS = 1,
	COMPLETE_PENDING = 2,
	INCOMPLETE_PENDING = 3,
	INCOMPLETE_DUMMY = 31
};

/*enum FS_STATE {
	initial_state = 0,
	fast_above_slow = 1,
	fast_below_slow = 2,
	fast_equal_slow = 3
};*/

// old version of bar interval enum
enum IntervalWeight {
	WEIGHT_1MIN = 1,
	WEIGHT_5MIN = 5,
	WEIGHT_60MIN = 60,
	WEIGHT_DAILY = 1440,
	WEIGHT_WEEKLY = 10080,
	WEIGHT_MONTHLY = 43200,
	WEIGHT_INVALID = -1,
	WEIGHT_LARGEST = WEIGHT_MONTHLY,
	WEIGHT_SMALLEST = WEIGHT_1MIN
};

// BarType from TradeStation's EasyLanguage API
enum BarType {
	TickBar = 0,
	Minute = 1,
	Daily = 2,
	Weekly = 3,
	Monthly =  4,
	Point_and_Figure = 5,
	Reserved1 = 6,
	Reserved2 = 7,
	Kagi = 8,
	Kase = 9,
	LineBreak = 10,
	Momentum = 11,
	Range = 12,
	Renko = 13,
	Second = 14
};

// Represent Interval weight in minutes unit
/*enum IntervalMinutes {
	e_1min = 1,
	e_5min = 5,
	e_60min = 60,
	e_Daily = 1440,
	e_Weekly = 10080,
	e_Monthly = 43200,
	e_Invalid = -1
}; */

// Barcolor representation
enum BarColor {
	BARCOLOR_GREEN = 0, // close > open
	BARCOLOR_RED = 1, // close < open
	BARCOLOR_DOJI = 3 // close = open
};

// alternative of BardataTuple, we provide only essential indicators
typedef struct {
	long int rowid;
	QDate _Date;
	QTime _Time;
	QString s_date; // string representation of date
	QString s_time; // string representation of time
	QString prev_barcolor;
	double _Open;
	double _High;
	double _Low;
	double _Close;
	long int _Volume;
	long double MACD;
	long double MACD_Avg;
	long double MACD_Diff;
	double RSI;
	double Slow_K;
	double Slow_D;
	double Fast_Avg;
	double Slow_Avg;
	double Dist_F;
	double Dist_S;
	double Dist_FS;
	double ATR;
	double prev_ATR;
	int FGS; // F > S
	int FLS; // F < S
	int CGF; // C > F
	int CLF; // C < F
	int CGS; // C > S
	int CLS; // C < S
	long int parent;
	long int parent_monthly;
	long int parent_prev;
	long int parent_prev_monthly;
	bool openbar;

	// custom value for undefined indicators
	double value1;
	double value2;
	double value3;
	double value4;
	double value5;
} struct_bardata;


// trim from start
static inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

class Globals {
	public:
		// typical usage of get_parent_interval and get_child_interval
		// is to doing iteration from Monthly to 5min interval
		// we could use WEIGHT_INVALID as ending flag in iteration

		static IntervalWeight getParentInterval(IntervalWeight interval)
		{
			switch (interval)
			{
				case WEIGHT_MONTHLY: return WEIGHT_INVALID;
				case WEIGHT_WEEKLY: return WEIGHT_MONTHLY;
				case WEIGHT_DAILY: return WEIGHT_WEEKLY;
				case WEIGHT_60MIN: return WEIGHT_DAILY;
				case WEIGHT_5MIN: return WEIGHT_60MIN;
				case WEIGHT_1MIN: return WEIGHT_5MIN;
			}
			return WEIGHT_INVALID;
		}

		static IntervalWeight getChildInterval(IntervalWeight interval)
		{
			switch (interval)
			{
				case WEIGHT_MONTHLY: return WEIGHT_WEEKLY;
				case WEIGHT_WEEKLY: return WEIGHT_DAILY;
				case WEIGHT_DAILY: return WEIGHT_60MIN;
				case WEIGHT_60MIN: return WEIGHT_5MIN;
				case WEIGHT_5MIN: return WEIGHT_1MIN;
				case WEIGHT_1MIN: return WEIGHT_INVALID;
			}
			return WEIGHT_INVALID;
		}

		static QString intervalWeightToString(IntervalWeight interval)
		{
			switch (interval)
			{
				case WEIGHT_MONTHLY: return "Monthly";
				case WEIGHT_WEEKLY: return "Weekly";
				case WEIGHT_DAILY: return "Daily";
				case WEIGHT_60MIN: return "60min";
				case WEIGHT_5MIN: return "5min";
				case WEIGHT_1MIN: return "1min";
			}
			return "";
		}

		static std::string intervalWeightToStdString(IntervalWeight interval)
		{
			switch (interval)
			{
				case WEIGHT_MONTHLY: return "Monthly";
				case WEIGHT_WEEKLY: return "Weekly";
				case WEIGHT_DAILY: return "Daily";
				case WEIGHT_60MIN: return "60min";
				case WEIGHT_5MIN: return "5min";
				case WEIGHT_1MIN: return "1min";
			}
			return "";
		}

		static IntervalWeight WeightFromMovingAvgName(const QString &moving_avg_name)
		{
			if (moving_avg_name.contains("-"))
			{
				QString interval = moving_avg_name.toLower(); // to make case insensitive
				if (interval.startsWith("month")) return WEIGHT_MONTHLY;
				if (interval.startsWith("week")) return WEIGHT_WEEKLY;
				if (interval.startsWith("day")) return WEIGHT_DAILY;
				if (interval.startsWith("60")) return WEIGHT_60MIN;
				if (interval.startsWith("5")) return WEIGHT_5MIN;
				if (interval.startsWith("1")) return WEIGHT_1MIN;
			}
			return WEIGHT_INVALID;
		}

		static IntervalWeight IntervalWeightFromString(const QString &interval_name)
		{
			QString interval = interval_name.toLower(); // to make case insensitive
			if (interval.startsWith("monthly")) return WEIGHT_MONTHLY;
			if (interval.startsWith("weekly")) return WEIGHT_WEEKLY;
			if (interval.startsWith("daily")) return WEIGHT_DAILY;
			if (interval.startsWith("60min")) return WEIGHT_60MIN;
			if (interval.startsWith("5min")) return WEIGHT_5MIN;
			if (interval.startsWith("1min")) return WEIGHT_1MIN;
			return WEIGHT_INVALID;
		}

		static QString MovingAvgNameByInterval(IntervalWeight interval, int length)
		{
			QString N = QString::number(length);
			if (interval == WEIGHT_MONTHLY) return "Month-" + N;
			if (interval == WEIGHT_WEEKLY) return "Week-" + N;
			if (interval == WEIGHT_DAILY) return "Day-" + N;
			if (interval == WEIGHT_60MIN) return "60-" + N;
			if (interval == WEIGHT_5MIN) return "5-" + N;
			if (interval == WEIGHT_1MIN) return "1-" + N;
			return "";
		}

		static QString MovingAvgNameByInterval(IntervalWeight interval)
		{
			if (interval == WEIGHT_MONTHLY) return "Month";
			if (interval == WEIGHT_WEEKLY) return "Week";
			if (interval == WEIGHT_DAILY) return "Day";
			if (interval == WEIGHT_60MIN) return "60";
			if (interval == WEIGHT_5MIN) return "5";
			if (interval == WEIGHT_1MIN) return "1";
			return "";
		}

		// remove trailing zero after decimal point
		// examples: 0.2500 into 0.25, 5.0 into 5

		static QString remove_trailing_zero(const QString &value)
		{
			if (value.toDouble() == 0) return "0";

			QString temp = value;
			const int idx0 = value.indexOf("."); // decimal point position
			int idx1 = value.length() - 1; // last digit position

			while (idx1 > idx0)
			{
				if (temp[idx1] != QChar('0')) break;
				--idx1;
			}

			if (idx1 < value.length() - 1 && idx1 >= idx0)
			{
				if (idx1 > idx0)
				{
					temp.remove(idx1 + 1, value.length() - idx1 - 1);
				}
				else
				{
					temp.remove(idx1, value.length() - idx1);
				}
			}

			return temp;
		}

		static QString create_database_alias(IntervalWeight interval)
		{
			return "db" + Globals::intervalWeightToString(interval);
		}

		static bool is_almost_equal(double a, double b, double epsilon = 0.00001)
		{
			double diff = std::fabs(a - b);
			return (diff == 0.0 || diff < epsilon);
		}

		static double round_decimals(double value, int decimals_place = 6)
		{
			double p = std::pow(10, decimals_place);
			return std::floor(std::round (value * p)) / p;
		}

		static int get_decimal_precision(const std::string &symbol)
		{
			if (symbol == "@CL" || symbol == "@ES" || symbol == "@GC" || symbol == "@NQ") return 2;
			if (symbol == "@AD" || symbol == "@JY" || symbol == "@EC") return 5;
			if (symbol == "@NIY" || symbol == "HHI" || symbol == "HSI") return 0;
			return 3;
		}

		static bool is_file_exists(const std::string &name)
		{
			struct stat buffer; return (stat (name.c_str(), &buffer) == 0);
		}

		static QString generate_random_str(int length)
		{
			std::string chars(
						"abcdefghijklmnopqrstuvwxyz"
						"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						"1234567890"
						"#-_.");

			std::string result = "";

			boost::random::random_device rng;
			boost::random::uniform_int_distribution<> index_dist(0, (int)chars.size() - 1);

			for (int i = 0; i < length; ++i)
			{
				result += chars[index_dist(rng)];
			}

			return QString::fromStdString(result);
		}


		//
		// shortcut for config handler getter and setter
		//
		static QString getTradingStrategyDirectory()
		{
			return XmlConfigHandler::get_instance()->get_ts_strategy_dir();
		}

		static void setTradingStrategyDirectory(const QString &dir)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_ts_strategy_dir(dir);
			config->write_config();
		}

		static QString getSearchPatternDirectory()
		{
			return XmlConfigHandler::get_instance()->get_searchpattern_dir();
		}

		static void setSearchPatternDirectory(const QString &dir)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_searchpattern_dir(dir);
			config->write_config();
		}

		static double getInitialCapital()
		{
			return XmlConfigHandler::get_instance()->get_initial_capital();
		}

		static void setInitialCapital(double value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_inital_capital(value);
			config->write_config();
		}

		static QString get_ipcname()
		{
			return XmlConfigHandler::get_instance()->get_ipc_name();
		}

		static void set_ipcname(const QString &value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_ipc_name(value);
			config->write_config();
		}

		static double get_timer_interval()
		{
			return XmlConfigHandler::get_instance()->get_timer_interval();
		}

		static void set_timer_interval(double value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_timer_interval(value);
			config->write_config();
		}

		static int get_reduce_number()
		{
			return XmlConfigHandler::get_instance()->get_reduce_number();
		}

		static void set_reduce_number(int value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_reduce_number(value);
			config->write_config();
		}

		static QString get_tws_ip()
		{
			return XmlConfigHandler::get_instance()->get_tws_ip();
		}

		static QString get_tws_port()
		{
			return XmlConfigHandler::get_instance()->get_tws_port();
		}

		static QString get_tws_id()
		{
			return XmlConfigHandler::get_instance()->get_tws_id();
		}

		static QMap<QString,t_twsmapping> get_tws_mapping()
		{
			return XmlConfigHandler::get_instance()->get_tws_mapping();
		}

		static void set_tws_ip(const QString &value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_tws_ip(value);
			config->write_config();
		}

		static void set_tws_port(const QString &value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_tws_port(value);
			config->write_config();
		}

		static void set_tws_id(const QString &value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_tws_id(value);
			config->write_config();
		}

		static void set_tws_mapping(const QMap<QString,t_twsmapping> &value)
		{
			XmlConfigHandler *config = XmlConfigHandler::get_instance();
			config->set_tws_mapping(value);
			config->write_config();
		}

		static void print_info(const QString &msg)
		{
			qInstallMessageHandler(myMessageOutput);
			qInfo(msg.toLocal8Bit());
			qInstallMessageHandler(NULL);
		}


		// V3
		static std::string get_database_lastdatetime_v3(const std::string &symbol, IntervalWeight interval)
		{
			QMutexLocker locker(&_mutex);

			std::string symbol_interval = symbol + "_" + intervalWeightToStdString(interval);

			return (database_lastdatetime.count(symbol_interval) > 0) ? database_lastdatetime[symbol_interval] : "";
		}

		static int get_database_lastrowid_v3(const std::string &symbol, IntervalWeight interval)
		{
			QMutexLocker locker(&_mutex);

			std::string symbol_interval = symbol + "_" + intervalWeightToStdString(interval);

			return (database_lastrowid.count(symbol_interval) > 0) ? database_lastrowid[symbol_interval] : 0;
		}

		static void set_database_lastdatetime_rowid_v3(const QString &database_dir, const std::string &symbol, IntervalWeight interval)
		{
			QMutexLocker locker(&_mutex);

			std::string symbol_interval = symbol + "_" + intervalWeightToStdString(interval);

			QSqlDatabase database;
			database = QSqlDatabase::addDatabase("QSQLITE", "globals.setlastdatetimev3." + QString::fromStdString(symbol_interval));
			database.setDatabaseName(database_dir + "/" + QString::fromStdString(symbol_interval) + ".db");

			if (database.open())
			{
				QSqlQuery q(database);
				QString last_datetime = "";
				int last_rowid = 0;

				q.setForwardOnly(true);
				q.exec("PRAGMA temp_store = MEMORY;");
				q.exec("PRAGMA cache_size = 10000;");
				q.exec("PRAGMA journal_mode = OFF;");
				q.exec("PRAGMA synchronous = OFF;");
				q.exec("BEGIN TRANSACTION;");
				q.exec("delete from bardata where completed > 1");
				q.exec("COMMIT;");
				q.exec("select rowid, date_||' '||time_ from bardata where completed=" + QString::number(CompleteStatus::COMPLETE_SUCCESS) +
							 " order by rowid desc limit 1");

				if (q.next())
				{
					last_rowid = q.value(0).toInt();
					last_datetime = q.value(1).toString();
				}

				if (database_lastdatetime.count(symbol_interval) == 0)
				{
					database_lastdatetime.insert(std::make_pair(symbol_interval, last_datetime.toStdString()));
					database_lastrowid.insert(std::make_pair(symbol_interval, last_rowid));
				}
				else
				{
					database_lastdatetime[symbol_interval] = last_datetime.toStdString();
					database_lastrowid[symbol_interval] = last_rowid;
				}
			}

			if (database.isOpen())
			{
				QString conn = database.connectionName();
				database.close();
				database = QSqlDatabase();
				QSqlDatabase::removeDatabase(conn);
			}
		}

		// V1
		// last datetime format: yyyy-MM-dd hh:mm
		// database_name format: symbol_interval.db
		static std::string get_database_lastdatetime(const std::string &database_name)
		{
			QMutexLocker locker(&_mutex);

			return (database_lastdatetime.count(database_name) > 0) ? database_lastdatetime[database_name] : "";
		}

		// globals: last datetime (using unordered map) --
		// last datetime format: yyyy-MM-dd hh:mm
		static void set_database_lastdatetime(const std::string &database_name, const std::string &last_datetime)
		{
			QMutexLocker locker(&_mutex);

			if (database_lastdatetime.count(database_name) == 0)
			{
				database_lastdatetime.insert(std::make_pair(database_name, last_datetime));
			}
			else
			{
				database_lastdatetime[database_name] = last_datetime;
			}
		}

		static void set_database_lastrowid(const std::string &database_name, int last_rowid)
		{
			QMutexLocker locker(&_mutex);

			if (database_lastrowid.count(database_name) == 0)
			{
				database_lastrowid.insert(std::make_pair(database_name, last_rowid));
			}
			else
			{
				database_lastrowid[database_name] = last_rowid;
			}
		}

		static int get_database_lastrowid(const std::string &database_name)
		{
			QMutexLocker locker(&_mutex);

			return (database_lastrowid.count(database_name) > 0) ? database_lastrowid[database_name] : 0;
		}

		static void initialize_database_lastdatetime()
		{
			QMutexLocker locker(&_mutex);

			database_lastdatetime.clear();
		}

		static void initialize_database_lastrowid()
		{
			QMutexLocker locker(&_mutex);
			database_lastrowid.clear();
		}

		static std::string debug_lastdatetime_rowid()
		{
			QMutexLocker locker(&_mutex);

			std::string res = "";

			qDebug("-- last datetime");

			for (auto it = database_lastdatetime.begin(); it != database_lastdatetime.cend(); ++it)
			{
				std::cout << it->first << " " << it->second << '\n';
				res += it->first + " " + it->second + "\n";
			}

			qDebug("-- last rowid");

			for (auto it = database_lastrowid.begin(); it != database_lastrowid.cend(); ++it)
			{
				std::cout << it->first << " " << it->second << '\n';
				res += it->first + " " + std::to_string(it->second) + "\n";
			}

			return res;
		}

		static void debug_database_conn()
		{
			QStringList s = QSqlDatabase::connectionNames();
			qDebug("# of database conn: %d", s.length());

			for (int i = 0; i < s.length(); ++i)
			{
				qDebug() << s[i];
			}
		}


		//
		// @deprecated: lastdatetime logger using file (helper for dbupdater)
		//
		/*static std::map<std::string,std::string> read_symbol_lastdatetime(const std::string &filename)
		{
			std::ifstream file(filename);
			std::map<std::string,std::string> result; // key = database name, value = datetime

			if (file.is_open())
			{
				// format of the line should be "date time symbol_interval"
				// put symbol_interval in last position to make parsing easier,
				// since database name could be any length
				// example: 2015-01-01 17:00 @CL_Daily.db
				std::string line;

				while (!file.eof())
				{
					std::getline(file, line);
					if (line.length() == 0) break;
					result.emplace(std::pair<std::string,std::string>(line.substr(17), line.substr(0, 16)));
				}

				file.close();
			}

			return result;
		}*/

		/*static std::string read_symbol_lastdatetime(const std::string &filename, const std::string &database_name)
		{
			std::ifstream file(filename);
			std::string result = "";

			if (file.is_open())
			{
				// format of the line should be "date time symbol_interval"
				// put symbol_interval in last position to make parsing easier,
				// since database name could be any length
				// example: 2015-01-01 17:00 @CL_Daily.db
				std::string line = "";

				while (!file.eof())
				{
					std::getline(file, line);

					if (line.length() == 0)
						break;

					if (line.substr(17) == database_name)
					{
						result = line.substr(0, 16);
						break;
					}
				}
			}

			file.close();
			return result;
		}*/

		/*static void write_symbol_lastdatetime(const std::string &filename, const std::string &database_name, const std::string &datetime)
		{
			std::map<std::string,std::string> result = read_symbol_lastdatetime(filename);

			if (result.count(database_name) > 0)
			{
				result[database_name] = datetime;
			}
			else
			{
				result.emplace(std::pair<std::string,std::string>(database_name, datetime));
			}

			std::ofstream file(filename);

			if (file.is_open())
			{
				// format of the line should be "date time symbol_interval"
				// put symbol_interval in last position to make parsing easier,
				// since database name could be any length
				// example: 2015-01-01 17:00 @CL_Daily.db

				for (auto it = result.begin(); it != result.end(); ++it)
				{
					file << it->second << " " << it->first << '\n';
				}
			}

			file.close();
		}*/

#ifdef _DBUPDATER_
		typedef struct {
			// prev close is essential to many indicator calculation, (such as rsi)
			double prev_close;

			double macd_m12;
			double macd_m26;
			double macd_avg_prev;

			double rsi_change;
			double rsi_netchgavg;
			double rsi_totchgavg;

			// moving avg (for fast and slow)
			QVector<double> close_last50;

			// we only need to calculate slowk in here, because
			// slowd = average 3 period of slowk
			QVector<double> slowk_lowest_low;
			QVector<double> slowk_highest_high;
			double slowk_num1_0;
			double slowk_num1_1;
			double slowk_den1_0;
			double slowk_den1_1;
			double slowk_last_0; // previous 1st slowk
			double slowk_last_1; // previous 2nd slowk
		} dbupdater_info;

		static void initialize_last_indicator_dbupdater(const QString &database_name, const std::string &key_databasename)
		{
			QMutexLocker locker(&_mutex);

			if (database_name.isEmpty() || !is_file_exists(database_name.toStdString())) return;

//      QTime time; time.start();
			QString connstring = "globals.tempdb";
			QSqlDatabase db;
			db = QSqlDatabase::addDatabase("QSQLITE", connstring);
			db.setDatabaseName(database_name);
			db.open();

			QSqlQuery q(db);
			q.setForwardOnly(true);
			q.exec("select high_, low_, close_, macd, rsi, slowk, slowd, fastavg, slowavg, macdavg from bardata"
						 " where rowid > (select rowid from bardata order by rowid desc limit 1)-5000 and completed=1"
						 " order by rowid");

			dbupdater_info data;
			const double sf_12 = SF_MACD_FAST;
			const double sf_26 = SF_MACD_SLOW;
			const double sf_rsi = SF_RSI;
			const double sf_stochastic = SF_STOCHASTIC;
			const double rsi_length = RSI_LENGTH;

			double _high, _low, _close;
			double m_12 = 0;
			double m_26 = 0;
			double prev_m_12 = 0;
			double prev_m_26 = 0;
			double _macd = 0;
			double _rsi = 0;
			double _slowk = 0;
			double _slowd = 0;
			double _fastavg = 0;
			double _slowavg = 0;

			double first_close = 0.0;
			double rsi_change = 0.0;
			double rsi_netchgavg = 0.0;
			double rsi_totchgavg = 0.0;
			double prev_rsi_netchgavg = 0.0;
			double prev_rsi_totchgavg = 0.0;

			double sum_change = 0.0;
			int ctr = 0;
//      int macd_ok_ctr = 0;
//      int rsi_ok_ctr = 0;
//      int slowk_ok_ctr = 0;
//      int slowd_ok_ctr = 0;
//      int fastavg_ok_ctr = 0;
//      int slowavg_ok_ctr = 0;

			QVector<double> HH, LL;
			QVector<double> num1, den1, slowk_1;
			QVector<double> close_last_50;
			double _hh, _ll;
			double sum_num1 = 0.0;
			double sum_den1 = 0.0;
			double sum_slowk = 0.0;

			data.prev_close = 0.0;
			data.macd_m12 = 0.0;
			data.macd_m26 = 0.0;
			data.macd_avg_prev = 0.0;
			data.rsi_change = 0.0;
			data.rsi_netchgavg = 0.0;
			data.rsi_totchgavg = 0.0;
			data.slowk_den1_0 = 0.0;
			data.slowk_den1_1 = 0.0;
			data.slowk_num1_0 = 0.0;
			data.slowk_num1_1 = 0.0;
			data.slowk_last_0 = 0.0;
			data.slowk_last_1 = 0.0;

			while (q.next())
			{
				++ctr;
				_high  = q.value(0).toDouble();
				_low   = q.value(1).toDouble();
				_close = q.value(2).toDouble();

				if (ctr == 1) first_close = _close;

				// keep only latest 50 close price
				if (close_last_50.size() + 1 > MA_SLOWLENGTH)
				{
					close_last_50.pop_front();
				}

				close_last_50.push_back(_close);

				if (close_last_50.size() >= MA_FASTLENGTH)
				{
					double sum_fast = 0;
					int last_index = close_last_50.size()-1;
					for (int i = 0; i < MA_FASTLENGTH; ++i) sum_fast += close_last_50[last_index - i];
					_fastavg = sum_fast / MA_FASTLENGTH;
				}

				if (close_last_50.size() == MA_SLOWLENGTH)
				{
					double sum_slow = 0;
					for (int i = 0; i < MA_SLOWLENGTH; ++i) sum_slow += close_last_50[i];
					_slowavg = sum_slow / MA_SLOWLENGTH;
				}

				// macd
				if (ctr < MACD_FASTLENGTH) m_12 = m_12 + _close; else
				if (ctr == MACD_FASTLENGTH) m_12 = (m_12 + _close) / 12.0;
				else m_12 = prev_m_12 + (sf_12 * (_close - prev_m_12));

				if (ctr < MACD_SLOWLENGTH) m_26 = m_26 + _close; else
				if (ctr == MACD_SLOWLENGTH) m_26 = (m_26 + _close) / 26.0;
				else m_26 = prev_m_26 + (sf_26 * (_close - prev_m_26));

				_macd = m_12 - m_26;

				// rsi
				if (data.prev_close > 0)
				{
					rsi_change = _close - data.prev_close;
				}

				if (ctr < RSI_LENGTH)
				{
					sum_change += std::fabs(rsi_change);
				}
				else if (ctr == RSI_LENGTH)
				{
					rsi_netchgavg = (_close - first_close) / rsi_length;
					rsi_totchgavg = (sum_change + std::fabs(rsi_change)) / rsi_length;
				}
				else
				{
					rsi_netchgavg = prev_rsi_netchgavg + (sf_rsi * (rsi_change - prev_rsi_netchgavg));
					rsi_totchgavg = prev_rsi_totchgavg + (sf_rsi * (std::fabs(rsi_change) - prev_rsi_totchgavg));
				}

				_rsi = std::floor(std::round((rsi_totchgavg != 0 ? 50.0 * (1 + (rsi_netchgavg / rsi_totchgavg)) : 50.0) * 100.0)) / 100.0;

				// slowk and slowd
				if (HH.size() + 1 > STOCHASTIC_LENGTH)
				{
					HH.pop_front();

					if (!LL.empty()) LL.pop_front();
				}

				HH.push_back(_high);
				LL.push_back(_low);

				_hh = HH[0];
				_ll = LL[0];

				for (int i = 1; i < HH.size(); ++i)
				{
					_hh = std::fmax(_hh, HH[i]);
					_ll = std::fmin(_ll, LL[i]);
				}

				if (num1.size() + 1 > SF_STOCHASTIC)
				{
					sum_num1 -= num1[0];
					sum_den1 -= den1[0];

					num1.pop_front();
					den1.pop_front();
				}

				num1.push_back(_close - _ll);
				den1.push_back(_hh - _ll);

				sum_num1 += num1.last();
				sum_den1 += den1.last();

				if (num1.size() == SF_STOCHASTIC)
				{
					if (slowk_1.size() + 1 > SF_STOCHASTIC)
					{
						sum_slowk -= slowk_1[0];
						slowk_1.pop_front();
					}

					if (sum_den1 == 0)
					{
						qFatal("Globals::initialize_last_indicator_dbudpater-> sum_den1 is zero");
					}

					_slowk = std::floor(std::round((sum_num1 / sum_den1) * 100.0 * 100.0)) / 100.0;
					slowk_1.push_back((sum_num1 / sum_den1) * 100.0);
					sum_slowk += slowk_1.last();

					if (slowk_1.size() == SF_STOCHASTIC)
					{
						_slowd = std::floor(std::round((sum_slowk / sf_stochastic) * 100.0)) / 100.0;
					}
				}

//        if (is_almost_equal(_macd, q.value(3).toDouble(), 0.0001)) ++macd_ok_ctr;
////        else {
////          if (database_name.contains("Monthly")) {
////            qDebug() << "MACD:" << _macd << q.value(3).toDouble();
////          }
////        }

//        if (is_almost_equal(_rsi, q.value(4).toDouble()) ) ++rsi_ok_ctr;
////        else {
////          if (database_name.contains("Monthly")) {
////            qDebug() << "RSI:" << _rsi << q.value(4).toDouble();
////          }
////        }

//        if (is_almost_equal(_slowk, q.value(5).toDouble(), 0.001)) ++slowk_ok_ctr;
////        else qDebug() << "slowk" << _slowk << q.value(5).toDouble() ;

//        if (is_almost_equal(_slowd, q.value(6).toDouble(), 0.001)) ++slowd_ok_ctr;
////        else qDebug() << "slowd" << _slowd << q.value(6).toDouble() << sum_slowk;

//        if (is_almost_equal(_fastavg, q.value(7).toDouble(), 0.001)) ++fastavg_ok_ctr;
//        if (is_almost_equal(_slowavg, q.value(8).toDouble(), 0.001)) ++slowavg_ok_ctr;

				prev_m_12 = m_12;
				prev_m_26 = m_26;
				prev_rsi_netchgavg = rsi_netchgavg;
				prev_rsi_totchgavg = rsi_totchgavg;
//        prev_close = _close;

				data.macd_avg_prev = q.value(9).toDouble();
				data.prev_close = _close;
			}

//      qDebug() << "----" << database_name << QString::fromStdString(key_databasename);
//      qDebug() << "macd match:"   << macd_ok_ctr  << "/" << ctr << "ratio:" << (macd_ok_ctr/(double)ctr);
//      qDebug() << "rsi match:"    << rsi_ok_ctr   << "/" << ctr << "ratio:" << (rsi_ok_ctr/(double)ctr);
//      qDebug() << "slowk match:"  << slowk_ok_ctr << "/" << ctr << "ratio:" << (slowk_ok_ctr/(double)ctr);
//      qDebug() << "slowd match:"  << slowd_ok_ctr << "/" << ctr << "ratio:" << (slowd_ok_ctr/(double)ctr);
//      qDebug() << "fastavg match:"  << fastavg_ok_ctr << "/" << ctr << "ratio:" << (fastavg_ok_ctr/(double)ctr);
//      qDebug() << "slowavg match:"  << slowavg_ok_ctr << "/" << ctr << "ratio:" << (slowavg_ok_ctr/(double)ctr);
//      qDebug() << "elapsed time:" << (time.elapsed()/1000.0) << "secs";

			data.macd_m12 = m_12;
			data.macd_m26 = m_26;
			data.rsi_change = rsi_change;
			data.rsi_netchgavg = rsi_netchgavg;
			data.rsi_totchgavg = rsi_totchgavg;
			data.slowk_den1_0 = den1.size() == 3 ? den1[1] : 0;
			data.slowk_den1_1 = den1.size() == 3 ? den1[2] : 0;
			data.slowk_num1_0 = num1.size() == 3 ? num1[1] : 0;
			data.slowk_num1_1 = num1.size() == 3 ? num1[2] : 0;
			data.slowk_last_0 = slowk_1.size() == 3 ? slowk_1[1] : 0;
			data.slowk_last_1 = slowk_1.size() == 3 ? slowk_1[2] : 0;
			data.slowk_highest_high = HH;
			data.slowk_lowest_low = LL;
			data.close_last50 = close_last_50;

			dbupdater_lastinfo.insert(std::make_pair(key_databasename, data));

			db.close();
			db = QSqlDatabase();
			QSqlDatabase::removeDatabase(connstring);
		}

		static dbupdater_info get_last_indicator_for_dbupdater(const std::string &key_databasename)
		{
			QMutexLocker locker(&_mutex);

			if (dbupdater_lastinfo.count(key_databasename) > 0)
			{
				return dbupdater_lastinfo[key_databasename];
			}

			return dbupdater_info();
		}

		static int get_last_indicator_for_dbupdater_v2(dbupdater_info *out, const std::string &key_databasename)
		{
			QMutexLocker locker(&_mutex);

			if (out != NULL && dbupdater_lastinfo.count(key_databasename) > 0)
			{
				*out = dbupdater_lastinfo[key_databasename];
				return 1;
			}

			return 0;
		}

		static void set_last_indicator_dbupdater(const std::string &key_databasename, const dbupdater_info &data)
		{
			QMutexLocker locker(&_mutex);

			if (dbupdater_lastinfo.count(key_databasename) > 0)
			{
				dbupdater_lastinfo[key_databasename] = data;
			}
		}
#endif

	private:
		static QMutex _mutex;
		static std::unordered_map<std::string, std::string> database_lastdatetime;
		static std::unordered_map<std::string, int> database_lastrowid;

#ifdef _DBUPDATER_
		static std::unordered_map<std::string,dbupdater_info> dbupdater_lastinfo;
#endif

		static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
		{
			if (type == QtInfoMsg)
			{
				QMessageBox::information(0, "information", "Info: " + msg +
																		" (" + QString::fromLocal8Bit(context.file) + ":" +
																					 QString::number(context.line) + ", " +
																					 QString::fromLocal8Bit(context.function) + ")");
			}
		}
};

#endif // GLOBALS
