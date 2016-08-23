#include "dataupdater_v2.h"

#include "bardatacalculator.h"
#include "bardatadefinition.h"
#include "fileloader_v2.h"

#include "searchbar/barbuffer.h"
#include "searchbar/bardataquerystream.h"
#include "searchbar/resistancetagline.h"
#include "searchbar/sqlitehandler.h"
#include "searchbar/streamprocessor.h"
#include "searchbar/supporttagline.h"

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QThread>

#include <chrono>
#include <climits>
#include <ctime>

using namespace std::chrono;

#define DEBUG_MODE

#define LOG_ERROR(...) try{_logger->error(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_WARN(...) try{_logger->warn(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_INFO(...) try{_logger->info(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}

#define GET_ELAPSED_TIME() (std::chrono::duration_cast<milliseconds>(high_resolution_clock::now()-_time).count()/1000.0)

#ifdef DEBUG_MODE
#define PRINT_ELAPSED_TIME(s,symbol) qDebug()<<symbol<<s<<(duration_cast<std::chrono::milliseconds>(high_resolution_clock::now()-_time).count()/1000.0)<<"Sec(s)"
#define IS_UPDATE_ERROR(s) if(is_update_error){err_message = s;goto exit_update_incomplete;}
#else
#define PRINT_ELAPSED_TIME(s,symbol)
#define IS_UPDATE_ERROR(s)
#endif

void debug_print_cache(Logger _logger, const std::vector<BardataTuple> &cache, const QString &symbol, const QString &interval)
{
	int N = static_cast<int>(cache.size());

	for (int i = 0; i < N; ++i)
	{
		LOG_DEBUG ("{} {}: {},{},{}, completed:{},"
							 "OHLC:{},{},{},{}, f/s:{},{},"
							 "fgs:{},{}, fls:{},{},"
							 "macd:{},{}, rsi:{},{},"
							 "slowk:{},{}, slowd:{},{},"
							 "atr:{},{}, prevdailyatr:{},"
							 "distfs:{},{},"
							 "distof:{},{}, distos:{},{},"
							 "disthf:{},{}, disths:{},{},"
							 "distlf:{},{}, distls:{},{},"
							 "distcf:{},{}, distcs:{},{}",
								symbol.toStdString(), interval.toStdString(),
								cache[i].rowid, cache[i].date, cache[i].time, cache[i].completed,
								cache[i].open, cache[i].high, cache[i].low, cache[i].close,
								cache[i].fast_avg, cache[i].slow_avg,
								cache[i].fgs, cache[i].get_data_string(bardata::COLUMN_NAME_FGS_RANK),
								cache[i].fls, cache[i].get_data_string(bardata::COLUMN_NAME_FLS_RANK),
								cache[i].macd, cache[i].get_data_string(bardata::COLUMN_NAME_MACD_RANK),
								cache[i].rsi, cache[i].get_data_string(bardata::COLUMN_NAME_RSI_RANK),
								cache[i].slow_k, cache[i].get_data_string(bardata::COLUMN_NAME_SLOWK_RANK),
								cache[i].slow_d, cache[i].get_data_string(bardata::COLUMN_NAME_SLOWD_RANK),
								cache[i].atr, cache[i].get_data_string(bardata::COLUMN_NAME_ATR_RANK),
								cache[i].get_data_string(bardata::COLUMN_NAME_PREV_DAILY_ATR),
								cache[i].dist_fs, cache[i].get_data_string(bardata::COLUMN_NAME_DISTFS_RANK),
								cache[i].dist_of, cache[i].get_data_string(bardata::COLUMN_NAME_DISTOF_RANK),
								cache[i].dist_os, cache[i].get_data_string(bardata::COLUMN_NAME_DISTOS_RANK),
								cache[i].dist_hf, cache[i].get_data_string(bardata::COLUMN_NAME_DISTHF_RANK),
								cache[i].dist_hs, cache[i].get_data_string(bardata::COLUMN_NAME_DISTHS_RANK),
								cache[i].dist_lf, cache[i].get_data_string(bardata::COLUMN_NAME_DISTLF_RANK),
								cache[i].dist_ls, cache[i].get_data_string(bardata::COLUMN_NAME_DISTLS_RANK),
								cache[i].dist_cf, cache[i].get_data_string(bardata::COLUMN_NAME_DISTCF_RANK),
								cache[i].dist_cs, cache[i].get_data_string(bardata::COLUMN_NAME_DISTCS_RANK));
	}
}

int get_lastdatetime_rowid_db(const QString &database_name, QString *datetime, int *rowid)
{
	if (datetime == NULL || rowid == NULL) return -1;
	*datetime = "";
	*rowid = 0;

	QSqlDatabase database;
	SQLiteHandler::getDatabase_v2(database_name, &database);

	if (database.isOpen())
	{
		QSqlQuery q(database);
		q.exec("PRAGMA journal_mode = OFF;");
		q.exec("PRAGMA synchronous = OFF;");
		q.exec("BEGIN TRANSACTION;");
		q.exec("delete from bardata where completed > 1");
		q.exec("COMMIT;");
		q.exec("select rowid, date_||' '||time_ from bardata where completed=1 order by rowid desc limit 1");

		if (q.next())
		{
			*rowid = q.value(0).toInt();
			*datetime = q.value(1).toString();
		}

		SQLiteHandler::removeDatabase(&database);
	}

	return 0;
}


