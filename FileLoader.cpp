#include "fileloader.h"

#include "searchbar/sqlitehandler.h"
#include "bardatadefinition.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>

#define BUSY_TIMEOUT 2000

#define LOG_ERROR(...) try{_logger->error(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_WARN(...) try{_logger->warn(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_INFO(...) try{_logger->info(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace std::chrono;
using namespace std;

inline int get_moving_avg_state(double fast, double slow)
{
	return (fast > slow ? 1 : (fast < slow ? 2 : 3));
}

inline std::string parse_date(const std::string &mm_dd_yyyy)
{
	// convert MM/dd/yyyy into yyyy-MM-dd
	return mm_dd_yyyy.substr(6,4) + "-" + mm_dd_yyyy.substr(0,2) + "-" + mm_dd_yyyy.substr(3,2);
}

inline void parse_line_datetime(const std::string &line, BardataTuple *results)
{
	results->date = parse_date(line.substr(0, 10));
	results->time = line.substr(11, 5);
}

inline bool parse_line(const std::string &line, BardataTuple *results)
{
	std::vector<std::string> columns;
	boost::split(columns, line, boost::is_any_of(","));

	if (columns.size() < 20)
		return false;

	results->date      = parse_date(columns[0]);
	results->time      = columns[1];

	try
	{
		results->open      = std::stod(columns[2]);
		results->high      = std::stod(columns[3]);
		results->low       = std::stod(columns[4]);
		results->close     = std::stod(columns[5]);
		results->volume    = std::stoi(columns[6]); // int
		results->macd      = std::stod(columns[7]);
		results->macd_avg  = std::stod(columns[8]);
		results->macd_diff = std::stod(columns[9]);
		results->rsi       = std::stod(columns[10]);
		results->slow_k    = std::stod(columns[11]);
		results->slow_d    = std::stod(columns[12]);
		results->fast_avg  = std::stod(columns[13]);
		results->slow_avg  = std::stod(columns[14]);
		results->dist_f    = std::stod(columns[15]);
		results->dist_s    = std::stod(columns[16]);
		results->fgs       = std::stoi(columns[17]); // int
		results->fls       = std::stoi(columns[18]); // int
		results->openbar   = std::stoi(columns[19]); // int
	}
	catch (std::invalid_argument &)
	{
		return false;
	}
	catch (std::out_of_range &)
	{
		return false;
	}

	results->set_data(bardata::COLUMN_NAME_MACD, columns[7]);
	results->set_data(bardata::COLUMN_NAME_RSI, columns[10]);
	results->set_data(bardata::COLUMN_NAME_SLOWK, columns[11]);
	results->set_data(bardata::COLUMN_NAME_SLOWD, columns[12]);

	return true;
}

void FileLoader::set_logger(Logger logger)
{
	_logger = logger;
}

