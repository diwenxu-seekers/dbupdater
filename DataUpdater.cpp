#include "dataupdater.h"

#include "IPCServer/commserver.h"
#include "searchbar/bardataquerystream.h"
#include "searchbar/streamprocessor.h"
#include "searchbar/resistancetagline.h"
#include "searchbar/supporttagline.h"
#include "searchbar/barbuffer.h"
#include "spdlog/common.h"
#include "bardatacalculator.h"
#include "bardatadefinition.h"
#include "fileloader.h"

#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QCryptographicHash>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QThread>

#include <chrono>
#include <climits>
#include <ctime>
#include <fstream>
#include <iostream>

using namespace bardata;
using namespace std::chrono;

#define DEBUG_MODE

#define LOG_ERROR(...) try{_logger->error(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_WARN(...) try{_logger->warn(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_INFO(...) try{_logger->info(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}

#ifdef DEBUG_MODE
#define PRINT_ELAPSED_TIME(s,symbol) qDebug()<<symbol<<s<<(duration_cast<std::chrono::milliseconds>(high_resolution_clock::now()-_time).count()/1000.0)<<"Sec(s)"
#define IS_UPDATE_ERROR(s) if(is_update_error){err_message = s;goto exit_update_incomplete;}
#else
#define PRINT_ELAPSED_TIME(s,symbol)
#define IS_UPDATE_ERROR(s)
#endif

#define IF_INCOMPLETE_ERR(str) if(is_update_error){err_message=str;goto exit_update_incomplete;}

int DataUpdater::connection_id = 0;

void debug_print_cache(Logger _logger, const std::vector<BardataTuple> &cache, const char *symbol, const char *interval)
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
								symbol, interval,
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

int get_lastdatetime_rowid_db(const std::string &database_name, std::string *datetime, int *rowid)
{
	if (datetime == NULL || rowid == NULL)
		return SQLITE_ERROR;

	*datetime = "";
	*rowid = 0;

	sqlite3 *db = NULL;

	if (sqlite3_open(database_name.c_str(), &db) == SQLITE_OK)
	{
		// TODO: make sure rowid not jump in next insert
		// TODO: rollback support/resistance (from previous crash if timeframe >= DAILY)

		// delete all "pending bar"
		int rc = sqlite3_exec(db, "PRAGMA journal_mode = OFF;"
															"PRAGMA synchronous = OFF;"
															"BEGIN TRANSACTION;"
															"delete from bardata where completed > 1;"
															"COMMIT;", NULL, NULL, NULL);

		if (rc != SQLITE_OK)
		{
			qDebug("get lastdatetime from db: %s", sqlite3_errmsg(db));
			rc = sqlite3_errcode(db);
			sqlite3_close(db);
			return rc;
		}

		sqlite3_stmt *stmt = NULL;
		const char *select_stmt = "select rowid, date_||' '||time_ from bardata where completed=1 order by rowid desc limit 1";

		if (sqlite3_prepare_v2(db, select_stmt, static_cast<int>(strlen(select_stmt)), &stmt, NULL) == SQLITE_OK)
		{
			sqlite3_step(stmt);
			*rowid = sqlite3_column_int(stmt, 0);

			const char *dt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

			if (dt != NULL)
			{
				*datetime = std::string(dt);
			}
		}

		sqlite3_finalize(stmt);
	}

	sqlite3_close(db);

	return SQLITE_OK;
}

std::string sqlite_error_string(int errcode)
{
	// explain sqlite result codes: description are taken from sqlite documentation
	std::string msg = "";

	switch (errcode)
	{
		case SQLITE_FULL: msg = "database or disk full"; break;
		case SQLITE_IOERR: msg = "disk I/O error"; break;
		case SQLITE_BUSY: msg = "database in use by another process"; break;
		case SQLITE_NOMEM: msg = "out of memory"; break;
		case SQLITE_LOCKED: msg = "database locked by same process"; break;
		case SQLITE_ERROR: msg = "generic error"; break;
		default: msg = "unknown error code(" + std::to_string(errcode) + ")";
	}

	return msg;
}

DataUpdater::DataUpdater(const std::string &symbol, const std::string &input_path,
												 const std::string &database_path, Logger logger):_logger(logger)
{
	m_symbol = symbol;
	m_input_path = input_path;
	m_database_path = database_path;
	m_threshold = XmlConfigHandler::get_instance()->get_list_threshold()[QString::fromStdString(symbol)];

	setAutoDelete(false);
	init_update_statement();
	qDebug() << "DataUpdater Thread in constructor::" << QThread::currentThread() << QString::fromStdString(m_symbol);
}

DataUpdater::~DataUpdater()
{
	qDebug() << ("~DataUpdater " + QString::fromStdString(m_symbol) +" was called") << QThread::currentThread();
}

void DataUpdater::set_symbol(const std::string &symbol)
{
	m_symbol = symbol;
}

void DataUpdater::set_database_path(const std::string &path)
{
	m_database_path = path;
}

void DataUpdater::set_input_path(const std::string &path)
{
	m_input_path = path;
}

bool DataUpdater::is_found_new_data() const
{
	return _found_new;
}

int DataUpdater::count_new_data() const
{
	return _count_new_data;
}

void DataUpdater::init_update_statement()
{
	std::vector<std::string> projection;
	std::string _id;

//  for (int i = 0; i <= 4; ++i)
//  {
//    _id = "_" + std::to_string(i);
//    projection.emplace_back(bardata::COLUMN_NAME_CGF + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CLF + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CGS + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CLS + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_FGS + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_FLS + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CGF_RANK + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CLF_RANK + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CGS_RANK + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_CLS_RANK + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_FGS_RANK + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_FLS_RANK + _id);
//  }

	projection.emplace_back(bardata::COLUMN_NAME_DISTOF_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTOS_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTHF_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTHS_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTLF_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTLS_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCF_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCS_RANK);

	projection.emplace_back(bardata::COLUMN_NAME_N_UPTAIL);
	projection.emplace_back(bardata::COLUMN_NAME_N_DOWNTAIL);
	projection.emplace_back(bardata::COLUMN_NAME_N_BODY);
	projection.emplace_back(bardata::COLUMN_NAME_N_TOTALLENGTH);

	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_BODY_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK);

//  for (int i = 1; i <= 3; ++i)
//  {
//    _id = std::to_string(i);
//    projection.emplace_back(bardata::COLUMN_NAME_MACD + "_VALUE1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_MACD + "_VALUE2_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_MACD + "_RANK1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_MACD + "_RANK2_" + _id);

//    projection.emplace_back(bardata::COLUMN_NAME_RSI + "_VALUE1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_RSI + "_VALUE2_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_RSI + "_RANK1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_RSI + "_RANK2_" + _id);

//    projection.emplace_back(bardata::COLUMN_NAME_SLOWK + "_VALUE1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_SLOWK + "_VALUE2_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_SLOWK + "_RANK1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_SLOWK + "_RANK2_" + _id);

//    projection.emplace_back(bardata::COLUMN_NAME_SLOWD + "_VALUE1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_SLOWD + "_VALUE2_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_SLOWD + "_RANK1_" + _id);
//    projection.emplace_back(bardata::COLUMN_NAME_SLOWD + "_RANK2_" + _id);
//  }

//  projection.emplace_back(bardata::COLUMN_NAME_FGS_RANK);
//  projection.emplace_back(bardata::COLUMN_NAME_FLS_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_ATR_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTFS_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCC_FSCROSS);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCC_FSCROSS_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_MACD_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_PREV_DAILY_ATR);
	projection.emplace_back(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK);

	projection.emplace_back(bardata::COLUMN_NAME_UP_MOM_10);
	projection.emplace_back(bardata::COLUMN_NAME_UP_MOM_10_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DOWN_MOM_10);
	projection.emplace_back(bardata::COLUMN_NAME_DOWN_MOM_10_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTUD_10);
	projection.emplace_back(bardata::COLUMN_NAME_DISTUD_10_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTDU_10);
	projection.emplace_back(bardata::COLUMN_NAME_DISTDU_10_RANK);

	projection.emplace_back(bardata::COLUMN_NAME_UP_MOM_50);
	projection.emplace_back(bardata::COLUMN_NAME_UP_MOM_50_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DOWN_MOM_50);
	projection.emplace_back(bardata::COLUMN_NAME_DOWN_MOM_50_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTUD_50);
	projection.emplace_back(bardata::COLUMN_NAME_DISTUD_50_RANK);
	projection.emplace_back(bardata::COLUMN_NAME_DISTDU_50);
	projection.emplace_back(bardata::COLUMN_NAME_DISTDU_50_RANK);

	// currently only first threshold updated
	for (int i = 0; i < 1; ++i)
	{
		_id = "_" + std::to_string(i);
		projection.emplace_back(bardata::COLUMN_NAME_UGD_10 + _id);
		projection.emplace_back(bardata::COLUMN_NAME_UGD_10_RANK + _id);
		projection.emplace_back(bardata::COLUMN_NAME_DGU_10 + _id);
		projection.emplace_back(bardata::COLUMN_NAME_DGU_10_RANK + _id);
		projection.emplace_back(bardata::COLUMN_NAME_UGD_50 + _id);
		projection.emplace_back(bardata::COLUMN_NAME_UGD_50_RANK + _id);
		projection.emplace_back(bardata::COLUMN_NAME_DGU_50 + _id);
		projection.emplace_back(bardata::COLUMN_NAME_DGU_50_RANK + _id);
	}

	projection.emplace_back (bardata::COLUMN_NAME_COMPLETED);

	std::string _columns = projection[0] + "=?";
	int row_count = static_cast<int>(projection.size());

	for (int i = 1; i < row_count; ++i)
	{
		_columns += "," + projection[i] + "=?";
	}

	sql_update_bardata = "update bardata set " + _columns + " where rowid=?";
}