DataUpdaterV2::DataUpdaterV2(const QString &symbol, const QString &input_dir, const QString &database_dir, Logger logger):
	_symbol(symbol), _input_dir(input_dir), _database_dir(database_dir), _logger(logger)
{
	qDebug() << "DataUpdaterV2::" << QThread::currentThread();

	setAutoDelete(false);

	// initialize update stmt
	{
		std::vector<std::string> projection;
		projection.push_back(bardata::COLUMN_NAME_DISTOF_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTOS_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTHF_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTHS_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTLF_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTLS_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTCF_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTCS_RANK);
		projection.push_back(bardata::COLUMN_NAME_N_UPTAIL);
		projection.push_back(bardata::COLUMN_NAME_N_DOWNTAIL);
		projection.push_back(bardata::COLUMN_NAME_N_BODY);
		projection.push_back(bardata::COLUMN_NAME_N_TOTALLENGTH);
		projection.push_back(bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK);
		projection.push_back(bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK);
		projection.push_back(bardata::COLUMN_NAME_CANDLE_BODY_RANK);
		projection.push_back(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK);
		projection.push_back(bardata::COLUMN_NAME_ATR_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTFS_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTCC_FSCROSS);
		projection.push_back(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR);
		projection.push_back(bardata::COLUMN_NAME_DISTCC_FSCROSS_RANK);
		projection.push_back(bardata::COLUMN_NAME_MACD_RANK);
		projection.push_back(bardata::COLUMN_NAME_PREV_DAILY_ATR);
		projection.push_back(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK);
		projection.push_back(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK);
		projection.push_back(bardata::COLUMN_NAME_UP_MOM_10);
		projection.push_back(bardata::COLUMN_NAME_UP_MOM_10_RANK);
		projection.push_back(bardata::COLUMN_NAME_DOWN_MOM_10);
		projection.push_back(bardata::COLUMN_NAME_DOWN_MOM_10_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTUD_10);
		projection.push_back(bardata::COLUMN_NAME_DISTUD_10_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTDU_10);
		projection.push_back(bardata::COLUMN_NAME_DISTDU_10_RANK);
		projection.push_back(bardata::COLUMN_NAME_UP_MOM_50);
		projection.push_back(bardata::COLUMN_NAME_UP_MOM_50_RANK);
		projection.push_back(bardata::COLUMN_NAME_DOWN_MOM_50);
		projection.push_back(bardata::COLUMN_NAME_DOWN_MOM_50_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTUD_50);
		projection.push_back(bardata::COLUMN_NAME_DISTUD_50_RANK);
		projection.push_back(bardata::COLUMN_NAME_DISTDU_50);
		projection.push_back(bardata::COLUMN_NAME_DISTDU_50_RANK);
		projection.push_back(bardata::COLUMN_NAME_UGD_10 + "_0");
		projection.push_back(bardata::COLUMN_NAME_UGD_10_RANK + "_0");
		projection.push_back(bardata::COLUMN_NAME_DGU_10 + "_0");
		projection.push_back(bardata::COLUMN_NAME_DGU_10_RANK + "_0");
		projection.push_back(bardata::COLUMN_NAME_UGD_50 + "_0");
		projection.push_back(bardata::COLUMN_NAME_UGD_50_RANK + "_0");
		projection.push_back(bardata::COLUMN_NAME_DGU_50 + "_0");
		projection.push_back(bardata::COLUMN_NAME_DGU_50_RANK + "_0");
		projection.push_back(bardata::COLUMN_NAME_COMPLETED);

		std::string _columns = projection[0] + "=?";

		int row_count = static_cast<int>(projection.size());

		for (int i = 1; i < row_count; ++i)
		{
			_columns += "," + projection[i] + "=?";
		}

		_update_stmt = QString::fromStdString("update bardata set " + _columns + " where date_=? and time_=?");
	}
}

DataUpdaterV2::~DataUpdaterV2()
{
	qDebug() << "~DataUpdaterV2 called:" << _symbol;
}

bool DataUpdaterV2::is_found_new_data() const
{
	return is_found_new;
}