void FileLoader::continous_calculation(Globals::dbupdater_info *data, const BardataTuple &_old, BardataTuple *_new)
{
	if (_new == NULL) return;

	// macd
	data->macd_m12 = data->macd_m12 + (SF_MACD_FAST * (_new->close - data->macd_m12));
	data->macd_m26 = data->macd_m26 + (SF_MACD_SLOW * (_new->close - data->macd_m26));
	double real_macd = data->macd_m12 - data->macd_m26;

	_new->macd = Globals::round_decimals(real_macd, 6);
	_new->macd_avg = Globals::round_decimals(data->macd_avg_prev + (SF_MACD_LENGTH * (real_macd - data->macd_avg_prev)), 6);
	_new->macd_diff = Globals::round_decimals(_new->macd - _new->macd_avg, 6);
	data->macd_avg_prev = _new->macd_avg;

	// rsi
	data->rsi_change = _new->close - _new->prev_close;
	data->rsi_netchgavg = data->rsi_netchgavg + (SF_RSI * (data->rsi_change - data->rsi_netchgavg));
	data->rsi_totchgavg = data->rsi_totchgavg + (SF_RSI * (std::fabs(data->rsi_change) - data->rsi_totchgavg));

	if (data->rsi_totchgavg != 0.0)
	{
		_new->rsi = Globals::round_decimals(50 * (1 + (data->rsi_netchgavg / data->rsi_totchgavg)), 2);
	}
	else
	{
		_new->rsi = 50.0;
	}

	// slowk, slowd
	if (data->slowk_highest_high.size() + 1 > STOCHASTIC_LENGTH)
	{
			if (!data->slowk_highest_high.empty()) data->slowk_highest_high.pop_front();
			if (!data->slowk_lowest_low.empty()) data->slowk_lowest_low.pop_front();
	}

	data->slowk_highest_high.push_back(_new->high);
	data->slowk_lowest_low.push_back(_new->low);

	double _hh = data->slowk_highest_high[0];
	double _ll = data->slowk_lowest_low[0];

	for (int i = 1 ; i < data->slowk_highest_high.size(); ++i)
	{
		_hh = std::fmax(_hh, data->slowk_highest_high[i]);
		_ll = std::fmin(_ll, data->slowk_lowest_low[i]);
	}

	double curr_num1 = (_new->close - _ll);
	double curr_den1 = (_hh - _ll);
	double real_slowk = (data->slowk_num1_0 + data->slowk_num1_1 + curr_num1) * 100.0 /
											(data->slowk_den1_0 + data->slowk_den1_1 + curr_den1);

	_new->slow_k = Globals::round_decimals(real_slowk, 2);
	_new->slow_d = Globals::round_decimals((data->slowk_last_0 + data->slowk_last_1 + real_slowk) / 3.0, 2);

	data->slowk_num1_0 = data->slowk_num1_1;
	data->slowk_num1_1 = curr_num1;

	data->slowk_den1_0 = data->slowk_den1_1;
	data->slowk_den1_1 = curr_den1;

	data->slowk_last_0 = data->slowk_last_1;
	data->slowk_last_1 = real_slowk;

	// moving avg
	if (data->close_last50.size()+1 > MA_SLOWLENGTH)
	{
		data->close_last50.pop_front();
	}

	data->close_last50.push_back(_new->close);

	if (data->close_last50.size() >= MA_FASTLENGTH)
	{
		double sum_close = 0;
		int last_index = data->close_last50.size() - 1;

		for (int i = 0; i < MA_FASTLENGTH; ++i)
		{
			sum_close += data->close_last50[last_index-i];
		}

		_new->fast_avg = Globals::round_decimals(sum_close / MA_FASTLENGTH, 4);
	}

	if (data->close_last50.size() >= MA_SLOWLENGTH)
	{
		double sum_close = 0;

		for (int i = 0; i < MA_SLOWLENGTH; ++i)
		{
			sum_close += data->close_last50[i];
		}

		_new->slow_avg = Globals::round_decimals(sum_close / MA_SLOWLENGTH, 4);
	}

	_new->set_data(bardata::COLUMN_NAME_MACD, QString::number(_new->macd).toStdString());
	_new->set_data(bardata::COLUMN_NAME_RSI, QString::number(_new->rsi).toStdString());
	_new->set_data(bardata::COLUMN_NAME_SLOWK, QString::number(_new->slow_k).toStdString());
	_new->set_data(bardata::COLUMN_NAME_SLOWD, QString::number(_new->slow_d).toStdString());
	_new->set_data(bardata::COLUMN_NAME_FASTAVG, QString::number(_new->fast_avg).toStdString());
	_new->set_data(bardata::COLUMN_NAME_SLOWAVG, QString::number(_new->slow_avg).toStdString());

	// BarColor
	std::string barcolor = _old.prevbarcolor;
	if (barcolor.size() == 19) barcolor.erase (0, 2);
	if (barcolor.size() > 0) barcolor += ",";

	if (_old.close > _old.open) barcolor += "G";
	else if (_old.close < _old.open) barcolor += "R";
	else barcolor += "D";

	_new->prevbarcolor = barcolor;
	_new->prevdate = _old.date;
	_new->prevtime = _old.time;

	// f-slope, s-slope
	if (_old.fast_avg > 0.0)
	{
		_new->fastavg_slope = _new->fast_avg - _old.fast_avg;
		_new->set_data (bardata::COLUMN_NAME_FASTAVG_SLOPE, std::to_string(_new->fastavg_slope));
	}

	if (_old.slow_avg > 0.0)
	{
		_new->slowavg_slope = _new->slow_avg - _old.slow_avg;
		_new->set_data (bardata::COLUMN_NAME_SLOWAVG_SLOPE, std::to_string(_new->slowavg_slope));
	}

	// BarsAbove and BarsBelow
	{
		double _max_olc = std::max(_new->open, std::max(_new->close, _new->low));
		double _min_ohc = std::min(_new->open, std::min(_new->close, _new->high));

		_new->lgf = (_max_olc > _new->fast_avg) ? _old.lgf + 1 : 0;
		_new->lgs = (_max_olc > _new->slow_avg) ? _old.lgs + 1 : 0;
		_new->hlf = (_min_ohc < _new->fast_avg) ? _old.hlf + 1 : 0;
		_new->hls = (_min_ohc < _new->slow_avg) ? _old.hls + 1 : 0;
	}

	// ATR
	{
		double tr_1 = _new->high - _new->low;
		double tr_2 = 0.0;
		double tr_3 = 0.0;

		if (_old.close > 0.0)
		{
			tr_2 = std::fabs(_new->high - _old.close);
			tr_3 = std::fabs(_new->low - _old.close);
		}

		m_tr.push(std::fmax(std::fmax(tr_1, tr_2), tr_3));
		sum_tr += m_tr.back();

		if (m_tr.size() > 14)
		{
			// remove stale TR
			sum_tr = sum_tr - m_tr.front();
			m_tr.pop();

			// ATR for 14th bar onward
			_new->atr = sum_tr / 14.0;
		}
		else
		{
			if (_old.atr == 0.0)
			{
				// ATR for 1st Bar
				_new->atr = tr_1;
			}
			else
			{
				// ATR for 2nd - 13th Bar
				_new->atr = ((_old.atr * 13) + m_tr.back()) / 14.0;
			}
		}
	}

	// Dist (FS-Cross)
	if (curr_fs_state == 0)
	{
		curr_fs_state = get_moving_avg_state(_new->fast_avg, _new->slow_avg);
		_new->dist_cc_fscross = 0;
		_new->dist_cc_fscross_atr = 0;
		_new->fs_cross = 0;
	}
	else
	{
		curr_fs_state = get_moving_avg_state(_new->fast_avg, _new->slow_avg);

		if (prev_fs_state != curr_fs_state)
		{
			fscross_close = _old.close;

			// state changed (FS-Cross is found)
			_new->fs_cross = 1;
		}
		else
		{
			// continous state
			_new->fs_cross = 0;
		}

		if (fscross_close > 0)
		{
			_new->dist_cc_fscross = _new->close - fscross_close;

			if (m_interval >= WEIGHT_DAILY)
			{
				if (_new->atr != 0.0)
				{
					_new->dist_cc_fscross_atr = _new->dist_cc_fscross / _new->atr;
				}
				else
				{
					_new->dist_cc_fscross_atr = 0.0;
				}
			}
			else
			{
				_new->dist_cc_fscross_atr = _new->dist_cc_fscross;
			}
		}
		else
		{
			_new->dist_cc_fscross = 0.0;
			_new->dist_cc_fscross_atr = 0.0;
		}
	}

	_new->set_data(bardata::COLUMN_NAME_DISTCC_FSCROSS, std::to_string(_new->dist_cc_fscross));
	_new->set_data(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR, std::to_string(_new->dist_cc_fscross_atr));

	prev_fs_state = curr_fs_state;
}