void DataUpdater::save_result(const char *database_path, const std::vector<BardataTuple> &data)
{
	const int row_count = static_cast<int>(data.size());
	const int statement_length = static_cast<int>(sql_update_bardata.length());
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	std::string _id = "";
	int _idx = 0;
	int rc;

	if (sqlite3_open(database_path, &db) == SQLITE_OK)
	{
		sqlite3_exec(db, bardata::begin_transaction, NULL, NULL, NULL);
		rc = sqlite3_prepare_v2(db, sql_update_bardata.c_str(), statement_length, &stmt, NULL);

		if (rc != SQLITE_OK)
		{
			LOG_DEBUG ("sqlite prepare error{} {}", rc, sqlite3_errmsg(db));
		}

		for (int _i = 0; _i < row_count; ++_i)
		{
			_idx = 0;

			// Close-MA Threshold
//      for (int j = 0; j <= 4; ++j)
//      {
//        _id = "_" + std::to_string(j);
//        sqlite3_bind_int(stmt, ++_idx, data[i].get_data_int(bardata::COLUMN_NAME_CGF + _id));
//        sqlite3_bind_int(stmt, ++_idx, data[i].get_data_int(bardata::COLUMN_NAME_CLF + _id));
//        sqlite3_bind_int(stmt, ++_idx, data[i].get_data_int(bardata::COLUMN_NAME_CGS + _id));
//        sqlite3_bind_int(stmt, ++_idx, data[i].get_data_int(bardata::COLUMN_NAME_CLS + _id));
//        sqlite3_bind_int(stmt, ++_idx, data[i].get_data_int(bardata::COLUMN_NAME_FGS + _id));
//        sqlite3_bind_int(stmt, ++_idx, data[i].get_data_int(bardata::COLUMN_NAME_FLS + _id));
//        sqlite3_bind_double(stmt, ++_idx, data[i].get_data_double(bardata::COLUMN_NAME_CGF_RANK + _id));
//        sqlite3_bind_double(stmt, ++_idx, data[i].get_data_double(bardata::COLUMN_NAME_CLF_RANK + _id));
//        sqlite3_bind_double(stmt, ++_idx, data[i].get_data_double(bardata::COLUMN_NAME_CGS_RANK + _id));
//        sqlite3_bind_double(stmt, ++_idx, data[i].get_data_double(bardata::COLUMN_NAME_CLS_RANK + _id));
//        sqlite3_bind_double(stmt, ++_idx, data[i].get_data_double(bardata::COLUMN_NAME_FGS_RANK + _id));
//        sqlite3_bind_double(stmt, ++_idx, data[i].get_data_double(bardata::COLUMN_NAME_FLS_RANK + _id));
//      }

			// Distance OHLC-MA
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTOF_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTOS_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTHF_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTHS_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTLF_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTLS_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCF_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCS_RANK));

			// Candle
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_UPTAIL));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_DOWNTAIL));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_BODY));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH));

			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_BODY_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK));

//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_uptail_normal);
//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_downtail_normal);
//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_body_normal);
//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_totallength_normal);

//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_uptail_rank);
//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_downtail_rank);
//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_body_rank);
//			sqlite3_bind_double(stmt, ++_idx, data[_i].candle_totallength_rank);

			// MACD-RSI-SLOWK-SLOWD Threshold
//      for (int j = 1; j <= 3; ++j)
//      {
//        _id = "_" + std::to_string(j);
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_MACD_VALUE1 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_MACD_VALUE2 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_MACD_RANK1 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_MACD_RANK2 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_RSI_VALUE1 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_RSI_VALUE2 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_RSI_RANK1 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_RSI_RANK2 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_SLOWK_VALUE1 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_SLOWK_VALUE2 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_SLOWK_RANK1 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_SLOWK_RANK2 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_SLOWD_VALUE1 + _id));
//        sqlite3_bind_int    (stmt, ++_idx, data[i].get_data_int (bardata::COLUMN_NAME_SLOWD_VALUE2 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_SLOWD_RANK1 + _id));
//        sqlite3_bind_double (stmt, ++_idx, data[i].get_data_double (bardata::COLUMN_NAME_SLOWD_RANK2 + _id));
//      }

			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_ATR_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTFS_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_MACD_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_PREV_DAILY_ATR));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK));

			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_10));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_10_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_10));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_10_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_10));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_10_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_10));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_10_RANK));

			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_50));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_50_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_50));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_50_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_50));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_50_RANK));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_50));
			sqlite3_bind_double(stmt, ++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_50_RANK));

			// currently only update for first threshold
			for (int j = 0; j < 1; ++j)
			{
				_id = "_" + std::to_string(j);
				sqlite3_bind_int    (stmt, ++_idx, data[_i].get_data_int    (bardata::COLUMN_NAME_UGD_10 + _id));
				sqlite3_bind_double (stmt, ++_idx, data[_i].get_data_double (bardata::COLUMN_NAME_UGD_10_RANK + _id));
				sqlite3_bind_int    (stmt, ++_idx, data[_i].get_data_int    (bardata::COLUMN_NAME_DGU_10 + _id));
				sqlite3_bind_double (stmt, ++_idx, data[_i].get_data_double (bardata::COLUMN_NAME_DGU_10_RANK + _id));
				sqlite3_bind_int    (stmt, ++_idx, data[_i].get_data_int    (bardata::COLUMN_NAME_UGD_50 + _id));
				sqlite3_bind_double (stmt, ++_idx, data[_i].get_data_double (bardata::COLUMN_NAME_UGD_50_RANK + _id));
				sqlite3_bind_int    (stmt, ++_idx, data[_i].get_data_int    (bardata::COLUMN_NAME_DGU_50 + _id));
				sqlite3_bind_double (stmt, ++_idx, data[_i].get_data_double (bardata::COLUMN_NAME_DGU_50_RANK + _id));
			}

			switch (data[_i].completed)
			{
				case CompleteStatus::COMPLETE_PENDING:
					sqlite3_bind_int(stmt, ++_idx, CompleteStatus::COMPLETE_SUCCESS);
					break;

				case CompleteStatus::INCOMPLETE_DUMMY:
					sqlite3_bind_int(stmt, ++_idx, CompleteStatus::INCOMPLETE_DUMMY);
					break;

				default:
					sqlite3_bind_int(stmt, ++_idx, CompleteStatus::INCOMPLETE);
			}

			sqlite3_bind_int(stmt, ++_idx, data[_i].rowid);
			rc = sqlite3_step(stmt);

			if (rc != SQLITE_DONE)
			{
				qDebug("save result: errorcode: %d", rc);
				LOG_DEBUG ("save result: {},{},{},{}", data[_i].rowid, data[_i].date, data[_i].time, data[_i].completed);
				LOG_DEBUG ("sqlite step error {}: {} {}", rc, sqlite3_errmsg(db), sqlite3_errcode(db));
			}

			sqlite3_reset(stmt);
		}

		sqlite3_exec(db, bardata::end_transaction, NULL, NULL, NULL);
		sqlite3_finalize(stmt);
	}

	sqlite3_close(db);
}

void DataUpdater::save_result_v2(const QString& database_path, const std::vector<BardataTuple> &data)
{
	const int row_count = static_cast<int>(data.size());

	QSqlDatabase database;
	SQLiteHandler::getDatabase_v2(database_path, &database);

	QSqlQuery q(database);
	std::string _id = "";
	int _idx = 0;

	q.exec("PRAGMA cache_size = 10000;");
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA journal_mode = OFF;");
	q.exec("PRAGMA synchronous = OFF;");
	q.exec("BEGIN TRANSACTION;");
	q.prepare(QString::fromStdString(sql_update_bardata));

	for (int _i = 0; _i < row_count; ++_i)
	{
		_idx = -1;
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTOF_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTOS_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTHF_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTHS_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTLF_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTLS_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCF_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCS_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_UPTAIL));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_DOWNTAIL));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_BODY));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_BODY_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_ATR_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTFS_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_MACD_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_PREV_DAILY_ATR));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_10));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_10_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_10));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_10_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_10));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_10_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_10));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_10_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_50));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UP_MOM_50_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_50));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DOWN_MOM_50_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_50));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTUD_50_RANK));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_50));
		q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DISTDU_50_RANK));

		// currently only update for first threshold
		for (int j = 0; j < 1; ++j)
		{
			_id = "_" + std::to_string(j);
			q.bindValue(++_idx, data[_i].get_data_int   (bardata::COLUMN_NAME_UGD_10 + _id));
			q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UGD_10_RANK + _id));
			q.bindValue(++_idx, data[_i].get_data_int   (bardata::COLUMN_NAME_DGU_10 + _id));
			q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DGU_10_RANK + _id));
			q.bindValue(++_idx, data[_i].get_data_int   (bardata::COLUMN_NAME_UGD_50 + _id));
			q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_UGD_50_RANK + _id));
			q.bindValue(++_idx, data[_i].get_data_int   (bardata::COLUMN_NAME_DGU_50 + _id));
			q.bindValue(++_idx, data[_i].get_data_double(bardata::COLUMN_NAME_DGU_50_RANK + _id));
		}

		switch (data[_i].completed)
		{
			case CompleteStatus::COMPLETE_PENDING:
				q.bindValue(++_idx, CompleteStatus::COMPLETE_SUCCESS);
				break;

			case CompleteStatus::INCOMPLETE_DUMMY:
				q.bindValue(++_idx, CompleteStatus::INCOMPLETE_DUMMY);
				break;

			default:
				q.bindValue(++_idx, CompleteStatus::INCOMPLETE);
		}

		q.bindValue(++_idx, data[_i].rowid);
		q.exec();

		if (q.lastError().isValid())
		{
			qDebug() << q.lastError() << q.lastQuery();
		}
	}

	q.exec("COMMIT;");

	SQLiteHandler::removeDatabase(&database);
}