void DataUpdaterV2::save_result(const QSqlDatabase &database, std::vector<BardataTuple> *data)
{
	//update bardata using {date_, time_} instead of rowid
	const int row_count = static_cast<int>((*data).size());

	if (!database.isOpen())
	{
		LOG_DEBUG("SaveResult {}: open database error.", _symbol.toStdString());
		return;
	}

	QSqlQuery q(database);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("PRAGMA journal_mode = OFF;");
	q.exec("PRAGMA synchronous = OFF;");
	q.exec("BEGIN TRANSACTION;");
	q.prepare(_update_stmt);

	for (int _i = 0; _i < row_count; ++_i)
	{
		if ((*data)[_i].completed == CompleteStatus::COMPLETE_PENDING)
		{
			(*data)[_i].completed = CompleteStatus::COMPLETE_SUCCESS;
		}
		else
		{
			(*data)[_i].completed = CompleteStatus::INCOMPLETE;
		}

		int _idx = -1;
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTOF_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTOS_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTHF_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTHS_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTLF_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTLS_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTCF_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTCS_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_UPTAIL));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_DOWNTAIL));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_BODY));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_BODY_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_ATR_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTFS_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_MACD_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_PREV_DAILY_ATR));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_10));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_10_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_10));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_10_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_10));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_10_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_10));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_10_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_50));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_50_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_50));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_50_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_50));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_50_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_50));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_50_RANK));
		q.bindValue(++_idx, (*data)[_i].get_data_int   (bardata::COLUMN_NAME_UGD_10 + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_UGD_10_RANK + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_int   (bardata::COLUMN_NAME_DGU_10 + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DGU_10_RANK + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_int   (bardata::COLUMN_NAME_UGD_50 + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_UGD_50_RANK + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_int   (bardata::COLUMN_NAME_DGU_50 + "_0"));
		q.bindValue(++_idx, (*data)[_i].get_data_double(bardata::COLUMN_NAME_DGU_50_RANK + "_0"));
		q.bindValue(++_idx, (*data)[_i].completed);
		q.bindValue(++_idx, QString::fromStdString((*data)[_i].date));
		q.bindValue(++_idx, QString::fromStdString((*data)[_i].time));
		q.exec();

		if (q.lastError().isValid())
		{
			qDebug("SaveResult Error: %s", q.lastError().text().toStdString().c_str());
			LOG_DEBUG("SaveResult Error: {}", q.lastError().text().toStdString());
		}
	}

	q.exec("COMMIT;");
}

bool DataUpdaterV2::is_new_data_available(const QString &filename, const QString &last_datetime)
{
	QFile file(filename);
	bool bFound = false;

	if (file.open(QFile::ReadOnly))
	{
		QTextStream stream(&file);
		QString datetime, line;

		stream.readLineInto(NULL); // discard first line

		while (!stream.atEnd() && stream.readLineInto(&line))
		{
			if (line.length() < 16) continue;
			datetime = line.mid(6,4) + "-" + line.mid(0,2) + "-" + line.mid(3,2) + " " + line.mid(11,5);
			if (datetime > last_datetime) { bFound = true; break; }
		}

		file.close();
	}

	return bFound;
}

void DataUpdaterV2::create_database(IntervalWeight i, QSqlDatabase *d)
{
	QString database_name = _database_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(i) + ".db";
	QString input_name = _input_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(i) + ".txt";

	if (d->isOpen())
	{
		SQLiteHandler::removeDatabase(d);
	}

	if (QFile::exists(database_name) || QFile::exists(input_name))
	{
		QString connstring = "dtupdtrv2" + _symbol + Globals::generate_random_str(20);

		while (QSqlDatabase::contains(connstring))
		{
			connstring = "dtupdtrv2" + _symbol + Globals::generate_random_str(20);
		}

		*d = QSqlDatabase::addDatabase("QSQLITE", connstring);
		d->setDatabaseName(database_name);
		d->open();

		if (d->isOpenError())
		{
			LOG_DEBUG("{}: DataUpdaterV2:create_database error", _symbol.toStdString());
		}
	}
}

void DataUpdaterV2::run()
{
// main process to update database
// algorithm:
//  (1) check for if there's new 5min data, if yes then set flag to prepare to handle incomplete bar.
//  in here we seperate update into 2 phase, complete bar and incomplete bar.
//
//  handle complete bar
//  (2) iterate from Monthly to 5min:
//      - check for last datetime and rowid from database (once)
//      - if such new data exists, then load into database set status COMPLETE_PENDING(2)
//      - store new bar into cache (vector)
//      - calculate the indicators
//      - save results, with status COMPLETE(1)
//
//  handle incomplete bar:
//  (3) if 5min has new data, then handle incomplete bar: insert or update
//  (4) create incomplete bar with status INCOMPLETE_PENDING(3)
//  (5) calculate the indicators
//  (6) update incomplete status with INCOMPLETE(0)

	time_point<high_resolution_clock> _time = high_resolution_clock::now();

	LOG_DEBUG("{}: DataUpdater Start", _symbol.toStdString());
	qDebug() << "DataUpdater Thread (Run)::" << QThread::currentThread() << _symbol;

	QSqlDatabase p_db;
	QString symbol_interval = "";
	QString filename = "";
	QString database_absolute_path = "";
	QString last_datetime;
	int			last_rowid;
	IntervalWeight _interval = WEIGHT_LARGEST;

	std::vector<BardataTuple> m_cache;
	std::string m_symbol = _symbol.toStdString();
	std::string database_name = "";
	std::string err_message = "";
	std::string fname = "";

	// error_code = 0 (okay), anything else is error
	int is_update_error = 0;
	bool isNewDataAvailable;
	bool is5minHasUpdate = false;

	recalculate_flag = false;
	is_found_new = false;

	// pre-determine, if 5min has new data
	{
		QString symbol_interval_5min = _symbol + "_5min";
		QString db5min = _symbol + "_5min.db";
		QString last_5min_datetime = QString::fromStdString(Globals::get_database_lastdatetime(symbol_interval_5min.toStdString()));

		if (last_5min_datetime.isEmpty())
		{
			int rid = 0;

			Globals::set_database_lastdatetime_rowid_v3(_database_dir, _symbol.toStdString(), WEIGHT_5MIN);

			last_5min_datetime = QString::fromStdString(Globals::get_database_lastdatetime_v3(_symbol.toStdString(), WEIGHT_5MIN));
			rid = Globals::get_database_lastrowid_v3(_symbol.toStdString(), WEIGHT_5MIN);

			qDebug() << db5min << "last datetime from db(once):" << last_5min_datetime << rid;
			LOG_DEBUG("{}: last datetime from db(once):{}/id:{}", db5min.toStdString(), last_5min_datetime.toStdString(), rid);

//			Globals::set_database_lastdatetime(db5min.toStdString(), last_5min_datetime.toStdString());
//			Globals::set_database_lastrowid(db5min.toStdString(), rid);
		}

		if (!is5minHasUpdate)
		{
			is5minHasUpdate = is_new_data_available(_input_dir + "/" + _symbol + "_5min.txt", last_5min_datetime);
		}
	}

	// main update
	while (_interval != WEIGHT_INVALID)
	{
		symbol_interval = _symbol + "_" + Globals::intervalWeightToString(_interval);
		filename        = _input_dir + "/" + symbol_interval + ".txt";

		if (QFile::exists(filename))
		{
			m_cache.clear();

			is_update_error = 0;
			fname           = symbol_interval.toStdString() + ".txt";
			database_name   = symbol_interval.toStdString() + ".db";
			database_absolute_path = _database_dir + "/" + symbol_interval + ".db";
			last_datetime = QString::fromStdString(Globals::get_database_lastdatetime(symbol_interval.toStdString()));
			last_rowid    = Globals::get_database_lastrowid(symbol_interval.toStdString());

			// initialize lastdatetime from database (once)
			if (last_datetime == "")
			{
				Globals::set_database_lastdatetime_rowid_v3(_database_dir, _symbol.toStdString(), _interval);
				last_datetime = QString::fromStdString(Globals::get_database_lastdatetime(symbol_interval.toStdString()));
				last_rowid    = Globals::get_database_lastrowid(symbol_interval.toStdString());

				LOG_DEBUG ("{}: last datetime from db(once):{}/id:{}", database_name, last_datetime.toStdString(), last_rowid);
				qDebug() << "get lastdatetime from db(once):" << last_datetime << ", rowid:" << last_rowid;
			}

			isNewDataAvailable = is_new_data_available(filename, last_datetime);

			// insert new data or update incomplete bar
			if (isNewDataAvailable || is5minHasUpdate || recalculate_flag)
			{
				FileLoaderV2 floader(filename, _database_dir, _input_dir, _symbol, _interval, _logger);
				std::string err_message = "";

				eFlagNewData flag = isNewDataAvailable ? eFlagNewData::AVAILABLE : eFlagNewData::NOT_AVAILABLE;

				// ## catch error in fileloader -- (error code < 0)
				if (floader.load_input(&m_cache, flag) < 0)
				{
					is_update_error = 1;
					LOG_INFO("{}: fileload error:{}", symbol_interval.toStdString(), err_message.c_str());
					break;
				}

				if (recalculate_flag)
				{
					recalculate(_interval , recalculate_parent_rowid);
				}

				if (floader.request_recalculate())
				{
					if (!recalculate_flag)
					{
						recalculate_flag = true;
						recalculate_from_interval = _interval;
					}

					if (_interval >= WEIGHT_DAILY)
					{
						recalculate_from_interval = _interval;
					}

					if (floader.recalculate_from_parent_rowid() > 0)
					{
						recalculate_parent_rowid = floader.recalculate_from_parent_rowid();
					}

					qDebug() << "Request Recalculate:" << Globals::intervalWeightToString(_interval)
									 << "Start from" << floader.recalculate_from_parent_rowid();
				}

				PRINT_ELAPSED_TIME(("Elapsed (fileload " + QString::fromStdString(fname) + "):"), _symbol);
			}

			// core function to update
			if (m_cache.size() > 0)
			{
				// counting new data of complete bar (exclude incomplete)
				int _count_new_data = 0;

				for (int i = 0; i < m_cache.size(); ++i)
				{
					if (m_cache[i].completed == CompleteStatus::COMPLETE_PENDING)
					{
						++_count_new_data;
					}
				}

				if (_count_new_data > 0)
				{
					is_found_new = true;
					LOG_INFO("Found {} new record(s) in {}", _count_new_data, fname);
				}

				// Update Indicators
				create_database(_interval, &p_db);

				bool newdataflag = (_count_new_data > 0);
				if (!update(p_db, _interval, last_rowid, &m_cache, newdataflag))
				{
					// TODO: rollback here..
					break;
				}

				// Save Results
				SQLiteHandler::reopen_database(&p_db);
				save_result(p_db, &m_cache);
				debug_print_cache(_logger, m_cache, _symbol, Globals::intervalWeightToString(_interval));

				// Set last datetime and rowid into Globals
				Globals::set_database_lastdatetime_rowid_v3(_database_dir, _symbol.toStdString(), _interval);

				qDebug() << "lastdatetime after update"
								 << QString::fromStdString(Globals::get_database_lastdatetime_v3(_symbol.toStdString(), _interval))
								 << Globals::get_database_lastrowid_v3(_symbol.toStdString(), _interval);

				LOG_DEBUG("{} lastdatetime after update: {}/{}", symbol_interval.toStdString(),
									Globals::get_database_lastdatetime_v3(_symbol.toStdString(), _interval),
									Globals::get_database_lastrowid_v3(_symbol.toStdString(), _interval));

				PRINT_ELAPSED_TIME("Elapsed (SaveResult BardataTuple):", _symbol);

				if (_count_new_data > 0)
				{
					LOG_INFO("{} finish updated", symbol_interval.toStdString());
					_logger->flush();
				}
			}
		}

		_interval = Globals::getChildInterval(_interval);
	}

	SQLiteHandler::removeDatabase(&p_db);

	LOG_DEBUG("{}: complete bar finish in: {} secs", m_symbol, GET_ELAPSED_TIME());
	PRINT_ELAPSED_TIME("DBUpdater finish in:", _symbol);
}

void DataUpdaterV2::recalculate(IntervalWeight interval, int start_parent_rowid)
{
	if (Globals::getParentInterval(interval) == WEIGHT_INVALID) return;

	LOG_DEBUG("{} {}: Recalculate start parent: {}", _symbol.toStdString(), Globals::intervalWeightToStdString(interval), start_parent_rowid);
	qDebug("\nSTART RECALCULATE(%s) from _parent %d", Globals::intervalWeightToStdString(interval).c_str(), start_parent_rowid);

	IntervalWeight parent_interval = Globals::getParentInterval(interval);

	QSqlDatabase database;
	QString database_name = _database_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(interval) + ".db";
	SQLiteHandler::getDatabase_v2(database_name, &database);

	QSqlDatabase parent_database;
	QString parent_database_name = _database_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(parent_interval) + ".db";
	SQLiteHandler::getDatabase_v2(parent_database_name, &parent_database);

	// recalculate indicators that rely on _parent
	{
		QSqlQuery q_parent(parent_database);
		QSqlQuery q(database);
		QSqlQuery q_update(database);

		int start_rowid = 0;
//		bool is_intraday = (interval < WEIGHT_DAILY);

		q.setForwardOnly(true);
		q.exec("PRAGMA cache_size = 10000;");
		q.exec("select rowid, date_, time_ from bardata"
					 " where _parent=" + QString::number(start_parent_rowid) + " order by rowid asc limit 1");

//		qDebug() << q.lastQuery();
		LOG_DEBUG("{} {}: {}", _symbol.toStdString(), Globals::intervalWeightToStdString(interval), q.lastQuery().toStdString());

		if (q.next())
		{
			start_rowid = q.value(0).toInt() - 1;

			qDebug() << "RECALCULATE FROM" << start_rowid << "with _parent from" << recalculate_parent_rowid;
			recalculate_parent_rowid = start_rowid; // for next timeframe
		}

		if (parent_interval != WEIGHT_INVALID && start_rowid > 0)
		{
			QString time_modifier = "";

			switch (parent_interval)
			{
				case WEIGHT_MONTHLY: time_modifier = "-30 day"; break;
				case WEIGHT_WEEKLY: time_modifier = "-7 day"; break;
				case WEIGHT_DAILY: time_modifier = "-1 day"; break;
				case WEIGHT_60MIN: time_modifier = "-3600 second"; break;
				default: time_modifier = "";
			}

			// index _parent
			q.exec("select rowid, date_||' '||time_ from bardata where rowid >" + QString::number(start_rowid));

			q_update.exec("PRAGMA cache_size = 10000;");
			q_update.exec("PRAGMA journal_mode = OFF;");
			q_update.exec("PRAGMA synchronous = OFF;");
			q_update.exec("BEGIN TRANSACTION;");
			q_update.exec("update bardata set _parent=? where rowid=?");

			while (q.next())
			{
				q_parent.exec("select rowid, strftime('%Y-%m-%d %H:%M', date_||'  '||time_,'" + time_modifier + "') as LB, date_||' '||time_ as UB"
											" from bardata where LB <='" + q.value(1).toString() + "' and UB >='" + q.value(1).toString() + "'"
											" limit 1");

				q_update.bindValue(0, q_parent.next() ? q_parent.value(0).toInt() : 0);
				q_update.bindValue(1, q.value(0).toInt());
				q_update.exec();
			}

			q_update.exec("COMMIT;");
		}

		IntervalWeight child_interval = Globals::getChildInterval(interval);
		BardataCalculator::update_index_parent_prev(database, _database_dir, _symbol, Globals::getParentInterval(child_interval), start_rowid);
		qDebug("Reindex child");

		SQLiteHandler::reopen_database(&database);
		BardataCalculator::update_zone(database, _database_dir, _symbol, interval, start_rowid);
		qDebug("Recalculate Zone");

		if (interval < WEIGHT_DAILY)
		{
			SQLiteHandler::reopen_database(&database);
			BardataCalculator::update_moving_avg_approx_v2(database, _database_dir, _symbol, interval, WEIGHT_DAILY, start_rowid);
			qDebug("Recalculate MovingAvg Daily");
		}

		if (interval < WEIGHT_WEEKLY)
		{
			SQLiteHandler::reopen_database(&database);
			BardataCalculator::update_moving_avg_approx_v2(database, _database_dir, _symbol, interval, WEIGHT_WEEKLY, start_rowid);
			qDebug("Recalculate MovingAvg Weekly");
		}

		if (interval < WEIGHT_MONTHLY)
		{
			SQLiteHandler::reopen_database(&database);
			BardataCalculator::update_moving_avg_approx_v2(database, _database_dir, _symbol, interval, WEIGHT_MONTHLY, start_rowid);
			qDebug("Recalculate MovingAvg Monthly");
		}

//		if (is_intraday)
//		{
//			SQLiteHandler::reopen_database(&database);
//			BardataCalculator::update_prevdaily_atr(NULL, database, _database_dir, _symbol);

//			int start_rowid_intraday = -1;
//			{
//				// find exact -- primary
//				QSqlQuery q(database);
//				q.setForwardOnly(true);
//				q.exec("select rowid from bardata where openbar=1 and rowid <=" + QString::number(start_rowid) +
//							 " order by rowid desc limit 3");

//				while (q.next())
//				{
//					start_rowid_intraday = q.value(0).toInt();
//				}
//			}

//			QSqlDatabase db_daily;
//			if (start_rowid_intraday > -1)
//			{
//				StreamProcessor processor;
//				create_database(WEIGHT_DAILY, &db_daily);
//				create_database(interval, &database);

//				BardataQueryStream baseStream(database, start_rowid_intraday);
//				BardataQueryStream dailyStream(db_daily, 0);
//				processor.bind(&baseStream);
//				processor.setDailyStream(&dailyStream);
//				processor.exec();
//			}

//			SQLiteHandler::removeDatabase(&db_daily);
//			SQLiteHandler::reopen_database(&database);
//		}
	}

	SQLiteHandler::removeDatabase(&parent_database);
	SQLiteHandler::removeDatabase(&database);
	qDebug("END RECALCULATE\n--------");
}

bool DataUpdaterV2::update(QSqlDatabase &p_db, IntervalWeight interval, int start_rowid, std::vector<BardataTuple> *data, bool new_data_flag)
{
	time_point<high_resolution_clock> _time = high_resolution_clock::now();

	XmlConfigHandler *config = XmlConfigHandler::get_instance();
	const QVector<t_threshold_1> cf_macd = config->get_macd_threshold();
	const QVector<t_threshold_1> cf_rsi = config->get_rsi_threshold();
	const QVector<t_threshold_1> cf_slowk = config->get_slowk_threshold();
	const QVector<t_threshold_1> cf_slowd = config->get_slowk_threshold();
	const QVector<t_threshold_2> cf_cgf = config->get_cgf_threshold(_symbol);
	const QVector<t_threshold_2> cf_clf = config->get_clf_threshold(_symbol);
	const QVector<t_threshold_2> cf_cgs = config->get_cgs_threshold(_symbol);
	const QVector<t_threshold_2> cf_cls = config->get_cls_threshold(_symbol);
	const QVector<t_threshold_2> cf_fgs = config->get_fgs_threshold(_symbol);
	const QVector<t_threshold_2> cf_fls = config->get_fls_threshold(_symbol);
	QVector<t_sr_threshold> m_threshold = XmlConfigHandler::get_instance()->get_list_threshold()[_symbol];

	const bool is_intraday = (interval != WEIGHT_INVALID && interval < WEIGHT_DAILY);

	std::string __symbol = _symbol.toStdString();
	std::string interval_name = Globals::intervalWeightToStdString(interval);
	std::string err_message = "";

	int is_update_error = 0;

	LOG_DEBUG("{} {}: start rowid: {}, cache size: {}", _symbol.toStdString(), interval_name, start_rowid, data->size());

	SQLiteHandler::reopen_database(&p_db);

	is_update_error = BardataCalculator::update_candle_approx_v2(data, p_db, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (candle normalize):", _symbol);
	if (is_update_error) { err_message = "Error candle normalize"; goto exit_update; }

	is_update_error = BardataCalculator::update_zone(p_db, _database_dir, _symbol, interval, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (zone):", _symbol);
	if (is_update_error) { err_message = "Error calculating zone"; goto exit_update; }

	SQLiteHandler::reopen_database(&p_db);

	is_update_error = BardataCalculator::update_rate_of_change_approx(p_db, 14, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (rate of change):", _symbol);
	if (is_update_error) { err_message = "Error rate of change"; goto exit_update; }

	is_update_error = BardataCalculator::update_ccy_rate_controller(p_db, _database_dir, _symbol, WEIGHT_DAILY, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (ccy Rate):", _symbol);
	if (is_update_error) { err_message = "Error ccy_rate"; goto exit_update; }

	is_update_error = BardataCalculator::update_up_down_mom_approx(data, p_db, FASTAVG_LENGTH, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (up/down mom 10):", _symbol);
	if (is_update_error) { err_message = "Error up/down 10"; goto exit_update; }

	is_update_error = BardataCalculator::update_up_down_threshold_approx(data, p_db, FASTAVG_LENGTH, 0, 0, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (up/down threshold 10):", _symbol);
	if (is_update_error) { err_message = "Error up/down threshold 10"; goto exit_update; }

	is_update_error = BardataCalculator::update_up_down_mom_approx(data, p_db, SLOWAVG_LENGTH, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (up/down mom 50):", _symbol);
	if (is_update_error) { err_message = "Error up/down mom 50"; goto exit_update; }

	is_update_error = BardataCalculator::update_up_down_threshold_approx(data, p_db, SLOWAVG_LENGTH, 0, 0, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (up/down threshold 50):", _symbol);
	if (is_update_error) { err_message = "Error up/down threshold 50"; goto exit_update; }

	// require _parent_prev
//	is_update_error = BardataCalculator::update_moving_avg_approx (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_DAILY, start_rowid);
//	PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-D):", cc_symbol);
//	if (is_update_error) goto exit_update;

//	is_update_error = BardataCalculator::update_moving_avg_approx (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_WEEKLY, start_rowid);
//	PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-W):", cc_symbol);
//	if (is_update_error) goto exit_update;

//	is_update_error = BardataCalculator::update_moving_avg_approx (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_MONTHLY, start_rowid);
//	PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-M):", cc_symbol);
//	if (is_update_error) goto exit_update;

	SQLiteHandler::reopen_database(&p_db);

	is_update_error = BardataCalculator::update_moving_avg_approx_v2(p_db, _database_dir, _symbol, interval, WEIGHT_DAILY, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-D):", _symbol);
	if (is_update_error) { err_message = "Error MovingAvg(D)"; goto exit_update; }

	SQLiteHandler::reopen_database(&p_db);

	is_update_error = BardataCalculator::update_moving_avg_approx_v2(p_db, _database_dir, _symbol, interval, WEIGHT_WEEKLY, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-W):", _symbol);
	if (is_update_error) { err_message = "Error MovingAvg(W) "; goto exit_update; }

	SQLiteHandler::reopen_database(&p_db);

	is_update_error = BardataCalculator::update_moving_avg_approx_v2(p_db, _database_dir, _symbol, interval, WEIGHT_MONTHLY, start_rowid);
	PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-M):", _symbol);
	if (is_update_error) { err_message = "Error MovingAvg(M)"; goto exit_update; }

	LOG_DEBUG("{} {}: finish update movingavg", __symbol, interval_name);

	// PrevDailyATR and DayRange
	if (is_intraday)
	{
		SQLiteHandler::reopen_database(&p_db);

		is_update_error = BardataCalculator::update_prevdaily_atr(data, p_db, _database_dir, _symbol);
		if (is_update_error) { err_message = "Error PrevDailyATR"; goto exit_update; }

		int start_rowid_intraday = -1;
		{
			// find exact -- primary
			QSqlQuery q(p_db);
			q.setForwardOnly(true);
			q.exec("select rowid from bardata where openbar=1 and rowid <=" + QString::number(start_rowid) +
						 " order by rowid desc limit 3");

			while (q.next())
			{
				start_rowid_intraday = q.value(0).toInt();
			}

			LOG_DEBUG("{} {}: dayrange start rowid (exact): {}", __symbol, interval_name, start_rowid_intraday);

			// find approx -- secondary
			if (start_rowid_intraday <= 0)
			{
				LOG_DEBUG("{} {}: dayrange start rowid (approx): {}", __symbol, interval_name, start_rowid_intraday);
				start_rowid_intraday = start_rowid - (interval == WEIGHT_60MIN ? 96 : 1152);
			}
		}

		QSqlDatabase db_daily;
		{
			StreamProcessor processor;
			create_database(WEIGHT_DAILY, &db_daily);
			create_database(interval, &p_db);

			BardataQueryStream baseStream(p_db, start_rowid_intraday);
			BardataQueryStream dailyStream(db_daily, 0);
			processor.bind(&baseStream);
			processor.setDailyStream(&dailyStream);
			processor.exec();
		}
		SQLiteHandler::removeDatabase(&db_daily);
		SQLiteHandler::reopen_database(&p_db);
		PRINT_ELAPSED_TIME("Elapsed (dayrange):", _symbol);
	}

	//
	// (3) Support/Resistance -- for complete bar only
	//
	if (new_data_flag)
	{
		LOG_DEBUG ("{} {}: calculating support/resistance", __symbol, interval_name);

		for (int i = 0; i < m_threshold.size(); ++i)
		{
			if (m_threshold[i].break_threshold > 0 && m_threshold[i].react_threshold > 0)
			{
				qDebug("Support/Resistance: %.3f %.3f %d", m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);
				LOG_DEBUG("{} {}: support/resistance: {} {} {}", _symbol.toStdString(), interval_name,
									m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);

				//
				// Basic Support & Resistance calculation are only for { Daily, Weekly, Monthly }
				//
				if (is_intraday == false)
				{
					BarBuffer b;
					b.resistance_support_gen_dbupdater(p_db,
																						 m_threshold[i].break_threshold,
																						 m_threshold[i].react_threshold,
																						 m_threshold[i].test_point,
																						 start_rowid,
																						 m_threshold[i].t_id);
					LOG_DEBUG ("{} {}: S/R calculation Finish", __symbol, interval_name);
					PRINT_ELAPSED_TIME("Elapsed (Basic S/R):", _symbol);

					ResistanceTagline res;
					res.prepare_data(p_db, start_rowid);
					res.calculate(m_threshold[i].t_id, 250, m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);
					LOG_DEBUG ("{} {}: S/R Resitance Tagline Finish", __symbol, interval_name);
					PRINT_ELAPSED_TIME("Elapsed (Res-Tagline):", _symbol);

					SupportTagline sup;
					sup.prepare_data(p_db, start_rowid);
					sup.calculate(m_threshold[i].t_id, 250, m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);
					LOG_DEBUG ("{} {}: S/R Support Tagline Finish", __symbol, interval_name);
					PRINT_ELAPSED_TIME("Elapsed (Sup-Tagline):", _symbol);

					// dist-point & dist-atr
					BardataCalculator::update_sr_distpoint_distatr(p_db, m_threshold[i].t_id, start_rowid);
					LOG_DEBUG ("{} {}: S/R DistPoint and DistATR Finish", __symbol, interval_name);
					PRINT_ELAPSED_TIME("Elapsed (DistPoint and DistATR):", _symbol);

					// TODO: optimize this part
					StreamProcessor processor;
					BardataQueryStream base_stream(p_db, 0);
					ResistanceQueryStream resistance_stream;
					SupportQueryStream support_stream;
					resistance_stream.setDatabase(p_db, m_threshold[i].t_id);
					support_stream.setDatabase(p_db, m_threshold[i].t_id);
					processor.bind(&base_stream);
					processor.setResistanceStream(&resistance_stream); // TODO: set last rowid
					processor.setSupportStream(&support_stream); // TODO: set last rowid
					processor.exec();
					PRINT_ELAPSED_TIME("Elapsed (H/L Bars and Consec-T):", _symbol);
				}

				//
				// Derived Indicators from Support/Resistance { All timeframe }
				//
				SQLiteHandler::reopen_database(&p_db);

				try
				{
					is_update_error = BardataCalculator::update_dist_resistance_approx_v3(p_db, _database_dir, _symbol, interval, m_threshold[i].t_id, start_rowid, _logger); // XX
				}
				catch (const std::exception &ex)
				{
					LOG_DEBUG("{} {}: dist resistance error: {}", __symbol, interval_name, ex.what());
				}
				catch (...)
				{
					LOG_DEBUG("{} {}: dist resistance error: unhandled exception", __symbol, interval_name);
					qFatal("$s %s: dist resistance error: unhandled exception", __symbol.c_str(), interval_name.c_str());
				}

				PRINT_ELAPSED_TIME("Elapsed (resistance distance):", _symbol);
				LOG_DEBUG("{} {}: S/R Distance Resistance Finish", __symbol, interval_name);

				if (is_update_error) { err_message = "Error resistance distance"; goto exit_update; }

				SQLiteHandler::reopen_database(&p_db);

				try
				{
					is_update_error = BardataCalculator::update_dist_support_approx_v3(p_db, _database_dir, _symbol, interval, m_threshold[i].t_id, start_rowid, _logger); // XX
				}
				catch (const std::exception &ex)
				{
					LOG_DEBUG("{} {}: dist support error: {}", __symbol, interval_name, ex.what());
				}
				catch (...)
				{
					LOG_DEBUG("{} {}: dist support error: unhandled exception", __symbol, interval_name);
					qFatal("%s %s: dist support error: unhandled exception", __symbol.c_str(), interval_name.c_str());
				}

				PRINT_ELAPSED_TIME("Elapsed (support distance):", _symbol);
				LOG_DEBUG("{} {}: S/R Distance Support Finish", __symbol, interval_name);
				if (is_update_error) { err_message = "Error support distance"; goto exit_update; }

				SQLiteHandler::reopen_database(&p_db);

				is_update_error = BardataCalculator::update_sr_line_count(p_db, _database_dir, _symbol, interval, m_threshold[i].t_id, start_rowid);
				PRINT_ELAPSED_TIME("Elapsed (S/R line count):", _symbol);
				LOG_DEBUG("{} {}: S/R Line Count Finish", __symbol, interval_name);

				if (is_update_error) { err_message = "Error S/R line count"; goto exit_update; }

				// @deprecated
	//			create_database (_interval, &p_db, false);
	//			is_update_error = BardataCalculator::update_resistance_velocity(p_db, _interval, m_threshold[i].t_id, m_threshold[i].break_threshold, start_rowid);
	//			PRINT_ELAPSED_TIME ("Elapsed (resistance velocity):", cc_symbol);
	//			LOG_DEBUG ("{} {}: S/R Distance Support Finish", m_symbol, interval_name);
	//			if (is_update_error) goto exit_update;
	//
	//			create_database (_interval, &p_db, false);
	//			is_update_error = BardataCalculator::update_support_velocity(p_db, _interval, m_threshold[i].t_id, m_threshold[i].break_threshold, start_rowid);
	//			PRINT_ELAPSED_TIME ("Elapsed (support velocity):", cc_symbol);
	//			if (is_update_error) goto exit_update;
			}
		}
	}

	//
	// (4) Histogram Rank
	//
	LOG_DEBUG("{} {}: calculating histogram rank", __symbol, interval_name);

	is_update_error = BardataCalculator::update_rsi_slowk_slowd_rank_approx(p_db, start_rowid);
	PRINT_ELAPSED_TIME("Elapsed (RSI/Slowk/SlowD Rank):", _symbol);
	if (is_update_error) { err_message = "Error RSI/Slowk/SlowD rank"; goto exit_update; }

	if (is_intraday)
	{
		SQLiteHandler::reopen_database(&p_db);

		is_update_error = BardataCalculator::update_dayrange_rank_approx(p_db, start_rowid);
		PRINT_ELAPSED_TIME("Elapsed (DayRange Rank):", _symbol);
		if (is_update_error) { err_message = "Error DayRange Rank"; goto exit_update; }
	}

	SQLiteHandler::reopen_database(&p_db);

	is_update_error = BardataCalculator::update_distOHLC_rank_approx_v2r1(data, p_db, _database_dir, _symbol, interval, start_rowid);
	PRINT_ELAPSED_TIME("Elapsed (DistOHLC MA Rank): ", _symbol);
	if (is_update_error) { err_message = "Error DistOHLC Rank"; goto exit_update; }

	SQLiteHandler::reopen_database(&p_db);

	for (int i = 0; i < 3; ++i)
	{
		if (cf_macd[i].operator1.length() > 0 && cf_macd[i].operator2.length() > 0)
		{
			is_update_error = BardataCalculator::update_column_threshold_1
					(data, p_db, SQLiteHandler::COLUMN_NAME_MACD, cf_macd[i].t_id,
					cf_macd[i].operator1, QString::number(cf_macd[i].value1),
					cf_macd[i].operator2, QString::number(cf_macd[i].value2),
					start_rowid);

			if (is_update_error) { err_message = "Error MACD thres"; goto exit_update; }

			PRINT_ELAPSED_TIME("Elapsed (MACD-Thres):", _symbol);
		}

		if (cf_rsi[i].operator1.length() > 0 && cf_rsi[i].operator2.length() > 0)
		{
			is_update_error = BardataCalculator::update_column_threshold_1
				(data, p_db, SQLiteHandler::COLUMN_NAME_RSI, cf_rsi[i].t_id,
				cf_rsi[i].operator1, QString::number(cf_rsi[i].value1),
				cf_rsi[i].operator2, QString::number(cf_rsi[i].value2),
				start_rowid);

			if (is_update_error) { err_message = "Error RSI thres"; goto exit_update; }

			PRINT_ELAPSED_TIME("Elapsed (RSI-Thres):", _symbol);
		}

		if (cf_slowk[i].operator1.length() > 0 && cf_slowk[i].operator2.length() > 0)
		{
			is_update_error = BardataCalculator::update_column_threshold_1
					(data, p_db, SQLiteHandler::COLUMN_NAME_SLOWK, cf_slowk[i].t_id,
					cf_slowk[i].operator1, QString::number(cf_slowk[i].value1),
					cf_slowk[i].operator2, QString::number(cf_slowk[i].value2),
					start_rowid);

			if (is_update_error) { err_message = "Error SlowK thres"; goto exit_update; }

			PRINT_ELAPSED_TIME("Elapsed (SlowK-Thres):", _symbol);
		}

		if (cf_slowd[i].operator1.length() > 0 && cf_slowd[i].operator2.length() > 0)
		{
			is_update_error = BardataCalculator::update_column_threshold_1
					(data, p_db, SQLiteHandler::COLUMN_NAME_SLOWD, cf_slowd[i].t_id,
					cf_slowd[i].operator1, QString::number(cf_slowd[i].value1),
					cf_slowd[i].operator2, QString::number(cf_slowd[i].value2),
					start_rowid);

			if (is_update_error) { err_message = "Error SlowD thres"; goto exit_update; }

			PRINT_ELAPSED_TIME("Elapsed (SlowD-Thres):", _symbol);
		}
	}

	SQLiteHandler::reopen_database(&p_db);

	for (int i = 0; i < 5; ++i)
	{
		if (i < cf_cgf.size() && !cf_cgf[i].value.isEmpty())
		{
			is_update_error = BardataCalculator::update_close_threshold_v3
				(data, p_db, cf_cgf[i].t_id, cf_cgf[i].value.toDouble(), cf_clf[i].value.toDouble(),
				 cf_cgs[i].value.toDouble(), cf_cls[i].value.toDouble(), start_rowid);

			PRINT_ELAPSED_TIME ("Elapsed (Close-MA threshold):", _symbol);

			if (is_update_error) { err_message = "Error Close-MA thres"; goto exit_update; }

			is_update_error = BardataCalculator::update_fgs_fls_threshold_approx
				(data, p_db, cf_fgs[i].t_id, cf_fgs[i].value.toDouble(), cf_fls[i].value.toDouble(), start_rowid);

			PRINT_ELAPSED_TIME ("Elapsed (FGS/FLS threshold):", _symbol);

			if (is_update_error) { err_message = "Error FGS/FLS thres"; goto exit_update; }

			is_update_error = BardataCalculator::update_distfscross_threshold_approx
				(data, p_db, interval, cf_cgf[i].value.toDouble(), cf_cgf[i].t_id, start_rowid);

			PRINT_ELAPSED_TIME ("Elapsed (Dist-FSCross threshold):", _symbol);

			if (is_update_error) { err_message = "Error Dist-FSCross thres"; goto exit_update; }
			SQLiteHandler::reopen_database(&p_db);
		}
	}

exit_update:

	if (is_update_error)
	{
		LOG_INFO("{} {}: UpdateError: {}", __symbol, interval_name, err_message);

		return false;
	}

	return true;
}

/*bool DataUpdaterV2::file_load(const QString &filename, const QString &database_name, IntervalWeight interval, std::vector<BardataTuple> *data)
{
	if (QFile::exists(filename))
	{
		data->clear();

		QString last_datetime = QString::fromStdString(Globals::get_database_lastdatetime(database_name.toStdString()));
		int			last_rowid    = Globals::get_database_lastrowid(database_name.toStdString());

		// initialize lastdatetime from database (once)
		if (last_datetime == "")
		{
			if (get_lastdatetime_rowid_db(_database_dir + "/" + database_name, &last_datetime, &last_rowid) == 0)
			{
				Globals::set_database_lastdatetime(database_name.toStdString(), last_datetime.toStdString());
				Globals::set_database_lastrowid   (database_name.toStdString(), last_rowid);
				LOG_DEBUG ("{}: last datetime from db(once):{}/{}", database_name.toStdString(), last_datetime.toStdString(), last_rowid);
				qDebug() << "get lastdatetime from db(once):" << last_datetime << ", rowid:" << last_rowid;
			}
		}

		if (is_new_data_available(filename, last_datetime))
		{
			FileLoaderV2 floader(filename, _database_dir, _input_dir, _symbol, interval, _logger);
			if (!floader.load_input(data))
				return false;
		}

		return true;
	}

	return false;
}*/