void FileLoader::direct_calculation(BardataTuple *out)
{
	if (out != NULL)
	{
		out->dist_of = out->open - out->fast_avg;
		out->dist_os = out->open - out->slow_avg;
		out->dist_hf = out->high - out->fast_avg;
		out->dist_hs = out->high - out->slow_avg;
		out->dist_lf = out->low - out->fast_avg;
		out->dist_ls = out->low - out->slow_avg;
		out->dist_cf = out->close - out->fast_avg;
		out->dist_cs = out->close - out->slow_avg;
		out->dist_fs = out->fast_avg - out->slow_avg;

		// old version
//		if (out->close != 0.0)
//		{
//			out->dist_of_close = out->dist_of / out->close;
//			out->dist_os_close = out->dist_os / out->close;
//			out->dist_hf_close = out->dist_hf / out->close;
//			out->dist_hs_close = out->dist_hs / out->close;
//			out->dist_lf_close = out->dist_lf / out->close;
//			out->dist_ls_close = out->dist_ls / out->close;
//			out->dist_cf_close = out->dist_cf / out->close;
//			out->dist_cs_close = out->dist_cs / out->close;
//			out->dist_fs_close = out->dist_fs / out->close;
//		}

		// new version -- normalize by ATR
		if (out->atr != 0.0)
		{
			out->n_dist_of = out->dist_of / out->atr;
			out->n_dist_os = out->dist_os / out->atr;
			out->n_dist_hf = out->dist_hf / out->atr;
			out->n_dist_hs = out->dist_hs / out->atr;
			out->n_dist_lf = out->dist_lf / out->atr;
			out->n_dist_ls = out->dist_ls / out->atr;
			out->n_dist_cf = out->dist_cf / out->atr;
			out->n_dist_cs = out->dist_cs / out->atr;
			out->n_dist_fs = out->dist_fs / out->atr;
		}

		out->candle_uptail = std::fabs(out->high - std::max (out->open, out->close));
		out->candle_downtail = std::fabs(std::min (out->open, out->close) - out->low);
		out->candle_body = std::fabs(out->open - out->close);
		out->candle_totallength = std::fabs(out->high - out->low);
		out->f_cross = (out->fast_avg >= out->low && out->fast_avg <= out->high) ? 1 : 0;
		out->s_cross = (out->slow_avg >= out->low && out->slow_avg <= out->high) ? 1 : 0;
		out->bar_range = out->high - out->low;

		// new indicators since 1.7.0
		if (m_interval < WEIGHT_DAILY)
		{
			if (out->openbar == 1)
			{
				out->intraday_high = out->high;
				out->intraday_low = out->low;
				_intraday_high = out->high;
				_intraday_low = out->low;
			}
			else
			{
				out->intraday_high = std::fmax(out->high, _intraday_high);
				out->intraday_low = _intraday_low != 0.0 ? std::fmin(out->low, _intraday_low) : out->low;
				_intraday_high = out->intraday_high;
				_intraday_low = out->intraday_low;
			}

			out->intraday_range = out->intraday_high - out->intraday_low;
		}
	}
}

void FileLoader::finalize_parent_stream()
{
	sqlite3_finalize(parent_stmt);
	sqlite3_finalize(parent_prev_stmt);
	sqlite3_close(parent);

	parent_stmt = NULL;
	parent_prev_stmt = NULL;
	parent = NULL;
}

std::string FileLoader::get_insert_statement()
{
	std::vector<string> projection;
	projection.emplace_back(bardata::COLUMN_NAME_DATE);
	projection.emplace_back(bardata::COLUMN_NAME_TIME);
	projection.emplace_back(bardata::COLUMN_NAME_OPEN);
	projection.emplace_back(bardata::COLUMN_NAME_HIGH);
	projection.emplace_back(bardata::COLUMN_NAME_LOW);
	projection.emplace_back(bardata::COLUMN_NAME_CLOSE);
	projection.emplace_back(bardata::COLUMN_NAME_VOLUME);
	projection.emplace_back(bardata::COLUMN_NAME_MACD);
	projection.emplace_back(bardata::COLUMN_NAME_MACDAVG);
	projection.emplace_back(bardata::COLUMN_NAME_MACDDIFF);
	projection.emplace_back(bardata::COLUMN_NAME_RSI);
	projection.emplace_back(bardata::COLUMN_NAME_SLOWK);
	projection.emplace_back(bardata::COLUMN_NAME_SLOWD);
	projection.emplace_back(bardata::COLUMN_NAME_FASTAVG);
	projection.emplace_back(bardata::COLUMN_NAME_SLOWAVG);
	projection.emplace_back(bardata::COLUMN_NAME_DISTF);
	projection.emplace_back(bardata::COLUMN_NAME_DISTS);
	projection.emplace_back(bardata::COLUMN_NAME_FGS);
	projection.emplace_back(bardata::COLUMN_NAME_FLS);
	projection.emplace_back(bardata::COLUMN_NAME_OPENBAR);
	projection.emplace_back(bardata::COLUMN_NAME_PREV_DATE);
	projection.emplace_back(bardata::COLUMN_NAME_PREV_TIME);
	projection.emplace_back(bardata::COLUMN_NAME_PREV_BARCOLOR);
	projection.emplace_back(bardata::COLUMN_NAME_FASTAVG_SLOPE);
	projection.emplace_back(bardata::COLUMN_NAME_SLOWAVG_SLOPE);
	projection.emplace_back(bardata::COLUMN_NAME_FCROSS);
	projection.emplace_back(bardata::COLUMN_NAME_SCROSS);
	projection.emplace_back(bardata::COLUMN_NAME_ATR);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_UPTAIL);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_DOWNTAIL);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_BODY);
	projection.emplace_back(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCC_FSCROSS);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR);
	projection.emplace_back(bardata::COLUMN_NAME_DISTFS);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTFS);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCF);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTCF);
	projection.emplace_back(bardata::COLUMN_NAME_DISTCS);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTCS);
	projection.emplace_back(bardata::COLUMN_NAME_DISTOF);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTOF);
	projection.emplace_back(bardata::COLUMN_NAME_DISTOS);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTOS);
	projection.emplace_back(bardata::COLUMN_NAME_DISTHF);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTHF);
	projection.emplace_back(bardata::COLUMN_NAME_DISTHS);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTHS);
	projection.emplace_back(bardata::COLUMN_NAME_DISTLF);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTLF);
	projection.emplace_back(bardata::COLUMN_NAME_DISTLS);
	projection.emplace_back(bardata::COLUMN_NAME_N_DISTLS);
	projection.emplace_back(bardata::COLUMN_NAME_HLF);
	projection.emplace_back(bardata::COLUMN_NAME_HLS);
	projection.emplace_back(bardata::COLUMN_NAME_LGF);
	projection.emplace_back(bardata::COLUMN_NAME_LGS);
	projection.emplace_back(bardata::COLUMN_NAME_FSCROSS);
	projection.emplace_back(bardata::COLUMN_NAME_IDPARENT);
	projection.emplace_back(bardata::COLUMN_NAME_COMPLETED);
	projection.emplace_back(bardata::COLUMN_NAME_BAR_RANGE);
	projection.emplace_back(bardata::COLUMN_NAME_INTRADAY_HIGH);
	projection.emplace_back(bardata::COLUMN_NAME_INTRADAY_LOW);
	projection.emplace_back(bardata::COLUMN_NAME_INTRADAY_RANGE);

	std::string _columns = projection[0];
	std::string _args = "?";
	int count = static_cast<int>(projection.size());

	for (int i = 1; i < count; ++i)
	{
		_columns += "," + projection[i];
		_args += ",?";
	}

	return ("insert or ignore into bardata(" + _columns + ")values(" + _args + ")");
}