bool DataUpdater::create_database(IntervalWeight _interval, QSqlDatabase *out, bool enable_create_table, bool enable_preset_sqlite)
{
	if (out == NULL || _interval == WEIGHT_INVALID)
	{
		return false;
	}

	const QString _intervalname = "_" + Globals::intervalWeightToString(_interval);

	if (out->isOpen()) SQLiteHandler::removeDatabase(out);

	if (Globals::is_file_exists(m_input_path + "/" + m_symbol + _intervalname.toStdString() + ".txt") ||
			Globals::is_file_exists(m_database_path + "/" + m_symbol + _intervalname.toStdString() + ".db"))
	{
		QString dbname = QString::fromStdString(m_symbol) + _intervalname + ".db";
//		QString connstring = "dataupdater15x.createdb." + dbname + "." + QString::number(++connection_id);
		QString connstring = "dataupdater15x.createdb." + dbname + "." + QTime::currentTime().toString("sszzz");
		connstring = QString::fromStdString(QCryptographicHash::hash(connstring.toLocal8Bit(), QCryptographicHash::Md5).toHex().toStdString());

		if (!QSqlDatabase::contains(connstring))
		{
			*out = QSqlDatabase::addDatabase("QSQLITE", connstring);
			out->setDatabaseName(QString::fromStdString(m_database_path) + "/" + dbname);
			out->open();

			if (enable_preset_sqlite)
			{
				QSqlQuery q(*out);
				QString page_size = (_interval >= WEIGHT_DAILY) ? "32768" : "65536";

				q.exec("PRAGMA temp_store = MEMORY;");
				q.exec("PRAGMA page_size = " + page_size + ";");
				q.exec("PRAGMA cache_size = -16384;");

				if (enable_create_table)
				{
					q.exec(SQLiteHandler::SQL_CREATE_TABLE_BARDATA_V2);
					q.exec(SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_V1);
					q.exec(SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_V1);
					q.exec(SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_DATE_V1);
					q.exec(SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_DATE_V1);
					q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT);
					q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV);
					q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_MONTHLY);
					q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PREVDATETIME);
				}
			}

			return true;
		}
	}

	return false;
}

bool DataUpdater::check_new_data(const std::string &file_path, const std::string &last_datetime)
{
	std::ifstream file(file_path,std::ios::in);
	bool result = false;

	if (file.is_open())
	{
		std::string datetime;
		std::string line = "";
		std::getline(file, line); // discard first line

		while (!file.eof())
		{
			std::getline(file, line);

			if (line.empty()) break;
			if (line.length() < 16) continue;

			datetime = line.substr(6,4) + "-" + line.substr(0,2) + "-" + line.substr(3,2) + " " + line.substr(11,5);

			if (datetime > last_datetime)
			{
				result = true;
				break;
			}
		}

		file.close();
	}

	return result;
}

bool DataUpdater::check_new_data_v2(const QString &file_path, const QString &last_datetime)
{
	QFile file(file_path);
	bool result = false;

	if (file.open(QFile::ReadOnly))
	{
		QTextStream stream(&file);

		// discard first line
		if (stream.readLineInto(NULL))
		{
			QString datetime;
			QString line = "";

			while (!stream.atEnd() && stream.readLineInto(&line))
			{
				if (line.length() < 16) continue;

				datetime = line.mid(6,4) + "-" + line.mid(0,2) + "-" + line.mid(3,2) + " " + line.mid(11,5);

				if (datetime > last_datetime)
				{
					result = true;
					break;
				}
			}
		}

		file.close();
	}

	return result;
}

void DataUpdater::run()
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
	LOG_DEBUG ("{}: DataUpdater Start", m_symbol);
	qDebug() << "DataUpdater Thread (Run)::" << QThread::currentThread() << QString::fromStdString(m_symbol);

	const QString cc_database_path = QString::fromStdString(m_database_path);
	const QString cc_symbol = QString::fromStdString(m_symbol);

	XmlConfigHandler *config = XmlConfigHandler::get_instance();
	const QVector<t_threshold_1> cf_macd = config->get_macd_threshold();
	const QVector<t_threshold_1> cf_rsi = config->get_rsi_threshold();
	const QVector<t_threshold_1> cf_slowk = config->get_slowk_threshold();
	const QVector<t_threshold_1> cf_slowd = config->get_slowk_threshold();
	const QVector<t_threshold_2> cf_cgf = config->get_cgf_threshold(cc_symbol);
	const QVector<t_threshold_2> cf_clf = config->get_clf_threshold(cc_symbol);
	const QVector<t_threshold_2> cf_cgs = config->get_cgs_threshold(cc_symbol);
	const QVector<t_threshold_2> cf_cls = config->get_cls_threshold(cc_symbol);
	const QVector<t_threshold_2> cf_fgs = config->get_fgs_threshold(cc_symbol);
	const QVector<t_threshold_2> cf_fls = config->get_fls_threshold(cc_symbol);

	QSqlDatabase p_db;
	QString database_absolute_path;
	IntervalWeight _interval = WEIGHT_LARGEST;
	std::string database_name = "";
	std::string interval_name = "";
	std::string err_message = "";
	std::string fname = "";
	int start_rowid = 0;
	int is_update_error = 0; // sqlite error code

	_count_new_data = 0;
	_found_new = false;

	bool is_5min_has_update = false;
	{
//		std::string db5min = m_symbol + "_5min.db";
//		std::string last_5min_datetime = Globals::get_database_lastdatetime(db5min);
		std::string last_5min_datetime = Globals::get_database_lastdatetime_v3(m_symbol, WEIGHT_5MIN);

		if (last_5min_datetime.empty())
		{
			qDebug("last5min datetime is empty!");
			LOG_DEBUG("last5min datetime is empty! -- try to initialize (once)");

			Globals::set_database_lastdatetime_rowid_v3(QString::fromStdString(m_database_path), m_symbol, WEIGHT_5MIN);
			last_5min_datetime = Globals::get_database_lastdatetime_v3(m_symbol, WEIGHT_5MIN);

//			int rid = 0;

//			if (get_lastdatetime_rowid_db(m_database_path + "/" + db5min, &last_5min_datetime, &rid) != SQLITE_OK)
//			{
//				LOG_DEBUG("get_lastdatetime_rowid_db FAILED: %s", db5min.c_str());
//			}

//			qDebug("%s: last datetime from db(once):%s/id:%d", db5min.c_str(), last_5min_datetime.c_str(), rid);
//			LOG_DEBUG("{}: last datetime from db(once):{}/id:{}", db5min.c_str(), last_5min_datetime.c_str(), rid);


			qDebug("%s: last datetime from db(once):%s/id:%d", m_symbol.c_str(), last_5min_datetime.c_str(), Globals::get_database_lastrowid_v3(m_symbol,WEIGHT_5MIN));
			LOG_DEBUG("{}: last datetime from db(once):{}/id:{}", m_symbol, last_5min_datetime.c_str(), Globals::get_database_lastrowid_v3(m_symbol,WEIGHT_5MIN));

//			if (!trim(last_5min_datetime).empty())
//			{
//				Globals::set_database_lastdatetime(db5min, last_5min_datetime);
//				Globals::set_database_lastrowid(db5min, rid);
//			}
		}

		if (!is_5min_has_update)
		{
//			is_5min_has_update = check_new_data(m_input_path + "/" + m_symbol + "_5min.txt", last_5min_datetime);
			is_5min_has_update = check_new_data_v2(QString::fromStdString(m_input_path + "/" + m_symbol + "_5min.txt"),
																						 QString::fromStdString(last_5min_datetime));
		}
	}

	// Observation:
	// (1) ensure database updated in sequence order, some indicators rely on other indicators
	// (2) if current timeframe being locked, we break the iteration and skip current iteration update onward.
	// (3) if current iteration not perfectly updated, incomplete bar calculation will be skipped

	while (_interval != WEIGHT_INVALID)
	{
		interval_name = Globals::intervalWeightToStdString(_interval);
		database_name = m_symbol + "_" + interval_name + ".db";
		fname = m_symbol + "_" + interval_name + ".txt";
		is_update_error = 0;

		if (Globals::is_file_exists(m_input_path + "/" + fname))
		{
			//
			// (1) load raw files and basic indicators
			//
			database_absolute_path = QString::fromStdString(m_database_path + "/" + database_name);
			m_cache.clear();

//			std::string last_datetime = Globals::get_database_lastdatetime(database_name);
//			int last_rowid = Globals::get_database_lastrowid(database_name);
			std::string last_datetime = Globals::get_database_lastdatetime_v3(m_symbol, _interval);
			int last_rowid = Globals::get_database_lastrowid_v3(m_symbol, _interval);

			// initialize lastdatetime from database (first time only)
			if (last_datetime.empty())
			{
//				is_update_error = get_lastdatetime_rowid_db(database_absolute_path.toStdString(), &last_datetime, &last_rowid);

//				if (is_update_error)
//				{
//					LOG_DEBUG ("{}: initialize datetime error. {}.", database_name, sqlite_error_string(is_update_error));
//					err_message = "initialize datetime error";
//					break;
//				}

//				Globals::set_database_lastdatetime(database_name, last_datetime);
//				Globals::set_database_lastrowid(database_name, last_rowid);

				Globals::set_database_lastdatetime_rowid_v3(QString::fromStdString(m_database_path), m_symbol, _interval);
				last_datetime = Globals::get_database_lastdatetime_v3(m_symbol, _interval);
				last_rowid = Globals::get_database_lastrowid_v3(m_symbol, _interval);

				LOG_DEBUG ("{}: last datetime from db(once):{}/id:{}", database_name.c_str(), last_datetime.c_str(), last_rowid);
				qDebug("get lastdatetime from db(once): %s, rowid: %d", last_datetime.c_str(), last_rowid);
			}

			// check for new data in input text file w/o access database
			if (check_new_data_v2(QString::fromStdString(m_input_path + "/" + fname), QString::fromStdString(last_datetime)))
//			if (check_new_data(m_input_path + "/" + fname, last_datetime))
			{
				// v1.7.0: add remove pending rows in fileloader
				FileLoader file_load;
				std::string err_message = "";

				file_load.set_logger(_logger);

				if (!file_load.load((m_input_path + "/" + fname), m_database_path, m_symbol, _interval, &m_cache, &err_message))
				{
					// mark file load is error
					is_update_error = 1;

					qDebug("fileload error: {}", err_message.c_str());
					LOG_DEBUG ("fileload error: {}", err_message.c_str());
					LOG_INFO ("Database {} is busy or locked.", database_name);

					break;
				}

				PRINT_ELAPSED_TIME (("Elapsed (fileload "+QString::fromStdString(fname)+"):"), cc_symbol);
			}

			// @experimental: create Incomplete Dummy (to be delete later)
			if (is_5min_has_update && _interval > WEIGHT_5MIN)
			{
				QString input_file = QString::fromStdString(m_input_path);
				QDateTime lastdate;
				BardataTuple data;
				data.completed = CompleteStatus::INCOMPLETE_DUMMY;

				if (!m_cache.empty())
				{
					BardataTuple last_row = m_cache.back();
					lastdate = QDateTime::fromString(QString::fromStdString(last_row.date + " " + last_row.time), "yyyy-MM-dd hh:mm");
				}
				else
				{
//					lastdate = QDateTime::fromString(QString::fromStdString(Globals::get_database_lastdatetime(database_name)),"yyyy-MM-dd hh:mm");
					lastdate = QDateTime::fromString(QString::fromStdString(Globals::get_database_lastdatetime_v3(m_symbol, _interval)),"yyyy-MM-dd hh:mm");
				}

				create_database(_interval, &p_db, true, true);

				bool res = false;

				if (m_cache.empty())
				{
					BardataTuple _emptydata;
					res = BardataCalculator::create_incomplete_bar(p_db, cc_database_path, input_file,
									cc_symbol, _interval, lastdate, &data, _emptydata, _logger);
				}
				else
				{
					res = BardataCalculator::create_incomplete_bar(p_db, cc_database_path, input_file,
									cc_symbol, _interval, lastdate, &data, m_cache.back(), _logger);
				}

				if (res)
				{
					data.completed = CompleteStatus::INCOMPLETE_DUMMY;
					m_cache.emplace_back(data);
				}

				SQLiteHandler::removeDatabase(&p_db);
			}

			// create incomplete bar v2
			// to calculate all indicators on incomplete bar
			if (!m_cache.empty())
			{
				_count_new_data = 0;

				for (int i = 0; i < m_cache.size(); ++i)
				{
					if (m_cache[i].completed == CompleteStatus::COMPLETE_PENDING)
					{
						++_count_new_data;
						_found_new = true;
					}
				}

				// unless complete bar exists, we omit incomplete dummy (alone) from being updated
				if (_count_new_data > 0)
				{
					start_rowid = last_rowid;

					LOG_INFO ("Found {} new record(s) in {}", _count_new_data, fname);
					LOG_DEBUG ("{} {}: start rowid: {}, cache size: {}", m_symbol, interval_name, start_rowid, m_cache.size());

					create_database (_interval, &p_db, false);
					is_update_error = BardataCalculator::update_candle_approx_v2 (&m_cache, p_db, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (candle normalize):", cc_symbol);
					if (is_update_error) { err_message = "Error candle normalize"; goto exit_update; }

					is_update_error = BardataCalculator::update_zone (p_db, cc_database_path, cc_symbol, _interval, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (zone):", cc_symbol);
					if (is_update_error) { err_message = "Error calculating zone"; goto exit_update; }

					is_update_error = BardataCalculator::update_rate_of_change_approx (p_db, 14, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (rate of change):", cc_symbol);
					if (is_update_error) { err_message = "Error rate of change"; goto exit_update; }

					is_update_error = BardataCalculator::update_ccy_rate_controller (p_db, cc_database_path, cc_symbol, WEIGHT_DAILY, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (ccy Rate):", cc_symbol);
					if (is_update_error) { err_message = "Error ccy_rate"; goto exit_update; }

					is_update_error = BardataCalculator::update_up_down_mom_approx (&m_cache, p_db, FASTAVG_LENGTH, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (up/down mom 10):", cc_symbol);
					if (is_update_error) { err_message = "Error up/down 10"; goto exit_update; }

					is_update_error = BardataCalculator::update_up_down_threshold_approx (&m_cache, p_db, FASTAVG_LENGTH, 0, 0, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (up/down threshold 10):", cc_symbol);
					if (is_update_error) { err_message = "Error up/down threshold 10"; goto exit_update; }

					is_update_error = BardataCalculator::update_up_down_mom_approx (&m_cache, p_db, SLOWAVG_LENGTH, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (up/down mom 50):", cc_symbol);
					if (is_update_error) { err_message = "Error up/down mom 50"; goto exit_update; }

					is_update_error = BardataCalculator::update_up_down_threshold_approx (&m_cache, p_db, SLOWAVG_LENGTH, 0, 0, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (up/down threshold 50):", cc_symbol);
					if (is_update_error) { err_message = "Error up/down threshold 50"; goto exit_update; }

					//
					// (2) indexing (prev_parent)
					//
					if (_interval < WEIGHT_LARGEST)
					{
						LOG_DEBUG ("{} {}: begin indexing", m_symbol, interval_name);

						create_database (_interval, &p_db, false);
						is_update_error = BardataCalculator::update_index_parent_prev (p_db, cc_database_path, cc_symbol, Globals::getParentInterval(_interval), start_rowid);
						PRINT_ELAPSED_TIME ("Elapsed (parent prev):", cc_symbol);
						if (is_update_error) { err_message = "Error parent_prev"; goto exit_update; }

						is_update_error = BardataCalculator::update_parent_monthly_v2 (p_db, cc_database_path, cc_symbol);
						PRINT_ELAPSED_TIME ("Elapsed (parent monthly):", cc_symbol);
						if (is_update_error) { err_message = "Error parent monthly"; goto exit_update; }

						LOG_DEBUG ("{} {}: finish indexing", m_symbol, interval_name);
					}

					// require _parent_prev
	//        is_update_error = BardataCalculator::update_moving_avg_approx (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_DAILY, start_rowid);
	//        PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-D):", cc_symbol);
	//        if (is_update_error) goto exit_update;

	//        is_update_error = BardataCalculator::update_moving_avg_approx (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_WEEKLY, start_rowid);
	//        PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-W):", cc_symbol);
	//        if (is_update_error) goto exit_update;

	//        is_update_error = BardataCalculator::update_moving_avg_approx (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_MONTHLY, start_rowid);
	//        PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-M):", cc_symbol);
	//        if (is_update_error) goto exit_update;

					is_update_error = BardataCalculator::update_moving_avg_approx_v2 (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_DAILY, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-D):", cc_symbol);
					if (is_update_error) { err_message = "Error MovingAvg(D)"; goto exit_update; }

					is_update_error = BardataCalculator::update_moving_avg_approx_v2 (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_WEEKLY, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-W):", cc_symbol);
					if (is_update_error) { err_message = "Error MovingAvg(W) "; goto exit_update; }

					is_update_error = BardataCalculator::update_moving_avg_approx_v2 (p_db, cc_database_path, cc_symbol, _interval, WEIGHT_MONTHLY, start_rowid);
					PRINT_ELAPSED_TIME ("Elapsed (MovingAvg realtime-M):", cc_symbol);
					if (is_update_error) { err_message = "Error MovingAvg(M)"; goto exit_update; }

					LOG_DEBUG ("{} {}: finish update movingavg", m_symbol, interval_name);

					// DayRange
					if (_interval < WEIGHT_DAILY)
					{
						// prevdaily atr
						create_database (_interval, &p_db, false);
						is_update_error = BardataCalculator::update_prevdaily_atr(&m_cache, p_db, cc_database_path, cc_symbol);
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

//							LOG_DEBUG("{}", q.lastQuery().toStdString());
							LOG_DEBUG("{} {}: dayrange start rowid (exact): {}",
												 m_symbol, Globals::intervalWeightToStdString(_interval), start_rowid_intraday);

							// find approx -- secondary
							if (start_rowid_intraday <= 0)
							{
								LOG_DEBUG("{} {}: dayrange start rowid (approx): {}",
													 m_symbol, Globals::intervalWeightToStdString(_interval), start_rowid_intraday);

								start_rowid_intraday = start_rowid - (_interval == WEIGHT_60MIN ? 96 : 1152);
							}
						}

						StreamProcessor processor;
						QSqlDatabase db_daily;
						create_database(WEIGHT_DAILY, &db_daily, false);
						create_database(_interval, &p_db, false);

						if (_interval == WEIGHT_60MIN)
						{
//							BardataQueryStream baseStream(p_db, start_rowid - 72);
//							BardataQueryStream baseStream(p_db, start_rowid - 96);
							BardataQueryStream baseStream(p_db, start_rowid_intraday);
							BardataQueryStream dailyStream(db_daily, 0);

							processor.bind(&baseStream);
							processor.setDailyStream(&dailyStream);
							processor.exec();
						}
						else if (_interval == WEIGHT_5MIN)
						{
//							BardataQueryStream baseStream(p_db, start_rowid - 864);
//							BardataQueryStream baseStream(p_db, start_rowid - 1152);
							BardataQueryStream baseStream(p_db, start_rowid_intraday);
							BardataQueryStream dailyStream(db_daily, 0);

//							QStringList sql_statement;
//							QStringList projection;
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_ROWID);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_DATE);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_TIME);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_OPEN);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_HIGH);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_LOW);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_CLOSE);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_FASTAVG);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWAVG);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_MACD);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_RSI);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWK);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWD);
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_IDPARENT);
//              projection.push_back("b." + SQLiteHandler::COLUMN_NAME_IDPARENT_PREV); // XX
//              projection.push_back("a." + SQLiteHandler::COLUMN_NAME_ATR);
//              sql_statement.push_back("attach database '" + cc_database_path + "/" + SQLiteHandler::getDatabaseName(cc_symbol,WEIGHT_60MIN) + "' as db60min");
//              sql_statement.push_back("select "+ projection.join(",") + " from bardata a"
//                                      " join db60min.bardata b on b.rowid=(case when a._parent=0"
//                                      " then a._parent_prev else a._parent end)"
//                                      " where a.rowid >" + QString::number(start_rowid - 864));
//							baseStream.setQuery(sql_statement, p_db);
							processor.bind(&baseStream);
							processor.setDailyStream(&dailyStream);
							processor.exec();
						}
						/*else if (_interval == WEIGHT_1MIN)
						{
							BardataQueryStream baseStream;
							BardataQueryStream dailyStream(db_daily, 0);

							QStringList sql_statement;
							QStringList projection;
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_ROWID);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_DATE);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_TIME);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_OPEN);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_HIGH);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_LOW);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_CLOSE);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_FASTAVG);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWAVG);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_MACD);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_RSI);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWK);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWD);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_IDPARENT);
							projection.push_back("b." + SQLiteHandler::COLUMN_NAME_IDPARENT_PREV);
							projection.push_back("a." + SQLiteHandler::COLUMN_NAME_ATR);

							sql_statement.push_back("attach database '" + cc_database_path + "/" + SQLiteHandler::getDatabaseName (cc_symbol,WEIGHT_60MIN) + "' as db60min");
							sql_statement.push_back("attach database '" + cc_database_path + "/" + SQLiteHandler::getDatabaseName (cc_symbol,WEIGHT_5MIN) + "' as db5min");
							sql_statement.push_back("select "+ projection.join(",") +
																			" from bardata a"
																			" join db5min.bardata b on a._parent=b.rowid"
																			" join db60min.bardata c on b._parent=c.rowid"
																			" where a.rowid >" + QString::number(start_rowid - 4320));

							baseStream.setQuery(sql_statement, p_db);
							processor.bind(&baseStream);
							processor.setDailyStream(&dailyStream);
							processor.exec();
						}*/

						SQLiteHandler::removeDatabase(&db_daily);
						SQLiteHandler::reopen_database(&p_db);
						PRINT_ELAPSED_TIME ("Elapsed (dayrange):", cc_symbol);
					}

					//
					// (3) Support/Resistance
					//
					LOG_DEBUG ("{} {}: calculating support/resistance", m_symbol, Globals::intervalWeightToStdString(_interval));

					for (int i = 0; i < m_threshold.size(); ++i)
					{
						if (m_threshold[i].break_threshold > 0 && m_threshold[i].react_threshold > 0)
						{
							qDebug ("Support/Resistance: %.3f %.3f %d", m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);

							LOG_DEBUG ("{} {}: support/resistance: {} {} {}", m_symbol, interval_name,
													m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);

							//
							// Basic Support & Resistance calculation are only for { Daily, Weekly, Monthly }
							//
							if (_interval >= WEIGHT_DAILY)
							{
								BarBuffer b;
								b.resistance_support_gen_dbupdater(p_db,
																									 m_threshold[i].break_threshold,
																									 m_threshold[i].react_threshold,
																									 m_threshold[i].test_point,
																									 start_rowid,
																									 m_threshold[i].t_id);
								LOG_DEBUG ("{} {}: S/R calculation Finish", m_symbol, interval_name);
								PRINT_ELAPSED_TIME ("Elapsed (Basic S/R):", cc_symbol);

								ResistanceTagline res;
								res.prepare_data(p_db, start_rowid);
								res.calculate(m_threshold[i].t_id, 250, m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);
								LOG_DEBUG ("{} {}: S/R Resitance Tagline Finish", m_symbol, interval_name);
								PRINT_ELAPSED_TIME ("Elapsed (Res-Tagline):", cc_symbol);

								SupportTagline sup;
								sup.prepare_data(p_db, start_rowid);
								sup.calculate(m_threshold[i].t_id, 250, m_threshold[i].break_threshold, m_threshold[i].react_threshold, m_threshold[i].test_point);
								LOG_DEBUG ("{} {}: S/R Support Tagline Finish", m_symbol, interval_name);
								PRINT_ELAPSED_TIME ("Elapsed (Sup-Tagline):", cc_symbol);

								BardataCalculator::update_sr_distpoint_distatr (p_db, m_threshold[i].t_id, start_rowid);
								LOG_DEBUG ("{} {}: S/R DistPoint and DistATR Finish", m_symbol, interval_name);
								PRINT_ELAPSED_TIME ("Elapsed (DistPoint and DistATR):", cc_symbol);

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
								PRINT_ELAPSED_TIME ("Elapsed (H/L Bars and Consec-T):", cc_symbol);
							}

							//
							// Derived Indicators from Support/Resistance { All timeframe }
							//
							create_database (_interval, &p_db, false);

							try
							{
								is_update_error = BardataCalculator::update_dist_resistance_approx_v3 (p_db, cc_database_path, cc_symbol, _interval, m_threshold[i].t_id, start_rowid, _logger); // XX
							}
							catch (const std::exception &ex)
							{
								LOG_DEBUG ("{} {}: dist resistance error: {}", m_symbol, interval_name, ex.what());
							}
							catch (...)
							{
								LOG_DEBUG ("{} {}: dist resistance error: unhandled exception", m_symbol, interval_name);
								qFatal ("$s %s: dist resistance error: unhandled exception", m_symbol.c_str(), interval_name.c_str());
							}

							PRINT_ELAPSED_TIME ("Elapsed (resistance distance):", cc_symbol);
							LOG_DEBUG ("{} {}: S/R Distance Resistance Finish", m_symbol, interval_name);

							if (is_update_error) goto exit_update;

							create_database (_interval, &p_db, false);

							try
							{
								is_update_error = BardataCalculator::update_dist_support_approx_v3 (p_db, cc_database_path, cc_symbol, _interval, m_threshold[i].t_id, start_rowid, _logger); // XX
							}
							catch (const std::exception &ex)
							{
								LOG_DEBUG ("{} {}: dist support error: {}", m_symbol, interval_name, ex.what());
							}
							catch (...)
							{
								LOG_DEBUG ("{} {}: dist support error: unhandled exception", m_symbol, interval_name);
								qFatal ("%s %s: dist support error: unhandled exception", m_symbol.c_str(), interval_name.c_str());
							}

							PRINT_ELAPSED_TIME ("Elapsed (support distance):", cc_symbol);
							LOG_DEBUG ("{} {}: S/R Distance Support Finish", m_symbol, interval_name);

							if (is_update_error) goto exit_update;

							create_database (_interval, &p_db, false);
							is_update_error = BardataCalculator::update_sr_line_count (p_db, cc_database_path, cc_symbol, _interval, m_threshold[i].t_id, start_rowid);
							PRINT_ELAPSED_TIME ("Elapsed (S/R line count):", cc_symbol);
							LOG_DEBUG ("{} {}: S/R Line Count Finish", m_symbol, interval_name);

							if (is_update_error) goto exit_update;

	//            create_database (_interval, &p_db, false);
	//            is_update_error = BardataCalculator::update_resistance_velocity (p_db, _interval, m_threshold[i].t_id, m_threshold[i].break_threshold, start_rowid);
	//            PRINT_ELAPSED_TIME ("Elapsed (resistance velocity):", cc_symbol);
	//            LOG_DEBUG ("{} {}: S/R Distance Support Finish", m_symbol, interval_name);
	//            if (is_update_error) goto exit_update;

	//            create_database (_interval, &p_db, false);
	//            is_update_error = BardataCalculator::update_support_velocity (p_db, _interval, m_threshold[i].t_id, m_threshold[i].break_threshold, start_rowid);
	//            PRINT_ELAPSED_TIME ("Elapsed (support velocity):", cc_symbol);
	//            if (is_update_error) goto exit_update;
						}
					}

					//
					// (4) Histogram Rank
					//
					LOG_DEBUG ("{} {}: calculating histogram rank", m_symbol, interval_name);

					is_update_error = BardataCalculator::update_rsi_slowk_slowd_rank_approx (p_db, start_rowid);

					PRINT_ELAPSED_TIME ("Elapsed (RSI/Slowk/SlowD Rank):", cc_symbol);

					if (is_update_error) { err_message = "Error RSI/Slowk/SlowD rank"; goto exit_update; }

					if (_interval < WEIGHT_DAILY)
					{
						create_database (_interval, &p_db, false);
						is_update_error = BardataCalculator::update_dayrange_rank_approx (p_db, start_rowid);

						PRINT_ELAPSED_TIME ("Elapsed (DayRange Rank):", cc_symbol);

						if (is_update_error) { err_message = "Error DayRange Rank"; goto exit_update; }
					}

					// calculation rank for: {FGS/FLS} {MACD} {ATR} {Dist(FS-Cross)} {Dist(FS)}
					create_database (_interval, &p_db);

					is_update_error = BardataCalculator::update_distOHLC_rank_approx_v2
						(&m_cache, p_db, cc_database_path, cc_symbol, _interval, start_rowid);

					PRINT_ELAPSED_TIME ("Elapsed (DistOHLC MA Rank): ", cc_symbol);

					if (is_update_error) { err_message = "Error DistOHLC Rank"; goto exit_update; }

					create_database (_interval, &p_db);

					for (int i = 0; i < 3; ++i)
					{
						if (cf_macd[i].operator1.length() > 0 && cf_macd[i].operator2.length() > 0)
						{
							is_update_error = BardataCalculator::update_column_threshold_1 (&m_cache, p_db,
								SQLiteHandler::COLUMN_NAME_MACD, cf_macd[i].t_id,
								cf_macd[i].operator1, QString::number(cf_macd[i].value1),
								cf_macd[i].operator2, QString::number(cf_macd[i].value2),
								start_rowid);

							if (is_update_error) { err_message = "Error MACD thres"; goto exit_update; }

							PRINT_ELAPSED_TIME ("Elapsed (MACD-Thres):", cc_symbol);
						}

						if (cf_rsi[i].operator1.length() > 0 && cf_rsi[i].operator2.length() > 0)
						{
							is_update_error = BardataCalculator::update_column_threshold_1 (&m_cache, p_db,
								 SQLiteHandler::COLUMN_NAME_RSI, cf_rsi[i].t_id,
								 cf_rsi[i].operator1, QString::number(cf_rsi[i].value1),
								 cf_rsi[i].operator2, QString::number(cf_rsi[i].value2),
								 start_rowid);

							if (is_update_error) { err_message = "Error RSI thres"; goto exit_update; }

							PRINT_ELAPSED_TIME ("Elapsed (RSI-Thres):", cc_symbol);
						}

						if (cf_slowk[i].operator1.length() > 0 && cf_slowk[i].operator2.length() > 0)
						{
							is_update_error = BardataCalculator::update_column_threshold_1 (&m_cache, p_db,
								 SQLiteHandler::COLUMN_NAME_SLOWK, cf_slowk[i].t_id,
								 cf_slowk[i].operator1, QString::number(cf_slowk[i].value1),
								 cf_slowk[i].operator2, QString::number(cf_slowk[i].value2),
								 start_rowid);

							if (is_update_error) { err_message = "Error SlowK thres"; goto exit_update; }

							PRINT_ELAPSED_TIME ("Elapsed (SlowK-Thres):", cc_symbol);
						}

						if (cf_slowd[i].operator1.length() > 0 && cf_slowd[i].operator2.length() > 0)
						{
							is_update_error = BardataCalculator::update_column_threshold_1 (&m_cache, p_db,
								 SQLiteHandler::COLUMN_NAME_SLOWD, cf_slowd[i].t_id,
								 cf_slowd[i].operator1, QString::number(cf_slowd[i].value1),
								 cf_slowd[i].operator2, QString::number(cf_slowd[i].value2),
								 start_rowid);

							if (is_update_error) { err_message = "Error SlowD thres"; goto exit_update; }

							PRINT_ELAPSED_TIME ("Elapsed (SlowD-Thres):", cc_symbol);
						}
					}

					create_database (_interval, &p_db, false);

					for (int i = 0; i < 5; ++i)
					{
						if (i < cf_cgf.size() && !cf_cgf[i].value.isEmpty())
						{
							is_update_error = BardataCalculator::update_close_threshold_v3
																	(&m_cache, p_db, cf_cgf[i].t_id,
																	 cf_cgf[i].value.toDouble(), cf_clf[i].value.toDouble(),
																	 cf_cgs[i].value.toDouble(), cf_cls[i].value.toDouble(), start_rowid);

							PRINT_ELAPSED_TIME ("Elapsed (Close-MA threshold):", cc_symbol);

							if (is_update_error) { err_message = "Error Close-MA thres"; goto exit_update; }

							is_update_error = BardataCalculator::update_fgs_fls_threshold_approx (&m_cache, p_db, cf_fgs[i].t_id,
															cf_fgs[i].value.toDouble(), cf_fls[i].value.toDouble(), start_rowid);

							PRINT_ELAPSED_TIME ("Elapsed (FGS/FLS threshold):", cc_symbol);

							if (is_update_error) { err_message = "Error FGS/FLS thres"; goto exit_update; }

							is_update_error = BardataCalculator::update_distfscross_threshold_approx (&m_cache, p_db, _interval,
															cf_cgf[i].value.toDouble(), cf_cgf[i].t_id, start_rowid);

							PRINT_ELAPSED_TIME ("Elapsed (Dist-FSCross threshold):", cc_symbol);

							if (is_update_error) { err_message = "Error Dist-FSCross thres"; goto exit_update; }

							create_database (_interval, &p_db, false);
						}
					}

					SQLiteHandler::reopen_database(&p_db);
				}

				// write into database