long int FileLoader::indexing_calculation(const std::string &_date, const std::string &_time)
{
	if (parent == NULL || parent_stmt == NULL)
	{
		// patch to pre-index "incomplete parent"
		if (!last_parent_date.empty() && !last_parent_time.empty())
		{
			ptime child_datetime(time_from_string(_date + " " + _time));
			ptime last_parent_datetime(time_from_string(last_parent_date + " " + last_parent_time));

			if (child_datetime > last_parent_datetime)
			{
				return last_parent_rowid + 1;
			}
		}

		return 0;
	}

	long int parent_rowid = sqlite3_column_int(parent_stmt, 0);
	const char *_ccDate = reinterpret_cast<const char*>(sqlite3_column_text(parent_stmt,1));
	const char *_ccTime = reinterpret_cast<const char*>(sqlite3_column_text(parent_stmt,2));
	std::string parent_date = "";
	std::string parent_time = "";
	int rc;

	if (_ccDate != NULL && _ccTime != NULL)
	{
		parent_date = std::string(_ccDate);
		parent_time = std::string(_ccTime);
	}

	while (parent_date < _date || (parent_date == _date && parent_time < _time))
	{
		rc = sqlite3_step(parent_stmt);

		if (rc == SQLITE_ROW)
		{
			parent_rowid = sqlite3_column_int(parent_stmt, 0);
			parent_date = std::string(reinterpret_cast<const char*>(sqlite3_column_text(parent_stmt,1)));
			parent_time = std::string(reinterpret_cast<const char*>(sqlite3_column_text(parent_stmt,2)));
		}
		else
		{
			sqlite3_finalize(parent_stmt);
			parent_stmt = 0;
			break;
		}
	}

	if (parent_date == "" || parent_time == "") return 0;

	ptime parent_upper_bound(time_from_string(parent_date + " " + parent_time));
	ptime parent_lower_bound(time_from_string(parent_date + " " + parent_time));
	ptime child_datetime(time_from_string(_date + " " + _time));

	parent_lower_bound -= boost::posix_time::minutes(Globals::getParentInterval(m_interval));

	if (child_datetime > parent_lower_bound && child_datetime <= parent_upper_bound)
	{
		last_parent_date = parent_date;
		last_parent_time = parent_time;
		last_parent_rowid = parent_rowid;

		return parent_rowid;
	}
	else if (child_datetime > parent_upper_bound)
	{
		last_parent_date = parent_date;
		last_parent_time = parent_time;
		last_parent_rowid = parent_rowid;

		return parent_rowid + 1;
	}
	else
	{
		// patch to pre-index "incomplete parent"
		if (!last_parent_date.empty() && !last_parent_time.empty())
		{
			ptime last_parent_datetime(time_from_string(last_parent_date + " " + last_parent_time));
			if (child_datetime > last_parent_datetime)
			{
				return last_parent_rowid + 1;
			}
		}

		return 0;
	}
}

void FileLoader::indexing_calculation_patch()
{
	sqlite3 *db = NULL;
	sqlite3_stmt *select_stmt = NULL;
	sqlite3_stmt *update_stmt = NULL;
	std::string database_file = m_database_path + "/" + m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db";
	std::string sql_select = "select rowid, date_, time_, _parent from bardata where rowid >= (select max(rowid) from bardata where _parent > 0)";
	std::string sql_update = "update bardata set _parent=? where rowid=?";
	std::string sqlite_pragma = "PRAGMA temp_store = MEMORY; PRAGMA busy_timeout =" + std::to_string(BUSY_TIMEOUT) + ";";
	std::string _date, _time;
	long int _rowid = 0;
	long int _parent = 0;

	if (sqlite3_open(database_file.c_str(), &db) != SQLITE_OK)
	{
		sqlite3_close(db);
		return;
	}

	sqlite3_exec(db, sqlite_pragma.c_str(), NULL, NULL, NULL);
	sqlite3_prepare_v2(db, sql_select.c_str(), static_cast<int>(sql_select.length()), &select_stmt, NULL);
	sqlite3_prepare_v2(db, sql_update.c_str(), static_cast<int>(sql_update.length()), &update_stmt, NULL);
	sqlite3_exec(db, bardata::begin_transaction, NULL, NULL, NULL);

	if (sqlite3_step(select_stmt) == SQLITE_ROW)
	{
		_parent = sqlite3_column_int(select_stmt, 3);
		prepare_parent_stream(_parent);
	}

	while (sqlite3_step(select_stmt) == SQLITE_ROW)
	{
		_rowid = sqlite3_column_int(select_stmt, 0);
		_date = std::string(reinterpret_cast<const char*>(sqlite3_column_text(select_stmt, 1)));
		_time = std::string(reinterpret_cast<const char*>(sqlite3_column_text(select_stmt, 2)));
		_parent = indexing_calculation(_date, _time);
		sqlite3_bind_int(update_stmt, 1, _parent);
		sqlite3_bind_int(update_stmt, 2, _rowid);
		sqlite3_step(update_stmt);
		sqlite3_reset(update_stmt);
	}

	sqlite3_exec(db, bardata::end_transaction, NULL, NULL, NULL);
	sqlite3_finalize(select_stmt);
	sqlite3_finalize(update_stmt);
	finalize_parent_stream();
	sqlite3_close(db);
}

bool FileLoader::insert_database(std::vector<BardataTuple> *out, std::string *err_msg)
{
	ifstream file(m_filename, std::ifstream::in | std::ifstream::out);
	bool result_code = true;

	if (file.is_open())
	{
		sqlite3 *db = NULL;
		sqlite3_stmt *stmt = NULL;
		std::string database_file = m_database_path + "/" + m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db";
		std::string line, page_size;
		BardataTuple t_new, t_old, t_incomplete;
		const int date_length = 10; // yyyy-MM-dd
		const int time_length = 5; // HH:mm
		int rc = 0;

		// get last datetime from globals
//		line = Globals::get_database_lastdatetime(m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db");
		line = Globals::get_database_lastdatetime_v3(m_symbol, m_interval);

		// get last indicator information (to calculate macd, rsi, slowk, slowd)
		Globals::dbupdater_info data = Globals::get_last_indicator_for_dbupdater
			(m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db");

		std::vector<std::string> columns;
		boost::split(columns, line, boost::is_any_of(" "));
		std::string c_lastdate = columns.size() > 0 ? columns[0] : "";
		std::string c_lasttime = columns.size() > 1 ? columns[1] : "";

		LOG_DEBUG ("{} {} (FILELOADER)::last datetime: {} {}",
							 m_symbol.c_str(), Globals::intervalWeightToStdString(m_interval).c_str(),
							 c_lastdate.c_str(), c_lasttime.c_str());

		if (sqlite3_open_v2(database_file.c_str(), &db, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE, 0) != SQLITE_OK)
		{
			if (err_msg != NULL)
			{
				*err_msg += "can't open database: "+ std::string(sqlite3_errmsg(db)) + "\n";
			}
			sqlite3_close(db);
			return false;
		}

		// because we're already had fat columns schema
		// it's a good idea to allocate large page size
		page_size = m_interval >= WEIGHT_DAILY ? "32768" : "65536";

		std::string create_table_stmt =
			"PRAGMA page_size=" + page_size + ";" +
			SQLiteHandler::SQL_CREATE_TABLE_BARDATA_V2.toStdString() +
			SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_V1.toStdString() +
			SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_DATE_V1.toStdString() +
			SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_V1.toStdString() +
			SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_DATE_V1.toStdString() +
			SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT.toStdString() +
			SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV.toStdString() +
			SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_MONTHLY.toStdString() +
			SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV_MONTHLY.toStdString() +
			SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PREVDATETIME.toStdString();

		sqlite3_exec(db, create_table_stmt.c_str(), NULL, NULL, NULL);

		// database locked checking + delete inconsistent rows
		{
			std::string sql_stmt =
				"PRAGMA busy_timeout=" + std::to_string(BUSY_TIMEOUT) + ";"
				"delete from bardata where completed > 1;"; // new in v1.7.0
//				"update bardata set completed=0 where completed=0;"; // @deprecated

			rc = sqlite3_exec(db, sql_stmt.c_str(), NULL, NULL, NULL);

			if (rc != SQLITE_OK)
			{
				if (err_msg != NULL)
				{
					*err_msg += std::string(sqlite3_errmsg(db)) + "(" + std::to_string(rc) + ")";
				}
				sqlite3_close(db);
				file.close();
				return false;
			}
		}

		// get last incomplete information for patch later
		load_last_incomplete(&t_incomplete);

		// try to patch _parent indexing
		indexing_calculation_patch();

		// collect last state information for continous calculation
		if (!load_last_state (&t_old))
		{
			if (err_msg != NULL)
			{
				*err_msg = "can't load last state.";
			}
			result_code = false;
		}

		// initalize datetime from log (for empty case only)
		if (c_lastdate.empty() || c_lasttime.empty())
		{
			c_lastdate = t_old.date;
			c_lasttime = t_old.time;
		}

		// prepare insert statement
		std::string sql_insert = get_insert_statement();
		sqlite3_exec(db, bardata::begin_transaction, NULL, NULL, NULL);
		sqlite3_prepare_v2(db, sql_insert.c_str(), static_cast<int>(sql_insert.length()), &stmt, NULL);

		// prepare parent stream
		prepare_parent_stream (t_old._parent_prev);

		// discard first line (column label information)
		std::getline(file, line);

		LOG_DEBUG ("{} {} (FILELOADER):: start to read file", m_symbol.c_str(), Globals::intervalWeightToStdString(m_interval).c_str());

		while (file.good() && !file.eof())
		{
			std::getline(file, line);
			if (line.empty()) break;

			// initially we only parse date time, to make scanning faster
			parse_line_datetime (line, &t_new);

			// compare last complete date from database -- @deprecated
//      if (t_new.date < t_old.date || (t_new.date == t_old.date && t_new.time <= t_old.time))

			// compare last complete date from globals (alternative)
			if (t_new.date < c_lastdate || (t_new.date == c_lastdate && t_new.time <= c_lasttime))
			{
				continue;
			}
			else
			{
				// check incomplete bar
				if (t_incomplete.completed == 0)
				{
					if (t_new.date == t_incomplete.date && t_new.time == t_incomplete.time)
					{
						// normal case when actual complete bar has created
						// action: OVERRIDE incomplete bar with complete bar
						sqlite3_exec(db, "delete from bardata where completed=0;", NULL, NULL, NULL);

						// invalidate record
						t_incomplete.completed = -1;
					}
					else if (t_new.date > t_incomplete.date || (t_new.date == t_incomplete.date && t_new.time > t_incomplete.time))
					{
						// usually this bar is daily bar which fall on holiday
						// action: DELETE previous incomplete bar and maintain rowid sequence
						sqlite3_exec(db, "delete from bardata where completed=0;", NULL, NULL, NULL);

						// invalidate record
						t_incomplete.completed = -1;
					}
				}
			}

			if (!parse_line(line, &t_new))
			{
				result_code = false;
				break;
			}

			// get previous close (needed for next calculation)
			t_new.prev_close = t_old.close;

			// sequence of calculation matter! don't reorder without knowing the impact
			// calculate (1): macd, rsi, slowk, slowd
			// calculate (2): basic DERIVED indicators (such as dist-OHLC x MovingAvg)
			continous_calculation(&data, t_old, &t_new);
			direct_calculation(&t_new);

			t_new._parent = indexing_calculation(t_new.date, t_new.time);
			t_new.rowid = t_old.rowid + 1;
			t_new.completed = CompleteStatus::COMPLETE_PENDING;

			// override old with new record
			t_old = t_new;

			// bind values to statements
			sqlite3_bind_text   (stmt, 1, t_new.date.c_str(), date_length, 0);
			sqlite3_bind_text   (stmt, 2, t_new.time.c_str(), time_length, 0);
			sqlite3_bind_double (stmt, 3, t_new.open);
			sqlite3_bind_double (stmt, 4, t_new.high);
			sqlite3_bind_double (stmt, 5, t_new.low);
			sqlite3_bind_double (stmt, 6, t_new.close);
			sqlite3_bind_int64  (stmt, 7, t_new.volume);
			sqlite3_bind_double (stmt, 8, t_new.macd);
			sqlite3_bind_double (stmt, 9, t_new.macd_avg);
			sqlite3_bind_double (stmt, 10, t_new.macd_diff);
			sqlite3_bind_double (stmt, 11, t_new.rsi);
			sqlite3_bind_double (stmt, 12, t_new.slow_k);
			sqlite3_bind_double (stmt, 13, t_new.slow_d);
			sqlite3_bind_double (stmt, 14, t_new.fast_avg);
			sqlite3_bind_double (stmt, 15, t_new.slow_avg);
			sqlite3_bind_double (stmt, 16, t_new.dist_f);
			sqlite3_bind_double (stmt, 17, t_new.dist_s);
			sqlite3_bind_int    (stmt, 18, t_new.fgs);
			sqlite3_bind_int    (stmt, 19, t_new.fls);
			sqlite3_bind_int    (stmt, 20, t_new.openbar);
			sqlite3_bind_text   (stmt, 21, t_new.prevdate.c_str(), static_cast<int>(t_new.prevdate.size()), 0);
			sqlite3_bind_text   (stmt, 22, t_new.prevtime.c_str(), static_cast<int>(t_new.prevtime.size()), 0);
			sqlite3_bind_text   (stmt, 23, t_new.prevbarcolor.c_str(), static_cast<int>(t_new.prevbarcolor.size()), 0);
			sqlite3_bind_double (stmt, 24, Globals::round_decimals(t_new.fastavg_slope, 4));
			sqlite3_bind_double (stmt, 25, Globals::round_decimals(t_new.slowavg_slope, 4));
			sqlite3_bind_int    (stmt, 26, t_new.f_cross);
			sqlite3_bind_int    (stmt, 27, t_new.s_cross);
			sqlite3_bind_double (stmt, 28, Globals::round_decimals(t_new.atr, 5));
			sqlite3_bind_double (stmt, 29, Globals::round_decimals(t_new.candle_uptail, 4));
			sqlite3_bind_double (stmt, 30, Globals::round_decimals(t_new.candle_downtail, 4));
			sqlite3_bind_double (stmt, 31, Globals::round_decimals(t_new.candle_body, 4));
			sqlite3_bind_double (stmt, 32, Globals::round_decimals(t_new.candle_totallength, 4));
			sqlite3_bind_double (stmt, 33, Globals::round_decimals(t_new.dist_cc_fscross, 4));
			sqlite3_bind_double (stmt, 34, Globals::round_decimals(t_new.dist_cc_fscross_atr, 4));
			sqlite3_bind_double (stmt, 35, Globals::round_decimals(t_new.dist_fs, 4));
			sqlite3_bind_double (stmt, 36, Globals::round_decimals(t_new.n_dist_fs, 8));
			sqlite3_bind_double (stmt, 37, Globals::round_decimals(t_new.dist_cf, 4));
			sqlite3_bind_double (stmt, 38, Globals::round_decimals(t_new.n_dist_cf, 8));
			sqlite3_bind_double (stmt, 39, Globals::round_decimals(t_new.dist_cs, 4));
			sqlite3_bind_double (stmt, 40, Globals::round_decimals(t_new.n_dist_cs, 8));
			sqlite3_bind_double (stmt, 41, Globals::round_decimals(t_new.dist_of, 4));
			sqlite3_bind_double (stmt, 42, Globals::round_decimals(t_new.n_dist_of, 8));
			sqlite3_bind_double (stmt, 43, Globals::round_decimals(t_new.dist_os, 4));
			sqlite3_bind_double (stmt, 44, Globals::round_decimals(t_new.n_dist_os, 8));
			sqlite3_bind_double (stmt, 45, Globals::round_decimals(t_new.dist_hf, 4));
			sqlite3_bind_double (stmt, 46, Globals::round_decimals(t_new.n_dist_hf, 8));
			sqlite3_bind_double (stmt, 47, Globals::round_decimals(t_new.dist_hs, 4));
			sqlite3_bind_double (stmt, 48, Globals::round_decimals(t_new.n_dist_hs, 8));
			sqlite3_bind_double (stmt, 49, Globals::round_decimals(t_new.dist_lf, 4));
			sqlite3_bind_double (stmt, 50, Globals::round_decimals(t_new.n_dist_lf, 8));
			sqlite3_bind_double (stmt, 51, Globals::round_decimals(t_new.dist_ls, 4));
			sqlite3_bind_double (stmt, 52, Globals::round_decimals(t_new.n_dist_ls, 8));
			sqlite3_bind_int    (stmt, 53, t_new.hlf);
			sqlite3_bind_int    (stmt, 54, t_new.hls);
			sqlite3_bind_int    (stmt, 55, t_new.lgf);
			sqlite3_bind_int    (stmt, 56, t_new.lgs);
			sqlite3_bind_int    (stmt, 57, t_new.fs_cross);
			sqlite3_bind_int    (stmt, 58, t_new._parent);
			sqlite3_bind_int    (stmt, 59, t_new.completed);
			sqlite3_bind_double (stmt, 60, Globals::round_decimals(t_new.bar_range, 5));
			sqlite3_bind_double (stmt, 61, Globals::round_decimals(t_new.intraday_high, 5));
			sqlite3_bind_double (stmt, 62, Globals::round_decimals(t_new.intraday_low, 5));
			sqlite3_bind_double (stmt, 63, Globals::round_decimals(t_new.intraday_range, 5));

			rc = sqlite3_step(stmt);

			if (rc != SQLITE_DONE)
			{
				qDebug(sqlite3_errmsg(db));
				break;
			}

			if (out != NULL) out->emplace_back(t_new);

			sqlite3_reset(stmt);
		}

		sqlite3_exec(db, bardata::end_transaction, NULL, NULL, NULL);

		// release database resources
		finalize_parent_stream();
		sqlite3_finalize(stmt);
		sqlite3_close(db);

		if (out != NULL && !out->empty())
		{
			if (t_new.date.empty())
			{
				if (err_msg != NULL)
				{
					*err_msg = "last date is empty.";
				}
				result_code = false;
			}

			LOG_DEBUG ("{} {} (FILELOADER) last datetime after insert: {} {}",
								 m_symbol.c_str(), Globals::intervalWeightToStdString(m_interval).c_str(),
								 t_new.date.c_str(), t_new.time.c_str());

			// write back last datetime and indicators calculation infomation into Globals
			// for use in next iteration

			// @disabled -- we only set lastdatetime after it completely updated not partial
//      Globals::set_database_lastdatetime
//        (m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db",
//         t_new.date + " " + t_new.time);

			if (result_code)
			{
				Globals::set_last_indicator_dbupdater(m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db", data);
			}
		}

		file.close();
	}
	else
	{
		result_code = false;
	}

	return result_code;
}

bool FileLoader::load(const std::string &filename, const std::string &database_path,
											const std::string &symbol, IntervalWeight interval,
											std::vector<BardataTuple> *result, std::string *err_msg)
{
	m_filename = filename;
	m_database_path = database_path;
	m_symbol = symbol;
	m_interval = interval;
	return insert_database(result, err_msg);
}

bool FileLoader::load_last_state(BardataTuple *_old)
{
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	std::string database_file = m_database_path + "/" + m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db";
	std::string sql_statement;
	double prev_close = -1;
	int last_rowid = 0;
	int rc;

	if (sqlite3_open(database_file.c_str(), &db) != SQLITE_OK)
	{
		sqlite3_close(db);
		return false;
	}

	// prepare basic indicator
	{
		sql_statement = "select max(rowid), date_, time_,"
										"hlf, hls, lgf, lgs, fastavg, slowavg, prevbarcolor,"
										"open_, high_, low_, close_, atr, _parent, _parent_prev"
										" from bardata where completed=1";

		rc = sqlite3_prepare_v2(db, sql_statement.c_str(), static_cast<int>(sql_statement.length()), &stmt, NULL);

		if (rc == SQLITE_OK)
		{
			sqlite3_step(stmt);
			last_rowid = sqlite3_column_int(stmt, 0);
			_old->rowid = last_rowid;

			const char * _date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			const char * _time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

			if (_date != NULL && _time != NULL)
			{
				_old->date     = std::string(_date);
				_old->time     = std::string(_time);
				_old->hlf      = sqlite3_column_int(stmt, 3);
				_old->hls      = sqlite3_column_int(stmt, 4);
				_old->lgf      = sqlite3_column_int(stmt, 5);
				_old->lgs      = sqlite3_column_int(stmt, 6);
				_old->fast_avg = sqlite3_column_double(stmt, 7);
				_old->slow_avg = sqlite3_column_double(stmt, 8);
				const char *tmp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));

				if (tmp != NULL)
				{
					_old->prevbarcolor = std::string(tmp);
				}

				_old->open         = sqlite3_column_double(stmt, 10);
				_old->high         = sqlite3_column_double(stmt, 11);
				_old->low          = sqlite3_column_double(stmt, 12);
				_old->close        = sqlite3_column_double(stmt, 13);
				_old->atr          = sqlite3_column_double(stmt, 14);
				_old->_parent      = sqlite3_column_int(stmt, 15);
				_old->_parent_prev = sqlite3_column_int(stmt, 16);
			}
		}

		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	// prepare intraday range
	if (m_interval < WEIGHT_DAILY)
		sql_statement = "select high_, low_ from bardata where rowid >="
										"(select rowid from bardata where openbar=1 order by rowid desc limit 1)";
	{
		rc = sqlite3_prepare(db, sql_statement.c_str(), static_cast<int>(sql_statement.size()), &stmt, NULL);

		if (rc == SQLITE_OK)
		{
			if (sqlite3_step(stmt) == SQLITE_ROW)
			{
				_intraday_high = sqlite3_column_double(stmt,0);
				_intraday_low = sqlite3_column_double(stmt,1);
			}

			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				_intraday_high = std::fmax(_intraday_high, sqlite3_column_double(stmt,0));
				_intraday_low = std::fmin(_intraday_low, sqlite3_column_double(stmt,1));
			}
		}

		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	// prepare Dist(FS-Cross)
	{
		sql_statement =
			"select close_ from bardata where rowid=(select max(rowid)-1"
			" from bardata where fs_cross=1 and rowid<=" + std::to_string(last_rowid) + " and completed=1)";

		rc = sqlite3_prepare(db, sql_statement.c_str(), static_cast<int>(sql_statement.size()), &stmt, NULL);

		if (rc == SQLITE_OK)
		{
			sqlite3_step(stmt);
			fscross_close = sqlite3_column_double(stmt, 0);
		}

		sqlite3_finalize(stmt);
		stmt = NULL;

		prev_fs_state = get_moving_avg_state(_old->fast_avg, _old->slow_avg);
		curr_fs_state = prev_fs_state;
	}

	// prepare ATR information
	{
		double t1, t2, t3, _tr;
		double m_high, m_low;

		sql_statement =
			"select high_, low_, close_ from bardata"
			" where rowid >" + std::to_string(last_rowid - 15) + " and completed=1"
			" order by rowid asc limit 15";

		sqlite3_prepare_v2(db, sql_statement.c_str(), static_cast<int> (sql_statement.size()), &stmt, NULL);

		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			prev_close = sqlite3_column_double(stmt, 2);
		}

		sum_tr = 0.0;

		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			m_high = sqlite3_column_double(stmt, 0);
			m_low  = sqlite3_column_double(stmt, 1);

			if (prev_close > -1)
			{
				t1 = m_high - m_low;
				t2 = std::fabs(m_high - prev_close);
				t3 = std::fabs(m_low - prev_close);
				_tr = std::fmax(std::fmax(t1, t2), t3);
				m_tr.emplace(_tr);
				sum_tr += _tr;
			}
			else
			{
				m_tr.emplace(0);
			}

			prev_close = sqlite3_column_double(stmt, 2);
		}
	}

	_old->close = prev_close;
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return true;
}