//				save_result(database_absolute_path.toLocal8Bit(), m_cache);
				save_result_v2(database_absolute_path, m_cache);
				debug_print_cache(_logger, m_cache, m_symbol.c_str(), Globals::intervalWeightToStdString(_interval).c_str());

				// set last datetime and rowid into Globals
//				for (int i = static_cast<int>(m_cache.size()) - 1; i >= 0; --i)
//				{
//					if (m_cache[i].completed != CompleteStatus::INCOMPLETE_PENDING &&
//							m_cache[i].completed != CompleteStatus::INCOMPLETE_DUMMY &&
//							m_cache[i].completed != CompleteStatus::INCOMPLETE)
//					{
//						LOG_DEBUG ("{} {} lastdatetime after update: {}/{}",
//											 m_symbol, interval_name, (m_cache[i].date + " " + m_cache[i].time).c_str(), m_cache[i].rowid);

//						Globals::set_database_lastdatetime(database_name, m_cache[i].date + " " + m_cache[i].time);
//						Globals::set_database_lastrowid(database_name, m_cache[i].rowid);
//						break;
//					}
//				}

				Globals::set_database_lastdatetime_rowid_v3(QString::fromStdString(m_database_path), m_symbol, _interval);
				LOG_DEBUG ("{} {} lastdatetime after update: {}/{}",
									 m_symbol, interval_name,
									 Globals::get_database_lastdatetime_v3(m_symbol, _interval),
									 Globals::get_database_lastrowid_v3(m_symbol, _interval));

				PRINT_ELAPSED_TIME ("Elapsed (SaveResult BardataTuple):", cc_symbol);

				if (_count_new_data > 0)
				{
					LOG_INFO ("{} {} finish updated", m_symbol, interval_name);
				}
			}
		}

		_interval = Globals::getChildInterval(_interval);
	}

	LOG_DEBUG ("{}: complete bar finish in: {} secs", m_symbol, std::chrono::duration_cast<milliseconds>(high_resolution_clock::now()-_time).count()/1000.0);

exit_update:

	if (!is_update_error)
	{
		if (_count_new_data > 0)
		{
			QTime time2; time2.start();
			QString input_file = QString::fromStdString(m_input_path);

			for (_interval = WEIGHT_LARGEST;
					 _interval > WEIGHT_5MIN;
					 _interval = Globals::getChildInterval(_interval))
			{
				// initialize the cache
				m_cache.clear();

				interval_name = Globals::intervalWeightToStdString(_interval);
				database_name = m_symbol + "_" + interval_name + ".db";
				database_absolute_path = QString::fromStdString(m_database_path + "/" + database_name);
				fname = m_symbol + "_" + interval_name + ".txt";

				if (Globals::is_file_exists(m_input_path + "/" + fname))
				{
					create_database(_interval, &p_db, true, true);

					// delete incomplete dummy that created in the first place
					{
						QSqlQuery q(p_db);
						q.exec("PRAGMA cache_size = 10000;");
						q.exec("PRAGMA busy_timeout = 2000;");
						q.exec("PRAGMA journal_mode = OFF;");
						q.exec("PRAGMA synchronous = OFF;");
						q.exec("begin transaction;");
//						q.exec("delete from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE_DUMMY));
						q.exec("delete from bardata where completed > 1");
						q.exec("commit;");
					}

					// prepare previous information to create incomplete bar
//					std::string last_datetime = Globals::get_database_lastdatetime(database_name);
					std::string last_datetime = Globals::get_database_lastdatetime_v3(m_symbol, _interval);

					QDateTime lastdt = QDateTime::fromString(QString::fromStdString(last_datetime),"yyyy-MM-dd hh:mm");
					BardataTuple curr_row;
					BardataTuple prev_row;
					QSqlQuery qs(p_db);

					qs.setForwardOnly(true);
					qs.exec("select open_, close_, prevbarcolor, fastavg, slowavg, intraday_high, intraday_low"
									" from bardata where completed=1 order by rowid desc limit 1");

					if (qs.next())
					{
						prev_row.open = qs.value(0).toDouble();
						prev_row.close = qs.value(1).toDouble();
						prev_row.prevbarcolor = qs.value(2).toString().toStdString();
						prev_row.fast_avg = qs.value(3).toDouble();
						prev_row.slow_avg = qs.value(4).toDouble();
						prev_row.intraday_high = qs.value(5).toDouble();
						prev_row.intraday_low = qs.value(6).toDouble();
					}

					LOG_DEBUG ("{} {}: start CREATE incomplete bar", m_symbol, Globals::intervalWeightToStdString(_interval));

					curr_row.completed = CompleteStatus::INCOMPLETE_PENDING;

					bool is_create_incomplete_success = BardataCalculator::create_incomplete_bar(p_db, cc_database_path,
						input_file, cc_symbol, _interval, lastdt, &curr_row, prev_row, _logger);

					if (!is_create_incomplete_success)
					{
						LOG_DEBUG ("{} {}: CREATE incomplete bar FAILED. (error or curr time = 5min time)", m_symbol, Globals::intervalWeightToStdString(_interval));
					}
					else
					{
						LOG_DEBUG ("{} {}: start CALCULATE incomplete bar.", m_symbol, Globals::intervalWeightToStdString(_interval));

						curr_row.completed = CompleteStatus::INCOMPLETE_PENDING;
						m_cache.emplace_back(curr_row);

						start_rowid = Globals::get_database_lastrowid(database_name);

						create_database(_interval, &p_db, false, true);

						is_update_error = BardataCalculator::update_candle_approx_v2(&m_cache, p_db, start_rowid);
						IF_INCOMPLETE_ERR("update_candle_approx_v2");

						is_update_error = BardataCalculator::update_zone(p_db, cc_database_path, cc_symbol, _interval, start_rowid);
						IF_INCOMPLETE_ERR("update_zone");

						is_update_error = BardataCalculator::update_rate_of_change_approx(p_db, 14, start_rowid);
						IF_INCOMPLETE_ERR("update_rate_of_change_approx");

						is_update_error = BardataCalculator::update_ccy_rate_controller(p_db, cc_database_path, cc_symbol, WEIGHT_DAILY, start_rowid);
						IF_INCOMPLETE_ERR("update_ccy_rate_controller");

						is_update_error = BardataCalculator::update_up_down_mom_approx(&m_cache, p_db, FASTAVG_LENGTH, start_rowid);
						IF_INCOMPLETE_ERR("update_up_down_mom_approx(F)");

						is_update_error = BardataCalculator::update_up_down_threshold_approx(&m_cache, p_db, FASTAVG_LENGTH, 0, 0, start_rowid);
						IF_INCOMPLETE_ERR("update_up_down_threshold_approx(F)");

						is_update_error = BardataCalculator::update_up_down_mom_approx(&m_cache, p_db, SLOWAVG_LENGTH, start_rowid);
						IF_INCOMPLETE_ERR("update_up_down_mom_approx(S)");

						is_update_error = BardataCalculator::update_up_down_threshold_approx(&m_cache, p_db, SLOWAVG_LENGTH, 0, 0, start_rowid);
						IF_INCOMPLETE_ERR("update_up_down_threshold_approx(S)");

						LOG_DEBUG ("{} {}: calculate incomplete(A)", m_symbol, Globals::intervalWeightToStdString(_interval));

						if (_interval < WEIGHT_MONTHLY)
						{
							is_update_error = BardataCalculator::update_index_parent_prev(p_db, cc_database_path, cc_symbol, Globals::getParentInterval(_interval), start_rowid);
							IF_INCOMPLETE_ERR("update_index_parent_prev");

							is_update_error = BardataCalculator::update_parent_monthly_v2(p_db, cc_database_path, cc_symbol);
							IF_INCOMPLETE_ERR("update_parent_monthly_v2");

							LOG_DEBUG ("{} {}: calculate for incomplete(B)", m_symbol, Globals::intervalWeightToStdString(_interval));
						}

						create_database(_interval, &p_db, false);
//            is_update_error = BardataCalculator::update_moving_avg_approx_v2(p_db, cc_database_path, cc_symbol, _interval, WEIGHT_DAILY, start_rowid);
//            is_update_error = BardataCalculator::update_moving_avg_approx_v2(p_db, cc_database_path, cc_symbol, _interval, WEIGHT_WEEKLY, start_rowid);
//            is_update_error = BardataCalculator::update_moving_avg_approx_v2(p_db, cc_database_path, cc_symbol, _interval, WEIGHT_MONTHLY, start_rowid);

						is_update_error = BardataCalculator::update_moving_avg_approx(p_db, cc_database_path, cc_symbol, _interval, WEIGHT_DAILY, start_rowid);
						IF_INCOMPLETE_ERR("update_moving_avg_approx(Daily)");

						is_update_error = BardataCalculator::update_moving_avg_approx(p_db, cc_database_path, cc_symbol, _interval, WEIGHT_WEEKLY, start_rowid);
						IF_INCOMPLETE_ERR("update_moving_avg_approx(Weekly)");

						is_update_error = BardataCalculator::update_moving_avg_approx(p_db, cc_database_path, cc_symbol, _interval, WEIGHT_MONTHLY, start_rowid);
						IF_INCOMPLETE_ERR("update_moving_avg_approx(Monthly)");

						LOG_DEBUG("{} {}: calculate incomplete(C)", m_symbol, Globals::intervalWeightToStdString(_interval));

						// DayRange
						if (_interval < WEIGHT_DAILY)
						{
							// prevdaily atr
							create_database (_interval, &p_db, false);
							is_update_error = BardataCalculator::update_prevdaily_atr(&m_cache, p_db, cc_database_path, cc_symbol);
							IF_INCOMPLETE_ERR("update_prevdaily_atr");

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

//								LOG_DEBUG("{}", q.lastQuery().toStdString());
								LOG_DEBUG("{} {}: dayrange start rowid (exact): {}",
													 m_symbol, Globals::intervalWeightToStdString(_interval), start_rowid_intraday);

								// find approx -- secondary
								if (start_rowid_intraday <= 0)
								{
									LOG_DEBUG("{} {}: dayrange start rowid (approx): {}",
														 m_symbol, Globals::intervalWeightToStdString(_interval), start_rowid_intraday);

									start_rowid_intraday = start_rowid - (_interval == WEIGHT_60MIN ? 96 : 1152);
								}
							}

							StreamProcessor processor;
							QSqlDatabase db_daily;
							create_database(WEIGHT_DAILY, &db_daily, false);
							create_database(_interval, &p_db, false);

							if (_interval == WEIGHT_60MIN)
							{
//								BardataQueryStream baseStream(p_db, start_rowid - 72);
//								BardataQueryStream baseStream(p_db, start_rowid - 96);
								BardataQueryStream baseStream(p_db, start_rowid_intraday);
								BardataQueryStream dailyStream(db_daily, 0);

								processor.bind(&baseStream);
								processor.setDailyStream(&dailyStream);
								processor.exec();
							}
							else if (_interval == WEIGHT_5MIN)
							{
//								BardataQueryStream baseStream(p_db, start_rowid - 864);
//								BardataQueryStream baseStream(p_db, start_rowid - 1152);
								BardataQueryStream baseStream(p_db, start_rowid_intraday);
								BardataQueryStream dailyStream(db_daily, 0);

//								QStringList sql_statement;
//								QStringList projection;
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_ROWID);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_DATE);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_TIME);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_OPEN);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_HIGH);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_LOW);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_CLOSE);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_FASTAVG);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWAVG);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_MACD);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_RSI);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWK);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_SLOWD);
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_IDPARENT);
//								projection.push_back("b." + SQLiteHandler::COLUMN_NAME_IDPARENT_PREV); // XX
//								projection.push_back("a." + SQLiteHandler::COLUMN_NAME_ATR);