bool FileLoader::load_last_incomplete(BardataTuple *t)
{
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	std::string database_file = m_database_path + "/" + m_symbol + "_" + Globals::intervalWeightToStdString(m_interval) + ".db";
	bool ret = true;
	t->completed = 1; // set incomplete not found yet

	if (sqlite3_open(database_file.c_str(), &db) != SQLITE_OK)
	{
		sqlite3_close(db);
		return false;
	}

	// finally, query for incomplete bar
	std::string sql_statement = "select rowid, date_, time_ from bardata where completed=0 order by rowid desc limit 1";
	int rc = sqlite3_prepare_v2(db, sql_statement.c_str(), static_cast<int>(sql_statement.length()), &stmt, NULL);

	if (rc == SQLITE_OK)
	{
		sqlite3_step(stmt);
		const char * _date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		const char * _time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

		if (_date != NULL && _time != NULL)
		{
			t->rowid = sqlite3_column_int(stmt, 0);
			t->date = std::string(_date);
			t->time = std::string(_time);
			t->completed = 0;
		}
	}
	else
	{
		ret = false;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return ret;
}

void FileLoader::prepare_parent_stream(int start_rowid)
{
	IntervalWeight _parent_interval = Globals::getParentInterval(m_interval);

	// Check if there is interval of parent or parent connection is still in used
	if (_parent_interval == WEIGHT_INVALID || parent != NULL)
		return;

	std::string database_file =
		m_database_path + "/" + m_symbol + "_" +
		Globals::intervalWeightToStdString(_parent_interval) + ".db";

	int rc = sqlite3_open(database_file.c_str(), &parent);

	if (rc != SQLITE_OK)
	{
		sqlite3_finalize(parent_stmt);
		sqlite3_finalize(parent_prev_stmt);
		sqlite3_close(parent);
		parent_stmt = NULL;
		parent_prev_stmt = NULL;
		parent = NULL;
		return;
	}

	// Try to create table if not exists
	string page_size;
	if (_parent_interval == WEIGHT_MONTHLY) page_size = "4096"; else
	if (_parent_interval >= WEIGHT_DAILY) page_size = "16384";
	else page_size = "65536";

	string create_table_stmt =
		"PRAGMA page_size =" + page_size + ";" +
		"PRAGMA temp_store = MEMORY;" +
		"PRAGMA soft_heap_limit = 10000;" +
		SQLiteHandler::SQL_CREATE_TABLE_BARDATA_V2.toStdString() +
		SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_V1.toStdString() +
		SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_DATE_V1.toStdString() +
		SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_V1.toStdString() +
		SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_DATE_V1.toStdString() +
		SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT.toStdString() +
		SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV.toStdString() +
		SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_MONTHLY.toStdString() +
		SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PREVDATETIME.toStdString();

	rc = sqlite3_exec(parent, create_table_stmt.c_str(), NULL, NULL, NULL);

	// post parent
	string select_parent =
		"select rowid, date_, time_, atr from bardata"
		" where rowid >=" + std::to_string(start_rowid) +
		" order by rowid asc";

	sqlite3_prepare_v2(parent, select_parent.c_str(), static_cast<int>(select_parent.length()), &parent_stmt, NULL);
	sqlite3_step(parent_stmt);

	// pre parent
	select_parent =
		"select rowid, date_, time_, atr from bardata"
		" where rowid >=" + std::to_string(start_rowid) +
		" order by rowid asc";

	sqlite3_prepare_v2(parent, select_parent.c_str(), static_cast<int>(select_parent.length()), &parent_prev_stmt, NULL);
	sqlite3_step(parent_prev_stmt);
}

long int FileLoader::pre_indexing_calculation(const std::string &_date, const std::string &_time)
{
	if (parent == 0 || parent_prev_stmt == 0) return 0;

	long int curr_parent_rowid = 0;
	long int next_parent_rowid;
	const char *_ccDate = reinterpret_cast<const char*> (sqlite3_column_text(parent_prev_stmt,1));
	const char *_ccTime = reinterpret_cast<const char*> (sqlite3_column_text(parent_prev_stmt,2));
	string curr_parent_date = "";
	string curr_parent_time = "";

	if (_ccDate != 0 && _ccTime != 0) {
		curr_parent_date = std::string(_ccDate);
		curr_parent_time = std::string(_ccTime);
		curr_parent_rowid = sqlite3_column_int (parent_prev_stmt, 0);
	}

	if (curr_parent_date < _date || (curr_parent_date == _date && curr_parent_time <= _time)) {
		return curr_parent_rowid;
	}
	else {
		std::string next_parent_date = "";
		std::string next_parent_time = "";

		if (sqlite3_step(parent_prev_stmt) == SQLITE_ROW) {
			const char *_ccDate1 = reinterpret_cast<const char*>(sqlite3_column_text(parent_prev_stmt,1));
			const char *_ccTime1 = reinterpret_cast<const char*>(sqlite3_column_text(parent_prev_stmt,2));
			next_parent_date = std::string(_ccDate1);
			next_parent_time = std::string(_ccTime1);
			next_parent_rowid = sqlite3_column_int(parent_prev_stmt, 0);

			while (next_parent_date < _date || (next_parent_date == _date && next_parent_time < _time)) {
				if (sqlite3_step(parent_prev_stmt) != SQLITE_ROW) {
					sqlite3_finalize(parent_prev_stmt);
					parent_prev_stmt = 0;
					break;
				}

				curr_parent_rowid = next_parent_rowid;
				curr_parent_date = next_parent_date;
				curr_parent_time = next_parent_time;

				next_parent_rowid = sqlite3_column_int(parent_prev_stmt, 0);
				next_parent_date = std::string(reinterpret_cast<const char*>(sqlite3_column_text(parent_prev_stmt,1)));
				next_parent_time = std::string(reinterpret_cast<const char*>(sqlite3_column_text(parent_prev_stmt,2)));
			}
		}

		if (curr_parent_date == "" || curr_parent_time == "") {
			return 0;
		}

		if (next_parent_date == "" || next_parent_time == "") {
			next_parent_date = curr_parent_date;
			next_parent_time = curr_parent_time;
		}

		ptime parent_upper_bound (time_from_string (curr_parent_date + " " + curr_parent_time));
		ptime parent_lower_bound (time_from_string (next_parent_date + " " + next_parent_time));
		ptime child_datetime (time_from_string (_date + " " + _time));

		if (child_datetime >= parent_lower_bound && child_datetime < parent_upper_bound)
			return curr_parent_rowid;
		else
			return 0;
	}
}