//								sql_statement.push_back("attach database '" + cc_database_path + "/" + SQLiteHandler::getDatabaseName(cc_symbol,WEIGHT_60MIN) + "' as db60min");
//								sql_statement.push_back("select "+ projection.join(",") + " from bardata a"
//																				" join db60min.bardata b on b.rowid=(case when a._parent=0"
//																				" then a._parent_prev else a._parent end)"
//																				" where a.rowid >" + QString::number(start_rowid - 864));

//								baseStream.setQuery(sql_statement, p_db);
								processor.bind(&baseStream);
								processor.setDailyStream(&dailyStream);
								processor.exec();
							}

							is_update_error = BardataCalculator::update_dayrange_rank_approx(p_db, start_rowid);
							SQLiteHandler::removeDatabase(&db_daily);
//							SQLiteHandler::removeDatabase(&p_db);

							IF_INCOMPLETE_ERR("update_dayrange_rank_approx");
							PRINT_ELAPSED_TIME ("Elapsed (dayrange):", cc_symbol);
						}

						create_database(_interval, &p_db, false);

						is_update_error = BardataCalculator::update_rsi_slowk_slowd_rank_approx (p_db, start_rowid);
						IF_INCOMPLETE_ERR("update_rsi_slowk_slowd_rank_approx");

						is_update_error = BardataCalculator::update_distOHLC_rank_approx_v2
							(&m_cache, p_db, cc_database_path, cc_symbol, _interval, start_rowid);

						if (is_update_error) { err_message = "dist-OHLC rank"; goto exit_update_incomplete; }

						LOG_DEBUG ("{} {}: calculate incomplete(D)", m_symbol, Globals::intervalWeightToStdString(_interval));

						for (int i = 0; i < 3; ++i)
						{
							if (!cf_macd[i].operator1.isEmpty() && !cf_macd[i].operator2.isEmpty())
							{
								is_update_error = BardataCalculator::update_column_threshold_1(&m_cache, p_db,
									SQLiteHandler::COLUMN_NAME_MACD, cf_macd[i].t_id,
									cf_macd[i].operator1, QString::number(cf_macd[i].value1),
									cf_macd[i].operator2, QString::number(cf_macd[i].value2),
									start_rowid);

								if (is_update_error) { err_message = "MACD thres"; goto exit_update_incomplete; }

								PRINT_ELAPSED_TIME ("Elapsed (MACD-Thres):", cc_symbol);
							}

							if (!cf_rsi[i].operator1.isEmpty() && !cf_rsi[i].operator2.isEmpty())
							{
								is_update_error = BardataCalculator::update_column_threshold_1(&m_cache, p_db,
									 SQLiteHandler::COLUMN_NAME_RSI, cf_rsi[i].t_id,
									 cf_rsi[i].operator1, QString::number(cf_rsi[i].value1),
									 cf_rsi[i].operator2, QString::number(cf_rsi[i].value2),
									 start_rowid);

								if (is_update_error) { err_message = "RSI thres"; goto exit_update_incomplete; }

								PRINT_ELAPSED_TIME ("Elapsed (RSI-Thres):", cc_symbol);
							}

							if (!cf_slowk[i].operator1.isEmpty() && !cf_slowk[i].operator2.isEmpty())
							{
								is_update_error = BardataCalculator::update_column_threshold_1(&m_cache, p_db,
									 SQLiteHandler::COLUMN_NAME_SLOWK, cf_slowk[i].t_id,
									 cf_slowk[i].operator1, QString::number(cf_slowk[i].value1),
									 cf_slowk[i].operator2, QString::number(cf_slowk[i].value2),
									 start_rowid);

								if (is_update_error) { err_message = "SlowK thres"; goto exit_update_incomplete; }

								PRINT_ELAPSED_TIME ("Elapsed (SlowK-Thres):", cc_symbol);
							}

							if (!cf_slowd[i].operator1.isEmpty() && !cf_slowd[i].operator2.isEmpty())
							{
								is_update_error = BardataCalculator::update_column_threshold_1(&m_cache, p_db,
									 SQLiteHandler::COLUMN_NAME_SLOWD, cf_slowd[i].t_id,
									 cf_slowd[i].operator1, QString::number(cf_slowd[i].value1),
									 cf_slowd[i].operator2, QString::number(cf_slowd[i].value2),
									 start_rowid);

								if (is_update_error) { err_message = "SlowD thres"; goto exit_update_incomplete; }

								PRINT_ELAPSED_TIME ("Elapsed (SlowD-Thres):", cc_symbol);
							}
						}

						create_database(_interval, &p_db, false);

						LOG_DEBUG ("{} {}: calculate incomplete(E)", m_symbol, Globals::intervalWeightToStdString(_interval));

						for (int i = 0; i < 5; ++i)
						{
							if (i < cf_cgf.size() && !cf_cgf[i].value.isEmpty())
							{
								is_update_error = BardataCalculator::update_close_threshold_v3
									(&m_cache, p_db, cf_cgf[i].t_id,
									 cf_cgf[i].value.toDouble(), cf_clf[i].value.toDouble(),
									 cf_cgs[i].value.toDouble(), cf_cls[i].value.toDouble(), start_rowid);

								IF_INCOMPLETE_ERR("update_close_threshold_v3_"+std::to_string(i));
								PRINT_ELAPSED_TIME("Elapsed (Close-MA threshold):", cc_symbol);

								is_update_error = BardataCalculator::update_fgs_fls_threshold_approx
									(&m_cache, p_db, cf_fgs[i].t_id, cf_cgf[i].value.toDouble(), cf_cgf[i].value.toDouble(), start_rowid);

								IF_INCOMPLETE_ERR("update_fgs_fls_threshold_approx_"+std::to_string(i));
								PRINT_ELAPSED_TIME("Elapsed (FGS/FLS threshold):", cc_symbol);

								is_update_error = BardataCalculator::update_distfscross_threshold_approx
									(&m_cache, p_db, _interval, cf_cgf[i].value.toDouble(), cf_cgf[i].t_id, start_rowid);

								IF_INCOMPLETE_ERR("update_distfscross_threshold_approx_"+std::to_string(i));
								PRINT_ELAPSED_TIME("Elapsed (Dist-FSCross threshold):", cc_symbol);

								create_database(_interval, &p_db, false);
							}
						}

						LOG_DEBUG ("{} {}: calculate incomplete(F)", m_symbol, Globals::intervalWeightToStdString(_interval));

//						save_result(database_absolute_path.toLocal8Bit(), m_cache);
						save_result_v2(database_absolute_path, m_cache);
						debug_print_cache(_logger, m_cache, m_symbol.c_str(), Globals::intervalWeightToStdString(_interval).c_str());

						LOG_DEBUG ("{} {}: incomplete bar finish save result\n", m_symbol, Globals::intervalWeightToStdString(_interval));
					}

				} // end if file_exists
			} // end for

exit_update_incomplete:

			if (is_update_error)
			{
				LOG_DEBUG ("{}: error update incomplete bar: %s", m_symbol, err_message.c_str());
				qDebug("%s: error update incomplete bar. code(%d): %s", m_symbol.c_str(), is_update_error, err_message.c_str());
			}

			LOG_DEBUG ("{}: incomplete bar finish in: {} seconds)", m_symbol, time2.elapsed()/1000.0);
		}
	}
	else
	{
		LOG_INFO("{} update skipped: {}.", database_name, err_message);
	}

	SQLiteHandler::removeDatabase(&p_db);

	LOG_DEBUG("{}: DataUpdater finish in: {} secs", m_symbol, duration_cast<milliseconds>(high_resolution_clock::now()-_time).count()/1000.0);
	PRINT_ELAPSED_TIME("DBUpdater finish in:", cc_symbol);

	// @disabled: create incomplete bar v1
	// update if only there's a new data and update was successed previously
	/*if (m_found_new && !is_update_error)
	{
		QString ccy_database =
				cc_symbol == "@NIY" ? "USDJPY_Daily.db" :
			 (cc_symbol == "HHI" || cc_symbol == "HSI" ? "USDHKD_Daily.db" : "");

		_interval = WEIGHT_60MIN;

		while (_interval != WEIGHT_INVALID)
		{
			try {
				if (create_database (WEIGHT_5MIN, &p_db, true, true))
				{
					BardataCalculator::update_incomplete_bar
						(p_db, cc_database_path, ccy_database, cc_symbol, WEIGHT_5MIN, _interval, _logger);
				}
			}
			catch (const std::exception &ex)
			{
				LOG_DEBUG ("{} {}: update incomplete bar error: {}", m_symbol,
									 Globals::intervalWeightToStdString(_interval), ex.what());
			}
			catch (...)
			{
				LOG_DEBUG ("{} {}: update incomplete bar error: unhandled exception", m_symbol,
									 Globals::intervalWeightToStdString(_interval));

				qFatal ("%s %s: update incomplete bar error: unhandled exception", m_symbol.c_str(),
								Globals::intervalWeightToStdString(_interval).c_str());
			}

			_interval = Globals::getParentInterval(_interval);
		}

		PRINT_ELAPSED_TIME ("Elapsed (Update incomplete bar):", cc_symbol);
		LOG_DEBUG ("{}: Elapsed (Update incomplete bar): {} seconds", m_symbol,
							 (duration_cast<chrono::milliseconds>(high_resolution_clock::now()-_time).count()/1000.0));
	}

	SQLiteHandler::removeDatabase(&p_db);*/
}

/*void DataUpdater::rollback() {
	sqlite3 *db = 0;
	IntervalWeight _interval = WEIGHT_LARGEST;
	std::string database_name;
	std::string interval_name;
	std::string delete_stmt;
	int rc;

	while (_interval != WEIGHT_INVALID) {
		interval_name = Globals::intervalWeightToString(_interval).toStdString();
		database_name = m_database_path + "/" + m_symbol + "_" + interval_name + ".db";
		rc = sqlite3_open(database_name.c_str(), &db);

		if (rc == SQLITE_OK) {
			delete_stmt = "delete from bardata where rowid >" + std::to_string(m_last_rowid[_interval]);
			sqlite3_exec(db, bardata::begin_transaction, 0, 0, 0);
			sqlite3_exec(db, delete_stmt.c_str(), 0, 0, 0);
			sqlite3_exec(db, bardata::end_transaction, 0, 0, 0);
		}

		sqlite3_close(db);
		_interval = Globals::getChildInterval(_interval);
	}
}*/
