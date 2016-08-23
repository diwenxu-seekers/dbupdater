#include "bardatacalculator.h"

#include "bardatadefinition.h"
#include "datetimehelper.h"
#include "searchbar/sqlitehandler.h"
#include "sqlite3/sqlite3.h"

#include <iostream>
#include <list>
#include <numeric>
#include <queue>

#define DEBUG_MODE
#define BUSY_TIMEOUT 1000
#define GET_RANK() q.next()?QString::number(q.value(0).toDouble(),'f',4).toStdString():"0"

#ifdef DEBUG_MODE
#define QUERY_LAST_ERROR(q,m) if(q.lastError().isValid()){result_code=false;qDebug()<<m<<q.lastError();}
#define QUERY_ERROR_CODE(q,e,m) if(q.lastError().isValid()){e=q.lastError().number();qDebug()<<m<<q.lastError();}
#define QUERY_ERROR(q,m) if(q.lastError().isValid()){qDebug()<<m<<q.lastError();}
#else
#define QUERY_LAST_ERROR(q,m) if(q.lastError().isValid()){result_code=false;}
#define QUERY_ERROR_CODE(q,e,m) if(q.lastError().isValid()){e=q.lastError().number();}
#define QUERY_ERROR(q,m)
#endif

#define BEGIN_TRANSACTION(q) \
	q.exec("PRAGMA cache_size = 20000;"); \
	q.exec("PRAGMA temp_store = MEMORY;"); \
	q.exec("PRAGMA journal_mode = OFF;"); \
	q.exec("PRAGMA synchronous = OFF;"); \
	q.exec("BEGIN TRANSACTION;");

#define END_TRANSACTION(q) q.exec("COMMIT;")
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define STRINGNUMBER(a,digits) QString::number(a,'f',digits).toStdString()

#define UPDATE_INCOMPLETE_OHLC_ONLY   0
#define DISABLE_INCOMPLETE_HISTOGRAM  0
#define DISABLE_INCOMPLETE_SR         1

using namespace std;

typedef struct {
	char date[11];
	char time[6];
	double open;
	double high;
	double low;
	double close;
} t_ohlc;

typedef struct {
	QString date_, react_date;
	double price_level;
} t_price;

typedef struct {
	QString date_;
	long int rowid;
} t_record;

typedef struct {
	QString date, time;
	double rate, prevrate;
} t_ccy;

inline int check_zone(double price, double fast, double slow)
{
	if (fast < 0 || slow < 0) return 0; // invalid zone
	if (price >= fast && fast >= slow) return 1;
	if (fast >= price && price >= slow) return 2;
	if (fast >= slow && slow >= price) return 3;
	if (price >= slow && slow >= fast) return 4;
	if (slow >= price && price >= fast) return 5;
	if (slow >= fast && fast >= price) return 6;
	return 0;
}

inline int get_moving_avg_state(double fast, double slow)
{
	return (fast > slow ? 1 : (fast < slow ? 2 : 3));
}

inline void continous_calculation(const BardataTuple &_old, BardataTuple *_new, bool is_intraday)
{
	if (_new == NULL) return;

	std::string barcolor = _old.prevbarcolor;
	if (barcolor.size() == 19) barcolor.erase(0, 2);
	if (barcolor.size() > 0) barcolor += ",";

	if (_old.close > _old.open) barcolor += "G"; else
	if (_old.close < _old.open) barcolor += "R";
	else barcolor += "D";

	_new->prevbarcolor = barcolor;
	_new->prevdate = _old.date;
	_new->prevtime = _old.time;

	if (_old.fast_avg > 0.0)
	{
		_new->fastavg_slope = _new->fast_avg - _old.fast_avg;
		_new->set_data(bardata::COLUMN_NAME_FASTAVG_SLOPE, QString::number(_new->fastavg_slope,'f',4).toStdString());
	}

	if (_old.slow_avg > 0.0)
	{
		_new->slowavg_slope = _new->slow_avg - _old.slow_avg;
		_new->set_data(bardata::COLUMN_NAME_SLOWAVG_SLOPE, QString::number(_new->slowavg_slope,'f',4).toStdString());
	}

	_new->hlf = (_new->high < _new->fast_avg) ? _old.hlf + 1 : 0;
	_new->hls = (_new->high < _new->slow_avg) ? _old.hls + 1 : 0;
	_new->lgf = (_new->low > _new->fast_avg) ? _old.lgf + 1 : 0;
	_new->lgs = (_new->low > _new->slow_avg) ? _old.lgs + 1 : 0;

	_new->dist_of = _new->open - _new->fast_avg;
	_new->dist_os = _new->open - _new->slow_avg;
	_new->dist_hf = _new->high - _new->fast_avg;
	_new->dist_hs = _new->high - _new->slow_avg;
	_new->dist_lf = _new->low - _new->fast_avg;
	_new->dist_ls = _new->low - _new->slow_avg;
	_new->dist_cf = _new->close - _new->fast_avg;
	_new->dist_cs = _new->close - _new->slow_avg;
	_new->dist_fs = _new->fast_avg - _new->slow_avg;

	// old version
//	if (_new->close != 0.0)
//	{
//		_new->dist_of_close = _new->dist_of / _new->close;
//		_new->dist_os_close = _new->dist_os / _new->close;
//		_new->dist_hf_close = _new->dist_hf / _new->close;
//		_new->dist_hs_close = _new->dist_hs / _new->close;
//		_new->dist_lf_close = _new->dist_lf / _new->close;
//		_new->dist_ls_close = _new->dist_ls / _new->close;
//		_new->dist_cf_close = _new->dist_cf / _new->close;
//		_new->dist_cs_close = _new->dist_cs / _new->close;
//		_new->dist_fs_close = _new->dist_fs / _new->close;
//	}

	// new version -- normalize by atr
	if (_new->atr != 0.0)
	{
		_new->n_dist_of = _new->dist_of / _new->atr;
		_new->n_dist_os = _new->dist_os / _new->atr;
		_new->n_dist_hf = _new->dist_hf / _new->atr;
		_new->n_dist_hs = _new->dist_hs / _new->atr;
		_new->n_dist_lf = _new->dist_lf / _new->atr;
		_new->n_dist_ls = _new->dist_ls / _new->atr;
		_new->n_dist_cf = _new->dist_cf / _new->atr;
		_new->n_dist_cs = _new->dist_cs / _new->atr;
		_new->n_dist_fs = _new->dist_fs / _new->atr;
	}

	_new->candle_uptail = std::fabs(_new->high - std::fmax(_new->open, _new->close));
	_new->candle_downtail = std::fabs(std::fmin(_new->open, _new->close) - _new->low);
	_new->candle_body = std::fabs(_new->open - _new->close);
	_new->candle_totallength = std::fabs(_new->high - _new->low);

	_new->f_cross = (_new->fast_avg >= _new->low && _new->fast_avg <= _new->high) ? 1 : 0;
	_new->s_cross = (_new->slow_avg >= _new->low && _new->slow_avg <= _new->high) ? 1 : 0;

	// new indicators since 1.7.0
	_new->bar_range = _new->high - _new->low;

	if (is_intraday)
	{
		if (_new->openbar == 1)
		{
			_new->intraday_high = _new->high;
			_new->intraday_low = _new->low;
		}
		else
		{
			_new->intraday_high = std::fmax(_new->high, _old.intraday_high);
			_new->intraday_low = _old.intraday_low != 0.0 ? std::fmin(_new->low, _old.intraday_low) : _new->low;
		}

		_new->intraday_range = _new->intraday_high - _new->intraday_low;
	}
}

inline void continous_calculation_v2(Globals::dbupdater_info *data, BardataTuple *r)
{
	if (data == NULL || r == NULL)
	{
		return;
	}

	// macd
	data->macd_m12 = data->macd_m12 + (SF_MACD_FAST * (r->close - data->macd_m12));
	data->macd_m26 = data->macd_m26 + (SF_MACD_SLOW * (r->close - data->macd_m26));
	double real_macd = data->macd_m12 - data->macd_m26;

	r->macd = Globals::round_decimals(real_macd, 6);
	r->macd_avg = Globals::round_decimals(data->macd_avg_prev + (SF_MACD_LENGTH * (real_macd - data->macd_avg_prev)), 6);
	r->macd_diff = Globals::round_decimals(r->macd - r->macd_avg, 6);

	data->macd_avg_prev = r->macd_avg;

	// rsi
	data->rsi_change = r->close - r->prev_close;
	data->rsi_netchgavg = data->rsi_netchgavg + (SF_RSI * (data->rsi_change - data->rsi_netchgavg));
	data->rsi_totchgavg = data->rsi_totchgavg + (SF_RSI * (std::fabs(data->rsi_change) - data->rsi_totchgavg));

	if (data->rsi_totchgavg != 0.0)
	{
		r->rsi = Globals::round_decimals(50.0 * (1.0 + (data->rsi_netchgavg / data->rsi_totchgavg)), 2);
	}
	else
	{
		r->rsi = 50.0;
	}

	// slowk & slowd
	if (data->slowk_highest_high.size() + 1 > STOCHASTIC_LENGTH)
	{
		if (!data->slowk_highest_high.empty())
		{
			data->slowk_highest_high.pop_front();
		}

		if (!data->slowk_lowest_low.empty())
		{
			data->slowk_lowest_low.pop_front();
		}
	}

	data->slowk_highest_high.push_back(r->high);
	data->slowk_lowest_low.push_back(r->low);

	double _hh = data->slowk_highest_high[0];
	double _ll = data->slowk_lowest_low[0];

	for (int i = 1 ; i < data->slowk_highest_high.size(); ++i)
	{
		_hh = std::fmax(_hh, data->slowk_highest_high[i]);
		_ll = std::fmin(_ll, data->slowk_lowest_low[i]);
	}

	double curr_num1 = (r->close - _ll);
	double curr_den1 = (_hh - _ll);
	double real_slowk = (data->slowk_num1_0 + data->slowk_num1_1 + curr_num1) * 100.0 /
											(data->slowk_den1_0 + data->slowk_den1_1 + curr_den1);

	r->slow_k = Globals::round_decimals(real_slowk, 2);
	r->slow_d = Globals::round_decimals((data->slowk_last_0 + data->slowk_last_1 + real_slowk) / 3.0, 2);

	data->slowk_num1_0 = data->slowk_num1_1;
	data->slowk_num1_1 = curr_num1;
	data->slowk_den1_0 = data->slowk_den1_1;
	data->slowk_den1_1 = curr_den1;
	data->slowk_last_0 = data->slowk_last_1;
	data->slowk_last_1 = real_slowk;

	// moving avg
	if (data->close_last50.size() + 1 > MA_SLOWLENGTH)
	{
		data->close_last50.pop_front();
	}

	data->close_last50.push_back(r->close);

	if (data->close_last50.size() >= MA_FASTLENGTH)
	{
		double sum_close = 0;
		int last_index = data->close_last50.size() - 1;
		int _index;

		for (int i = 0; i < MA_FASTLENGTH; ++i)
		{
			_index = last_index - i;

			// index bound checking
			if (_index > -1 && _index < data->close_last50.size())
			{
				sum_close += data->close_last50[_index];
			}
		}

		r->fast_avg = Globals::round_decimals(sum_close / MA_FASTLENGTH, 4);
	}

	if (data->close_last50.size() >= MA_SLOWLENGTH)
	{
		double sum_close = 0.0;

		for (int i = 0; i < MA_SLOWLENGTH; ++i)
		{
			sum_close += data->close_last50[i];
		}

		r->slow_avg = Globals::round_decimals(sum_close / MA_SLOWLENGTH, 4);
	}
}

/*int exec_with_errcode(QSqlQuery &query, const QString &stmt, QString *errString = NULL)
{
	if (query.exec(stmt)) return 0; // query success

	if (query.lastError().isValid())
	{
		if (errString != NULL) *errString = query.lastError().text();
		return query.lastError().number(); // native err code
	}

	return 99; // stmt error
}*/

/*void exec_busy_query(QSqlQuery &query, const QString &stmt)
{
	int retry = 0;
	int rc = exec_with_errcode(query, stmt, NULL);

	while (rc != 0 && rc != 99) {
		if (++retry >= 10) break;
		rc = exec_with_errcode(query, stmt, NULL);
	}
}*/


int BardataCalculator::update_candle_approx_v2(std::vector<BardataTuple> *out, const QSqlDatabase &database, int start_rowid)
{
	const int row_count = static_cast<int>(out->size());
	QSqlQuery q(database);
	QString _rowid = QString::number(start_rowid);
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	for (int i = 0; i < row_count; ++i)
	{
		q.exec("select " + SQLiteHandler::COLUMN_NAME_N_UPTAIL + "," + SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL_RANK +
					 " from bardata where rowid <" + _rowid + " and " +
							SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL + "<=" + QString::number((*out)[i].candle_uptail,'f') +
					 " order by " + SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "update candle");

		if (q.next())
		{
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_UPTAIL, q.value(0).toString().toStdString());
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK, q.value(1).toString().toStdString());
//			(*out)[i].candle_uptail_normal = q.value(0).toDouble();
//			(*out)[i].candle_uptail_rank = q.value(1).toDouble();
		}

		q.exec("select " + SQLiteHandler::COLUMN_NAME_N_DOWNTAIL + "," + SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL_RANK +
					 " from bardata where rowid <" + _rowid + " and " +
						 SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL + "<=" + QString::number((*out)[i].candle_downtail,'f') +
					 " order by " + SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "update candle");

		if (q.next())
		{
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_DOWNTAIL, q.value(0).toString().toStdString());
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK, q.value(1).toString().toStdString());
//			(*out)[i].candle_downtail_normal = q.value(0).toDouble();
//			(*out)[i].candle_downtail_rank = q.value(1).toDouble();
		}

		q.exec("select " + SQLiteHandler::COLUMN_NAME_N_BODY + "," + SQLiteHandler::COLUMN_NAME_CANDLE_BODY_RANK +
					 " from bardata where rowid <" + _rowid + " and " +
						 SQLiteHandler::COLUMN_NAME_CANDLE_BODY + "<=" + QString::number((*out)[i].candle_body,'f') +
					 " order by " + SQLiteHandler::COLUMN_NAME_CANDLE_BODY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "update candle");

		if (q.next())
		{
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_BODY, q.value(0).toString().toStdString());
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_BODY_RANK, q.value(1).toString().toStdString());
//			(*out)[i].candle_body_normal = q.value(0).toDouble();
//			(*out)[i].candle_body_rank = q.value(1).toDouble();
		}

		q.exec("select " + SQLiteHandler::COLUMN_NAME_N_TOTALLENGTH + "," + SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK +
					 " from bardata where rowid <" + _rowid + " and " +
						 SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH + "<=" + QString::number((*out)[i].candle_totallength,'f') +
					 " order by " + SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "update candle");

		if (q.next())
		{
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH, q.value(0).toString().toStdString());
			(*out)[i].set_data(bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK, q.value(1).toString().toStdString());
//			(*out)[i].candle_totallength_normal = q.value(0).toDouble();
//			(*out)[i].candle_totallength_rank = q.value(1).toDouble();
		}
	}

	return errcode;
}

int BardataCalculator::update_index_parent_prev(const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																								const IntervalWeight parent, int start_rowid)
{
	QSqlQuery q(database);
	QString _rowid_from_base;
	QString _rowid_from_parent;
	int start_from_rowid_base = start_rowid;
	int errcode = 0;
	bool enable_optimization = true;

	q.setForwardOnly(true);

	if (enable_optimization)
	{
		q.exec("select max(rowid) from bardata where _parent_prev > 0 and completed=1");
		_rowid_from_base = q.next() ? q.value(0).toString() : "0";

		q.exec("select _parent_prev from bardata where rowid=" + QString::number(start_from_rowid_base));
		_rowid_from_parent = q.next() ? q.value(0).toString() : "0";
	}

	QString parent_alias = "db" + Globals::intervalWeightToString(parent);
	QString parent_bardata = parent_alias + "." + SQLiteHandler::TABLE_NAME_BARDATA;
	QString parent_database_path = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, parent);

	q.exec("attach database '" + parent_database_path + "' AS " + parent_alias);
	QUERY_ERROR_CODE (q,errcode,"index parent prev (attach)");

	BEGIN_TRANSACTION (q);
	q.exec("update bardata set _parent_prev ="
					 "ifnull(("
					 "select rowid from " + parent_bardata + " a"
					 " where bardata.rowid >=" + _rowid_from_parent +
					 " and (bardata.date_ > a.date_ or (bardata.date_ = a.date_ and bardata.time_ > a.time_))"
					 " order by a.rowid desc limit 1),0)"
				 " where rowid >=" + _rowid_from_base);
	QUERY_ERROR_CODE (q,errcode,"index parent prev (update)");
	END_TRANSACTION (q);

	q.exec("detach database " + parent_alias);

	return errcode;
}

int BardataCalculator::update_parent_monthly_v2(const QSqlDatabase &base_database, const QString &database_path, const QString &symbol)
{
	std::vector<t_record> monthly_data;
	t_record row;

	QSqlQuery q(base_database);
	QSqlQuery q_update(base_database);

	const QString _dbmonthly = "dbmonthly";
	const QString _dbmonthly_bardata = _dbmonthly + ".bardata";
	const QString _dbmonthly_path = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_MONTHLY);
	QString start_rowid_base_monthly_prev = "";
	QString start_rowid_parent_1 = "";
//  QString start_rowid_base_monthly = "";
//  QString start_rowid_parent_2 = "";
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	// prev parent monthly
	q.exec("select rowid, _parent_prev_monthly from bardata where _parent_prev_monthly > 0 order by rowid desc limit 1");
	QUERY_ERROR_CODE (q, errcode, "");

	if (q.next() && q.value(0).toInt() > 0)
	{
		// @deprecated: original
//    start_rowid_base_monthly_prev = "rowid >=" + q.value(0).toString() + " and ";
//    start_rowid_parent_1 = "bardata.rowid >=" + q.value(1).toString() + " and ";

		start_rowid_base_monthly_prev = "rowid >=" + q.value(0).toString();
		start_rowid_parent_1 = "rowid >=" + q.value(1).toString();
	}

	// @deprecated: original
	{
	// parent monthly
//  q.exec ("select rowid, _parent_prev_monthly from bardata where _parent_monthly > 0 order by rowid desc limit 1");

//  if (q.next() && q.value(0).toInt() > 0)
//  {
//    start_rowid_base_monthly = "rowid >=" + q.value(0).toString() + " and ";
//    start_rowid_parent_2 = "bardata.rowid >=" + q.value(1).toString() + " and ";
//  }
	}

	// store monthly data into local variable
	q.exec("attach database '" + _dbmonthly_path + "' AS " + _dbmonthly);
	QUERY_ERROR_CODE (q, errcode, "");

	q.exec("select rowid, strftime('%Y%m',date_) from " + _dbmonthly_bardata + " where " + start_rowid_parent_1 + " order by date_");
	QUERY_ERROR_CODE (q, errcode, "");

	while (q.next())
	{
		row.rowid = q.value(0).toInt();
		row.date_ = q.value(1).toString();
		monthly_data.push_back(row);
	}

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set _parent_monthly=?, _parent_prev_monthly=? where rowid=?");
	q.exec("select rowid, strftime('%Y%m',date_) from bardata where " + start_rowid_base_monthly_prev);
	QUERY_ERROR_CODE (q, errcode, "");

	QString _date = "";
	int start_idx = 0;
	int monthly_data_length = static_cast<int>(monthly_data.size());

	while (q.next())
	{
		_date = q.value(1).toString();

		if (start_idx < monthly_data_length && _date >= monthly_data[start_idx].date_)
		{
			// try to align monthly data but dont do over last record
			while (start_idx < monthly_data_length - 1 && monthly_data[start_idx].date_ < _date)
			{
				++start_idx;
			}

			if (start_idx < monthly_data_length && monthly_data[start_idx].date_ == _date)
			{
				q_update.bindValue(0, monthly_data[start_idx].rowid);
				q_update.bindValue(1, start_idx - 1 > -1 ? monthly_data[start_idx - 1].rowid : QVariant());
			}
			else
			{
				q_update.bindValue(0, monthly_data[start_idx].rowid + 1);
				q_update.bindValue(1, monthly_data[start_idx].rowid);
			}

			q_update.bindValue(2, q.value(0));
			q_update.exec();

			QUERY_ERROR_CODE (q_update,errcode, "");
		}
	}

	// @deprecated: original
	{
//  q.exec ("update bardata"
//          " set _parent_prev_monthly = ifnull((select rowid from " + _dbmonthly_bardata + " a where " + start_rowid_parent_1 +
//          " (bardata.date_ > a.date_ or (bardata.date_ = a.date_ and bardata.time_ > a.time_)) order by rowid desc limit 1),0)" +
//          " where " + start_rowid_base_monthly_prev +
//          " exists (select 1 from " + _dbmonthly_bardata + " a where " + start_rowid_parent_1 +
//          " (bardata.date_ > a.date_ or (bardata.date_ = a.date_ and bardata.time_ > a.time_)) limit 1)");

//  QUERY_LAST_ERROR (q,"update parent monthly prev");

//  q.exec ("update bardata"
//          " set _parent_monthly = (select rowid from " + _dbmonthly_bardata + " a where " + start_rowid_parent_2 +
//          " strftime('%m%Y',bardata.date_) = strftime('%m%Y',a.date_) limit 1)"
//          " where " + start_rowid_base_monthly +
//          " exists (select 1 from " + _dbmonthly_bardata + " a where " + start_rowid_parent_2 +
//          " strftime('%m%Y',bardata.date_)=strftime('%m%Y',a.date_) limit 1);");

//  QUERY_LAST_ERROR (q,"update parent monthly");
	}

	END_TRANSACTION (q_update);

	q.exec("detach database " + _dbmonthly);

	return errcode;
}

int BardataCalculator::update_sr_distpoint_distatr(const QSqlDatabase &db, int id_threshold, int start_rowid)
{
	QSqlQuery q(db);
	QSqlQuery q_update(db);
	QString _start_rowid = QString::number(start_rowid);
	QString _ID = QString::number(id_threshold);
	QString lastdate_res = "";
	QString lastdate_sup = "";

	double last_resistance = 0;
	double last_support = 0;
	double dist_point = 0;
	double dist_atr = 0;
	double resistance_price = 0;
	double support_price = 0;
	int errcode = 0;

	q.setForwardOnly(true);

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update " + SQLiteHandler::TABLE_NAME_RESISTANCE + " set " +
										SQLiteHandler::COLUMN_NAME_DIST_POINT + "=?, " +
										SQLiteHandler::COLUMN_NAME_DIST_ATR + "=?" +
									 " where rowid=?");

	q.exec("select a.rowid, a.resistance, b.atr, a.date_"
				 " from resistancedata a join bardata b on a.date_=b.date_"
				 " where b.rowid >" + _start_rowid +
				 " and b.completed in(1,2) and b.atr > 0 and a.id_threshold=" + _ID +
				 " order by a.date_, resistance desc");

	QUERY_ERROR_CODE (q, errcode, "sr distpoint/distatr (res)");

	while (q.next())
	{
		if (lastdate_res != q.value(3).toString()) {
			lastdate_res = q.value(3).toString();
			dist_point = 1000;
			last_resistance = q.value(1).toDouble();
		}
		else {
			resistance_price = q.value(1).toDouble();
			dist_point = last_resistance - resistance_price;
			last_resistance = resistance_price;
		}

		if (q.value(2).toDouble() != 0) {
			dist_atr = dist_point / q.value(2).toDouble();
		}
		else {
			dist_atr = dist_point;
#ifdef DEBUG_MODE
			qDebug("atr is zero in update_sr_distpoint_distatr (resistance)");
#endif
		}

		q_update.bindValue(0, QString::number(dist_point,'f',4));
		q_update.bindValue(1, QString::number(dist_atr,'f',4));
		q_update.bindValue(2, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "");
		if (errcode) break;
	}

	q_update.prepare("update " + SQLiteHandler::TABLE_NAME_SUPPORT + " set " +
										SQLiteHandler::COLUMN_NAME_DIST_POINT + "=?, " +
										SQLiteHandler::COLUMN_NAME_DIST_ATR + "=? where " +
										SQLiteHandler::COLUMN_NAME_ROWID + "=?");

	q.exec("select a.rowid, a.support, b.atr, a.date_"
				 " from supportdata a join bardata b on a.date_=b.date_"
				 " where b.rowid >" + _start_rowid +
				 " and b.completed in(1,2) and b.atr > 0 and a.id_threshold=" + _ID +
				 " order by a.date_, support asc");

	QUERY_ERROR_CODE (q, errcode, "sr distpoint/distatr (sup)");

	while (q.next())
	{
		if (lastdate_sup != q.value(3).toString())
		{
			lastdate_sup = q.value(3).toString();
			dist_point = 1000;
			last_support = q.value(1).toDouble();
		}
		else
		{
			support_price = q.value(1).toDouble();
			dist_point = support_price - last_support;
			last_support = support_price;
		}

		if (q.value(2).toDouble() != 0)
		{
			dist_atr = dist_point / q.value(2).toDouble();
		}
		else
		{
			dist_atr = dist_point;
#ifdef DEBUG_MODE
			qDebug("atr is zero in update_sr_distpoint_distatr (support)");
#endif
		}

		q_update.bindValue(0, QString::number(dist_point,'f',4));
		q_update.bindValue(1, QString::number(dist_atr,'f',4));
		q_update.bindValue(2, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "update sr distpoint distatr err");
		if (errcode) break;
	}

	END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_dist_resistance_approx_v3(const QSqlDatabase &db, const QString &database_path,
																												const QString &symbol, const IntervalWeight base_interval,
																												int id_threshold, int start_rowid, Logger _logger)
{
	const QString _start_rowid = QString::number(start_rowid);
	const QString _id_threshold = QString::number(id_threshold);
	const QString column_id = "_" + QString::number(id_threshold + 1); // non-zero based
	const QString c_res = SQLiteHandler::COLUMN_NAME_RES + column_id;
	const QString c_last_reactdate = SQLiteHandler::COLUMN_NAME_RES_LASTREACTDATE + column_id;
//  const QString c_distres = SQLiteHandler::COLUMN_NAME_DISTRES + column_id;
//  const QString c_distres_atr = SQLiteHandler::COLUMN_NAME_DISTRES_ATR + column_id;
//  const QString c_distres_rank = SQLiteHandler::COLUMN_NAME_DISTRES_RANK + column_id;

	QSqlQuery q1(db);
	QSqlQuery q2(db);
	QSqlQuery q_update(db);
	std::vector<t_price> vec_data;
	std::list<QString> detach_stmt;
	QStringList reactdate_list;
	QString select_bardata = "";
	QString _date, _last_reactdate;
//  double _atr = 0.0;
	double _close = 0.0;
	double _resistance = 0.0;
	int _index = 0;
	int errcode = 0;

	q1.setForwardOnly(true);
	q1.exec("PRAGMA temp_store = MEMORY;");
	q1.exec("PRAGMA cache_size = 10000;");

	q2.setForwardOnly(true);
	q2.exec("PRAGMA temp_store = MEMORY;");
	q2.exec("PRAGMA cache_size = 10000;");

	if (base_interval < WEIGHT_DAILY)
	{
		QString _dbdaily = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_DAILY);
		QString _db60min = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_60MIN);
//    QString _db5min = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_5MIN);
		QString join_clause = "";

		q1.exec("attach database '" + _dbdaily + "' as dbdaily");

		if (base_interval == WEIGHT_5MIN)
		{
			q1.exec("attach database '" + _db60min + "' as db60min");
			detach_stmt.emplace_back("detach database db60min");

			// for case when incomplete bar created before update
			join_clause +=
				" join db60min.bardata x on a._parent=x.rowid"
				" join dbdaily.bardata y on x._parent_prev=y.rowid";

			// TODO: optimize & recheck this join
			// reference -- for case incomplete is created after update
//      join_clause +=
//        " join db60min.bardata x on (case when a.time_<>x.time_ then a._parent_prev else a._parent end)=x.rowid"
//        " join dbdaily.bardata y on (case when y.time_<>x.time_ then x._parent_prev else x._parent end)=y.rowid";

			// experiment 1
//      join_clause += " join db60min.bardata x on (case when a._parent=0 then a._parent_prev else a._parent end)=x.rowid";
//      join_clause += " join dbdaily.bardata y on x._parent_prev=y.rowid";

			// experiment 2
//      join_clause += " join db60min.bardata x on a._parent_prev=x.rowid";
//      join_clause += " join dbdaily.bardata y on x._parent=y.rowid";
		}
		else if (base_interval == WEIGHT_60MIN)
		{
			join_clause += " join dbdaily.bardata y on a._parent_prev=y.rowid";
		}
		/*else if (base_interval == WEIGHT_1MIN)
		{
			q1.exec("attach database '" + _db60min + "' as db60min");
			q1.exec("attach database '" + _db5min + "' as db5min");
			detach_stmt.emplace_back("detach database db60min");
			detach_stmt.emplace_back("detach database db5min");

			join_clause +=
				" join db5min.bardata z on a._parent=z.rowid"
				" join db60min.bardata x on z._parent=x.rowid"
				" join dbdaily.bardata y on x._parent_prev=y.rowid";
		}*/


#if DISABLE_INCOMPLETE_SR
		// -- backup
		q1.exec("select y.date_ from bardata a" + join_clause +
						" where a.rowid >" + _start_rowid + " and a.completed in(1,2)"
						" order by a.date_ asc limit 1");

		select_bardata =
			"select a.rowid, y.date_, a.close_, y.atr from bardata a " + join_clause +
			" where a.rowid >" + _start_rowid + " and a.completed in(1,2)";
#else
		// -- backup
		q1.exec("select y.date_ from bardata a" + join_clause + " where a.rowid >" + _start_rowid + " order by a.date_ asc limit 1");

		// -- new
//    q1.exec("select date_ from bardata  where rowid >" + _start_rowid + " order by date_ asc limit 1");

		select_bardata =
			"select a.rowid, y.date_, a.close_, y.atr from bardata a " + join_clause +
			" where a.rowid >" + _start_rowid;
#endif
	}
	else
	{
#if DISABLE_INCOMPLETE_SR
		q1.exec("select date_ from bardata where rowid >" + _start_rowid +
						" and completed in(1,2)"
						" order by date_ asc limit 1");

		select_bardata =
			"select rowid, date_, close_, atr from bardata"
			" where rowid >" + _start_rowid + " and completed in(1,2)";
#else
		q1.exec("select date_ from bardata where rowid >" + _start_rowid + " order by date_ asc limit 1");
		select_bardata =
			"select rowid, date_, close_, atr from bardata where rowid >" + _start_rowid;
#endif
	}

	QUERY_ERROR_CODE (q1, errcode, "res prebatch 1");

	// batch 1: find local lowest resistance line foreach day
	if (q1.next())
	{
		if (base_interval < WEIGHT_DAILY)
		{
			q2.exec("select a.date_, a.resistance, b.react_date"
							" from dbdaily.resistancedata a"
							" join dbdaily.resistancereactdate b on a.rowid = b.rid"
							" where resistance_count >= 3 and id_threshold=" + _id_threshold +
							" and a.date_ >='" + q1.value(0).toString() + "'"
							" order by a.date_ asc, a.resistance asc, b.react_date desc");
		}
		else
		{
			q2.exec("select a.date_, a.resistance, b.react_date"
							" from resistancedata a"
							" join resistancereactdate b on a.rowid = b.rid"
							" where resistance_count >= 3 and id_threshold=" + _id_threshold +
							" and a.date_ >='" + q1.value(0).toString() + "'"
							" order by a.date_ asc, a.resistance asc, b.react_date desc");
		}

		QUERY_ERROR_CODE (q2, errcode, "res batch 1");

		t_price data;
		while (q2.next())
		{
			data.date_ = q2.value(0).toString();
			data.price_level = q2.value(1).toDouble();
			data.react_date = q2.value(2).toString();
			vec_data.emplace_back(data);
		}
	}

	// batch 2: update displacement resistance foreach day
	q1.exec(select_bardata);

//  qDebug() << select_bardata;

	QUERY_ERROR_CODE (q1, errcode, "res batch 2");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set " + c_res + "=?," + c_last_reactdate +"=? where rowid=?");

	// @deprecated
//  q_update.prepare("update bardata set " +
//                     c_res + "=?," +
//                     c_distres + "=?," +
//                     c_distres_atr + "=?," +
//                     c_last_reactdate +"=?"
//                   " where rowid=?");

	const int vecdata_length = static_cast<int>(vec_data.size());

	while (q1.next())
	{
		_date = q1.value(1).toString();
		_close = q1.value(2).toDouble();
//    _atr = q1.value(3).toDouble();
		_last_reactdate = "";

		for (int i = _index; i < vecdata_length; ++i)
		{
			if (vec_data[i].date_ == _date)
			{
				// find first lowest line
				_resistance = vec_data[i].price_level;
				_last_reactdate = vec_data[i].react_date;

				reactdate_list.clear();
				reactdate_list.push_back(vec_data[i].react_date);

				while (i < vecdata_length && vec_data[i].price_level == _resistance && vec_data[i].date_ == _date)
				{
					reactdate_list.push_back(vec_data[i].react_date);
					++i;
				}

				// realign index's cursor
				--i;

				// remaining lines
				while (i > -1 && i < vecdata_length && vec_data[i].date_ == _date)
				{
					// find higher resistance line that can subsume lowest line
					if (vec_data[i].price_level > _resistance && reactdate_list.contains(vec_data[i].react_date))
					{
						_resistance = vec_data[i].price_level;
						_last_reactdate = vec_data[i].react_date;
					}
					++i;
				}

				break;
			}
			else if (vec_data[i].date_ > _date)
			{
				_index = i;
				break;
			}
		}

		if (_last_reactdate != "")
		{
			q_update.bindValue(0, _resistance);
			q_update.bindValue(1, _last_reactdate);
			q_update.bindValue(2, q1.value(0).toInt());

			// @deprecated columns
//      if (_atr == 0) {
//        LOG_DEBUG ("dist resistance: ATR is zero!");
//        _atr = 1;
//      }
//      q_update.bindValue(1, QString::number(_resistance-_close,'f',4));
//      q_update.bindValue(2, QString::number((_resistance-_close)/_atr, 'f', 6));
//      q_update.bindValue(3, _last_reactdate);
//      q_update.bindValue(4, q1.value(0).toInt());

			q_update.exec();

			QUERY_ERROR_CODE (q_update, errcode, "update distres approx v3");
			if (errcode) break;
		}
	}

	END_TRANSACTION (q_update);

	for (auto it = detach_stmt.begin(); it != detach_stmt.end(); ++it)
	{
		q1.exec(*it);
	}

	// @deprecated by searchlines
	// update distance resistance rank (positive value only)
//  q1.exec("select rowid," + c_distres_atr + " from bardata where " + c_distres_atr + "> 0 and rowid >" + _start_rowid);
//  QUERY_ERROR_CODE (q1, errcode, "dist resistance approx(rank)");

//  BEGIN_TRANSACTION (q_update);
//  q_update.prepare("update bardata set " + c_distres_rank + "=? where rowid=?");

//  while (q1.next()) {
//    q2.exec("select " + c_distres_rank + " from bardata"
//            " where " + c_distres_atr + "> 0 and " + c_distres_atr + "<=" + q1.value(1).toString() + " and rowid <=" + _start_rowid +
//            " order by " + c_distres_atr + " desc limit 1");

//    QUERY_ERROR_CODE (q2, errcode, "update distresistance approx v3(select rank)");
//    if (errcode) break;

//    if (q2.next()) {
//      q_update.bindValue(0, QString::number(q2.value(0).toDouble(),'f',4));
//      q_update.bindValue(1, q1.value(0));
//      q_update.exec();

//      QUERY_ERROR_CODE (q_update, errcode, "update distresistance approx v3(rank)");
//      if (errcode) break;
//    }
//  }

//  END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_dist_support_approx_v3(const QSqlDatabase &database, const QString &database_path,
																										 const QString &symbol, const IntervalWeight base_interval,
																										 int id_threshold, int start_rowid, Logger _logger)
{
	const QString _start_rowid = QString::number(start_rowid);
	const QString _id_threshold = QString::number(id_threshold);
	const QString column_id = "_" + QString::number(id_threshold + 1); // non-zero based
	const QString c_sup = SQLiteHandler::COLUMN_NAME_SUP + column_id;
	const QString c_last_reactdate = SQLiteHandler::COLUMN_NAME_SUP_LASTREACTDATE + column_id;
//  const QString c_distsup = SQLiteHandler::COLUMN_NAME_DISTSUP + column_id;
//  const QString c_distsup_atr = SQLiteHandler::COLUMN_NAME_DISTSUP_ATR + column_id;
//  const QString c_distsup_rank = SQLiteHandler::COLUMN_NAME_DISTSUP_RANK + column_id;

	QSqlQuery q1(database);
	QSqlQuery q2(database);
	QSqlQuery q_update(database);
	std::vector<t_price> vec_data;
	std::list<QString> detach_stmt;
	QStringList reactdate_list;
	QString select_bardata = "";
	QString _date, _last_reactdate;
//  double _atr = 0;
	double _close = 0;
	double _support = 0;
	int _index = 0;
	int errcode = 0;

	q1.setForwardOnly(true);
	q1.exec("PRAGMA temp_store = MEMORY;");
	q1.exec("PRAGMA cache_size = 10000;");

	q2.setForwardOnly(true);
	q2.exec("PRAGMA temp_store = MEMORY;");
	q2.exec("PRAGMA cache_size = 10000;");

	if (base_interval < WEIGHT_DAILY)
	{
		QString _dbdaily = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_DAILY);
		QString _db60min = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_60MIN);
//    QString _db5min = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_5MIN);
		QString join_clause = "";

		q1.exec("attach database '" + _dbdaily + "' as dbdaily");

		if (base_interval == WEIGHT_5MIN)
		{
			q1.exec("attach database '" + _db60min + "' as db60min");
			detach_stmt.emplace_back("detach database db60min");

			join_clause +=
				" join db60min.bardata x on a._parent=x.rowid"
				" join dbdaily.bardata y on x._parent_prev=y.rowid";

			// TODO: optimize & recheck this join
			// reference
//      join_clause +=
//          " join db60min.bardata x on (case when a.time_<>x.time_ then a._parent_prev else a._parent end)=x.rowid"
//          " join dbdaily.bardata y on (case when y.time_<>x.time_ then x._parent_prev else x._parent end)=y.rowid";

			// experiment
//      join_clause += " join db60min.bardata x on (case when a._parent=0 then a._parent_prev else a._parent end)=x.rowid";
//      join_clause += " join dbdaily.bardata y on x._parent_prev=y.rowid";
		}
		else if (base_interval == WEIGHT_60MIN)
		{
			join_clause += " join dbdaily.bardata y on a._parent_prev=y.rowid";
		}
		/*else if (base_interval == WEIGHT_1MIN)
		{
			q1.exec("attach database '" + _db60min + "' as db60min");
			q1.exec("attach database '" + _db5min + "' as db5min");
			detach_stmt.emplace_back("detach database db60min");
			detach_stmt.emplace_back("detach database db5min");

			join_clause +=
				" join db5min.bardata z on a._parent=z.rowid"
				" join db60min.bardata x on z._parent=x.rowid"
				" join dbdaily.bardata y on x._parent_prev=y.rowid";
		}*/

#if DISABLE_INCOMPLETE_SR
		// -- backup
		q1.exec("select y.date_ from bardata a" + join_clause +
						" where a.rowid >" + _start_rowid + " and a.completed in(1,2)"
						" order by a.date_ asc limit 1");

		select_bardata =
			"select a.rowid, y.date_, a.close_ from bardata a" + join_clause +
			" where a.rowid >" + _start_rowid + " and a.completed in(1,2)";
#else
		// -- backup
		q1.exec("select y.date_ from bardata a" + join_clause + " where a.rowid >" + _start_rowid + " order by a.date_ asc limit 1");

		// -- new
//    q1.exec("select date_ from bardata where rowid >" + _start_rowid + " order by date_ asc limit 1");

		// @deprecated (2016-02-02)
//    select_bardata =
//      "select a.rowid, y.date_, a.close_, y.atr from bardata a" + join_clause +
//      " where a.rowid >" + _start_rowid;

		select_bardata =
			"select a.rowid, y.date_, a.close_ from bardata a" + join_clause +
			" where a.rowid >" + _start_rowid;
#endif
	}
	else
	{
#if DISABLE_INCOMPLETE_SR
		q1.exec("select date_ from bardata where rowid >" + _start_rowid + " and completed in(1,2)"
						" order by date_ asc limit 1");

		select_bardata = "select rowid, date_, close_ from bardata"
										 " where rowid >" + _start_rowid + " and completed in(1,2)";
#else
		q1.exec("select date_ from bardata where rowid >" + _start_rowid + " order by date_ asc limit 1");

		// @deprecated (2016-02-02)
//    select_bardata = "select rowid, date_, close_, atr from bardata where rowid >" + _start_rowid;

		select_bardata = "select rowid, date_, close_ from bardata where rowid >" + _start_rowid;
#endif
	}

	QUERY_ERROR_CODE (q1, errcode, "");

	// batch 1: find local highest support line foreach day
	if (q1.next())
	{
		if (base_interval < WEIGHT_DAILY)
		{
			q2.exec("select a.date_, a.support, b.react_date"
							" from dbdaily.supportdata a"
							" join dbdaily.supportreactdate b on a.rowid = b.rid"
							" where support_count >= 3 and id_threshold=" + _id_threshold +
							" and a.date_ >='" + q1.value(0).toString() + "'"
							" order by a.date_ asc, a.support desc, b.react_date desc");
		}
		else
		{
			q2.exec("select a.date_, a.support, b.react_date"
							" from supportdata a"
							" join supportreactdate b on a.rowid = b.rid"
							" where support_count >= 3 and id_threshold=" + _id_threshold +
							" and a.date_ >='" + q1.value(0).toString() + "'"
							" order by a.date_ asc, a.support desc, b.react_date desc");
		}

		QUERY_ERROR_CODE (q2, errcode, "sup batch 1");

		t_price data;
		while (q2.next())
		{
			data.date_ = q2.value(0).toString();
			data.price_level = q2.value(1).toDouble();
			data.react_date = q2.value(2).toString();
			vec_data.emplace_back(data);
		}
	}

	// batch 2: update displacement support foreach day
	q1.exec (select_bardata);
	QUERY_ERROR_CODE (q1, errcode, "sup batch 2");

	BEGIN_TRANSACTION (q_update);
//  q_update.prepare("update bardata set " +
//                     c_sup + "=?," +
//                     c_distsup + "=?," +
//                     c_distsup_atr + "=?," +
//                     c_last_reactdate + "=?" +
//                    " where rowid=?");

	q_update.prepare("update bardata set " + c_sup + "=?," + c_last_reactdate + "=? where rowid=?");

	const int vecdata_length = static_cast<int>(vec_data.size());

	while (q1.next())
	{
		_date = q1.value(1).toString();
		_close = q1.value(2).toDouble();
//    _atr = q1.value(3).toDouble();
		_last_reactdate = "";

		for (int i = _index; i < vecdata_length; ++i)
		{
			if (vec_data[i].date_ == _date)
			{
				// find first highest line
				_support = vec_data[i].price_level;
				_last_reactdate = vec_data[i].react_date;

				reactdate_list.clear();
				reactdate_list.push_back(vec_data[i].react_date);

				while (i < vecdata_length && vec_data[i].date_ == _date && vec_data[i].price_level == _support)
				{
					reactdate_list.push_back(vec_data[i].react_date);
					++i;
				}

				// realign index's cursor
				--i;

				// remaining lines
				while (i > -1 && i < vecdata_length && vec_data[i].date_ == _date)
				{
					// find lower support line that can subsume highest line
					if (vec_data[i].price_level < _support && reactdate_list.contains(vec_data[i].react_date))
					{
						_support = vec_data[i].price_level;
						_last_reactdate = vec_data[i].react_date;
					}
					++i;
				}

				break;
			}
			else if (vec_data[i].date_ > _date)
			{
				_index = i;
				break;
			}
		}

		if (!_last_reactdate.isEmpty())
		{
			q_update.bindValue(0, _support);
			q_update.bindValue(1, _last_reactdate);
			q_update.bindValue(2, q1.value(0).toInt());

			// @deprecated
//      if (_atr == 0) {
//        LOG_DEBUG ("dist support: ATR is zero!");
//        _atr = 1;
//      }
//      q_update.bindValue(1, QString::number(_close-_support,'f',4));
//      q_update.bindValue(2, QString::number((_close-_support)/_atr, 'f', 6)); // tobe deprecated
//      q_update.bindValue(3, _last_reactdate);
//      q_update.bindValue(4, q1.value(0).toInt());

			q_update.exec();

			QUERY_ERROR_CODE (q_update, errcode, "update distsupportapproxv3");
			if (errcode) break;
		}
	}

	END_TRANSACTION (q_update);

	for (auto it = detach_stmt.begin(); it != detach_stmt.end(); ++it)
	{
		q1.exec(*it);
	}

	// @deprecated by searchlines
	// update distance support rank (positive value only)
//  q1.exec("select rowid," + c_distsup_atr + " from bardata where " + c_distsup_atr + "> 0 and rowid >" + _start_rowid);
//  QUERY_ERROR_CODE (q1, errcode, "dist support approx (rank)");

//  BEGIN_TRANSACTION (q_update);
//  q_update.prepare("update bardata set " + c_distsup_rank + "=? where rowid=?");

//  while (q1.next()) {
//    q2.exec("select " + c_distsup_rank + " from bardata"
//            " where " + c_distsup_atr + "> 0 and " + c_distsup_atr + "<=" + q1.value(1).toString() + " and rowid <=" + _start_rowid +
//            " order by " + c_distsup_atr + " desc limit 1");

//    QUERY_ERROR_CODE (q2, errcode, "update distsupport approx v3 (select rank)");
//    if (errcode) break;

//    q_update.bindValue(0, QString::number (q2.next() ? q2.value(0).toDouble() : 0,'f',4));
//    q_update.bindValue(1, q1.value(0));
//    q_update.exec();

//    QUERY_ERROR_CODE (q_update, errcode, "update distsupport approx v3 (rank)");
//    if (errcode) break;
//  }

//  END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_resistance_velocity(const QSqlDatabase &database, const IntervalWeight base_interval,
																									int id_threshold, double reactThreshold, int start_rowid)
{
	const QString _rowid = QString::number(start_rowid);
	const QString _ID = "_" + QString::number(id_threshold + 1);
	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QString select_velocity = "";
	int errcode = 0;

	// the result is not accurate in some scenario, caused by
	// join b.date_=a.resLastReactDate is not really correct
	// they are missing time slot 19:00 - 23:55 of previous day

	// intraday
	if (base_interval < WEIGHT_DAILY)
	{
//    select_velocity =
//      "select a.rowid, count(1)"
//      " from bardata a join bardata b on ("
//      "(b.date_ >= a.reslastreactdate" + mID + ") and "
//      "(b.date_ < a.date_ or (b.date_=a.date_ and b.time_ <= a.time_)))"
//      "group by a.rowid";

		// backup
//    select_velocity =
//      "select a.rowid, a.rowid - min(ifnull(b.rowid,0)) + 1"
//      " from bardata a join bardata b on"
//      " b.date_ = a.reslastreactdate" + _ID + " and"
//      " b.high_ >= a.res" + _ID + "-" + QString::number(reactThreshold) +
//      " where a.rowid >" + _rowid + " and a.reslastreactdate" + _ID + " is not null"
//      " group by a.rowid";

		select_velocity =
			"select a.rowid, a.rowid - min(ifnull(b.rowid,0)) + 1"
			" from bardata a join bardata b on b.date_ = a.reslastreactdate" + _ID +
			" where a.rowid >" + _rowid +
			" and a.reslastreactdate" + _ID + " is not null"
			" and b.high_ >= a.res" + _ID + "-" + QString::number(reactThreshold) +
			" group by a.rowid";
	}
	// non intraday
	else
	{
		select_velocity =
			"select a.rowid, count(1)"
			" from bardata a join bardata b on b.date_ >= a.reslastreactdate" + _ID + " and b.date_ <= a.date_"
			" where a.rowid >" + _rowid +
			" group by a.rowid";
	}

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set " + SQLiteHandler::COLUMN_NAME_DOWNVEL_DISTBAR + _ID + "=? where rowid=?");

	q.setForwardOnly(true);
	q.exec(select_velocity);
	QUERY_ERROR_CODE (q, errcode, "resistance velocity");

	while (q.next())
	{
		q_update.bindValue(0, q.value(1));
		q_update.bindValue(1, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "update resistance velocity");
		if (errcode) break;
	}

	END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_support_velocity(const QSqlDatabase &database, const IntervalWeight base_interval,
																							 int id_threshold, double reactThreshold, int start_rowid)
{
	const QString _rowid = QString::number(start_rowid);
	const QString _ID = "_" + QString::number(id_threshold + 1);
	QSqlQuery q(database);
	QSqlQuery q_update(database);
	int errcode = 0;

	q.setForwardOnly(true);

	// intraday
	if (base_interval < WEIGHT_DAILY)
	{
//        select_velocity =
//          "select a.rowid, count(1)"
//          " from bardata a join bardata b on ("
//          "(b.date_ >= a.SupLastReactDate" + mID + ") and"
//          "(b.date_ < a.date_ or (b.date_=a.date_ and b.time_ <= a.time_)))"
//          "group by a.rowid";

		q.exec("select a.rowid, a.rowid - min(ifnull(b.rowid,0)) + 1"
					 " from bardata a join bardata b on"
					 " b.date_ = a.suplastreactdate" + _ID + " and"
					 " b.low_ <= a.sup" + _ID + "+" + QString::number(reactThreshold) +
					 " where a.rowid >" + _rowid + " and a.suplastreactdate" + _ID + " is not null"
					 " group by a.rowid");
	}
	// non intraday
	else
	{
		q.exec("select a.rowid, count(1) from bardata a"
					 " join bardata b on b.date_ >= a.suplastreactdate" + _ID + " and b.date_ <= a.date_"
					 " where a.rowid >" + _rowid +
					 " group by a.rowid");
	}

	QUERY_ERROR_CODE (q, errcode, "");

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set " + SQLiteHandler::COLUMN_NAME_UPVEL_DISTBAR + _ID + "=? where rowid=?");

	while (q.next())
	{
		q_update.bindValue(0, q.value(1));
		q_update.bindValue(1, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "update support velocity");
		if (errcode) break;
	}

	END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_sr_line_count(const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																						const IntervalWeight base_interval, int id_threshold, int start_rowid)
{
	const QString _column_id = "_" + QString::number(id_threshold + 1);
	const QString _id_threshold = QString::number(id_threshold);
	const QString _start_rowid = QString::number(start_rowid);
	const QString dbmonthly = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_MONTHLY);
	const QString dbweekly = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_WEEKLY);
	const QString dbdaily = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_DAILY);
	const QString db60min = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_60MIN);
	const QString db5min = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, WEIGHT_5MIN);
	QSqlQuery q_update(database);
	QSqlQuery q(database);
	QString RLine, SLine;
	IntervalWeight _interval = base_interval;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("attach database '" + dbmonthly + "' as dbMonthly");
	QUERY_ERROR_CODE (q, errcode, "");
	if (errcode) return errcode;

	q.exec("attach database '" + dbweekly + "' as dbWeekly");
	QUERY_ERROR_CODE (q, errcode, "");
	if (errcode) return errcode;

	q.exec("attach database '" + dbdaily + "' as dbDaily");
	QUERY_ERROR_CODE (q, errcode, "");
	if (errcode) return errcode;

	q.exec("attach database '" + db60min + "' as db60min");
	QUERY_ERROR_CODE (q, errcode, "");
	if (errcode) return errcode;

	q.exec("attach database '" + db5min + "' as db5min");
	QUERY_ERROR_CODE (q, errcode, "");
	if (errcode) return errcode;

	while (_interval != WEIGHT_INVALID)
	{
		if (_interval == WEIGHT_MONTHLY)
		{
			RLine = SQLiteHandler::COLUMN_NAME_MONTHLY_RLINE + _column_id;
			SLine = SQLiteHandler::COLUMN_NAME_MONTHLY_SLINE + _column_id;
		}
		else if (_interval == WEIGHT_WEEKLY)
		{
			RLine = SQLiteHandler::COLUMN_NAME_WEEKLY_RLINE + _column_id;
			SLine = SQLiteHandler::COLUMN_NAME_WEEKLY_SLINE + _column_id;
		}
		else
		{
			RLine = SQLiteHandler::COLUMN_NAME_DAILY_RLINE + _column_id;
			SLine = SQLiteHandler::COLUMN_NAME_DAILY_SLINE + _column_id;
		}

		QString select_res = "";
		QString select_sup = "";

		// handle join
		if (base_interval < _interval)
		{
			QString alias = "a";
			QString parent_database = "db" + Globals::intervalWeightToString (_interval) + ".";
			QString parent_alias = "";

			if (_interval == WEIGHT_MONTHLY) parent_alias = "m.";
			else if (_interval == WEIGHT_WEEKLY) parent_alias = "w.";
			else parent_alias = "d.";

			select_res += "select a.rowid, count(1) from bardata a";
			select_sup += "select a.rowid, count(1) from bardata a";

			if (base_interval < WEIGHT_5MIN)
			{
				if (base_interval < WEIGHT_1MIN) alias = "m1";
				select_res += " left join db5min.bardata m5 ON " + alias + "._parent_prev=m5.rowid";
				select_sup += " left join db5min.bardata m5 ON " + alias + "._parent_prev=m5.rowid";
			}

			if (base_interval < WEIGHT_60MIN)
			{
				if (base_interval < WEIGHT_5MIN) alias = "m5";
				select_res += " left join db60min.bardata m60 ON " + alias + "._parent_prev=m60.rowid";
				select_sup += " left join db60min.bardata m60 ON " + alias + "._parent_prev=m60.rowid";
			}

			if (base_interval < WEIGHT_DAILY)
			{
				if (base_interval < WEIGHT_60MIN) alias = "m60";
				select_res += " left join dbDaily.bardata d ON " + alias + "._parent_prev=d.rowid";
				select_sup += " left join dbDaily.bardata d ON " + alias + "._parent_prev=d.rowid";
			}

			if (base_interval < WEIGHT_WEEKLY)
			{
				if (base_interval < WEIGHT_DAILY) alias = "d";
				select_res += " left join dbWeekly.bardata w ON " + alias + "._parent_prev=w.rowid";
				select_sup += " left join dbWeekly.bardata w ON " + alias + "._parent_prev=w.rowid";
			}

			if (base_interval < WEIGHT_MONTHLY)
			{
				select_res += " left join dbMonthly.bardata m ON a._parent_prev_monthly=m.rowid";
				select_sup += " left join dbMonthly.bardata m ON a._parent_prev_monthly=m.rowid";
			}

			select_res +=
				" join " + parent_database + "resistancedata b on " + parent_alias + "date_=b.date_ and " + parent_alias + "time_=b.time_"
				" where a.rowid >" + _start_rowid +
				" and a.completed in(1,2)" +
				" and b.id_threshold=" + _id_threshold +
				" and b.resistance >= a.low_ and b.resistance <= a.high_"
				" group by a.rowid";

			select_sup +=
				" join " + parent_database + "supportdata b on " + parent_alias + "date_=b.date_ and " + parent_alias + "time_=b.time_"
				" where a.rowid >" + _start_rowid +
				" and a.completed in(1,2)"
				" and b.id_threshold=" + _id_threshold +
				" and b.support >= a.low_ and b.support <= a.high_"
				" group by a.rowid";
		}
		else
		{
			select_res = "select a.rowid, count(1)"
									 " from bardata a join resistancedata b on a.prevdate=b.date_ and a.prevtime=b.time_"
									 " where a.rowid >" + _start_rowid +
									 " and a.completed in(1,2)"
									 " and b.id_threshold=" + _id_threshold +
									 " and b.resistance >= a.low_ and b.resistance <= a.high_"
									 " group by a.rowid";

			select_sup = "select a.rowid, count(1)"
									 " from bardata a join supportdata b on a.prevdate=b.date_ and a.prevtime=b.time_"
									 " where a.rowid >" + _start_rowid +
									 " and a.completed in(1,2)"
									 " and b.id_threshold=" + _id_threshold +
									 " and b.support >= a.low_ and b.support <= a.high_"
									 " group by a.rowid";
		}

		BEGIN_TRANSACTION (q_update);
		q_update.prepare("update bardata set " + RLine + "=? where rowid=?");
		q.exec(select_res);
		QUERY_ERROR_CODE (q, errcode, "");

		while (q.next())
		{
			q_update.bindValue(0, q.value(1).toString());
			q_update.bindValue(1, q.value(0).toString());
			q_update.exec();

			QUERY_ERROR_CODE (q_update, errcode, "");
			if (errcode) break;
		}

		q_update.prepare("update bardata set " + SLine + "=? where rowid=?");
		q.exec(select_sup);
		QUERY_ERROR_CODE (q, errcode, "");

		while (q.next())
		{
			q_update.bindValue(0, q.value(1).toString());
			q_update.bindValue(1, q.value(0).toString());
			q_update.exec();

			QUERY_ERROR_CODE (q_update, errcode, "");
			if (errcode) break;
		}

		END_TRANSACTION (q_update);

		_interval = Globals::getParentInterval(_interval);
	}

	q.exec("detach database dbMonthly");
	q.exec("detach database dbWeekly");
	q.exec("detach database dbDaily");
	q.exec("detach database db60min");
	q.exec("detach database db5min");

	return errcode;
}

int BardataCalculator::update_zone(const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																	 const IntervalWeight base_interval, int start_rowid)
{
	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QStringList database_alias;
	QStringList table_alias;
	QStringList projection;
	QString sql_select = "";
	QString _rowid = QString::number(start_rowid);
	IntervalWeight w = Globals::getParentInterval(base_interval);
	double m_open, m_high, m_low, m_close, m_fast, m_slow;
	int index;
	int errcode = 0;
	bool result_code = true;

	if (w == WEIGHT_INVALID) return 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	// Init dummy field name { Monthly, Weekly, Daily, 60min, 5min }
	for (int i = 0; i < 5; ++i)
	{
		projection.push_back("0"); // Fast
		projection.push_back("0"); // Slow
	}

	while (w != WEIGHT_INVALID)
	{
		table_alias.push_back("t_" + Globals::intervalWeightToString(w));
		database_alias.push_back("db" + table_alias.last());

		q.exec("attach database '" + database_path + "/" + SQLiteHandler::getDatabaseName(symbol, w) + "' AS " + database_alias.last());

		switch (w)
		{
			case WEIGHT_MONTHLY: index = 0; break;
			case WEIGHT_WEEKLY: index = 2; break;
			case WEIGHT_DAILY: index = 4; break;
			case WEIGHT_60MIN: index = 6; break;
			case WEIGHT_5MIN: index = 8; break;
			default: index = -1;
		}

		projection[index] = table_alias.last() + ".fastavg";
		projection[index+1] = table_alias.last() + ".slowavg";

		w = Globals::getParentInterval(w);
	}

	sql_select +=
		"select A.rowid, A.open_, A.high_, A.low_, A.close_, A.fastavg, A.slowavg," + projection.join(",") +
		" from bardata A";

	if (!database_alias.isEmpty())
	{
		QString prev_table_alias = table_alias[0];

		sql_select +=
			" left join " + database_alias[0] + ".bardata " + table_alias[0] +
			" on A._parent=" + table_alias[0] + ".rowid";

		for (int i = 1; i < database_alias.size(); ++i)
		{
			if (i < table_alias.size())
			{
				sql_select +=
					" left join " + database_alias[i] + ".bardata " + table_alias[i] +
					" on " + prev_table_alias + "._parent=" + table_alias[i] + ".rowid";

				prev_table_alias = table_alias[i];
			}
		}
	}

	sql_select += " where A.rowid >" + _rowid;

	q.exec(sql_select);

	QUERY_ERROR_CODE (q, errcode, "update zone");

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set " +
										 SQLiteHandler::COLUMN_NAME_OPEN_ZONE +"=?," +
										 SQLiteHandler::COLUMN_NAME_OPEN_ZONE_MONTHLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_OPEN_ZONE_WEEKLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_OPEN_ZONE_DAILY + "=?," +
										 SQLiteHandler::COLUMN_NAME_OPEN_ZONE_60MIN + "=?," +
										 SQLiteHandler::COLUMN_NAME_HIGH_ZONE +"=?," +
										 SQLiteHandler::COLUMN_NAME_HIGH_ZONE_MONTHLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_HIGH_ZONE_WEEKLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_HIGH_ZONE_DAILY + "=?," +
										 SQLiteHandler::COLUMN_NAME_HIGH_ZONE_60MIN + "=?," +
										 SQLiteHandler::COLUMN_NAME_LOW_ZONE +"=?," +
										 SQLiteHandler::COLUMN_NAME_LOW_ZONE_MONTHLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_LOW_ZONE_WEEKLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_LOW_ZONE_DAILY + "=?," +
										 SQLiteHandler::COLUMN_NAME_LOW_ZONE_60MIN + "=?," +
										 SQLiteHandler::COLUMN_NAME_CLOSE_ZONE +"=?," +
										 SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_MONTHLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_WEEKLY + "=?," +
										 SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_DAILY + "=?," +
										 SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_60MIN + "=?" +
									 " where rowid=?");

	QUERY_ERROR_CODE (q_update, errcode, "update zone");

	while (q.next())
	{
		m_open  = q.value(1).toDouble();
		m_high  = q.value(2).toDouble();
		m_low   = q.value(3).toDouble();
		m_close = q.value(4).toDouble();
		m_fast  = q.value(5).toDouble();
		m_slow  = q.value(6).toDouble();

		q_update.bindValue(0, check_zone(m_open, m_fast, m_slow));
		q_update.bindValue(1, check_zone(m_open, q.value(7).toDouble(), q.value(8).toDouble()));
		q_update.bindValue(2, check_zone(m_open, q.value(9).toDouble(), q.value(10).toDouble()));
		q_update.bindValue(3, check_zone(m_open, q.value(11).toDouble(), q.value(12).toDouble()));
		q_update.bindValue(4, check_zone(m_open, q.value(13).toDouble(), q.value(14).toDouble()));
		q_update.bindValue(5, check_zone(m_high, m_fast, m_slow));
		q_update.bindValue(6, check_zone(m_high, q.value(7).toDouble(), q.value(8).toDouble()));
		q_update.bindValue(7, check_zone(m_high, q.value(9).toDouble(), q.value(10).toDouble()));
		q_update.bindValue(8, check_zone(m_high, q.value(11).toDouble(), q.value(12).toDouble()));
		q_update.bindValue(9, check_zone(m_high, q.value(13).toDouble(), q.value(14).toDouble()));
		q_update.bindValue(10, check_zone(m_low, m_fast, m_slow));
		q_update.bindValue(11, check_zone(m_low, q.value(7).toDouble(), q.value(8).toDouble()));
		q_update.bindValue(12, check_zone(m_low, q.value(9).toDouble(), q.value(10).toDouble()));
		q_update.bindValue(13, check_zone(m_low, q.value(11).toDouble(), q.value(12).toDouble()));
		q_update.bindValue(14, check_zone(m_low, q.value(13).toDouble(), q.value(14).toDouble()));
		q_update.bindValue(15, check_zone(m_close, m_fast, m_slow));
		q_update.bindValue(16, check_zone(m_close, q.value(7).toDouble(), q.value(8).toDouble()));
		q_update.bindValue(17, check_zone(m_close, q.value(9).toDouble(), q.value(10).toDouble()));
		q_update.bindValue(18, check_zone(m_close, q.value(11).toDouble(), q.value(12).toDouble()));
		q_update.bindValue(19, check_zone(m_close, q.value(13).toDouble(), q.value(14).toDouble()));
		q_update.bindValue(20, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "update zone");
		if (!result_code) break;
	}

	END_TRANSACTION (q_update);

	q.exec("detach database dbt_Monthly;");
	q.exec("detach database dbt_Weekly;");
	q.exec("detach database dbt_Daily;");
	q.exec("detach database dbt_60min;");

	return errcode;
}

int BardataCalculator::update_dayrange_rank_approx(const QSqlDatabase &database, int start_rowid)
{
	QSqlQuery q_update(database);
	QSqlQuery q_select(database);
	QSqlQuery q(database);
	const QString _rowid = QString::number(start_rowid);
	double H[3], L[3];
	int errcode = 0;

	q_select.setForwardOnly(true);
	q_select.exec("PRAGMA temp_store = MEMORY;");
	q_select.exec("PRAGMA cache_size = 10000;");

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

#if DISABLE_INCOMPLETE_HISTOGRAM
	q.exec("select rowid," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY +
				 " from bardata where rowid >" + _rowid +
				 " and completed in(1,2)");
#else
	q.exec("select rowid," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + "," +
					SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY +
				 " from bardata where rowid >" + _rowid);
#endif

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set " +
										SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY + "=?," +
										SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY + "=?," +
										SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY + "=?," +
										SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY + "=?," +
										SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY + "=?," +
										SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY + "=?" +
									 " where rowid=?");

	while (q.next())
	{
		H[0] = H[1] = H[2] = 0.0;
		L[0] = L[1] = L[2] = 0.0;

		if (!q.value(1).isNull())
		{
			q_select.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY + " from bardata"
										" where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + " is not null and " +
																SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + "<=" + q.value(1).toString() +
										" and rowid <" + _rowid +
										" order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "DayRangeRank H1");

			H[0] = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (!q.value(2).isNull())
		{
			q_select.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY + " from bardata"
										" where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + " is not null and " +
																SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + "<=" + q.value(2).toString() +
										" and rowid <" + _rowid +
										" order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "DayRangeRank H2");

			H[1] = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (!q.value(3).isNull())
		{
			q_select.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY + " from bardata"
										" where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + " is not null and " +
																SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + "<=" + q.value(3).toString() +
										" and rowid <" + _rowid +
										" order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "DayRangeRank H3");

			H[2] = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (!q.value(4).isNull())
		{
			q_select.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY + " from bardata"
										" where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + " is not null and " +
																SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + "<=" + q.value(4).toString() +
										" and rowid <" + _rowid +
										" order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "DayRangeRank L1");

			L[0] = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (!q.value(5).isNull())
		{
			q_select.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY + " from bardata"
										" where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + " is not null and " +
																SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + "<=" + q.value(5).toString() +
										" and rowid <" + _rowid +
										" order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "DayRangeRank L2");

			L[1] = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (!q.value(6).isNull())
		{
			q_select.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY + " from bardata"
										" where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY + " is not null and " +
																SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY + "<=" + q.value(6).toString() +
										" and rowid <" + _rowid +
										" order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "DayRangeRank L3");

			L[2] = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		q_update.bindValue(0, QString::number(H[0],'f',4));
		q_update.bindValue(1, QString::number(H[1],'f',4));
		q_update.bindValue(2, QString::number(H[2],'f',4));
		q_update.bindValue(3, QString::number(L[0],'f',4));
		q_update.bindValue(4, QString::number(L[1],'f',4));
		q_update.bindValue(5, QString::number(L[2],'f',4));
		q_update.bindValue(6, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "update dayrange rank approx");

		if (errcode) break;
	}

	END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_moving_avg_approx(const QSqlDatabase &database, const QString &database_path,
																								const QString &symbol, const IntervalWeight baseInterval,
																								const IntervalWeight MAInterval, int start_rowid)
{
	// if they're on same or smaller time frame then skip the update
	if (baseInterval >= MAInterval || baseInterval == WEIGHT_INVALID) return 0;

	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QStringList detach_database;
	QString _rowid = QString::number(start_rowid);
	QString dbAttachName;
	QString select_mafast, select_maslow;
	QString dbAlias, tableAlias;
	QString prevTableAlias = "a";
	IntervalWeight w = Globals::getParentInterval(baseInterval);
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	// header statement
	select_mafast = "select a.rowid, (sum(b.close_)+(case when a.time_<>b.time_ then a.close_-b.close_ else 0 end))/10.0"
									" from bardata a";

	select_maslow = "select a.rowid, (sum(b.close_)+(case when a.time_<>b.time_ then a.close_-b.close_ else 0 end))/50.0"
									" from bardata a";

	// body statement
	while (w <= MAInterval && w != WEIGHT_INVALID)
	{
		dbAttachName = database_path + "/" + SQLiteHandler::getDatabaseName(symbol, w);
		dbAlias = "db" + Globals::intervalWeightToString(w);
		tableAlias = "t" + dbAlias;
		detach_database.push_back(dbAlias);

		q.exec("attach database '" + dbAttachName + "' as " + dbAlias);

		QUERY_ERROR_CODE (q,errcode,"");

		if (errcode) break;

		// TODO: optimize this join
		if (w != MAInterval)
		{
			// backup (original) -- handle if incomplete bar is created later after all update finish
//      select_mafast +=
//        " left join " + dbAlias + ".bardata " + tableAlias + " on" +
//        "(case when " + prevTableAlias + "._parent=0 then " + prevTableAlias + "._parent_prev"
//             " else " + prevTableAlias + "._parent end)=" + tableAlias + ".rowid";

//      select_maslow +=
//        " left join " + dbAlias + ".bardata " + tableAlias + " on " +
//        "(case when " + prevTableAlias + "._parent=0 then " + prevTableAlias + "._parent_prev"
//             " else " + prevTableAlias + "._parent end)=" + tableAlias + ".rowid";

			select_mafast +=
				" left join " + dbAlias + ".bardata " + tableAlias + " on" +
				"(case when ifnull(" + prevTableAlias + "._parent,0)=0"
										" then " + prevTableAlias + "._parent_prev"
										" else " + prevTableAlias + "._parent end)=" + tableAlias + ".rowid";

			select_maslow +=
				" left join " + dbAlias + ".bardata " + tableAlias + " on " +
				"(case when ifnull(" + prevTableAlias + "._parent,0)=0"
										" then " + prevTableAlias + "._parent_prev"
										" else " + prevTableAlias + "._parent end)=" + tableAlias + ".rowid";

			// optimize version -- valid if incomplete bar created after file load
//      select_mafast +=
//        " left join " + dbAlias + ".bardata " + tableAlias + " on " + prevTableAlias + "._parent =" + tableAlias + ".rowid";

//      select_maslow +=
//        " left join " + dbAlias + ".bardata " + tableAlias + " on " + prevTableAlias + "._parent =" + tableAlias + ".rowid";
		}
		else
		{
			// TODO: optimize this join
			select_mafast +=
				" left join " + dbAlias + ".bardata b on"
				"(b.rowid > (case when ifnull(" + prevTableAlias + "._parent,0)=0" +
															 " then " + prevTableAlias + "._parent_prev" +
															 " else " + prevTableAlias + "._parent end)-10 and "
				 "b.rowid <= (case when ifnull(" + prevTableAlias + "._parent,0)=0"
																" then " + prevTableAlias + "._parent_prev" +
																" else " + prevTableAlias + "._parent end))" +
				" where a.rowid >" + _rowid +
				" group by a.rowid having count(1)=10"
				" order by a.rowid";

			select_maslow +=
				" left join " + dbAlias + ".bardata b on"
				"(b.rowid > (case when ifnull(" + prevTableAlias + "._parent,0)=0"
															 " then " + prevTableAlias + "._parent_prev"
															 " else " + prevTableAlias + "._parent end)-50 and "
				 "b.rowid <= (case when ifnull(" + prevTableAlias + "._parent,0)=0"
																" then " + prevTableAlias + "._parent_prev"
																" else " + prevTableAlias + "._parent end))"
				" where a.rowid >" + _rowid +
				" group by a.rowid having count(1)=50"
				" order by a.rowid";
		}

		prevTableAlias = tableAlias;
		w = Globals::getParentInterval(w);
	}

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set " + Globals::MovingAvgNameByInterval(MAInterval) + "10=? where rowid=?");
	q.exec(select_mafast);
//  qDebug() << select_mafast;

	QUERY_ERROR_CODE (q, errcode, "moving avg approx(10)");

	while (q.next())
	{
		q_update.bindValue(0, QString::number(q.value(1).toDouble(),'f',4));
		q_update.bindValue(1, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "");
		if (errcode) break;
	}

	q_update.prepare("update bardata set " + Globals::MovingAvgNameByInterval(MAInterval) + "50=? where rowid=?");
	q.exec(select_maslow);
//  qDebug() << select_maslow;

	QUERY_ERROR_CODE (q, errcode, "moving avg approx(50)");

	while (q.next())
	{
		q_update.bindValue(0, QString::number(q.value(1).toDouble(),'f',4));
		q_update.bindValue(1, q.value(0));
		q_update.exec();

		QUERY_ERROR_CODE (q_update, errcode, "");
		if (errcode) break;
	}

	END_TRANSACTION (q_update);

	for (int i = 0; i < detach_database.size(); ++i)
	{
		q.exec("detach database " + detach_database[i]);
	}

	return errcode;
}

int BardataCalculator::update_moving_avg_approx_v2(const QSqlDatabase &database, const QString &database_path,
																									 const QString &symbol, const IntervalWeight baseInterval,
																									 const IntervalWeight MAInterval, int start_rowid)
{
	// if they are on same or smaller time frame then skip the update
	if (baseInterval >= MAInterval || baseInterval == WEIGHT_INVALID) return 0;

	// definition:
	// calculate Moving Average (N) using {previous Close (N-1) from its parent and its own Close price(1)} divide by (N)

	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QString parent_database_name = symbol + "_" + Globals::intervalWeightToString(MAInterval) + ".db";
	QString start_date = "";
	QString start_time = "";
	QString start_parent_rowid = "";

	std::list<QString> parent_date;
	std::list<QString> parent_time;
	std::list<double> parent_close;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("attach database '" + database_path + "/" + parent_database_name + "' as parentdb");
	q.exec("select date_, time_ from bardata where rowid=" + QString::number(start_rowid + 1));

	if (q.next())
	{
		start_date = q.value(0).toString();
		start_time = q.value(1).toString();
	}

	// get lower bound of parent -- FIX by adding completed checking
	q.exec("select rowid from("
					"select rowid from parentdb.bardata"
					" where (date_ <'" + start_date + "' or (date_ ='" + start_date + "' and time_ <='" + start_time + "'))"
					" order by rowid desc limit " + QString::number(SLOWAVG_LENGTH) + ")"
				 "order by rowid asc limit 1");

	start_parent_rowid = q.next() ? q.value(0).toString() : "0";

	// cache parent close price
	q.exec("select date_, time_, close_ from parentdb.bardata"
				 " where rowid >" + start_parent_rowid + " order by rowid asc");

	while (q.next())
	{
		parent_date.emplace_back(q.value(0).toString());
		parent_time.emplace_back(q.value(1).toString());
		parent_close.emplace_back(q.value(2).toDouble());
	}

	q.exec("detach database parentdb;");

	// if parent is empty or not enough data for calculation then quit
	if (parent_close.empty() || parent_close.size() < FASTAVG_LENGTH) return 0;

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set " +
										Globals::MovingAvgNameByInterval(MAInterval) + "10=?," +
										Globals::MovingAvgNameByInterval(MAInterval) + "50=?" +
									 " where rowid=?");

	std::list<double> _fast;
	std::list<double> _slow;
	double ma_fast, ma_slow;
	auto it_date = parent_date.begin();
	auto it_time = parent_time.begin();
	auto it_close = parent_close.begin();

	q.exec("select rowid, date_, time_, close_ from bardata where rowid >" + QString::number(start_rowid));
	q.next();

	while (q.isValid())
	{
		ma_fast = 0.0;
		ma_slow = 0.0;

		if (it_close != parent_close.end())
		{
			// this condition is redundant with sql statement constraint
//      if (*it_date < q.value(1).toString() || (*it_date == q.value(1).toString() && *it_time <= q.value(2).toString()))
//      {
				_fast.emplace_back(*it_close);
				_slow.emplace_back(*it_close);

				if (_fast.size() > FASTAVG_LENGTH) _fast.pop_front();
				if (_slow.size() > SLOWAVG_LENGTH) _slow.pop_front();

				++it_date;
				++it_time;
				++it_close;
//      }
		}
		else
		{
			// if there's no way to add more data into cache
			// then quit to avoid infinite loop
			if (_fast.size() < FASTAVG_LENGTH || _slow.size() < SLOWAVG_LENGTH) break;
		}

		if (_fast.empty() && _slow.empty()) break;

		if (_fast.size() == FASTAVG_LENGTH)
		{
			ma_fast = (std::accumulate(_fast.begin(), _fast.end(), 0.0) - _fast.back() + q.value(3).toDouble()) / FASTAVG_LENGTH;
		}

		if (_slow.size() == SLOWAVG_LENGTH)
		{
			ma_slow = (std::accumulate(_slow.begin(), _slow.end(), 0.0) - _slow.back() + q.value(3).toDouble()) / SLOWAVG_LENGTH;
		}

		// we assume at least slow MovingAvg has to be filled
		if (ma_slow != 0.0)
		{
			q_update.bindValue(0, QString::number(ma_fast,'f',4));
			q_update.bindValue(1, QString::number(ma_slow,'f',4));
			q_update.bindValue(2, q.value(0).toInt());
			q_update.exec();
			q.next();
		}
	}

	END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_distOHLC_rank_approx_v2(std::vector<BardataTuple> *out,
																											const QSqlDatabase &source_database,
																											const QString &database_path,
																											const QString &symbol,
																											const IntervalWeight base_interval,
																											int start_rowid)
{
	if (out == NULL || out->empty())
		return 0;

	const QString _start_rowid = QString::number(start_rowid);
	const int row_count = static_cast<int>(out->size());

	QSqlQuery q(source_database);
//  QString select_prev_daily_atr = "";
	QString detach_stmt = "";
	double dist_fscross, _macd, _distfs, _fslope, _sslope;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	// handle intraday (prevdaily atr)
	if (base_interval < WEIGHT_DAILY)
	{
		if (base_interval == WEIGHT_60MIN)
		{
			q.exec("attach database '" + database_path + "/" + symbol + "_Daily.db' as dbdaily");
			detach_stmt = "detach database dbdaily";

			// @deprecated: old version
//      // TODO: optimize case-when in join is quite slow
////      select_prev_daily_atr =
////        "select round(a." + SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS + "/b.atr,4), b.atr"
////        " from bardata a join dbDaily.bardata b on b.rowid = (case when a.time_<>b.time_ then a._parent_prev else a._parent end)";

//      select_prev_daily_atr =
//        "select round(a." + SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS + "/b.atr,4), b.atr"
//        " from bardata a join dbDaily.bardata b on b.rowid = a._parent_prev";
		}
		else if (base_interval == WEIGHT_5MIN)
		{
			// new algorithm (since 1.6.7)
			// instead of doing join we directly query prevdaily atr from daily database
			// we find lower bound of daily row using child's last datetime
			// here we only need to prepare daily database attach

			q.exec("attach database '" + database_path + "/" + symbol + "_Daily.db' as dbdaily");
			detach_stmt = "detach database dbdaily";

			//@deprecated: old version
//      q.exec("ATTACH DATABASE '" + database_path + "/" + symbol + "_60min.db' as db60min");
//      detach_stmt = "DETACH DATABASE db60min";

//      // TODO: optimize case-when in join is quite slow
////      select_prev_daily_atr =
////        "select round(a." + SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS + "/b.prevdailyatr,4), b.prevdailyatr"
////        " from bardata a join db60min.bardata b on b.rowid = (case when a.time_<>b.time_ then a._parent_prev else a._parent end)";

//      select_prev_daily_atr =
//        "select round(a." + SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS + "/b.prevdailyatr,4), b.prevdailyatr"
//        " from bardata a join db60min.bardata b on b.rowid=a._parent";
		}
	}

	for (int i = 0; i < row_count; ++i)
	{
		//
		// Intraday only: PrevDaily ATR and Dist(FS-Cross)
		//
		if (base_interval < WEIGHT_DAILY)
		{
			QString date_ = QString::fromStdString((*out)[i].date);
			QString time_ = QString::fromStdString((*out)[i].time);

			if (date_ != "" && time_ != "")
			{
				q.exec("select atr from dbdaily.bardata"
							 " where date_ <'" + date_ + "' or (date_='" + date_ +"' and time_ <'" + time_ + "')"
							 " and completed=1 order by rowid desc limit 1");

				(*out)[i].set_data(bardata::COLUMN_NAME_PREV_DAILY_ATR, q.next()? q.value(0).toString().toStdString() : "0");
			}
		}

		_macd = (*out)[i].macd;
		_distfs = (*out)[i].n_dist_fs;
		_fslope = (*out)[i].get_data_double(bardata::COLUMN_NAME_FASTAVG_SLOPE);
		_sslope = (*out)[i].get_data_double(bardata::COLUMN_NAME_SLOWAVG_SLOPE);
		dist_fscross = (*out)[i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR);

		(*out)[i].set_data(bardata::COLUMN_NAME_MACD_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTFS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTOF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTOS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCS_RANK, "0");

		if (_macd > 0.0)
		{
			q.exec("select macd_rank from bardata"
						 " where macd > 0 and macd <=" + QString::number(_macd) +
						 " and rowid <=" + _start_rowid +
						 " order by macd desc limit 1");
		}
		else if (_macd < 0.0)
		{
			q.exec("select macd_rank from bardata"
						 " where macd < 0 and macd >=" + QString::number(_macd) +
						 " and rowid <=" + _start_rowid +
						 " order by macd asc limit 1");
		}

		QUERY_ERROR_CODE (q,errcode,"dist OHLC rank (MACD)");

		(*out)[i].set_data(bardata::COLUMN_NAME_MACD_RANK, GET_RANK());

		q.exec("select atr_rank from bardata"
					 " where atr <=" + QString::number((*out)[i].atr,'f') + " and rowid <=" + _start_rowid +
					 " order by atr desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "distOHLC rank(ATR)");

		(*out)[i].set_data(bardata::COLUMN_NAME_ATR_RANK, GET_RANK());

		if (_distfs > 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTFS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid + " and n_distfs > 0 and n_distfs <=" + QString::number((*out)[i].n_dist_fs,'f') +
						 " order by n_distfs desc limit 1");
		}
		else if (_distfs < 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTFS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid + " and n_distfs < 0 and n_distfs >=" + QString::number((*out)[i].n_dist_fs,'f') +
						 " order by n_distfs asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank(DistFS)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTFS_RANK, GET_RANK());

		if (_fslope >= 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid + " and fastavgslope >= 0 and fastavgslope <=" + QString::number(_fslope,'f') +
						 " order by fastavgslope desc limit 1");
		}
		else
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid + " and fastavgslope < 0 and fastavgslope >=" + QString::number(_fslope,'f') +
						 " order by fastavgslope asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank(F-Slope)");

		(*out)[i].set_data(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK, GET_RANK());

		if (_sslope >= 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid + " and slowavgslope >= 0 and slowavgslope <=" + QString::number(_sslope,'f') +
						 " order by slowavgslope desc limit 1");
		}
		else
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid + " and slowavgslope < 0 and slowavgslope >=" + QString::number(_sslope,'f') +
						 " order by slowavgslope asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank(S-Slope)");

		(*out)[i].set_data(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK, GET_RANK());

		if ((*out)[i].n_dist_of > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + "<=" + QString::number((*out)[i].n_dist_of,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_of < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + ">=" + QString::number((*out)[i].n_dist_of,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistOF)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTOF_RANK, GET_RANK());

		if ((*out)[i].n_dist_os > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + "<=" + QString::number((*out)[i].n_dist_os,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_os < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + ">=" + QString::number((*out)[i].n_dist_os,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q,errcode,"dist OHLC rank (DistOS)");

		(*out)[i].set_data (bardata::COLUMN_NAME_DISTOS_RANK, GET_RANK());

		if ((*out)[i].n_dist_hf > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + "<=" + QString::number((*out)[i].n_dist_hf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_hf < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + ">=" + QString::number((*out)[i].n_dist_hf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistHF)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHF_RANK, GET_RANK());

		if ((*out)[i].n_dist_hs > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + "<=" + QString::number((*out)[i].n_dist_hs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_hs < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + "< 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + ">=" + QString::number((*out)[i].n_dist_hs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q,errcode,"dist OHLC rank (DistHS)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHS_RANK, GET_RANK());

		if ((*out)[i].n_dist_lf > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + "<=" + QString::number((*out)[i].n_dist_lf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_lf < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + ">=" + QString::number((*out)[i].n_dist_lf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistLF)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLF_RANK, GET_RANK());

		if ((*out)[i].n_dist_ls > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + "<=" + QString::number((*out)[i].n_dist_ls,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_ls < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + "< 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + ">=" + QString::number((*out)[i].n_dist_ls,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistLS)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLS_RANK, GET_RANK());

		if ((*out)[i].n_dist_cf > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCF_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + "<=" + QString::number((*out)[i].n_dist_cf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTCF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_cf < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCF_RANK + " from bardata"
							" where rowid <" + _start_rowid +
							" and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + "< 0"
							" and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + ">=" + QString::number((*out)[i].n_dist_cf,'f') +
							" order by " + SQLiteHandler::COLUMN_NAME_N_DISTCF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistCF -)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCF_RANK, GET_RANK());

		if ((*out)[i].n_dist_cs > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + "<=" + QString::number((*out)[i].n_dist_cs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTCS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_cs < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCS_RANK + " from bardata"
						 " where rowid <" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + "< 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + ">=" + QString::number((*out)[i].n_dist_cs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTCS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistCS -)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCS_RANK, GET_RANK());
	}

	if (detach_stmt != "")
	{
		q.exec(detach_stmt);
		QUERY_ERROR_CODE (q, errcode, "detach stmt");
	}

	return errcode;
}

int BardataCalculator::update_distOHLC_rank_approx_v2r1(std::vector<BardataTuple> *out,
																												const QSqlDatabase &source_database,
																												const QString &database_path,
																												const QString &symbol,
																												const IntervalWeight base_interval,
																												int start_rowid)
{
	if (out == NULL || out->empty())
		return 0;

	const QString _start_rowid = QString::number(start_rowid - 1);
	const int row_count = static_cast<int>(out->size());

	QSqlQuery q(source_database);
	double dist_fscross, _macd, _distfs, _fslope, _sslope;
	int errcode = 0;
	bool is_intraday = base_interval < WEIGHT_DAILY;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	if (is_intraday)
	{
		q.exec("attach database '" + database_path + "/" + symbol + "_Daily.db' as dbdaily");
	}

	for (int i = 0; i < row_count; ++i)
	{
		//
		// Intraday only: PrevDaily ATR and Dist(FS-Cross)
		//
		if (is_intraday)
		{
			QString date_ = QString::fromStdString((*out)[i].date);
			QString time_ = QString::fromStdString((*out)[i].time);

			if (date_ != "" && time_ != "")
			{
				q.exec("select atr from dbdaily.bardata"
							 " where date_ <'" + date_ + "' or (date_='" + date_ +"' and time_ <'" + time_ + "')"
							 " and completed=1 order by rowid desc limit 1");

				(*out)[i].set_data(bardata::COLUMN_NAME_PREV_DAILY_ATR, q.next()? q.value(0).toString().toStdString() : "0");
			}
		}

		_macd = (*out)[i].macd;
		_distfs = (*out)[i].n_dist_fs;
		_fslope = (*out)[i].get_data_double(bardata::COLUMN_NAME_FASTAVG_SLOPE);
		_sslope = (*out)[i].get_data_double(bardata::COLUMN_NAME_SLOWAVG_SLOPE);
		dist_fscross = (*out)[i].get_data_double(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR);

		(*out)[i].set_data(bardata::COLUMN_NAME_MACD_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTFS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTOF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTOS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLS_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCF_RANK, "0");
		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCS_RANK, "0");

		if (_macd > 0.0)
		{
			q.exec("select macd_rank from bardata"
						 " where macd > 0 and macd <=" + QString::number(_macd) +
						 " and rowid <=" + _start_rowid +
						 " order by macd desc limit 1");
		}
		else if (_macd < 0.0)
		{
			q.exec("select macd_rank from bardata"
						 " where macd < 0 and macd >=" + QString::number(_macd) +
						 " and rowid <=" + _start_rowid +
						 " order by macd asc limit 1");
		}

		QUERY_ERROR_CODE (q,errcode,"dist OHLC rank (MACD -)");

		(*out)[i].set_data(bardata::COLUMN_NAME_MACD_RANK, GET_RANK());

		q.exec("select atr_rank from bardata"
					 " where atr <=" + QString::number((*out)[i].atr,'f') +
					 " and rowid <=" + _start_rowid +
					 " order by atr desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "distOHLC rank(ATR)");

		(*out)[i].set_data(bardata::COLUMN_NAME_ATR_RANK, GET_RANK());

		if (_distfs > 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTFS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and n_distfs > 0 and n_distfs <=" + QString::number((*out)[i].n_dist_fs,'f') +
						 " order by n_distfs desc limit 1");
		}
		else if (_distfs < 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTFS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and n_distfs < 0 and n_distfs >=" + QString::number((*out)[i].n_dist_fs,'f') +
						 " order by n_distfs asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank(DistFS)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTFS_RANK, GET_RANK());

		if (_fslope >= 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and fastavgslope >= 0 and fastavgslope <=" + QString::number(_fslope,'f') +
						 " order by fastavgslope desc limit 1");
		}
		else
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and fastavgslope < 0 and fastavgslope >=" + QString::number(_fslope,'f') +
						 " order by fastavgslope asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank(F-Slope)");

		(*out)[i].set_data(bardata::COLUMN_NAME_FASTAVG_SLOPE_RANK, GET_RANK());

		if (_sslope >= 0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and slowavgslope >= 0 and slowavgslope <=" + QString::number(_sslope,'f') +
						 " order by slowavgslope desc limit 1");
		}
		else
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and slowavgslope < 0 and slowavgslope >=" + QString::number(_sslope,'f') +
						 " order by slowavgslope asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank(S-Slope)");

		(*out)[i].set_data(bardata::COLUMN_NAME_SLOWAVG_SLOPE_RANK, GET_RANK());

		if ((*out)[i].n_dist_of > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + "<=" + QString::number((*out)[i].n_dist_of,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_of < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOF + ">=" + QString::number((*out)[i].n_dist_of,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistOF)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTOF_RANK, GET_RANK());

		if ((*out)[i].n_dist_os > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + "<=" + QString::number((*out)[i].n_dist_os,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_os < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTOS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTOS + ">=" + QString::number((*out)[i].n_dist_os,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTOS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q,errcode,"dist OHLC rank (DistOS)");

		(*out)[i].set_data (bardata::COLUMN_NAME_DISTOS_RANK, GET_RANK());

		if ((*out)[i].n_dist_hf > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + "<=" + QString::number((*out)[i].n_dist_hf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_hf < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHF + ">=" + QString::number((*out)[i].n_dist_hf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistHF)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHF_RANK, GET_RANK());

		if ((*out)[i].n_dist_hs > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + "> 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + "<=" + QString::number((*out)[i].n_dist_hs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_hs < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTHS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + "< 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTHS + ">=" + QString::number((*out)[i].n_dist_hs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTHS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q,errcode,"dist OHLC rank (DistHS)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTHS_RANK, GET_RANK());

		if ((*out)[i].n_dist_lf > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + "<=" + QString::number((*out)[i].n_dist_lf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_lf < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + "< 0" +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLF + ">=" + QString::number((*out)[i].n_dist_lf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistLF)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLF_RANK, GET_RANK());

		if ((*out)[i].n_dist_ls > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + "<=" + QString::number((*out)[i].n_dist_ls,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_ls < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTLS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + "< 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTLS + ">=" + QString::number((*out)[i].n_dist_ls,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTLS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistLS)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTLS_RANK, GET_RANK());

		if ((*out)[i].n_dist_cf > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCF_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + "<=" + QString::number((*out)[i].n_dist_cf,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTCF + " desc limit 1");
		}
		else if ((*out)[i].n_dist_cf < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCF_RANK + " from bardata"
							" where rowid <=" + _start_rowid +
							" and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + "< 0"
							" and " + SQLiteHandler::COLUMN_NAME_N_DISTCF + ">=" + QString::number((*out)[i].n_dist_cf,'f') +
							" order by " + SQLiteHandler::COLUMN_NAME_N_DISTCF + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistCF -)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCF_RANK, GET_RANK());

		if ((*out)[i].n_dist_cs > 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + "> 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + "<=" + QString::number((*out)[i].n_dist_cs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTCS + " desc limit 1");
		}
		else if ((*out)[i].n_dist_cs < 0.0)
		{
			q.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCS_RANK + " from bardata"
						 " where rowid <=" + _start_rowid +
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + "< 0"
						 " and " + SQLiteHandler::COLUMN_NAME_N_DISTCS + ">=" + QString::number((*out)[i].n_dist_cs,'f') +
						 " order by " + SQLiteHandler::COLUMN_NAME_N_DISTCS + " asc limit 1");
		}

		QUERY_ERROR_CODE (q, errcode, "dist OHLC rank (DistCS -)");

		(*out)[i].set_data(bardata::COLUMN_NAME_DISTCS_RANK, GET_RANK());
	}

	if (is_intraday)
	{
		q.exec("detach database dbdaily");
		QUERY_ERROR_CODE (q, errcode, "detach stmt");
	}

	return errcode;
}

int BardataCalculator::update_prevdaily_atr(std::vector<BardataTuple> *out, const QSqlDatabase &database, const QString &database_path, const QString &symbol)
{
	if (out == NULL || out->empty())
		return 0;

	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QString date_, time_;
	double _prevdailyatr;
	int row_count = static_cast<int>(out->size());
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("attach database '" + database_path + "/" + symbol + "_Daily.db' as dbdaily");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set"
									 " prevdailyatr=?"
									 ",n_distof=?"
									 ",n_distos=?"
									 ",n_disthf=?"
									 ",n_disths=?"
									 ",n_distlf=?"
									 ",n_distls=?"
									 ",n_distcf=?"
									 ",n_distcs=?"
									 ",n_distfs=?"
									 " where date_=? and time_=?");

	for (int i = 0; i < row_count; ++i)
	{
		date_ = QString::fromStdString((*out)[i].date);
		time_ = QString::fromStdString((*out)[i].time);

		if (date_ != "" && time_ != "")
		{
			q.exec("select atr from dbdaily.bardata"
						 " where date_ <'" + date_ + "' or (date_='" + date_ +"' and time_ <='" + time_ + "')"
						 " and completed=1 order by rowid desc limit 1");

			if (q.next())
			{
				_prevdailyatr = q.value(0).toDouble();

				(*out)[i].set_data(bardata::COLUMN_NAME_PREV_DAILY_ATR, std::to_string(_prevdailyatr));

				if (_prevdailyatr > 0.0)
				{
					(*out)[i].n_dist_of = (*out)[i].dist_of / _prevdailyatr;
					(*out)[i].n_dist_os = (*out)[i].dist_os / _prevdailyatr;
					(*out)[i].n_dist_hf = (*out)[i].dist_hf / _prevdailyatr;
					(*out)[i].n_dist_hs = (*out)[i].dist_hs / _prevdailyatr;
					(*out)[i].n_dist_lf = (*out)[i].dist_lf / _prevdailyatr;
					(*out)[i].n_dist_ls = (*out)[i].dist_ls / _prevdailyatr;
					(*out)[i].n_dist_cf = (*out)[i].dist_cf / _prevdailyatr;
					(*out)[i].n_dist_cs = (*out)[i].dist_cs / _prevdailyatr;
					(*out)[i].n_dist_fs = (*out)[i].dist_fs / _prevdailyatr;

					q_update.bindValue(0, _prevdailyatr);
					q_update.bindValue(1, QString::number((*out)[i].dist_of / _prevdailyatr,'f',8));
					q_update.bindValue(2, QString::number((*out)[i].dist_os / _prevdailyatr,'f',8));
					q_update.bindValue(3, QString::number((*out)[i].dist_hf / _prevdailyatr,'f',8));
					q_update.bindValue(4, QString::number((*out)[i].dist_hs / _prevdailyatr,'f',8));
					q_update.bindValue(5, QString::number((*out)[i].dist_lf / _prevdailyatr,'f',8));
					q_update.bindValue(6, QString::number((*out)[i].dist_ls / _prevdailyatr,'f',8));
					q_update.bindValue(7, QString::number((*out)[i].dist_cf / _prevdailyatr,'f',8));
					q_update.bindValue(8, QString::number((*out)[i].dist_cs / _prevdailyatr,'f',8));
					q_update.bindValue(9, QString::number((*out)[i].dist_fs / _prevdailyatr,'f',8));
					q_update.bindValue(10, date_);
					q_update.bindValue(11, time_);
					q_update.exec();
				}
			}
			else
			{
				QUERY_ERROR_CODE(q_update, errcode, "prevdaily atr");
			}
		}
	}

	END_TRANSACTION (q_update);
	q.exec("detach database dbdaily;");

	return errcode;
}

/*int BardataCalculator::update_prevdaily_atr_v2(const QSqlDatabase &database, const QString &database_path, const QString &symbol, int start_rowid)
{
	QSqlQuery q(database);
	QSqlQuery q_select(database);
	QSqlQuery q_update(database);
	QString date_, time_;
	QString _start_rowid = QString::number(start_rowid);
	double _prevdailyatr, open_, high_, low_, close_, fastavg, slowavg;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("attach database '" + database_path + "/" + symbol + "_Daily.db' as dbdaily");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set"
									 " prevdailyatr=?"
									 ",n_distof=?"
									 ",n_distos=?"
									 ",n_disthf=?"
									 ",n_disths=?"
									 ",n_distlf=?"
									 ",n_distls=?"
									 ",n_distcf=?"
									 ",n_distcs=?"
									 ",n_distfs=?"
									 " where rowid=?");

	q_select.exec("select rowid, date_, time_, open_, high_, low_, close_, fastavg, slowavg"
								" from bardata where rowid >=" + _start_rowid);

	while (q_select.next())
	{
		date_   = q_select.value(1).toString();
		time_   = q_select.value(2).toString();

		q.exec("select atr from dbdaily.bardata"
					 " where date_ <'" + date_ + "' or (date_='" + date_ +"' and time_ <='" + time_ + "')"
					 " and completed=1 order by rowid desc limit 1");

		if (q.next())
		{
			_prevdailyatr = q.value(0).toDouble();

			if (_prevdailyatr > 0.0)
			{
				open_   = q_select.value(3).toDouble();
				high_   = q_select.value(4).toDouble();
				low_    = q_select.value(5).toDouble();
				close_  = q_select.value(6).toDouble();
				fastavg = q_select.value(7).toDouble();
				slowavg = q_select.value(8).toDouble();

				q_update.bindValue(0, _prevdailyatr);
				q_update.bindValue(1, QString::number((open_   - fastavg) / _prevdailyatr,'f',8));
				q_update.bindValue(2, QString::number((open_   - slowavg) / _prevdailyatr,'f',8));
				q_update.bindValue(3, QString::number((high_   - fastavg) / _prevdailyatr,'f',8));
				q_update.bindValue(4, QString::number((high_   - slowavg) / _prevdailyatr,'f',8));
				q_update.bindValue(5, QString::number((low_    - fastavg) / _prevdailyatr,'f',8));
				q_update.bindValue(6, QString::number((low_    - slowavg) / _prevdailyatr,'f',8));
				q_update.bindValue(7, QString::number((close_  - fastavg) / _prevdailyatr,'f',8));
				q_update.bindValue(8, QString::number((close_  - slowavg) / _prevdailyatr,'f',8));
				q_update.bindValue(9, QString::number((fastavg - slowavg) / _prevdailyatr,'f',8));
				q_update.bindValue(10, q_select.value(0).toInt());
				q_update.exec();
			}
		}
	}

	END_TRANSACTION (q_update);
	q.exec("detach database dbdaily;");

	return errcode;
}*/


// XXX
/*int BardataCalculator::update_close_threshold_v3(std::vector<BardataTuple> *out, const QSqlDatabase &database, int id_threshold,
																								 double cgf_threshold, double clf_threshold,
																								 double cgs_threshold, double cls_threshold, int start_rowid)
{
	const int row_count = static_cast<int>(out->size());
	const QString _ID = "_" + QString::number(id_threshold); // zero-based
	const QString _CGF = SQLiteHandler::COLUMN_NAME_CGF + _ID;
	const QString _CLF = SQLiteHandler::COLUMN_NAME_CLF + _ID;
	const QString _CGS = SQLiteHandler::COLUMN_NAME_CGS + _ID;
	const QString _CLS = SQLiteHandler::COLUMN_NAME_CLS + _ID;
	const QString _cgf_rank = SQLiteHandler::COLUMN_NAME_CGF_RANK + _ID;
	const QString _clf_rank = SQLiteHandler::COLUMN_NAME_CLF_RANK + _ID;
	const QString _cgs_rank = SQLiteHandler::COLUMN_NAME_CGS_RANK + _ID;
	const QString _cls_rank = SQLiteHandler::COLUMN_NAME_CLS_RANK + _ID;

	QSqlQuery q(database);
	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	double m_close, m_fastavg, m_slowavg;
	int cgf_counter = 0;
	int clf_counter = 0;
	int cgs_counter = 0;
	int cls_counter = 0;
	int errcode = 0;

	// get previous row information
	q.exec("select " + _CGF + "," + _CLF + "," + _CGS + "," + _CLS +
				 " from bardata where rowid=" + QString::number(start_rowid));

	QUERY_ERROR_CODE (q, errcode, "close threshold v3");

	if (errcode) return errcode;

	if (q.next())
	{
		cgf_counter = q.value(0).toInt();
		clf_counter = q.value(1).toInt();
		cgs_counter = q.value(2).toInt();
		cls_counter = q.value(3).toInt();
	}

	for (int i = 0; i < row_count; ++i)
	{
		m_close = (*out)[i].close;
		m_fastavg = (*out)[i].fast_avg;
		m_slowavg = (*out)[i].slow_avg;

		cgf_counter = (m_close > m_fastavg - cgf_threshold) ? cgf_counter + 1 : 0;
		clf_counter = (m_close < m_fastavg + clf_threshold) ? clf_counter + 1 : 0;
		cgs_counter = (m_close > m_slowavg - cgs_threshold) ? cgs_counter + 1 : 0;
		cls_counter = (m_close < m_slowavg + cls_threshold) ? cls_counter + 1 : 0;

//    (*out)[i].set_data(_CGF.toStdString(), std::to_string(cgf_counter));
//    (*out)[i].set_data(_CLF.toStdString(), std::to_string(clf_counter));
//    (*out)[i].set_data(_CGS.toStdString(), std::to_string(cgs_counter));
//    (*out)[i].set_data(_CLS.toStdString(), std::to_string(cls_counter));

		(*out)[i].set_data(_CGF.toStdString(), QString::number(cgf_counter).toStdString());
		(*out)[i].set_data(_CLF.toStdString(), QString::number(clf_counter).toStdString());
		(*out)[i].set_data(_CGS.toStdString(), QString::number(cgs_counter).toStdString());
		(*out)[i].set_data(_CLS.toStdString(), QString::number(cls_counter).toStdString());

#if DISABLE_INCOMPLETE_HISTOGRAM
		if ((*out)[i].completed != CompleteStatus::INCOMPLETE_PENDING &&
				(*out)[i].completed != CompleteStatus::INCOMPLETE_DUMMY &&
				(*out)[i].completed != CompleteStatus::INCOMPLETE)
#endif
		{
			if (cgf_counter > 0)
			{
				q.exec("select " + _cgf_rank + " from bardata"
							 " where " + _CGF + "> 0 " +
							 " and " + _CGF + "<=" + QString::number(cgf_counter) +
							 " and " + _cgf_rank + "> 0"
							 " order by " + _CGF + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "close threshold(cgf)");

				if (errcode) break;
				(*out)[i].set_data(_cgf_rank.toStdString(), q.next()? q.value(0).toString().toStdString() : "0");
			}

			if (clf_counter > 0)
			{
				q.exec("select " + _clf_rank + " from bardata"
							 " where " + _CLF + "> 0" +
							 " and " + _CLF + "<=" + QString::number(clf_counter) +
							 " and " + _clf_rank + "> 0"
							 " order by " + _CLF + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "close threshold(clf)");

				if (errcode) break;
				(*out)[i].set_data(_clf_rank.toStdString(), q.next()? q.value(0).toString().toStdString() : "0");
			}

			if (cgs_counter > 0)
			{
				q.exec("select " + _cgs_rank + " from bardata"
							 " where " + _CGS + "> 0" +
							 " and " + _CGS + "<=" + QString::number(cgs_counter) +
							 " and " + _cgs_rank + "> 0" +
							 " order by " + _CGS + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "close threshold(cgs)");

				if (errcode) break;
				(*out)[i].set_data(_cgs_rank.toStdString(), q.next()? q.value(0).toString().toStdString() : "0");
			}

			if (cls_counter > 0)
			{
				q.exec("select " + _cls_rank + " from bardata"
							 " where " + _CLS + "> 0" +
							 " and " + _CLS + "<=" + QString::number(cls_counter) +
							 " and " + _cls_rank + "> 0" +
							 " order by " + _CLS + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "close threshold(cls)");

				if (errcode) break;
				(*out)[i].set_data(_cls_rank.toStdString(), q.next()? q.value(0).toString().toStdString() : "0");
			}
		}
	}

	return errcode;
}*/

int BardataCalculator::update_close_threshold_v3(std::vector<BardataTuple>* /*unused*/,
																								 const QSqlDatabase &database,
																								 int id_threshold, double cgf_threshold, double clf_threshold,
																								 double cgs_threshold, double cls_threshold, int start_rowid)
{
	const QString _ID = "_" + QString::number(id_threshold); // zero-based
	const QString _cgf = SQLiteHandler::COLUMN_NAME_CGF + _ID;
	const QString _clf = SQLiteHandler::COLUMN_NAME_CLF + _ID;
	const QString _cgs = SQLiteHandler::COLUMN_NAME_CGS + _ID;
	const QString _cls = SQLiteHandler::COLUMN_NAME_CLS + _ID;
	const QString _cgf_rank = SQLiteHandler::COLUMN_NAME_CGF_RANK + _ID;
	const QString _clf_rank = SQLiteHandler::COLUMN_NAME_CLF_RANK + _ID;
	const QString _cgs_rank = SQLiteHandler::COLUMN_NAME_CGS_RANK + _ID;
	const QString _cls_rank = SQLiteHandler::COLUMN_NAME_CLS_RANK + _ID;

	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QSqlQuery q_select(database);
	double m_close, m_fastavg, m_slowavg;
	double cgf_rank = 0.0;
	double clf_rank = 0.0;
	double cgs_rank = 0.0;
	double cls_rank = 0.0;
	int cgf_counter = 0;
	int clf_counter = 0;
	int cgs_counter = 0;
	int cls_counter = 0;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	q_update.exec("PRAGMA temp_store = MEMORY;");
	q_update.exec("PRAGMA cache_size = 10000;");

	q.exec("select rowid, close_, fastavg, slowavg," + _cgf + "," + _clf + "," + _cgs + "," + _cls +
				 " from bardata where rowid >=" + QString::number(start_rowid-2) +
				 " order by rowid asc");

	QUERY_ERROR_CODE (q, errcode, "close threshold v3");
	if (errcode) return errcode;

	// get previous row information
	if (q.next())
	{
		cgf_counter = q.value(4).toInt();
		clf_counter = q.value(5).toInt();
		cgs_counter = q.value(6).toInt();
		cls_counter = q.value(7).toInt();
	}

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set " +
										_cgf + "=?," + _cgf_rank + "=?," +
										_clf + "=?," + _clf_rank + "=?," +
										_cgs + "=?," + _cgs_rank + "=?," +
										_cls + "=?," + _cls_rank + "=?" +
										" where rowid=?");

	// get new rows
	while (q.next())
	{
		m_close = q.value(1).toDouble();
		m_fastavg = q.value(2).toDouble();
		m_slowavg = q.value(3).toDouble();

		cgf_counter = (m_close > m_fastavg - cgf_threshold) ? cgf_counter + 1 : 0;
		clf_counter = (m_close < m_fastavg + clf_threshold) ? clf_counter + 1 : 0;
		cgs_counter = (m_close > m_slowavg - cgs_threshold) ? cgs_counter + 1 : 0;
		cls_counter = (m_close < m_slowavg + cls_threshold) ? cls_counter + 1 : 0;

		cgf_rank = clf_rank = 0.0;
		cgs_rank = cls_rank = 0.0;

		if (cgf_counter > 0)
		{
			q_select.exec("select " + _cgf_rank + " from bardata"
										" where " + _cgf + "> 0 " +
										" and " + _cgf + "<=" + QString::number(cgf_counter) +
										" and " + _cgf_rank + "> 0"
										" order by " + _cgf + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "CGF threshold");
			if (errcode) break;

			cgf_rank = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (clf_counter > 0)
		{
			q_select.exec("select " + _clf_rank + " from bardata"
										 " where " + _clf + "> 0" +
										 " and " + _clf + "<=" + QString::number(clf_counter) +
										 " and " + _clf_rank + "> 0"
										 " order by " + _clf + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "CLF threshold");

			if (errcode) break;

			clf_rank = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (cgs_counter > 0)
		{
			q_select.exec("select " + _cgs_rank + " from bardata"
										" where " + _cgs + "> 0" +
										" and " + _cgs + "<=" + QString::number(cgs_counter) +
										" and " + _cgs_rank + "> 0" +
										" order by " + _cgs + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "CGS threshold");

			if (errcode) break;

			cgs_rank = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		if (cls_counter > 0)
		{
			q_select.exec("select " + _cls_rank + " from bardata"
										" where " + _cls + "> 0" +
										" and " + _cls + "<=" + QString::number(cls_counter) +
										" and " + _cls_rank + "> 0" +
										" order by " + _cls + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "CLS threshold");

			if (errcode) break;

			cls_rank = q_select.next() ? q_select.value(0).toDouble() : 0.0;
		}

		q_update.bindValue(0, cgf_counter);
		q_update.bindValue(1, cgf_rank);
		q_update.bindValue(2, clf_counter);
		q_update.bindValue(3, clf_rank);
		q_update.bindValue(4, cgs_counter);
		q_update.bindValue(5, cgs_rank);
		q_update.bindValue(6, cls_counter);
		q_update.bindValue(7, cls_rank);
		q_update.bindValue(8, q.value(0));
		q_update.exec();
	}

	END_TRANSACTION (q_update);

	return errcode;
}

// XXX
/*int BardataCalculator::update_column_threshold_1(std::vector<BardataTuple> *out, const QSqlDatabase &database,
																								 const QString &column_name, int id_threshold,
																								 const QString &operator_1, const QString &value_1,
																								 const QString &operator_2, const QString &value_2, int start_rowid)
{
	QSqlQuery q(database);
	const QString __t_id = QString::number(id_threshold);
	const QString __start_rowid = QString::number(start_rowid);
	const QString __column_value1 = column_name + "_VALUE1_" + __t_id;
	const QString __column_value2 = column_name + "_VALUE2_" + __t_id;
	const QString __column_rank1 = column_name + "_RANK1_" + __t_id;
	const QString __column_rank2 = column_name + "_RANK2_" + __t_id;
	const string column_value1 = __column_value1.toStdString();
	const string column_value2 = __column_value2.toStdString();
	const string column_rank1 = __column_rank1.toStdString();
	const string column_rank2 = __column_rank2.toStdString();
	const double thres_value1 = value_1.toDouble();
	const double thres_value2 = value_2.toDouble();
	const int row_count = static_cast<int>(out->size());
	double m_column;
	double n_rank1 = 0.0;
	double n_rank2 = 0.0;
	int n_column1 = 0;
	int n_column2 = 0;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("select " + __column_value1 + "," + __column_value2 + " from bardata where rowid=" + __start_rowid);
	QUERY_ERROR_CODE (q, errcode, "update_column_threshold_1");

	// get previous value
	if (q.next())
	{
		n_column1 = q.value(0).toInt();
		n_column2 = q.value(1).toInt();
	}

	for (int i = 0; i < row_count; ++i)
	{
		m_column = (*out)[i].get_data_double(column_name.toStdString());
		n_rank1 = 0.0;
		n_rank2 = 0.0;

		// Threshold 1
		if ((operator_1 == ">" && m_column > thres_value1) ||
				(operator_1 == "<" && m_column < thres_value1) ||
				(operator_1 == "=" && m_column == thres_value1))
		{
			++n_column1;

#if DISABLE_INCOMPLETE_HISTOGRAM
			if ((*out)[i].completed != CompleteStatus::INCOMPLETE_PENDING &&
					(*out)[i].completed != CompleteStatus::INCOMPLETE_DUMMY &&
					(*out)[i].completed != CompleteStatus::INCOMPLETE)
#endif
			{
				q.exec("select " + __column_rank1 + " from bardata"
							 " where " + __column_value1 + "> 0" +
							 " and " + __column_value1 + "<=" + QString::number(n_column1) +
							 " and rowid <=" + __start_rowid +
							 " order by " + __column_value1 + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "column threshold 1");
				n_rank1 = q.next()? q.value(0).toDouble() : 0.0;
			}
		}
		else
		{
			n_column1 = 0;
		}

		// Threshold 2
		if ((operator_2 == ">" && m_column > thres_value2) ||
				(operator_2 == "<" && m_column < thres_value2) ||
				(operator_2 == "=" && m_column == thres_value2))
		{
			++n_column2;

#if DISABLE_INCOMPLETE_HISTOGRAM
			if ((*out)[i].completed != CompleteStatus::INCOMPLETE_PENDING &&
					(*out)[i].completed != CompleteStatus::INCOMPLETE_DUMMY &&
					(*out)[i].completed != CompleteStatus::INCOMPLETE)
#endif
			{
				q.exec("select " + __column_rank2 + " from bardata"
							 " where " + __column_value2 + "> 0" +
							 " and " + __column_value2 + "<=" + QString::number(n_column2) +
							 " and rowid <=" + __start_rowid +
							 " order by " + __column_value2 + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "column threshold 1");
				n_rank2 = q.next()? q.value(0).toDouble() : 0.0;
			}
		}
		else
		{
			n_column2 = 0;
		}

		(*out)[i].set_data(column_value1, std::to_string(n_column1));
		(*out)[i].set_data(column_value2, std::to_string(n_column2));
		(*out)[i].set_data(column_rank1, QString::number(n_rank1).toStdString());
		(*out)[i].set_data(column_rank2, QString::number(n_rank2).toStdString());
	}

	return errcode;
}*/

int BardataCalculator::update_column_threshold_1(std::vector<BardataTuple>* /*unused*/,
																								 const QSqlDatabase &database,
																								 const QString &column_name, int id_threshold,
																								 const QString &operator_1, const QString &value_1,
																								 const QString &operator_2, const QString &value_2, int start_rowid)
{
	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QSqlQuery q_select(database);

	const QString __t_id = QString::number(id_threshold);
	const QString __start_rowid = QString::number(start_rowid-2);
	const QString __column_value1 = column_name + "_VALUE1_" + __t_id;
	const QString __column_value2 = column_name + "_VALUE2_" + __t_id;
	const QString __column_rank1 = column_name + "_RANK1_" + __t_id;
	const QString __column_rank2 = column_name + "_RANK2_" + __t_id;
	const double thres_value1 = value_1.toDouble();
	const double thres_value2 = value_2.toDouble();
	double m_column;
	double n_rank1 = 0.0;
	double n_rank2 = 0.0;
	int n_column1 = 0;
	int n_column2 = 0;
	int errcode = 0;

	q_update.exec("PRAGMA temp_store = MEMORY;");
	q_update.exec("PRAGMA cache_size = 10000;");

	q_select.setForwardOnly(true);
	q_select.exec("PRAGMA temp_store = MEMORY;");
	q_select.exec("PRAGMA cache_size = 10000;");

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("select rowid," + column_name + "," + __column_value1 + "," + __column_value2 + " from bardata where rowid >=" + __start_rowid);

	QUERY_ERROR_CODE (q, errcode, "update_column_threshold_1");

	// get previous value
	if (q.next())
	{
		n_column1 = q.value(2).toInt();
		n_column2 = q.value(3).toInt();
	}

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set " +
									 __column_value1 + "=?," +
									 __column_rank1 + "=?," +
									 __column_value2 + "=?," +
									 __column_rank2 + "=?"
									 " where rowid=?");

	// get new rows
	while (q.next())
	{
		m_column = q.value(1).toDouble();

		n_rank1 = n_rank2 = 0.0;
		n_column1 = ((operator_1 == ">" && m_column > thres_value1) ||
								 (operator_1 == "<" && m_column < thres_value1) ||
								 (operator_1 == "=" && m_column == thres_value1)) ? n_column1 + 1 : 0;

		n_column2 = ((operator_2 == ">" && m_column > thres_value2) ||
								 (operator_2 == "<" && m_column < thres_value2) ||
								 (operator_2 == "=" && m_column == thres_value2)) ? n_column2 + 1 : 0;

		if (n_column1 > 0)
		{
			q_select.exec("select " + __column_rank1 + " from bardata"
										" where " + __column_value1 + "> 0" +
										" and " + __column_value1 + "<=" + QString::number(n_column1) +
										" and rowid <=" + __start_rowid +
										" order by " + __column_value1 + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "column threshold 1");

			n_rank1 = q_select.next()? q_select.value(0).toDouble() : 0.0;
		}


		if (n_column2 > 0)
		{
			q_select.exec("select " + __column_rank2 + " from bardata"
						 " where " + __column_value2 + "> 0" +
						 " and " + __column_value2 + "<=" + QString::number(n_column2) +
						 " and rowid <=" + __start_rowid +
						 " order by " + __column_value2 + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "column threshold 2");

			n_rank2 = q_select.next()? q_select.value(0).toDouble() : 0.0;
		}

		q_update.bindValue(0, n_column1);
		q_update.bindValue(1, QString::number(n_rank1,'f',5));
		q_update.bindValue(2, n_column2);
		q_update.bindValue(3, QString::number(n_rank2,'f',5));
		q_update.bindValue(4, q.value(0));
		q_update.exec();
	}

	END_TRANSACTION (q_update);

	return errcode;
}

// XXX
/*int BardataCalculator::update_fgs_fls_threshold_approx(std::vector<BardataTuple> *out, const QSqlDatabase &database, int id_threshold,
																											 double fgs_threshold, double fls_threshold, int start_rowid)
{
	QSqlQuery q(database);
	const QString _column_id = "_" + QString::number(id_threshold);
	const QString _start_rowid = QString::number(start_rowid);
	const QString fgs_column = SQLiteHandler::COLUMN_NAME_FGS + _column_id;
	const QString fls_column = SQLiteHandler::COLUMN_NAME_FLS + _column_id;
	const QString fgs_rank_column = SQLiteHandler::COLUMN_NAME_FGS_RANK + _column_id;
	const QString fls_rank_column = SQLiteHandler::COLUMN_NAME_FLS_RANK + _column_id;
	const std::string _fgs_column = fgs_column.toStdString();
	const std::string _fls_column = fls_column.toStdString();
	const std::string _fgs_rank_column = fgs_rank_column.toStdString();
	const std::string _fls_rank_column = fls_rank_column.toStdString();
	double _fastavg, _slowavg;
	int _count_fgs = 0;
	int _count_fls = 0;
	int errcode = 0;

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("select " + fgs_column + "," + fls_column +" from bardata where rowid=" + _start_rowid);

	// get previous value -- for continuing duration counting
	if (q.next())
	{
		_count_fgs = q.value(0).toInt();
		_count_fls = q.value(1).toInt();
	}

	// begin counting start from last rowid
	for (int i = 0; i < out->size(); ++i)
	{
		_fastavg = (*out)[i].fast_avg;
		_slowavg = (*out)[i].slow_avg;

		_count_fgs = (_fastavg + fgs_threshold > _slowavg) ? _count_fgs + 1 : 0;
		_count_fls = (_fastavg - fls_threshold < _slowavg) ? _count_fls + 1 : 0;

		(*out)[i].set_data(_fgs_column, std::to_string(_count_fgs));
		(*out)[i].set_data(_fls_column, std::to_string(_count_fls));

#if DISABLE_INCOMPLETE_HISTOGRAM
		if ((*out)[i].completed != CompleteStatus::INCOMPLETE_PENDING &&
				(*out)[i].completed != CompleteStatus::INCOMPLETE_DUMMY &&
				(*out)[i].completed != CompleteStatus::INCOMPLETE)
#endif
		{
			if (_count_fgs > 0)
			{
				q.exec("select " + fgs_rank_column + " from bardata where rowid <=" + _start_rowid +
							 " and " + fgs_column + "> 0 "
							 " and " + fgs_column + "<=" + QString::number(_count_fgs) +
							 " order by " + fgs_column + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "update fgs thres");

				(*out)[i].set_data(_fgs_rank_column, q.next() ? q.value(0).toString().toStdString() : "0");
			}

			if (_count_fls > 0)
			{
				q.exec("select " + fls_rank_column + " from bardata where rowid <=" + _start_rowid +
							 " and " + fls_column + "> 0 "
							 " and " + fls_column + "<=" + QString::number(_count_fls) +
							 " order by " + fls_column + " desc limit 1");

				QUERY_ERROR_CODE (q, errcode, "update fls thres");

				(*out)[i].set_data(_fls_rank_column, q.next() ? q.value(0).toString().toStdString() : "0");
			}
		}
	}

	return errcode;
}*/

int BardataCalculator::update_fgs_fls_threshold_approx(std::vector<BardataTuple>* /*unused*/,
																											 const QSqlDatabase &database,
																											 int id_threshold,
																											 double fgs_threshold,
																											 double fls_threshold,
																											 int start_rowid)
{
	const QString _column_id = "_" + QString::number(id_threshold);
	const QString _start_rowid = QString::number(start_rowid - 2); // TODO:
	const QString fgs_column = SQLiteHandler::COLUMN_NAME_FGS + _column_id;
	const QString fls_column = SQLiteHandler::COLUMN_NAME_FLS + _column_id;
	const QString fgs_rank_column = SQLiteHandler::COLUMN_NAME_FGS_RANK + _column_id;
	const QString fls_rank_column = SQLiteHandler::COLUMN_NAME_FLS_RANK + _column_id;

	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QSqlQuery q_select(database);
	double _fastavg, _slowavg, fgs_rank, fls_rank;
	int fgs_count = 0;
	int fls_count = 0;
	int errcode = 0;

	q_update.exec("PRAGMA temp_store = MEMORY;");
	q_update.exec("PRAGMA cache_size = 10000;");

	q_select.setForwardOnly(true);
	q_select.exec("PRAGMA temp_store = MEMORY;");
	q_select.exec("PRAGMA cache_size = 10000;");

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("select rowid, fastavg, slowavg," + fgs_column + "," + fls_column +" from bardata where rowid >=" + _start_rowid);

	// get previous value -- for continuing duration counting
	if (q.next())
	{
		fgs_count = q.value(3).toInt();
		fls_count = q.value(4).toInt();
	}

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set " +
									 fgs_column + "=?," + fgs_rank_column + "=?," +
									 fls_column + "=?," + fls_rank_column + "=?" +
									 " where rowid=?");

	// begin counting start from last rowid
	while (q.next())
	{
		_fastavg = q.value(1).toDouble();
		_slowavg = q.value(2).toDouble();

		fgs_count = (_fastavg + fgs_threshold > _slowavg) ? fgs_count + 1 : 0;
		fls_count = (_fastavg - fls_threshold < _slowavg) ? fls_count + 1 : 0;
		fgs_rank = fls_rank = 0.0;

		if (fgs_count > 0)
		{
			q_select.exec("select " + fgs_rank_column + " from bardata"
										" where rowid <=" + _start_rowid +
										" and " + fgs_column + "> 0 "
										" and " + fgs_column + "<=" + QString::number(fgs_count) +
										" order by " + fgs_column + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "update fgs thres");

			fgs_rank = q_select.next() ? q_select.value(0).toDouble(): 0.0;
		}

		if (fls_count > 0)
		{
			q_select.exec("select " + fls_rank_column + " from bardata"
										" where rowid <=" + _start_rowid +
										" and " + fls_column + "> 0 "
										" and " + fls_column + "<=" + QString::number(fls_count) +
										" order by " + fls_column + " desc limit 1");

			QUERY_ERROR_CODE (q_select, errcode, "update fls thres");

			fls_rank = q_select.next() ? q_select.value(0).toDouble(): 0.0;
		}

		q_update.bindValue(0, fgs_count);
		q_update.bindValue(1, fgs_rank);
		q_update.bindValue(2, fls_count);
		q_update.bindValue(3, fls_rank);
		q_update.bindValue(4, q.value(0));
		q_update.exec();
	}

	END_TRANSACTION (q_update);

	return errcode;
}

int BardataCalculator::update_rsi_slowk_slowd_rank_approx(const QSqlDatabase &database, int start_rowid)
{
	const QString _start_rowid = QString::number(start_rowid - 2); // TODO:
	QSqlQuery q(database);
	QSqlQuery qs(database);
	QSqlQuery qu(database);
	double rsi_rank, slowk_rank, slowd_rank;
	int errcode = 0;

	q.setForwardOnly(true);
	qs.setForwardOnly(true);
	qs.exec("PRAGMA temp_store = MEMORY;");
	qs.exec("PRAGMA cache_size = 10000;");

	BEGIN_TRANSACTION (qu);
	qu.prepare("update bardata set rsi_rank=?, slowk_rank=?, slowd_rank=? where rowid=?");
	q.exec("select rowid, rsi, slowk, slowd from bardata where rowid >" + _start_rowid);

	QUERY_ERROR_CODE (q, errcode, "update rsi slowk slowd(1)");

	while (q.next())
	{
		qs.exec("select rsi_rank from bardata"
						" where rowid <=" + _start_rowid +
						" and rsi <=" + q.value(1).toString() +
						" and rsi_rank > 0" +
						" order by rsi desc limit 1");

		QUERY_ERROR_CODE (qs, errcode, "");

		rsi_rank = qs.next()? qs.value(0).toDouble() : 0.0;

		qs.exec("select slowk_rank from bardata"
						" where rowid <=" + _start_rowid +
						" and slowk <=" + q.value(2).toString() +
						" and slowk_rank > 0" +
						" order by slowk desc limit 1");

		QUERY_ERROR_CODE (qs, errcode, "");

		slowk_rank = qs.next()? qs.value(0).toDouble() : 0.0;

		qs.exec("select slowd_rank from bardata"
						" where rowid <=" + _start_rowid +
						" and slowd <=" + q.value(3).toString() +
						" and slowd_rank > 0" +
						" order by slowd desc limit 1");

		QUERY_ERROR_CODE (qs, errcode, "");

		slowd_rank = qs.next()? qs.value(0).toDouble() : 0.0;

		if (errcode) break;

		qu.bindValue(0, rsi_rank);
		qu.bindValue(1, slowk_rank);
		qu.bindValue(2, slowd_rank);
		qu.bindValue(3, q.value(0));
		qu.exec();

		QUERY_ERROR_CODE (qu, errcode, "update rsi,slowk,slowd rank");
	}

	END_TRANSACTION (qu);

	return errcode;
}

int BardataCalculator::update_rate_of_change_approx(const QSqlDatabase &database, int length, int start_rowid)
{
	const QString _start_rowid = QString::number(start_rowid - 2); // TODO:
	QSqlQuery q(database);
	QSqlQuery q_select(database);
	QSqlQuery q_update(database);
	QString _roc;
	double _rank = 0.0;
	int errcode = 0;

	q_select.setForwardOnly(true);
	q_select.exec("PRAGMA busy_timeout = 2000;");

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("PRAGMA busy_timeout = 1000;");
	q.exec("select a.rowid, (((a.close_/b.close_)-1)*100.0), a.completed"
				 " from (select rowid, close_, completed from bardata where rowid >" + QString::number(start_rowid) + ") a "
				 " join (select rowid, close_ from bardata where rowid >" + QString::number(start_rowid-length) + ") b on a.rowid=b.rowid+" + QString::number(length) +
				 " where a.rowid >" + QString::number(start_rowid));

	QUERY_ERROR_CODE (q, errcode, "roc approx");

	BEGIN_TRANSACTION (q_update);

	q_update.prepare("update bardata set roc=?, roc_rank=? where rowid=?");

	while (q.next())
	{
		if (q.value(0).toInt() > start_rowid)
		{
			_roc = QString::number(q.value(1).toDouble(),'f',4);
			_rank = 0.0;

#if DISABLE_INCOMPLETE_HISTOGRAM
			if (q.value(2).toInt() != CompleteStatus::INCOMPLETE_PENDING &&
					q.value(2).toInt() != CompleteStatus::INCOMPLETE_DUMMY &&
					q.value(2).toInt() != CompleteStatus::INCOMPLETE)
#endif
			{
				if (q.value(1).toDouble() >= 0)
				{
					q_select.exec("select roc_rank from bardata where roc >= 0 and roc <=" + _roc +
												" and rowid <=" + _start_rowid + " order by roc desc limit 1");
				}
				else
				{
					q_select.exec("select roc_rank from bardata where roc < 0 and roc >=" + _roc +
												" and rowid <=" + _start_rowid + " order by roc asc limit 1");
				}

				QUERY_ERROR_CODE (q_select, errcode, "update rate of change");

				_rank = q_select.next() ? q_select.value(0).toDouble() : 0.0;
			}

			q_update.bindValue(0, _roc);
			q_update.bindValue(1, _rank);
			q_update.bindValue(2, q.value(0).toInt());
			q_update.exec();

			QUERY_ERROR_CODE (q_update, errcode, "");

			if (errcode) break;
		}
	}

	END_TRANSACTION (q_update);

	return errcode;
}

/*void BardataCalculator::update_rate_of_change_approx_v2(std::vector<BardataTuple> *out, const QSqlDatabase &database, int length, int start_rowid)
{
	const QString _start_rowid = QString::number(start_rowid);
	const int out_length = static_cast<int>(out->size());

	QSqlQuery q (database);
	QSqlQuery q_select (database);
	QString rate_of_change;

	q_select.setForwardOnly (true);
//  q.exec ("select a.rowid,(((a.close_/b.close_)-1)*100.0)"
//          " from (select rowid, close_ from bardata where rowid>" + QString::number(start_rowid) + ") a "
//          " join (select rowid, close_ from bardata where rowid>" + QString::number(start_rowid-length) + ") b on a.rowid=b.rowid+" + QString::number(length) +
//          " where a.rowid >" + QString::number(start_rowid));

	for (int i = 0; i < out_length; ++i)
	{
		q_select.exec ("select rowid, close_ from bardata where rowid=" + QString::number((*out)[i].rowid-length));
		rate_of_change = QString::number((((*out)[i].close/q.value(1).toDouble())-1) * 100.0 ,'f',4);

		if (rate_of_change >= 0)
			q_select.exec ("select roc_rank from bardata where roc >= 0 and roc <=" + rate_of_change +
										 " and rowid <=" + _start_rowid +
										 " order by roc desc limit 1");
		else
			q_select.exec ("select roc_rank from bardata where roc < 0 and roc >=" + rate_of_change +
										 " and rowid <=" + _start_rowid +
										 " order by roc asc limit 1");

		(*out)[i].set_data (bardata::COLUMN_NAME_ROC, rate_of_change.toStdString());
		(*out)[i].set_data (bardata::COLUMN_NAME_ROC_RANK, std::to_string(q_select.next() ? q_select.value(0).toDouble() : 0));
	}
}*/

int BardataCalculator::update_ccy_rate(const QSqlDatabase &database, const QString &ccy_database, int start_rowid)
{
	static int _counter = 0;
	const QString dbtemp_alias = "dbccy" + QString::number(++_counter);

	std::vector<t_ccy> vec_ccy;
	t_ccy data;

	QSqlQuery q(database);
	QSqlQuery q_update(database);
	QString start_date = "";
	int errcode = 0;
	int start_rowid_2 = start_rowid;

	q.setForwardOnly(true);
	q.exec("attach database '" + ccy_database + "' as " + dbtemp_alias);
	q.exec("select max(rowid), date_ from bardata where ccy_rate <> 1.0");

	QUERY_ERROR_CODE (q, errcode, "update ccy rate (1)");
	if (errcode) return errcode;

	if (q.next())
	{
		start_rowid_2 = std::min(start_rowid, q.value(0).toInt());
		start_date = q.value(1).toString();
	}

	q.exec("select date_, time_, rate, prev_rate from " + dbtemp_alias + ".exchange_rate"
				 " where date_ >='" + start_date + "' order by rowid asc");

	QUERY_ERROR_CODE (q, errcode, "update ccy rate (2)");
	if (errcode) return errcode;

	while (q.next())
	{
		data.date = q.value(0).toString();
		data.time = q.value(1).toString();
		data.rate = q.value(2).toDouble();
		data.prevrate = q.value(3).toDouble();
		vec_ccy.push_back(data);
	}

	q.exec("detach database " + dbtemp_alias);

	if (!vec_ccy.empty())
	{
		BEGIN_TRANSACTION (q_update);
		q_update.prepare("update bardata set ccy_rate=? where rowid=?");

		q.exec("select rowid, date_, time_ from bardata"
					 " where rowid >" + QString::number(start_rowid_2) +
					 " order by rowid asc");

		QUERY_ERROR_CODE (q, errcode, "");
		if (errcode) return errcode;

		int curr_index = 0;

		while (q.next())
		{
			while (curr_index < vec_ccy.size() && vec_ccy[curr_index].date < q.value(1).toString())
				++curr_index;

			if (curr_index > vec_ccy.size() - 1) break;

			q_update.bindValue(0, vec_ccy[curr_index].prevrate);
			q_update.bindValue(1, q.value(0).toInt());
			q_update.exec();

			QUERY_ERROR_CODE (q_update, errcode, "");
			if (errcode) break;
		}

		END_TRANSACTION (q_update);

		// @deprecated
	//  q.exec("select a.rowid, b.prev_rate"
	//         " from (select rowid, date_ from bardata where rowid >" + QString::number(start_rowid_2) + ") a"
	//         " join " + dbtemp_alias + ".exchange_rate b on a.date_=b.date_"
	//         " where a.rowid >" + QString::number(start_rowid_2));

	//  QUERY_ERROR_CODE (q, errcode, "");

	//  while (q.next()) {
	//    q_update.bindValue(0, q.value(1).toDouble());
	//    q_update.bindValue(1, q.value(0).toInt());
	//    q_update.exec();

	//    QUERY_ERROR_CODE (q_update, errcode, "");
	//    if (errcode) break;
	//  }

	//  q.exec("select max(rowid), rate from " + dbtemp_alias + ".exchange_rate");
	//  QUERY_ERROR_CODE (q, errcode, "");

	//  double last_rate = q.next() ? q.value(1).toDouble() : 1.0;

	//  q.exec("select rowid from bardata where ccy_rate=1 and rowid >" + QString::number(start_rowid_2));
	//  QUERY_ERROR_CODE (q, errcode, "");

	//  while (q.next()) {
	//    q_update.bindValue(0, last_rate);
	//    q_update.bindValue(1, q.value(0).toInt());
	//    q_update.exec();

	//    QUERY_ERROR_CODE (q_update, errcode, "");
	//    if (errcode) break;
	//  }

	//  q.exec("detach database " + dbtemp_alias);
	}

	return errcode;
}

int BardataCalculator::update_ccy_rate_controller(const QSqlDatabase &database, const QString &database_dir,
																									const QString &symbol, const IntervalWeight interval, int start_rowid)
{
	if (symbol == "@NIY")
	{
		return update_ccy_rate(database, database_dir + "/USDJPY_" + Globals::intervalWeightToString(interval) + ".db", start_rowid);
	}
	else if (symbol == "HHI" || symbol == "HSI")
	{
		return update_ccy_rate(database, database_dir + "/USDHKD_" + Globals::intervalWeightToString(interval) + ".db", start_rowid);
	}
	else if (symbol == "IFB")
	{
		return update_ccy_rate(database, database_dir + "/USDCNH_" + Globals::intervalWeightToString(interval) + ".db", start_rowid);
	}

	return 0;
}

int BardataCalculator::update_distfscross_threshold_approx(std::vector<BardataTuple> *out,
																													 const QSqlDatabase &database,
																													 const IntervalWeight interval,
																													 double threshold,
																													 int column_id,
																													 int start_rowid)
{
	const QString _id = "_" + QString::number(column_id);
	const QString _start_rowid = QString::number(start_rowid);

	QSqlQuery q(database);
	double _fastavg, _slowavg, _close, _atr, _dist_fscross;
	double prev_close = 0;
	double fscross_close = 0;
	int fscross_start_rowid = 0;
	int prev_state = -1;
	int curr_state = -1;
	int curr_rowid = 0;
	int errcode = 0;

	// get information of previous row *before* last fs-cross
	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");

	if (threshold > 0.0)
	{
		// skip distfscross threshold 2 and so on
		return 0;

		// backup original
//    q.exec ("select rowid, close_, fastavg, slowavg from bardata where rowid >="
//            "(select max(rowid)-1 from bardata where fs_cross=1 and abs(fastavg-slowavg) >=" + QString::number(threshold) +
//            " and rowid <=" + _start_rowid + ")");

		// optimize 1: (abs checking)
//    q.exec ("select rowid, close_, fastavg, slowavg from bardata where rowid >="
//            "(select rowid from bardata where fs_cross=1 "
//            " and (distfs >=" + QString::number(threshold) + " or distfs <= -" + QString::number(threshold) + ")"
//            " and rowid <=" + _start_rowid + " order by rowid desc limit 1) - 1");

//    // optimize 1v2: (abs checking)
//    q.exec("select rowid, close_, fastavg, slowavg from bardata where rowid >="
//           "(select rowid from (select rowid, distfs from bardata where fs_cross=1) "
//           " where (distfs >=" + QString::number(threshold) + " or distfs <= -" + QString::number(threshold) + ")"
//           " and rowid <=" + _start_rowid + " order by rowid desc limit 1) - 1");
	}
	else
	{
		q.exec("select rowid, close_, fastavg, slowavg from bardata where rowid >="
					 "(select max(rowid)-1 from bardata where fs_cross=1 and rowid <=" + _start_rowid + ")");
	}

	QUERY_ERROR_CODE (q, errcode, "update distfscross threshold (1)");

	if (q.next())
	{
		fscross_start_rowid = q.value(0).toInt();
		fscross_close = q.value(1).toDouble();
		prev_close = fscross_close;
		prev_state = get_moving_avg_state(q.value(2).toDouble(), q.value(3).toDouble());
		curr_state = prev_state; // give any value except -1
	}

	// scan from last encountered dist-fscross onward
	q.exec("select rowid, fastavg, slowavg, close_, atr, prevdailyatr, completed from bardata"
				 " where rowid >" + QString::number(fscross_start_rowid) + " order by date_ asc, time_ asc");

	QUERY_ERROR_CODE (q, errcode, "update distfscross threshold (2)");

	QSqlQuery q_select(database);
	QSqlQuery q_update(database);

	BEGIN_TRANSACTION (q_update);
	q_update.prepare("update bardata set "
									 "distcc_fscross" + _id + "=?,"
									 "distcc_fscross_atr" + _id + "=?,"
									 "distcc_fscross_rank" + _id + "=?"
									 " where rowid=?");

	const int atr_index = interval >= WEIGHT_DAILY ? 4 : 5;
	const int row_count = static_cast<int>(out->size());
	double _rank;
	int _index = 0;

	while (q.next())
	{
		curr_rowid = q.value(0).toInt();
		_fastavg = q.value(1).toDouble();
		_slowavg = q.value(2).toDouble();
		_close = q.value(3).toDouble();
		_rank = 0.0;

		// if (intraday) then {use realtime daily atr} else {use its own atr}
		_atr = q.value(atr_index).toDouble();

		// try search atr from buffer for intra-day
		if (_atr == 0.0 && interval < WEIGHT_DAILY && curr_rowid > start_rowid)
		{
			for (int i = _index; i < row_count; ++i)
			{
				if ((*out)[i].rowid == curr_rowid)
				{
					_atr = (*out)[i].get_data_double(bardata::COLUMN_NAME_PREV_DAILY_ATR);
					_index = i;
					break;
				}
			}
		}

		// initial state
		if (curr_state == -1)
		{
			curr_state = get_moving_avg_state(_fastavg, _slowavg);
		}
		else
		{
			// continous state
			curr_state = get_moving_avg_state(_fastavg, _slowavg);

			// fs-cross encountered OR satisfied the threshold condition

			if ((threshold == 0 || std::fabs(_fastavg - _slowavg) >= threshold) && prev_state != curr_state)
			{
				fscross_close = prev_close;
			}

			if (fscross_close > 0.0)
			{
				// only update new records
				if (curr_rowid > start_rowid)
				{
					_dist_fscross = _close - fscross_close;

					if (_atr == 0.0)
					{
						if (q.value(6).toInt() == CompleteStatus::COMPLETE_SUCCESS ||
								q.value(6).toInt() == CompleteStatus::COMPLETE_PENDING)
						{
							qDebug("update distfscross threshold: atr is zero");
						}

						_atr = 1.0;
					}

#if DISABLE_INCOMPLETE_HISTOGRAM
					if (q.value(6).toInt() != CompleteStatus::INCOMPLETE_PENDING &&
							q.value(6).toInt() != CompleteStatus::INCOMPLETE_DUMMY &&
							q.value(6).toInt() != CompleteStatus::INCOMPLETE)
#endif
					{
						if (_dist_fscross > 0)
						{
							// positive rank
							q_select.exec("select distcc_fscross_rank" + _id + " from bardata"
														" where rowid <=" + _start_rowid + " and"
														" distcc_fscross_atr" + _id + "> 0 and"
														" distcc_fscross_atr" + _id + "<=" + QString::number(_dist_fscross/_atr,'f') +
														" order by distcc_fscross_atr" + _id + " desc limit 1");
						}
						else
						{
							// negative rank
							q_select.exec("select distcc_fscross_rank" + _id + " from bardata"
														" where rowid <=" + _start_rowid + " and"
														" distcc_fscross_atr" + _id + "< 0 and"
														" distcc_fscross_atr" + _id + ">=" + QString::number(_dist_fscross/_atr,'f') +
														" order by distcc_fscross_atr" + _id + " asc limit 1");
						}

						QUERY_ERROR_CODE (q_select, errcode, "");

						if (errcode) break;

						_rank = q_select.next()? q_select.value(0).toDouble() : 0.0;
					}

					q_update.bindValue(0, QString::number(_dist_fscross,'f',4));
					q_update.bindValue(1, QString::number(_atr != 0.0 ? _dist_fscross/_atr : 0.0,'f',4));
					q_update.bindValue(2, QString::number(_rank,'f',4));
					q_update.bindValue(3, curr_rowid);
					q_update.exec();

					QUERY_ERROR_CODE (q_update, errcode, "");
					if (errcode) break;
				}
			}
			else
			{
				// only update new records
				if (q.value(0).toInt() > start_rowid)
				{
					q_update.bindValue(0, 0);
					q_update.bindValue(1, 0);
					q_update.bindValue(2, 0);
					q_update.bindValue(3, q.value(0).toInt());
					q_update.exec();

					QUERY_ERROR_CODE (q_update, errcode, "");
				}
			}
		}

		prev_close = _close;
		prev_state = curr_state;
	}

	END_TRANSACTION (q_update);

	return errcode;
}

// @experimental
void reformat_date(char *out_date, const char *in_date)
{
	// reformat datetime input string: from MM/dd/yyyy into yyyy-MM-dd
	// in case the format is incorrect it will return empty string

	// routine NULL checking
	if (out_date == NULL || in_date == NULL)
	{
		return;
	}

	char _year[5], _month[3], _day[3];
	char copy_date[11];

	// initialize
	strcpy_s(_year, sizeof _year, "");
	strcpy_s(_month, sizeof _month, "");
	strcpy_s(_day, sizeof _day, "");

	// discard previous value from out_date
	strcpy_s(out_date, sizeof out_date, "");

	// copy input date into local variable so it won't modified original source
	strcpy_s(copy_date, sizeof copy_date, in_date);

	char *token = strtok(copy_date, "/");

	// month
	if (token != NULL)
	{
		if (strlen(token) < 3)
		{
			strcpy_s(_month, sizeof _month, token);
		}

		// day
		token = strtok(NULL, "/");

		if (token != NULL && strlen(token) < 3)
		{
			strcpy_s(_day, sizeof _day, token);

			// year
			token = strtok(NULL, "/");

			if (token != NULL && strlen(token) < 5)
			{
				strcpy_s(_year, sizeof _year, token);
			}
		}
	}

	if (strlen(_year) > 0)
	{
		strcat_s(out_date, sizeof out_date, _year);
		strcat_s(out_date, sizeof out_date, "-");
		strcat_s(out_date, sizeof out_date, _month);
		strcat_s(out_date, sizeof out_date, "-");
		strcat_s(out_date, sizeof out_date, _day);
	}
}

// @experimental: parsing using native c
bool read_file(const char filename[], t_ohlc *out_data, char *lastdatetime)
{
	// validate parameters
	if (strlen(filename) == 0 || out_data == NULL || lastdatetime == NULL)
	{
		return false;
	}

	FILE *fsource = fopen(filename, "r");

	if (fsource != NULL)
	{
		char buff[256];
		char date_input[11];
		t_ohlc r;

		// omit first which is column label line
		fscanf(fsource, "%[^\n]\n", buff);

		while (!feof(fsource) && fscanf(fsource, "%[^,],%[^,],%lf,%lf,%lf,%lf,%[^\n]\n",
						r.date, r.time, &r.open, &r.high, &r.low, &r.close, buff) == 7)
		{
			// reformat datetime input into yyyy-MM-dd hh:mm
			reformat_date(date_input, r.date);
			strcat_s(date_input, sizeof date_input, " ");
			strcat_s(date_input, sizeof date_input, r.time);

			// find for new data later than lastdatetime
			if (strcmp(date_input, lastdatetime) > 0)
			{
				strcpy_s(lastdatetime, sizeof lastdatetime, date_input);

				// open price should be assign for one time only, which is during market open
				if (out_data->open == 0.0)
				{
					out_data->open = r.open;
				}

				out_data->high = std::fmax(r.high, out_data->high);
				out_data->low = out_data->low > 0.0 ? std::fmin(r.low, out_data->low) : r.low;
				out_data->close = r.close;
			}
		}

		// make sure fsource is not NULL
		// attempt to fclose(NULL) will lead to crash
		fclose(fsource);

		return true;
	}

	return false;
}

bool BardataCalculator::create_incomplete_bar(const QSqlDatabase &database,
																							const QString &database_dir,
																							const QString &input_dir,        // input txt directory
																							const QString &symbol,           // current symbol
																							const IntervalWeight interval,   // current timeframe
																							const QDateTime &lastdatetime,   // last complete datetime of current timeframe
																							BardataTuple *out,               // result of incomplete bar (for insert into m_cache)
																							BardataTuple &old,
																							Logger _logger)
{
	// calculate basic indicator (replicate from fileloader):
	// OHLC, macd, macdavg, macddiff, rsi, slowk, slowd, fastavg, slowavg
	// dist OHLC x Fast/Slow

	// create incomplete bar bypassing requirement to query database
	// instead we use raw text file to generate OHLC by searching OHLC
	// with datetime greater than last complete datetime in base database

	// mapping data to create incomplete
	// Monthly, Weekly  -->  Daily (TXT) + 5min (TXT)
	// Daily, 60min     -->  5min (TXT)

	if (interval < WEIGHT_60MIN || out == NULL || lastdatetime.isNull())
		return false;

	if (database.isOpenError())
	{
		LOG_DEBUG ("{}: create incomplete error: database not opened", symbol.toStdString());
		return false;
	}

	QDateTime lastdatetime_5min;
	t_ohlc data;

	strcpy(data.date, "");
	strcpy(data.time, "");
	data.open = 0.0;
	data.high = 0.0;
	data.low = 0.0;
	data.close = 0.0;

	LOG_DEBUG ("CREATE incomplete bar:: {} {} {} {}",
						 input_dir.toStdString().c_str(),
						 symbol.toStdString().c_str(),
						 Globals::intervalWeightToStdString(interval).c_str(),
						 lastdatetime.toString("yyyy-MM-dd hh:mm").toStdString().c_str());

	// V1: by raw text files -- very fast
	if (interval >= WEIGHT_WEEKLY)
	{
		QStringList line;
		QDateTime _datetime, daily_lastdatetime;
		QString ln;

//    try
//    {
//      // (since v1.6.8) new algorithm, using native FILE handler to read text file
//      // for Monthly and Weekly, we create incomplete bar using Daily and 5min
//
//      char c_lastdatetime[17];
//      strcpy(c_lastdatetime, lastdatetime.toString("yyyy-MM-dd hh:mm").toStdString().c_str());
//
//      if (!read_file((input_dir + "/" + symbol + "_Daily.txt").toStdString().c_str(), &data, c_lastdatetime))
//      {
//        return false;
//      }
//      if (!read_file((input_dir + "/" + symbol + "_5min.txt").toStdString().c_str(), &data, c_lastdatetime))
//      {
//        return false;
//      }
//
//      lastdatetime_5min = QDateTime::fromString(QString::fromLocal8Bit(c_lastdatetime), "yyyy-MM-dd hh:mm");
//    }
//    catch (const std::exception &ex)
//    {
//      LOG_DEBUG(ex.what());
//    }
//    catch (...)
//    {
//      LOG_DEBUG("create incomplete bar error: unhandled exception");
//    }

		try
		{
			QFile fdaily(input_dir + "/" + symbol + "_Daily.txt");

			if (fdaily.open(QFile::ReadWrite))
//			if (fdaily.open(QFile::ReadOnly))
			{
				QTextStream stream(&fdaily);

				// skip first line (data label)
				if (stream.readLineInto(NULL))
				{
					while (!stream.atEnd() && stream.readLineInto(&ln))
					{
						line = ln.split(",", QString::KeepEmptyParts);

						if (line.length() >= 6)
						{
							_datetime = QDateTime::fromString(line[0] + " " + line[1], "MM/dd/yyyy hh:mm");

							if (_datetime > lastdatetime)
							{
								if (data.open == 0) data.open = line[2].toDouble(); // once
								data.high = std::fmax(line[3].toDouble(), data.high);
								data.low = data.low > 0 ? std::fmin(line[4].toDouble(), data.low) : line[4].toDouble();
								data.close = line[5].toDouble();
								daily_lastdatetime = _datetime;
							}
						}
					}
				}

				fdaily.close();
			}

			QFile f5min(input_dir + "/" + symbol + "_5min.txt");

			if (f5min.open(QFile::ReadWrite))
//			if (f5min.open(QFile::ReadOnly))
			{
				QTextStream stream(&f5min);

				// skip first line (data label)
				if (stream.readLineInto(NULL))
				{
					while (!stream.atEnd() && stream.readLineInto(&ln))
					{
						line = ln.split(",", QString::KeepEmptyParts);

						if (line.length() >= 6)
						{
							_datetime = QDateTime::fromString(line[0] + " " + line[1], "MM/dd/yyyy hh:mm");

							if (_datetime > daily_lastdatetime)
							{
								if (data.open == 0) data.open = line[2].toDouble(); // once
								data.high = std::fmax(line[3].toDouble(), data.high);
								data.low = data.low > 0 ? std::fmin(line[4].toDouble(), data.low) : line[4].toDouble();
								data.close = line[5].toDouble();
								lastdatetime_5min = _datetime;
							}
						}
					}
				}

				f5min.close();
			}
		}
		catch (const std::exception &){}
		catch (...) { LOG_DEBUG("create incomplete: unhandled exception(1)"); }
	}
	else if (interval >= WEIGHT_60MIN)
	{
		QFile file(input_dir + "/" + symbol + "_5min.txt");
		QDateTime _datetime;
		QStringList line;

		try
		{
			if (file.open(QFile::ReadWrite))
//			if (file.open(QFile::ReadOnly))
			{
				QTextStream stream(&file);
				QString ln;

				// skip first line (data label)
				if (stream.readLineInto(NULL))
				{
					while (!stream.atEnd() && stream.readLineInto(&ln))
					{
						line = ln.split(",");
						if (line.length() < 6) break;

						_datetime = QDateTime::fromString(line[0] + " " + line[1], "MM/dd/yyyy hh:mm");

						if (_datetime > lastdatetime)
						{
							if (data.open == 0) data.open = line[2].toDouble(); // once
							data.high = std::fmax(line[3].toDouble(), data.high);
							data.low = data.low > 0 ? std::fmin(line[4].toDouble(), data.low) : line[4].toDouble();
							data.close = line[5].toDouble();
							lastdatetime_5min = _datetime;
						}
					}
				}

				file.close();
			}
		}
		catch (const std::exception &){}
		catch (...) { LOG_DEBUG("create incomplete: unhandled exception(2)"); }
	}

	// V2: by database -- slow
	/*if (interval >= WEIGHT_WEEKLY)
	{
		QDateTime daily_lastdatetime;
		QDateTime _datetime;

		// daily
		{
			QSqlDatabase dbdaily;
			SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_Daily.db", &dbdaily);

			QSqlQuery qdaily(dbdaily);
			qdaily.setForwardOnly(true);
			qdaily.exec("PRAGMA temp_store = MEMORY;");
			qdaily.exec("PRAGMA cache_size = 10000;");
			qdaily.exec("PRAGMA query_only = true;");
			qdaily.exec("select date_, time_, open_, high_, low_, close_"
									" from bardata where date_>'" + lastdatetime.toString("yyyy-MM-dd") + "'"
									" order by rowid asc");

			while (qdaily.next())
			{
				_datetime = QDateTime::fromString(qdaily.value(0).toString() + " " + qdaily.value(1).toString(),
																					"yyyy-MM-dd hh:mm");

				if (_datetime > lastdatetime)
				{
					if (data.open == 0.0) data.open = qdaily.value(2).toDouble(); // once
					data.high = std::fmax(qdaily.value(3).toDouble(), data.high);
					data.low = data.low > 0 ? std::fmin(qdaily.value(4).toDouble(), data.low) : qdaily.value(4).toDouble();
					data.close = qdaily.value(5).toDouble();
					daily_lastdatetime = _datetime;
				}
			}

			SQLiteHandler::removeDatabase(&dbdaily);
		}

		// 5min
		if (!daily_lastdatetime.isNull())
		{
			QSqlDatabase db5min;
			SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_5min.db", &db5min);

			QSqlQuery q5min(db5min);
			QString daily_datetime = daily_lastdatetime.toString("yyyy-MM-dd");

			q5min.setForwardOnly(true);
			q5min.exec("PRAGMA temp_store = MEMORY;");
			q5min.exec("PRAGMA cache_size = 10000;");
			q5min.exec("PRAGMA query_only = true;");
			q5min.exec("select date_, time_, open_, high_, low_, close_"
								 " from bardata where date_>'" + daily_datetime + "' or "
								 " (date_='" + daily_datetime + "' and time_>'" + daily_lastdatetime.toString("hh:mm") + "')"
								 " order by rowid asc");

			while (q5min.next())
			{
				_datetime = QDateTime::fromString(q5min.value(0).toString() + " " + q5min.value(1).toString(),
																					"yyyy-MM-dd hh:mm");

				if (_datetime > daily_lastdatetime)
				{
					if (data.open == 0.0) data.open = q5min.value(2).toDouble(); // once
					data.high = std::fmax(q5min.value(3).toDouble(), data.high);
					data.low = data.low > 0 ? std::fmin(q5min.value(4).toDouble(), data.low) : q5min.value(4).toDouble();
					data.close = q5min.value(5).toDouble();
					lastdatetime_5min = _datetime;
				}
			}

			SQLiteHandler::removeDatabase(&db5min);
		}
	}
	else if (interval >= WEIGHT_60MIN)
	{
		// 5min
		QSqlDatabase db5min;
		SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_5min.db", &db5min);

		QSqlQuery q5min(db5min);
		QString ldate = lastdatetime.toString("yyyy-MM-dd");
		QDateTime _datetime;

		q5min.setForwardOnly(true);
		q5min.exec("select date_, time_, open_, high_, low_, close_"
							 " from bardata where date_>'" + ldate + "' or "
							 " (date_='" + ldate + "' and time_>'" + lastdatetime.toString("hh:mm") + "')"
							 " order by rowid asc");

		while (q5min.next())
		{
			_datetime = QDateTime::fromString(q5min.value(0).toString() + " " + q5min.value(1).toString(),
																				"yyyy-MM-dd hh:mm");

			if (_datetime > lastdatetime)
			{
				if (data.open == 0.0) data.open = q5min.value(2).toDouble(); // once
				data.high = std::fmax(q5min.value(3).toDouble(), data.high);
				data.low = data.low > 0 ? std::fmin(q5min.value(4).toDouble(), data.low) : q5min.value(4).toDouble();
				data.close = q5min.value(5).toDouble();
				lastdatetime_5min = _datetime;
			}
		}

		SQLiteHandler::removeDatabase(&db5min);
	}

	*/

#ifdef DEBUG_MODE
	qDebug("last datetime (%s): %s", symbol.toStdString().c_str(), lastdatetime.toString("yyyy-MM-dd hh:mm").toStdString().c_str());
	qDebug("last datetime (%s 5min): %s", symbol.toStdString().c_str(), lastdatetime_5min.toString("yyyy-MM-dd hh:mm").toStdString().c_str());
	qDebug("incomplete ohlc (%s): %.2f %.2f %.2f %.2f", symbol.toStdString().c_str(), data.open, data.high, data.low, data.close);
#endif

	LOG_DEBUG ("{} {}: incomplete bar OHLC: {} {} {} {} / {} / {}",
						 symbol.toStdString().c_str(), Globals::intervalWeightToStdString(interval).c_str(),
						 data.open, data.high, data.low, data.close,
						 lastdatetime.toString("yyyy-MM-dd hh:mm").toStdString().c_str(),
						 lastdatetime_5min.toString("yyyy-MM-dd hh:mm").toStdString().c_str());

	if (lastdatetime_5min.isNull())
	{
		qDebug("incomplete bar OHLC: lastdatetime 5min is null");
		LOG_DEBUG ("{} {}: incomplete bar OHLC: lastdatetime 5min is null (error OR current time = 5min datetime)",
							 symbol.toStdString().c_str(), Globals::intervalWeightToStdString(interval).c_str());
		return false;
	}

	if (data.open > 0.0)
	{
		out->rowid = 0;
		out->date = "";
		out->time = "";
		out->open = data.open;
		out->high = data.high;
		out->low = data.low;
		out->close = data.close;

		QSqlQuery q(database);
		QString last_incomplete_date = "";
		QString last_incomplete_time = "";
		int last_incomplete_rowid = 0;
		bool is_update = true;

		q.setForwardOnly(true);
		q.exec("PRAGMA busy_timeout = 3000;");
		q.exec("PRAGMA temp_store = MEMORY;");
		q.exec("PRAGMA cache_size = 10000;");
		q.exec("PRAGMA journal_mode = OFF;");
		q.exec("PRAGMA synchronous = OFF;");
		q.exec("select rowid, date_, time_ from bardata where completed=0");

		if (q.next())
		{
			last_incomplete_rowid = q.value(0).toInt();
			last_incomplete_date = q.value(1).toString();
			last_incomplete_time = q.value(2).toString();

			out->rowid = last_incomplete_rowid;
			out->date = last_incomplete_date.toStdString();
			out->time = last_incomplete_time.toStdString();
		}

		// generate incomplete datetime, if incomplete not exists yet
		if (last_incomplete_rowid == 0)
		{
			if (interval == WEIGHT_MONTHLY)
			{
				out->date = DateTimeHelper::next_monthly_date(lastdatetime.toString("yyyy-MM-dd").toStdString(), true);
				out->time = lastdatetime.toString("hh:mm").toStdString();
			}
			else if (interval == WEIGHT_WEEKLY)
			{
				std::string base_curr_date = lastdatetime_5min.toString("yyyy-MM-dd").toStdString();
				out->date = DateTimeHelper::next_weekly_date(lastdatetime.toString("yyyy-MM-dd").toStdString(), true);
				out->time = lastdatetime.toString("hh:mm").toStdString();

				while (!out->date.empty() && out->date < base_curr_date)
				{
					out->date = DateTimeHelper::next_weekly_date(out->date, true);
				}
			}
			else if (interval == WEIGHT_DAILY)
			{
				std::string base_curr_date = lastdatetime_5min.toString("yyyy-MM-dd").toStdString();
				out->date = DateTimeHelper::next_daily_date(lastdatetime.toString("yyyy-MM-dd").toStdString());
				out->time = lastdatetime.toString("hh:mm").toStdString();

				while (!out->date.empty() && out->date < base_curr_date)
				{
					out->date = DateTimeHelper::next_daily_date(out->date);
				}
			}
			else if (interval == WEIGHT_60MIN)
			{
				std::string base_curr_date = lastdatetime_5min.toString("yyyy-MM-dd").toStdString();
				std::string base_curr_time = lastdatetime_5min.toString("hh:mm").toStdString();
				std::string _timestring = DateTimeHelper::next_intraday_datetime(lastdatetime.toString("yyyy-MM-dd hh:mm").toStdString(), interval);

				if (_timestring.length() >= 16)
				{
					out->date = _timestring.substr(0, 10);
					out->time = _timestring.substr(11, 5);

					while (!_timestring.empty() && (base_curr_date > out->date ||
								 (base_curr_date == out->date && base_curr_time > out->time)))
					{
						_timestring = DateTimeHelper::next_intraday_datetime(_timestring, interval);

						if (_timestring.length() >= 16)
						{
							out->date = _timestring.substr(0, 10);
							out->time = _timestring.substr(11, 5);
						}
					}
				}
			}

			if (out->date.empty())
			{
				// failed to generate incomplete bar datetime
				return false;
			}

			// we will doing INSERT later so set update_flag to FALSE
			is_update = false;
		}

//		if (out->completed != CompleteStatus::INCOMPLETE_DUMMY)
		{
			LOG_DEBUG ("{} {}: incomplete bar-calculate ATR",
								 symbol.toStdString().c_str(), Globals::intervalWeightToStdString(interval).c_str());

			// calculate ATR from current timeframe TXT
			{
				out->atr = 0.0;

				QFile file(input_dir + "/" + symbol + "_" + Globals::intervalWeightToString(interval) + ".txt");

				try
				{
					if (file.open(QFile::ReadWrite))
//					if (file.open(QFile::ReadOnly))
					{
						std::list<double> tr_queue;
						QTextStream stream(&file);
						QStringList line;
						QString ln;
						double prev_close = 0.0;
						double _high, _low, tr_1, tr_2, tr_3;

						stream.readLineInto(NULL); // skip first line (label data)

						while (!stream.atEnd() && stream.readLineInto(&ln))
						{
							line = ln.split(",", QString::KeepEmptyParts);

							if (line.size() > 5)
							{
								_high = line[3].toDouble();
								_low = line[4].toDouble();

								tr_1 = _high - _low;

								if (prev_close > 0.0)
								{
									tr_2 = std::fabs(_high - prev_close);
									tr_3 = std::fabs(_low - prev_close);
									tr_queue.push_back(std::fmax(tr_1, std::fmax(tr_2, tr_3)));
								}
								else
								{
									tr_queue.push_back(tr_1);
								}

								if (tr_queue.size() > 14)
								{
									tr_queue.pop_front();
								}

								prev_close = line[5].toDouble();
							}
						}

						// include current incomplete bar into calculation
						if (prev_close > 0.0)
						{
							tr_1 = out->high - out->low;
							tr_2 = std::fabs(out->high - prev_close);
							tr_3 = std::fabs(out->low - prev_close);
							tr_queue.push_back(std::fmax(tr_1, std::fmax(tr_2, tr_3)));

							if (tr_queue.size() > 14)
							{
								tr_queue.pop_front();
							}

							out->atr = std::accumulate(tr_queue.begin(), tr_queue.end(), 0.0) / 14.0;
						}
						else
						{
							out->atr = out->high - out->low;
						}
					}

					file.close();
				}
				catch (const std::exception &){}
				catch (...)
				{
					LOG_DEBUG("incomplete bar:unhandle exception(3)");
				}

				if (out->atr == 0.0)
				{
					LOG_DEBUG ("{} {}: error incomplete atr is zero",
										 symbol.toStdString().c_str(), Globals::intervalWeightToStdString(interval).c_str());
				}
			}

//      Globals::dbupdater_info _data = Globals::get_last_indicator_for_dbupdater
//        (symbol.toStdString() + "_" + Globals::intervalWeightToStdString(interval) + ".db");
			Globals::dbupdater_info _data;
			int is_ok = Globals::get_last_indicator_for_dbupdater_v2
				(&_data, symbol.toStdString() + "_" + Globals::intervalWeightToStdString(interval) + ".db");

			if (!is_ok)
			{
				LOG_DEBUG ("{} {}: incomplete bar error in get_last_indicator_for_dbupdater_v2()",
									 symbol.toStdString().c_str(), Globals::intervalWeightToStdString(interval).c_str());
				return false;
			}

			// for RSI calculation
			out->prev_close = old.close;
			continous_calculation_v2(&_data, out);
			continous_calculation(old, out, interval < WEIGHT_DAILY);
		}

		if (is_update)
		{
			q.exec("update bardata set"
						 " open_=" + QString::number(out->open) +
						 ",high_=" + QString::number(out->high) +
						 ",low_=" + QString::number(out->low) +
						 ",close_=" + QString::number(out->close) +
						 ",bar_range=" + QString::number(out->bar_range) +
						 ",intraday_high=" + QString::number(out->intraday_high) +
						 ",intraday_low=" + QString::number(out->intraday_low) +
						 ",intraday_range=" + QString::number(out->intraday_range) +
						 ",atr=" + QString::number(out->atr,'f') +
						 ",macd=" + QString::number(out->macd,'f') +
						 ",rsi=" + QString::number(out->rsi,'f') +
						 ",slowk=" + QString::number(out->slow_k,'f') +
						 ",slowd=" + QString::number(out->slow_d,'f') +
						 ",fastavg=" + QString::number(out->fast_avg,'f') +
						 ",slowavg=" + QString::number(out->slow_avg,'f') +
						 ",fastavgslope=" + QString::number(out->fastavg_slope,'f') +
						 ",slowavgslope=" + QString::number(out->slowavg_slope,'f') +
						 ",candle_uptail=" + QString::number(out->candle_uptail,'f') +
						 ",candle_downtail=" + QString::number(out->candle_downtail,'f') +
						 ",candle_body=" + QString::number(out->candle_body,'f') +
						 ",candle_totallength=" + QString::number(out->candle_totallength,'f') +
						 ",distof=" + QString::number(out->dist_of,'f') +
						 ",n_distof=" + QString::number(out->n_dist_of,'f') +
						 ",distos=" + QString::number(out->dist_os,'f') +
						 ",n_distos=" + QString::number(out->n_dist_os,'f') +
						 ",disthf=" + QString::number(out->dist_hf,'f') +
						 ",n_disthf=" + QString::number(out->n_dist_hf,'f') +
						 ",disths=" + QString::number(out->dist_hs,'f') +
						 ",n_disths=" + QString::number(out->n_dist_hs,'f') +
						 ",distlf=" + QString::number(out->dist_lf,'f') +
						 ",n_distlf=" + QString::number(out->n_dist_lf,'f') +
						 ",distls=" + QString::number(out->dist_ls,'f') +
						 ",n_distls=" + QString::number(out->n_dist_ls,'f') +
						 ",distcf=" + QString::number(out->dist_cf,'f') +
						 ",n_distcf=" + QString::number(out->n_dist_cf,'f') +
						 ",distcs=" + QString::number(out->dist_cs,'f') +
						 ",n_distcs=" + QString::number(out->n_dist_cs,'f') +
						 ",completed=" + QString::number(CompleteStatus::INCOMPLETE_PENDING) +
						 " where rowid=" + QString::number(out->rowid));

			if (q.lastError().isValid())
			{
				LOG_DEBUG ("incomplete(update) error: {}", q.lastError().text().toStdString().c_str());
			}

			LOG_DEBUG (q.lastQuery().toStdString().c_str());
		}
		else
		{
			// _parent & _parent_prev
			{
				IntervalWeight _superparent = Globals::getParentInterval(interval);

				if (_superparent != WEIGHT_INVALID)
				{
					QSqlDatabase superdb;
					SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_" + Globals::intervalWeightToString(_superparent) + ".db", &superdb);

					if (superdb.isOpen())
					{
						QString _date = QString::fromStdString(out->date);
						QSqlQuery qs(superdb);
						qs.setForwardOnly(true);
						qs.exec("select rowid from bardata"
										" where date_ >'" + _date + "' or"
										" (date_='" + _date + "' and time_>='" + QString::fromStdString(out->time) + "')"
										" order by rowid asc limit 1");

						if (qs.next())
						{
							out->_parent = qs.value(0).toInt();
							out->_parent_prev = out->_parent > 0 ? out->_parent - 1 : 0;
						}
						else
						{
							LOG_DEBUG ("{}: create incomplete error: {}", symbol.toStdString(), qs.lastError().text().toStdString());
						}

						SQLiteHandler::removeDatabase(&superdb);
					}
				}
			}

			q.exec("insert into bardata("
						 "date_, time_, open_, high_, low_, close_"
						 ",bar_range, intraday_high, intraday_low, intraday_range"
						 ",macd, rsi, slowk, slowd, fastavg, slowavg, atr"
						 ",fastavgslope, slowavgslope"
						 ",candle_uptail"
						 ",candle_downtail"
						 ",candle_body"
						 ",candle_totallength"
						 ",distof, n_distof, distos, n_distos"
						 ",disthf, n_disthf, disths, n_disths"
						 ",distlf, n_distlf, distls, n_distls"
						 ",distcf, n_distcf, distcs, n_distcs"
						 ",distfs, n_distfs"
						 ",prevdate, prevtime, prevbarcolor"
						 ",fcross, scross"
						 ",_parent, _parent_prev, completed) values("
						 "'" + QString::fromStdString(out->date) + "'"
						 ",'" + QString::fromStdString(out->time) + "'"
						 "," + QString::number(out->open) +
						 "," + QString::number(out->high) +
						 "," + QString::number(out->low) +
						 "," + QString::number(out->close) +
						 "," + QString::number(out->bar_range) +
						 "," + QString::number(out->intraday_high) +
						 "," + QString::number(out->intraday_low) +
						 "," + QString::number(out->intraday_range) +
						 "," + QString::number(out->macd,'f') +
						 "," + QString::number(out->rsi,'f') +
						 "," + QString::number(out->slow_k,'f') +
						 "," + QString::number(out->slow_d,'f') +
						 "," + QString::number(out->fast_avg,'f') +
						 "," + QString::number(out->slow_avg,'f') +
						 "," + QString::number(out->atr,'f') +
						 "," + QString::number(out->fastavg_slope,'f') +
						 "," + QString::number(out->slowavg_slope,'f') +
						 "," + QString::number(out->candle_uptail,'f') +
						 "," + QString::number(out->candle_downtail,'f') +
						 "," + QString::number(out->candle_body,'f') +
						 "," + QString::number(out->candle_totallength,'f') +
						 "," + QString::number(out->dist_of,'f') +
						 "," + QString::number(out->n_dist_of,'f') +
						 "," + QString::number(out->dist_os,'f') +
						 "," + QString::number(out->n_dist_os,'f') +
						 "," + QString::number(out->dist_hf,'f') +
						 "," + QString::number(out->n_dist_hf,'f') +
						 "," + QString::number(out->dist_hs,'f') +
						 "," + QString::number(out->n_dist_hs,'f') +
						 "," + QString::number(out->dist_lf,'f') +
						 "," + QString::number(out->n_dist_lf,'f') +
						 "," + QString::number(out->dist_ls,'f') +
						 "," + QString::number(out->n_dist_ls,'f') +
						 "," + QString::number(out->dist_cf,'f') +
						 "," + QString::number(out->n_dist_cf,'f') +
						 "," + QString::number(out->dist_cs,'f') +
						 "," + QString::number(out->n_dist_cs,'f') +
						 "," + QString::number(out->dist_fs,'f') +
						 "," + QString::number(out->n_dist_fs,'f') +
						 ",'" + lastdatetime.toString("yyyy-MM-dd") + "'"
						 ",'" + lastdatetime.toString("hh:mm") + "'"
						 ",'" + QString::fromStdString(out->prevbarcolor) + "'"
						 "," + QString::number(out->f_cross) +
						 "," + QString::number(out->s_cross) +
						 "," + QString::number(out->_parent) +
						 "," + QString::number(out->_parent_prev) +
						 "," + QString::number(CompleteStatus::INCOMPLETE_PENDING) +
						 ")");

			if (q.lastError().isValid())
			{
				qDebug("incomplete(insert) error: %s", q.lastError().text().toStdString().c_str());
				LOG_DEBUG ("incomplete(insert) error: {}", q.lastError().text().toStdString().c_str());
			}

			LOG_DEBUG (q.lastQuery().toStdString().c_str());

//			q.exec("select last_insert_rowid()");
			q.exec("select rowid from bardata where date_='" + QString::fromStdString(out->date) + "'"
						 " and time_='" + QString::fromStdString(out->time) + "'");

			out->rowid = q.next() ? q.value(0).toInt() : 0;

			if (out->rowid == 0)
			{
				LOG_DEBUG("warning:incomplete insert rowid=0");
			}
		}

		out->set_data(bardata::COLUMN_NAME_MACD, STRINGNUMBER(out->macd, 6));
		out->set_data(bardata::COLUMN_NAME_RSI, STRINGNUMBER(out->rsi, 2));
		out->set_data(bardata::COLUMN_NAME_SLOWK, STRINGNUMBER(out->slow_k, 2));
		out->set_data(bardata::COLUMN_NAME_SLOWD, STRINGNUMBER(out->slow_d, 2));
		out->set_data(bardata::COLUMN_NAME_FASTAVG, STRINGNUMBER(out->fast_avg, 4));
		out->set_data(bardata::COLUMN_NAME_SLOWAVG, STRINGNUMBER(out->slow_avg, 4));

		return true;
	}

	return false;
}
// old version: v1
/*bool BardataCalculator::update_incomplete_bar(const QSqlDatabase &database, const QString &database_dir, const QString &ccy_database_dir,
																							 const QString &symbol, const IntervalWeight base_interval,
																							 const IntervalWeight target_interval, Logger _logger)
{
	if (target_interval == WEIGHT_INVALID) return false;

	// Observation:
	// (1) intra-day bar on Sunday (begin of Monday session) is supposed belong to Monday, so it is only 1 incomplete bar of daily
	// (2) for Weekly and Monthly:
	//      - get last business day of the week or month.
	//      - current implementation, set it to last day of the week/month (ignore whether it's business day or not)
	//        then after real complete bar arrived we compare
	//          if complete's datetime >= incomplete's datetime then just override incomplete bar

	QString base_curr_date = "";
	QString base_curr_time = "";
	QString first_parent_date = ""; // lower bound
	QString first_parent_time = "";
	QString last_parent_date = ""; // upper bound
	QString last_parent_time = "";
	double last_dayrange_high[3] = {0, 0, 0};
	double last_dayrange_low[3] = {0, 0, 0};
	double last_close = 0;
	int last_incomplete_rowid = 0;
	bool is_update = true;
	bool result_code = true;

	QSqlDatabase target_db;
	SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_" + Globals::intervalWeightToString(target_interval) + ".db", &target_db);

	if (target_db.isOpenError()) {
		LOG_DEBUG ("update incomplete bar: parent db open error! " +
							 (database_dir + "/" + symbol + "_" + Globals::intervalWeightToString(target_interval) + ".db").toStdString());
		SQLiteHandler::removeDatabase(&target_db);
		return false;
	}

	QSqlQuery q_target(target_db);
	q_target.setForwardOnly(true);
	q_target.exec("PRAGMA busy_timeout =" + QString::number(BUSY_TIMEOUT) + ";");
	q_target.exec("PRAGMA cache_size = -32768;");
	q_target.exec("PRAGMA journal_mode = OFF;");
	q_target.exec("PRAGMA synchronous = OFF;");

	QSqlQuery q(database);
	q.setForwardOnly(true);
	q.exec("PRAGMA busy_timeout =" + QString::number(BUSY_TIMEOUT) + ";");
	q.exec("PRAGMA cache_size = -32768;");
	q.exec("PRAGMA journal_mode = OFF;");
	q.exec("PRAGMA synchronous = OFF;");

	// we're using 5min time frame as reference to create incomplete bar
	// only if 5min's datetime is greater than it's upper timeframe then we create new incomplete bar

	// (1) find last datetime in base interval or 5min
	q.exec("select date_, time_ from bardata where completed=1 order by rowid desc limit 1");
	QUERY_LAST_ERROR (q, "");

	if (q.next())
	{
		base_curr_date = q.value(0).toString();
		base_curr_time = q.value(1).toString();
	}

	// error handling
	if (base_curr_date == "")
	{
		LOG_DEBUG ("{} {}: update incomplete bar: base curr date is empty.",
							 symbol.toStdString(), Globals::intervalWeightToStdString(base_interval));
		return false;
	}

	// (2) select last completed bar in parent
	if (target_interval == WEIGHT_60MIN)
	{
		// incomplete dayrange
		q_target.exec("select date_, time_, close_,"
									" dayrangehigh_1day, dayrangehigh_2day, dayrangehigh_3day,"
									" dayrangelow_1day, dayrangelow_2day, dayrangelow_3day"
									" from bardata where completed=1 order by rowid desc limit 1");

		QUERY_LAST_ERROR (q_target,"");

		if (q_target.next())
		{
			first_parent_date = q_target.value(0).toString();
			first_parent_time = q_target.value(1).toString();
			last_close = q_target.value(2).toDouble();
			last_dayrange_high[0] = q_target.value(3).toDouble();
			last_dayrange_high[1] = q_target.value(4).toDouble();
			last_dayrange_high[2] = q_target.value(5).toDouble();
			last_dayrange_low[0] = q_target.value(6).toDouble();
			last_dayrange_low[1] = q_target.value(7).toDouble();
			last_dayrange_low[2] = q_target.value(8).toDouble();
		}
	}
	else
	{
		q_target.exec("select date_, time_ from bardata where completed=1 order by rowid desc limit 1");

		QUERY_LAST_ERROR (q_target,"");

		if (q_target.next()) {
			first_parent_date = q_target.value(0).toString();
			first_parent_time = q_target.value(1).toString();
		}
	}

	// error handling
	if (first_parent_date == "" || first_parent_time == "")
	{
		LOG_DEBUG ("{} {}: update incomplete bar warning: first parent date is empty. {}.",
							 symbol.toStdString(), Globals::intervalWeightToStdString(base_interval),
							 q_target.lastError().text().toStdString());

#ifdef DEBUG_MODE
		qDebug("update incomplete bar: first parent date is empty.");
#endif

		return false;
	}

	// (3) select last incomplete bar in parent
	q_target.exec("select rowid, date_, time_ from bardata where completed=0 order by rowid desc limit 1");
	QUERY_LAST_ERROR (q_target,"");

	if (q_target.next())
	{
		last_incomplete_rowid = q_target.value(0).toInt();
		last_parent_date = q_target.value(1).toString();
		last_parent_time = q_target.value(2).toString();
	}

	// (4) try to create incomplete bar
	if (last_incomplete_rowid == 0)
	{
		if (base_curr_date > first_parent_date || (base_curr_date == first_parent_date && base_curr_time > first_parent_time)) {

			if (target_interval >= WEIGHT_DAILY)
			{
				if (target_interval == WEIGHT_MONTHLY)
					last_parent_date = QString::fromStdString(DateTimeHelper::next_monthly_date(first_parent_date.toStdString(), true));

				else if (target_interval == WEIGHT_WEEKLY)
					last_parent_date = QString::fromStdString(DateTimeHelper::next_weekly_date(first_parent_date.toStdString(), true));

				else if (target_interval == WEIGHT_DAILY) {
					last_parent_date = QString::fromStdString(DateTimeHelper::next_daily_date(first_parent_date.toStdString()));

					while (last_parent_date < base_curr_date) {
						last_parent_date = QString::fromStdString(DateTimeHelper::next_daily_date(last_parent_date.toStdString()));
					}
				}

				last_parent_time = first_parent_time;
			}
			else
			{
				// target_interval = 60min
				std::string _timestring = DateTimeHelper::next_intraday_datetime((first_parent_date + " " + first_parent_time).toStdString(), target_interval);

				if (_timestring.length() >= 16) {
					last_parent_date = QString::fromStdString(_timestring.substr(0, 10));
					last_parent_time = QString::fromStdString(_timestring.substr(11, 5));

					while (_timestring.length() > 0 && (base_curr_date > last_parent_date ||
								 (base_curr_date == last_parent_date && base_curr_time > last_parent_time)))
					{
						_timestring = DateTimeHelper::next_intraday_datetime(_timestring, target_interval);

						if (_timestring.length() >= 16)
						{
							last_parent_date = QString::fromStdString(_timestring.substr(0, 10));
							last_parent_time = QString::fromStdString(_timestring.substr(11, 5));
						}
					}
				}
			}

			// set to any number larger than zero, this rowid wont be used in insert anyway
			last_incomplete_rowid = 1;

			// set to insert operation instead of update
			is_update = false;
		}
	}

	// if incomplete bar is valid
	if (last_incomplete_rowid > 0 && last_parent_date != "")
	{
		BardataTuple r;
		double ccy_rate = 1.0;
		double dayrange_high[3] = {0, 0, 0};
		double dayrange_low[3] = {0, 0, 0};
		double upmom_10, upmom_50;
		double downmom_10, downmom_50;
		double ugd_10 = 0, dgu_10 = 0;
		double ugd_50 = 0, dgu_50 = 0;

		// get last indicator information (to calculate macd, rsi, slowk, slowd)
		Globals::dbupdater_info _indicators = Globals::get_last_indicator_for_dbupdater
				(symbol.toStdString() + "_" + Globals::intervalWeightToStdString(target_interval) + ".db");

		// update currency rate for @NIY, HHI, HSI
		if (ccy_database_dir != "")
		{
			QSqlDatabase ccy_database;
			SQLiteHandler::getDatabase_v2(database_dir + "/" + ccy_database_dir, &ccy_database);

			if (!ccy_database.isOpenError())
			{
				QSqlQuery q_ccy(ccy_database);
				q_ccy.exec("select prev_rate from exchange_rate where date_<='" + last_parent_date + "' order by date_ desc limit 1");
				ccy_rate = q_ccy.next() ? q_ccy.value(0).toDouble() : 1.0;
			}

			SQLiteHandler::removeDatabase(&ccy_database);
		}

		// we're using QMap to sort the rows because order by in sql is expensive operation
		QMap<int, t_ohlc> buffer;
		t_ohlc data;

		q.exec("select rowid, open_, high_, low_, close_ from bardata where "
					 "(date_>'" + first_parent_date + "' or (date_='" + first_parent_date + "' and time_ >'"  + first_parent_time + "')) and "
					 "(date_<'" + last_parent_date  + "' or (date_='" + last_parent_date  + "' and time_ <='" + last_parent_time  + "'))");

		QUERY_LAST_ERROR (q, "");

		while (q.next())
		{
			data.open  = q.value(1).toDouble();
			data.high  = q.value(2).toDouble();
			data.low   = q.value(3).toDouble();
			data.close = q.value(4).toDouble();
			buffer.insert(q.value(0).toInt(), data);
		}

		// previous close (for calculate incomplete RSI)
		q_target.exec("select close_ from bardata where completed=1 order by rowid desc limit 1");
		r.prev_close = q_target.next() ? q_target.value(0).toDouble() : 0;

		// update _parent indexing
		{
			IntervalWeight _superparent = Globals::getParentInterval(target_interval);

			if (_superparent != WEIGHT_INVALID) {
				QSqlDatabase superdb;
				SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_" + Globals::intervalWeightToString(_superparent) + ".db", &superdb);

				QSqlQuery q_super(superdb);
				q_super.setForwardOnly(true);
				q_super.exec("PRAGMA temp_store = MEMORY;");
				q_super.exec("PRAGMA busy_timeout =" + QString::number(BUSY_TIMEOUT) + ";");
				q_super.exec("select rowid from bardata"
										 " where date_<'" + last_parent_date + "' or (date_='" + last_parent_date + "' and time_<'" + last_parent_time + "')"
										 " order by rowid desc limit 1");

				r._parent_prev = q_super.next() ? q_super.value(0).toInt() : 0;
				SQLiteHandler::removeDatabase(&superdb);
			}

			if (target_interval < WEIGHT_MONTHLY) {
				QSqlDatabase superdb;
				SQLiteHandler::getDatabase_v2(database_dir + "/" + symbol + "_" + Globals::intervalWeightToString(WEIGHT_MONTHLY) + ".db", &superdb);

				QSqlQuery q_super(superdb);
				q_super.setForwardOnly(true);
				q_super.exec("PRAGMA temp_store = MEMORY;");
				q_super.exec("PRAGMA busy_timeout =" + QString::number(BUSY_TIMEOUT) + ";");
				q_super.exec("select rowid from bardata"
										 " where date_<'" + last_parent_date + "' or (date_='" + last_parent_date + "' and time_<'" + last_parent_time + "')"
										 " order by rowid desc limit 1");

				r._parent_prev_monthly = q_super.next() ? q_super.value(0).toInt() : 0;
				SQLiteHandler::removeDatabase(&superdb);
			}
		}

		for (auto it = buffer.begin(); it != buffer.end(); ++it)
		{
			data = it.value();

			if (it == buffer.begin())
			{
				// first row
				r.open  = data.open;
				r.high  = data.high;
				r.low   = data.low;
				r.close = data.close;
			}
			else
			{
				// remaining rows
				r.high  = std::fmax(data.high, r.high);
				r.low   = std::fmin(data.low, r.low);
				r.close = data.close;
			}
		}

		// create incomplete dayrange for 60min
		if (target_interval == WEIGHT_60MIN)
		{
			q_target.exec("attach database '" + database_dir + "/" + symbol + "_" + Globals::intervalWeightToString(WEIGHT_DAILY) + ".db' as dbdaily_temp");
			q_target.exec("select time_ from dbdaily_temp.bardata limit 1");

			QString closing_time = q_target.next() ? q_target.value(0).toString() : "";
			q_target.exec("detach database dbdaily_temp;");

			// first bar after closing time (special handling)
			if (closing_time != "" && last_parent_time > closing_time && first_parent_time == closing_time) {
				dayrange_high[0] = r.high - r.close;
				dayrange_high[1] = std::max(last_dayrange_high[0] + last_close, r.high) - r.close;
				dayrange_high[2] = std::max(last_dayrange_high[1] + last_close, r.high) - r.close;

				dayrange_low[0] = r.close - r.low;
				dayrange_low[1] = r.close - std::min(last_close - last_dayrange_low[0], r.low);
				dayrange_low[2] = r.close - std::min(last_close - last_dayrange_low[1], r.low);
			}
			// normal case
			else
			{
				dayrange_high[0] = std::fmax(last_dayrange_high[0] + last_close, r.high) - r.close;
				dayrange_high[1] = std::fmax(last_dayrange_high[1] + last_close, r.high) - r.close;
				dayrange_high[2] = std::fmax(last_dayrange_high[2] + last_close, r.high) - r.close;

				dayrange_low[0] = r.close - std::fmin(last_close - last_dayrange_low[0], r.low);
				dayrange_low[1] = r.close - std::fmin(last_close - last_dayrange_low[1], r.low);
				dayrange_low[2] = r.close - std::fmin(last_close - last_dayrange_low[2], r.low);
			}
		}

		// create incomplete atr
		q_target.exec("select rowid, high_, low_, close_ from bardata where date_ <'" + last_parent_date + "' or (date_='" + last_parent_date + "' and time_ < '" + last_parent_time + "')"
									" order by rowid desc limit 14");

		QUERY_LAST_ERROR (q_target, "update incompete bar (select parent)");

		buffer.clear();
		data.open = 0;

		while (q_target.next())
		{
			data.high  = q_target.value(1).toDouble();
			data.low   = q_target.value(2).toDouble();
			data.close = q_target.value(3).toDouble();
			buffer.insert(q_target.value(0).toInt(), data);
		}

		double sum_tr = 0;
		double prev_close = 0;
		double tr_1, tr_2, tr_3;

		for (auto it = buffer.begin(); it != buffer.end(); ++it)
		{
			if (it == buffer.begin())
			{
				// first row
				sum_tr = 0;
				prev_close = it.value().close;
			}
			else
			{
				// remaining rows
				data = it.value();
				tr_1 = data.high - data.low;
				tr_2 = std::fabs(data.high - prev_close);
				tr_3 = std::fabs(data.low - prev_close);
				sum_tr += std::fmax(tr_1, std::fmax (tr_2, tr_3));
				prev_close = data.close;
			}
		}

		// last from current incomplete bar
		tr_1 = r.high - r.low;
		tr_2 = std::fabs(r.high - prev_close);
		tr_3 = std::fabs(r.low - prev_close);

		sum_tr += std::fmax(tr_1, std::fmax(tr_2, tr_3));
		r.atr = sum_tr / 14.0;

		q_target.exec("select atr_rank from bardata where atr <=" + QString::number(r.atr,'f') + " order by atr desc limit 1");
		r.atr_rank = q_target.next() ? q_target.value(0).toDouble() : 0;

		// incomplete up/down mom

		if (is_update)
		{
			// 10
			q_target.exec("select low_, high_, atr from bardata where rowid=" + QString::number(last_incomplete_rowid) + "-9");

			if (q_target.next())
			{
				upmom_10 = (r.close - q_target.value(0).toDouble()) / (FASTAVG_LENGTH * q_target.value(2).toDouble());
				downmom_10 = (q_target.value(1).toDouble() - r.close) / (FASTAVG_LENGTH * q_target.value(2).toDouble());

				q_target.exec("select ugd_10_0, dgu_10_0 from bardata where completed = 1 order by rowid desc limit 1");
				q_target.next();

				if (upmom_10 > downmom_10)
				{
					ugd_10 = q_target.value(0).toInt() + 1;
					dgu_10 = 0;
				}
				else
				{
					ugd_10 = 0;
					dgu_10 = q_target.value(1).toInt() + 1;
				}
			}

			// 50
			q_target.exec("select low_, high_, atr from bardata where rowid=" + QString::number(last_incomplete_rowid) + "-49");

			if (q_target.next())
			{
				upmom_50 = (r.close - q_target.value(0).toDouble()) / (SLOWAVG_LENGTH * q_target.value(2).toDouble());
				downmom_50 = (q_target.value(1).toDouble() - r.close) / (SLOWAVG_LENGTH * q_target.value(2).toDouble());

				q_target.exec("select ugd_50_0, dgu_50_0 from bardata where completed = 1 order by rowid desc limit 1");
				q_target.next();

				if (upmom_50 > downmom_50)
				{
					ugd_50 = q_target.value(0).toInt() + 1;
					dgu_50 = 0;
				}
				else
				{
					ugd_50 = 0;
					dgu_50 = q_target.value(1).toInt() + 1;
				}
			}
		}
		else
		{
			q_target.exec("select low_, high_, atr from bardata where rowid=(select max(rowid) from bardata)-8");

			if (q_target.next())
			{
				upmom_10 = (r.close - q_target.value(0).toDouble()) / (FASTAVG_LENGTH * q_target.value(2).toDouble());
				downmom_10 = (q_target.value(1).toDouble() - r.close) / (FASTAVG_LENGTH * q_target.value(2).toDouble());

				q_target.exec("select ugd_10_0, dgu_10_0 from bardata order by rowid desc limit 1");
				q_target.next();

				if (upmom_10 > downmom_10)
				{
					ugd_10 = q_target.value(0).toInt() + 1;
					dgu_10 = 0;
				}
				else
				{
					ugd_10 = 0;
					dgu_10 = q_target.value(1).toInt() + 1;
				}
			}

			q_target.exec("select low_, high_, atr from bardata where rowid=(select max(rowid) from bardata)-48");

			if (q_target.next())
			{
				upmom_50 = (r.close - q_target.value(0).toDouble()) / (SLOWAVG_LENGTH * q_target.value(2).toDouble());
				downmom_50 = (q_target.value(1).toDouble() - r.close) / (SLOWAVG_LENGTH * q_target.value(2).toDouble());

				q_target.exec("select ugd_50_0, dgu_50_0 from bardata order by rowid desc limit 1");
				q_target.next();

				if (upmom_50 > downmom_50)
				{
					ugd_50 = q_target.value(0).toInt() + 1;
					dgu_50 = 0;
				}
				else
				{
					ugd_50 = 0;
					dgu_50 = q_target.value(1).toInt() + 1;
				}
			}
		}

		// calculate macd, rsi, slowk, slowd, fastavg, slowavg (since v1.5.5)
		continous_calculation_v2 (&_indicators, &r);

		// @deprecated/backup: 2015-12-14
		{
//    // select OHLC from base database to create incomplete bar
//    q.exec ("select open_, high_, low_, close_ from bardata where "
//            "(date_>'" + first_parent_date + "' or (date_='" + first_parent_date + "' and time_ >'"  + first_parent_time + "')) and "
//            "(date_<'" + last_parent_date  + "' or (date_='" + last_parent_date  + "' and time_ <='" + last_parent_time  + "'))");

//    // first row (get open price and initialize others)
//    if (q.next())
//    {
//      price_open  = q.value(0).toDouble();
//      price_high  = q.value(1).toDouble();
//      price_low   = q.value(2).toDouble();
//      price_close = q.value(3).toDouble();
//    }

//    // remaining rows
//    while (q.next())
//    {
//      price_high  = std::max (price_high, q.value(1).toDouble());
//      price_low   = std::min (price_low, q.value(2).toDouble());
//      price_close = q.value(3).toDouble();
//    }
		}

		// do update for existing data
		if (is_update)
		{
			q_target.exec("update bardata set"
										" open_=" + QString::number(r.open,'f') +
										",high_=" + QString::number(r.high,'f') +
										",low_=" + QString::number(r.low,'f') +
										",close_=" + QString::number(r.close,'f') +
										",ccy_rate=" + QString::number(ccy_rate,'f') +
										",atr=" + QString::number(r.atr,'f') +
										",atr_rank=" + QString::number(r.atr_rank,'f') +
										",macd=" + QString::number(r.macd,'f') +
										",rsi=" + QString::number(r.rsi,'f') +
										",slowk=" + QString::number(r.slow_k,'f') +
										",slowd=" + QString::number(r.slow_d,'f') +
										",fastavg=" + QString::number(r.fast_avg,'f') +
										",slowavg=" + QString::number(r.slow_avg,'f') +
										",dayrangehigh_1day=" + QString::number(dayrange_high[0],'f') +
										",dayrangehigh_2day=" + QString::number(dayrange_high[1],'f') +
										",dayrangehigh_3day=" + QString::number(dayrange_high[2],'f') +
										",dayrangelow_1day=" + QString::number(dayrange_low[0],'f') +
										",dayrangelow_2day=" + QString::number(dayrange_low[1],'f') +
										",dayrangelow_3day=" + QString::number(dayrange_low[2],'f') +
										",upmom_10=" + QString::number(upmom_10,'f') +
										",downmom_10=" + QString::number(downmom_10,'f') +
										",ugd_10_0=" + QString::number(ugd_10) +
										",dgu_10_0=" + QString::number(dgu_10) +
										",upmom_50=" + QString::number(upmom_50,'f') +
										",downmom_50=" + QString::number(downmom_50,'f') +
										",ugd_50_0=" + QString::number(ugd_50) +
										",dgu_50_0=" + QString::number(dgu_50) +
										",_parent_prev=" + QString::number(r._parent_prev) +
										",_parent_prev_monthly=" + QString::number(r._parent_prev_monthly) +
										" where rowid=" + QString::number(last_incomplete_rowid));

			QUERY_LAST_ERROR (q_target, "update incomplete bar(update)");
		}

		// do insert for new data
		else
		{
			q_target.exec("insert or ignore into bardata("
										"date_, time_, open_, high_, low_, close_, ccy_rate, atr, atr_rank,"
										"macd,rsi,slowk,slowd,fastavg,slowavg,"
										"dayrangehigh_1day, dayrangehigh_2day, dayrangehigh_3day,"
										"dayrangelow_1day, dayrangelow_2day, dayrangelow_3day,"
										"upmom_10, downmom_10, ugd_10_0, dgu_10_0,"
										"upmom_50, downmom_50, ugd_50_0, dgu_50_0,"
										"_parent_prev, _parent_prev_monthly, completed) values('" +
										last_parent_date + "','" +
										last_parent_time + "'," +
										QString::number(r.open,'f') + "," +
										QString::number(r.high,'f') + "," +
										QString::number(r.low,'f') + "," +
										QString::number(r.close,'f') + "," +
										QString::number(ccy_rate,'f') + "," +
										QString::number(r.atr,'f') + "," +
										QString::number(r.atr_rank,'f') + "," +
										QString::number(r.macd,'f') + "," +
										QString::number(r.rsi,'f') + "," +
										QString::number(r.slow_k,'f') + "," +
										QString::number(r.slow_d,'f') + "," +
										QString::number(r.fast_avg,'f') + "," +
										QString::number(r.slow_avg,'f') + "," +
										QString::number(dayrange_high[0],'f') + "," +
										QString::number(dayrange_high[1],'f') + "," +
										QString::number(dayrange_high[2],'f') + "," +
										QString::number(dayrange_low[0],'f') + "," +
										QString::number(dayrange_low[1],'f') + "," +
										QString::number(dayrange_low[2],'f') + "," +
										QString::number(upmom_10,'f') + "," +
										QString::number(downmom_10,'f') + "," +
										QString::number(ugd_10,'f') + "," +
										QString::number(dgu_10,'f') + "," +
										QString::number(upmom_50,'f') + "," +
										QString::number(downmom_50,'f') + "," +
										QString::number(ugd_50,'f') + "," +
										QString::number(dgu_50,'f') + "," +
										QString::number(r._parent_prev) + "," +
										QString::number(r._parent_prev_monthly) + ",0)");

			QUERY_LAST_ERROR (q_target, "update incomplete bar(insert)");
		}
	}

	if (q_target.lastError().isValid())
	{
		LOG_DEBUG ("{} {}: update incomplete bar error: {}",
							 symbol.toStdString(), Globals::intervalWeightToStdString(base_interval),
							 q_target.lastError().text().toStdString());
	}

	SQLiteHandler::removeDatabase(&target_db);

	return result_code;
}*/

int BardataCalculator::update_incomplete_bar_ccy(const QString &input_filename, const QString &database_filename)
{
	QFile file(input_filename);
	int errcode = 0;

	if (file.open(QFile::ReadWrite))
	{
		QSqlDatabase database;
		database = QSqlDatabase::addDatabase("QSQLITE","updateccyincomp");
		database.setDatabaseName(database_filename);
		database.open();

		QSqlQuery q(database);
		q.setForwardOnly(true);
		q.exec("PRAGMA page_size = 16384;");
		q.exec("create table if not exists exchange_rate("
					 "date_ TEXT NOT NULL,"
					 "time_ TEXT NOT NULL,"
					 "rate REAL REAL,"
					 "prev_rate REAL,"
					 "completed INT DEFAULT 1,"
					 "PRIMARY KEY(date_ asc, time_ asc));");

		QUERY_ERROR_CODE (q, errcode, "update incomplete ccy: create table");

		QTextStream stream(&file);
		QStringList columns;
		QString last_complete_date = "";
		QString last_complete_time = "";
		QString last_date = "";
		QString last_rate = "";
		QString line, parse_date, parse_time;
		QString prev_date = "";
		QString update_date = "";
		QString update_rate = "";

		q.exec("select date_, time_, rate from exchange_rate where completed=1 order by date_ desc limit 1");
		QUERY_ERROR_CODE (q, errcode, "");

		if (q.next())
		{
			last_complete_date = q.value(0).toString();
			last_complete_time = q.value(1).toString();
			last_rate = QString::number(q.value(2).toDouble(),'f',5);
		}

		q.exec("select date_ from exchange_rate order by date_ desc limit 1");
		QUERY_ERROR_CODE (q, errcode, "");

		last_date = q.next() ? q.value(0).toString() : "";

		BEGIN_TRANSACTION (q);

		while (!stream.atEnd() && stream.readLineInto(&line))
		{
			if (line.length() < 16) continue;
			if (line.startsWith("\"Date\"")) continue;

			parse_date = line.mid(6,4) + "-" + line.mid(0,2) + "-" + line.mid(3,2);
			parse_time = line.mid(11,5);

			if (parse_date > last_complete_date || (parse_date == last_complete_date && parse_time > last_complete_time))
			{
				columns = line.split(",", QString::KeepEmptyParts);

				if (columns.size() > 5)
				{
					if (parse_date > last_date || (parse_date == last_date && columns[1] > last_complete_time))
					{
						// get next business day
						last_date = QString::fromStdString(DateTimeHelper::next_daily_date(last_date.toStdString()));

						if (!last_date.isEmpty())
						{
							// try to create incomplete bar
							q.exec("insert or ignore into exchange_rate values('" + last_date + "','" + last_complete_time + "'," + columns[5] + "," + last_rate + ",0)");
							QUERY_ERROR_CODE (q, errcode, "exchange rate (1)");

							q.exec("select last_insert_rowid();");
							QUERY_ERROR_CODE (q, errcode, "exchange rate (1)");

							if (q.next() && q.value(0).toInt() > 0)
							{
								// close previous incompleted
								q.exec("update exchange_rate set completed=1 where completed=0 and rowid <>" + QString::number(q.value(0).toInt()));
								QUERY_ERROR_CODE (q, errcode, "exchange rate (1-update)");
							}
						}
					}
					else
					{
						update_date = last_date;
						update_rate = columns[5];

						if (prev_date != last_date)
						{
							if (!prev_date.isEmpty())
							{
								// update incomplete bar (prev)
								q.exec("update exchange_rate set rate=" + last_rate + " where date_='" + prev_date + "'");
								QUERY_ERROR_CODE (q, errcode, "exchange rate (2A-update)");
							}

							// update incomplete bar (curr)
							q.exec("update exchange_rate set rate=" + columns[5] + " where date_='" + last_date + "'");
							QUERY_ERROR_CODE (q, errcode, "exchange rate (2B-update)");
						}

						last_rate = columns[5];
						prev_date = last_date;
					}
				}
			}
		}

		if (!update_date.isEmpty())
		{
			// update incomplete bar (last)
			q.exec("update exchange_rate set rate=" + update_rate + " where date_='" + update_date + "'");
			QUERY_ERROR_CODE (q, errcode, "exchange rate (2-update)");
		}

		END_TRANSACTION (q);

		SQLiteHandler::removeDatabase(&database);

		file.close();
	}

	return errcode;
}

int BardataCalculator::update_incomplete_bar_ccy_v2(const QString &ccy_name, const QString &input_dir, const QString &database_dir)
{
	QFile file_daily(input_dir + "/" + ccy_name + "_Daily" + ".txt");
	QFile file_5min(input_dir + "/" + ccy_name + "_5min" + ".txt");
	int errcode = 0;

	if (file_daily.open(QFile::ReadOnly) && file_5min.open(QFile::ReadOnly))
	{
		QSqlDatabase database;
		database = QSqlDatabase::addDatabase("QSQLITE","updateccy" + Globals::generate_random_str(6));
		database.setDatabaseName(database_dir + "/" + ccy_name + "_Daily" + ".db");
		database.open();

		QSqlQuery q(database);
		q.setForwardOnly(true);
		q.exec("PRAGMA page_size = 16384;");
		q.exec("create table if not exists exchange_rate("
					 "date_ TEXT NOT NULL,"
					 "time_ TEXT NOT NULL,"
					 "rate REAL REAL,"
					 "prev_rate REAL,"
					 "completed INT DEFAULT 1,"
					 "PRIMARY KEY(date_ asc, time_ asc));");

		QStringList columns;
		QString last_complete_date = "";
		QString last_complete_time = "";
		QString last_complete_rate = "";
		QString last_incomplete_date = "";
		QString last_incomplete_rate = "";
		QString prev_incomplete_date = "";
		QString line, parse_date, parse_time, parse_ccyrate;

		q.exec("delete from exchange_rate where completed=0");
		q.exec("select date_, time_, rate from exchange_rate where completed=1 order by date_ desc limit 1");

		if (q.next())
		{
			last_complete_date = q.value(0).toString();
			last_complete_time = q.value(1).toString();
			last_complete_rate = QString::number(q.value(2).toDouble(),'f',6);
		}

		BEGIN_TRANSACTION (q);

		// daily
		{
			QTextStream stream_daily(&file_daily);
			stream_daily.readLineInto(NULL);

			while (!stream_daily.atEnd() && stream_daily.readLineInto(&line))
			{
				if (line.length() < 16) continue;

				parse_date = line.mid(6,4) + "-" + line.mid(0,2) + "-" + line.mid(3,2);
				parse_time = line.mid(11,5);

				if (parse_date > last_complete_date || (parse_date == last_complete_date && parse_time > last_complete_time))
				{
					columns = line.split(",", QString::KeepEmptyParts);
					if (columns.size() < 6) break;

					parse_ccyrate = columns[5];

					if (parse_date > last_complete_date && parse_time == last_complete_time)
					{
						q.exec("insert or replace into exchange_rate values"
									 "('" + parse_date + "','" + last_complete_time + "'," +
													parse_ccyrate + "," + last_complete_rate + ",1)");

						QUERY_ERROR_CODE(q, errcode, "exchange rate(daily)");

						last_complete_date = parse_date;
						last_complete_rate = parse_ccyrate;
					}
				}
			}

			last_incomplete_date = last_complete_date;
			last_incomplete_rate = last_complete_rate;
		}

		// 5min
		{
			QTextStream stream_5min(&file_5min);
			stream_5min.readLineInto(NULL);

			while (!stream_5min.atEnd() && stream_5min.readLineInto(&line))
			{
				if (line.length() < 16) continue;

				parse_date = line.mid(6,4) + "-" + line.mid(0,2) + "-" + line.mid(3,2);
				parse_time = line.mid(11,5);

				if (parse_date > last_complete_date || (parse_date == last_complete_date && parse_time > last_complete_time))
				{
					columns = line.split(",", QString::KeepEmptyParts);
					if (columns.size() < 6) break;

					parse_ccyrate = columns[5];

					while (last_incomplete_date != "" && last_incomplete_date <= parse_date)
					{
						last_incomplete_date = QString::fromStdString(DateTimeHelper::next_daily_date(last_incomplete_date.toStdString()));
					}

					if (last_incomplete_date.length() >= 10)
					{
						if (prev_incomplete_date == "" || (prev_incomplete_date != "" && prev_incomplete_date < last_incomplete_date))
						{
							q.exec("insert into exchange_rate values('" +
										 last_incomplete_date + "','" + last_complete_time + "'," +
										 parse_ccyrate + "," + last_complete_rate + ",0)");

							prev_incomplete_date = last_incomplete_date;
						}

						if (last_incomplete_date > last_complete_date)
						{
							q.exec("update exchange_rate set rate=" + parse_ccyrate + " where date_='" + last_incomplete_date + "'");
						}

						QUERY_ERROR_CODE(q, errcode, "exchange rate(5min)");
					}
				}
			}
		}

		END_TRANSACTION (q);

		SQLiteHandler::removeDatabase(&database);

		file_daily.close();
		file_5min.close();
	}

	return errcode;
}

int BardataCalculator::update_up_down_mom_approx(std::vector<BardataTuple> *out,
																								 const QSqlDatabase &database,
																								 const int length,
																								 const int start_rowid)
{
	const int out_length = static_cast<int>(out->size());
	const QString _start_rowid = QString::number(start_rowid);
	QSqlQuery q(database);
	QString _up_column, _up_rank;
	QString _down_column, _down_rank;
	QString _dist_updown, _dist_updown_rank;
	QString _dist_downup, _dist_downup_rank;

	std::queue<double> prev_high;
	std::queue<double> prev_low;
	std::queue<double> prev_atr;
	std::string up_column, up_rank;
	std::string down_column, down_rank;
	std::string dist_updown, dist_updown_rank;
	std::string dist_downup, dist_downup_rank;
	double up_mom, down_mom, atr_den;
	int errcode = 0;

	if (length == FASTAVG_LENGTH)
	{
		up_column        = bardata::COLUMN_NAME_UP_MOM_10;
		up_rank          = bardata::COLUMN_NAME_UP_MOM_10_RANK;
		down_column      = bardata::COLUMN_NAME_DOWN_MOM_10;
		down_rank        = bardata::COLUMN_NAME_DOWN_MOM_10_RANK;
		dist_updown      = bardata::COLUMN_NAME_DISTUD_10;
		dist_updown_rank = bardata::COLUMN_NAME_DISTUD_10_RANK;
		dist_downup      = bardata::COLUMN_NAME_DISTDU_10;
		dist_downup_rank = bardata::COLUMN_NAME_DISTDU_10_RANK;

		_up_column        = SQLiteHandler::COLUMN_NAME_UP_MOM_10;
		_up_rank          = SQLiteHandler::COLUMN_NAME_UP_MOM_10_RANK;
		_down_column      = SQLiteHandler::COLUMN_NAME_DOWN_MOM_10;
		_down_rank        = SQLiteHandler::COLUMN_NAME_DOWN_MOM_10_RANK;
		_dist_updown      = SQLiteHandler::COLUMN_NAME_DISTUD_10;
		_dist_updown_rank = SQLiteHandler::COLUMN_NAME_DISTUD_10_RANK;
		_dist_downup      = SQLiteHandler::COLUMN_NAME_DISTDU_10;
		_dist_downup_rank = SQLiteHandler::COLUMN_NAME_DISTDU_10_RANK;
	}
	else
	{
		up_column        = bardata::COLUMN_NAME_UP_MOM_50;
		up_rank          = bardata::COLUMN_NAME_UP_MOM_50_RANK;
		down_column      = bardata::COLUMN_NAME_DOWN_MOM_50;
		down_rank        = bardata::COLUMN_NAME_DOWN_MOM_50_RANK;
		dist_updown      = bardata::COLUMN_NAME_DISTUD_50;
		dist_updown_rank = bardata::COLUMN_NAME_DISTUD_50_RANK;
		dist_downup      = bardata::COLUMN_NAME_DISTDU_50;
		dist_downup_rank = bardata::COLUMN_NAME_DISTDU_50_RANK;

		_up_column        = SQLiteHandler::COLUMN_NAME_UP_MOM_50;
		_up_rank          = SQLiteHandler::COLUMN_NAME_UP_MOM_50_RANK;
		_down_column      = SQLiteHandler::COLUMN_NAME_DOWN_MOM_50;
		_down_rank        = SQLiteHandler::COLUMN_NAME_DOWN_MOM_50_RANK;
		_dist_updown      = SQLiteHandler::COLUMN_NAME_DISTUD_50;
		_dist_updown_rank = SQLiteHandler::COLUMN_NAME_DISTUD_50_RANK;
		_dist_downup      = SQLiteHandler::COLUMN_NAME_DISTDU_50;
		_dist_downup_rank = SQLiteHandler::COLUMN_NAME_DISTDU_50_RANK;
	}

	// load previous information
	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("select high_, low_, atr from bardata"
				 " where rowid >" + QString::number(start_rowid - length) +
				 " order by rowid limit " + QString::number(length));

	QUERY_ERROR_CODE (q, errcode, "");

	// get previous information
	while (q.next())
	{
		prev_high.emplace(q.value(0).toDouble());
		prev_low.emplace(q.value(1).toDouble());
		prev_atr.emplace(q.value(2).toDouble());
	}

	// process new records
	for (int i = 0; i < out_length; ++i)
	{
		if (!prev_high.empty() && (int)prev_high.size() + 1 > length)
		{
			prev_high.pop();
			prev_low.pop();
			prev_atr.pop();
		}

		prev_high.emplace((*out)[i].high);
		prev_low.emplace((*out)[i].low);
		prev_atr.emplace((*out)[i].atr);

		if (prev_high.size() >= length)
		{
			{
				atr_den = prev_atr.front() * length;

				if (atr_den != 0.0)
				{
					up_mom = ((*out)[i].close - prev_low.front()) / atr_den;
					down_mom = (prev_high.front() - (*out)[i].close) / atr_den;
				}
				else
				{
					up_mom = 0.0;
					down_mom = 0.0;
				}

//        (*out)[i].set_data(up_column, std::to_string(Globals::round_decimals(up_mom, 5)));
//        (*out)[i].set_data(down_column, std::to_string(Globals::round_decimals(down_mom, 5)));
//        (*out)[i].set_data(dist_updown, std::to_string(Globals::round_decimals(up_mom - down_mom, 5)));
//        (*out)[i].set_data(dist_downup, std::to_string(Globals::round_decimals(down_mom - up_mom, 5)));

				(*out)[i].set_data(up_column, QString::number(Globals::round_decimals(up_mom, 5)).toStdString());
				(*out)[i].set_data(down_column, QString::number(Globals::round_decimals(down_mom, 5)).toStdString());
				(*out)[i].set_data(dist_updown, QString::number(Globals::round_decimals(up_mom - down_mom, 5)).toStdString());
				(*out)[i].set_data(dist_downup, QString::number(Globals::round_decimals(down_mom - up_mom, 5)).toStdString());
			}


			{
				q.exec(up_mom >= 0.0 ?

					// positive
					"select " + _up_rank + " from bardata where rowid <" + _start_rowid + " and " +
					_up_column + "> 0 and " + _up_column + "<=" + QString::number(Globals::round_decimals(up_mom, 5)) +
					" order by " + _up_column + " desc limit 1" :

					// negative
					"select " + _up_rank + " from bardata where rowid <" + _start_rowid + " and " +
					_up_column + "< 0 and " + _up_column + ">=" + QString::number(Globals::round_decimals(up_mom, 5)) +
					" order by " + _up_column + " asc limit 1");

				QUERY_ERROR_CODE (q, errcode, "");

				(*out)[i].set_data(up_rank, q.next() ? QString::number(q.value(0).toDouble(),'f',5).toStdString() : "0");

				q.exec(down_mom >= 0.0 ?

					// positive
					"select " + _down_rank + " from bardata where rowid <" + _start_rowid + " and " +
					_down_column + "> 0 and " + _down_column + "<=" + QString::number(Globals::round_decimals(down_mom, 5)) +
					" order by " + _down_column + " desc limit 1" :

					// negative
					"select " + _down_rank + " from bardata where rowid <" + _start_rowid + " and " +
					_down_column + "< 0 and " + _down_column + ">=" + QString::number(Globals::round_decimals(down_mom, 5)) +
					" order by " + _down_column + " asc limit 1");

				QUERY_ERROR_CODE (q, errcode, "");

				(*out)[i].set_data(down_rank, q.next() ? QString::number(q.value(0).toDouble(),'f',4).toStdString() : "0");

				// dist (up-down) rank
				if (up_mom > down_mom)
				{
					q.exec("select " + _dist_updown_rank + " from bardata where rowid <" + _start_rowid + " and " +
								 _dist_updown + "> 0 and " + _dist_updown + "<=" + QString::number(Globals::round_decimals(up_mom - down_mom, 5)) +
								 " order by " + _dist_updown + " desc limit 1");

					QUERY_ERROR_CODE (q, errcode, "");

					(*out)[i].set_data(dist_updown_rank, q.next() ? QString::number(q.value(0).toDouble(),'f',5).toStdString():"0");
				}
				else
				{
					(*out)[i].set_data(dist_updown_rank, "0");
				}

				// dist (down-up) rank
				if (down_mom > up_mom)
				{
					q.exec("select " + _dist_downup_rank + " from bardata where rowid <" + _start_rowid + " and " +
								 _dist_downup + "> 0 and " + _dist_downup + "<=" + QString::number(Globals::round_decimals(down_mom - up_mom, 5)) +
								 " order by " + _dist_downup + " desc limit 1");

					QUERY_ERROR_CODE (q, errcode, "");

					(*out)[i].set_data(dist_downup_rank, q.next()? QString::number(q.value(0).toDouble(),'f',5).toStdString():"0");
				}
				else
				{
					(*out)[i].set_data(dist_downup_rank,"0");
				}
			}
		}
		else
		{
			(*out)[i].set_data(up_column, "0");
			(*out)[i].set_data(up_rank, "0");
			(*out)[i].set_data(down_column, "0");
			(*out)[i].set_data(down_rank, "0");
			(*out)[i].set_data(dist_updown, "0");
			(*out)[i].set_data(dist_updown_rank, "0");
			(*out)[i].set_data(dist_downup, "0");
			(*out)[i].set_data(dist_downup_rank, "0");
		}
	}

	return errcode;
}

int BardataCalculator::update_up_down_threshold_approx(std::vector<BardataTuple> *out,
																											 const QSqlDatabase &database,
																											 const int length,
																											 const int id_threshold,
																											 const double /*threshold*/,
																											 const int start_rowid)
{
	QSqlQuery q(database);
	const int out_length = static_cast<int>(out->size());
	const QString _start_rowid = QString::number(start_rowid);

	QString up_column, down_column;
	QString ugd_column, ugd_rank_column;
	QString dgu_column, dgu_rank_column;
	double up_value, down_value;
	int ugd_counter = 0;
	int dgu_counter = 0;
	int errcode = 0;

	if (length == FASTAVG_LENGTH)
	{
		up_column       = SQLiteHandler::COLUMN_NAME_UP_MOM_10;
		down_column     = SQLiteHandler::COLUMN_NAME_DOWN_MOM_10;
		ugd_column      = SQLiteHandler::COLUMN_NAME_UGD_10 + "_" + QString::number(id_threshold);
		ugd_rank_column = SQLiteHandler::COLUMN_NAME_UGD_10_RANK + "_" + QString::number(id_threshold);
		dgu_column      = SQLiteHandler::COLUMN_NAME_DGU_10 + "_" + QString::number(id_threshold);
		dgu_rank_column = SQLiteHandler::COLUMN_NAME_DGU_10_RANK + "_" + QString::number(id_threshold);
	}
	else
	{
		up_column       = SQLiteHandler::COLUMN_NAME_UP_MOM_50;
		down_column     = SQLiteHandler::COLUMN_NAME_DOWN_MOM_50;
		ugd_column      = SQLiteHandler::COLUMN_NAME_UGD_50 + "_" + QString::number(id_threshold);
		ugd_rank_column = SQLiteHandler::COLUMN_NAME_UGD_50_RANK + "_" + QString::number(id_threshold);
		dgu_column      = SQLiteHandler::COLUMN_NAME_DGU_50 + "_" + QString::number(id_threshold);
		dgu_rank_column = SQLiteHandler::COLUMN_NAME_DGU_50_RANK + "_" + QString::number(id_threshold);
	}

	q.setForwardOnly(true);
	q.exec("PRAGMA temp_store = MEMORY;");
	q.exec("PRAGMA cache_size = 10000;");
	q.exec("select " + ugd_column + "," + dgu_column + " from bardata where rowid=" + _start_rowid);

	QUERY_ERROR_CODE (q, errcode, "");

	if (q.next())
	{
		ugd_counter = q.value(0).toInt();
		dgu_counter = q.value(1).toInt();
	}

	for (int i = 0; i < out_length; ++i)
	{
		up_value = (*out)[i].get_data_double(up_column.toStdString());
		down_value = (*out)[i].get_data_double(down_column.toStdString());

		ugd_counter = up_value > down_value ? (ugd_counter + 1) : 0;
		dgu_counter = down_value > up_value ? (dgu_counter + 1) : 0;

		(*out)[i].set_data(ugd_column.toStdString(), ugd_counter > 0 ? std::to_string(ugd_counter) : "0");
		(*out)[i].set_data(dgu_column.toStdString(), dgu_counter > 0 ? std::to_string(dgu_counter) : "0");

#if DISABLE_INCOMPLETE_HISTOGRAM
		if ((*out)[i].completed != CompleteStatus::INCOMPLETE_PENDING &&
				(*out)[i].completed != CompleteStatus::INCOMPLETE_DUMMY &&
				(*out)[i].completed != CompleteStatus::INCOMPLETE)
#endif
		{
			q.exec("select " + ugd_rank_column + " from bardata"
						 " where " + ugd_column + "<=" + QString::number(ugd_counter) +
						 " order by " + ugd_column + " desc limit 1");

			QUERY_ERROR_CODE (q, errcode, "up/down threshold");

			(*out)[i].set_data(ugd_rank_column.toStdString(),
												 q.next()? QString::number(q.value(0).toDouble(),'f',4).toStdString() : "0");

			q.exec("select " + dgu_rank_column + " from bardata"
						 " where " + dgu_column + "<=" + QString::number(dgu_counter) +
						 " order by " + dgu_column + " desc limit 1");

			QUERY_ERROR_CODE (q, errcode, "up/down threshold");

			(*out)[i].set_data(dgu_rank_column.toStdString(),
												 q.next()? QString::number(q.value(0).toDouble(),'f',4).toStdString() : "0");
		}
	}

	return errcode;
}

/** @deprecated, @experimental **/
/*int BardataCalculator::update_dayrange(std::vector<BardataTuple> *out, const QSqlDatabase &database, const QSqlDatabase &daily_database) {
	if (out == NULL || (*out).size() == 0) return 0;

	QSqlQuery q(database);
//  QSqlQuery q_daily(database);
	QString closing_time = "17:00"; // dummy
	QString current_time = "";
	QString prev_time = "";
	int out_size = static_cast<int>((*out).size());

	q.setForwardOnly(true);
//  q_daily.setForwardOnly(true);
//  q_daily.exec("select date_, high_, low_ from bardata where date_ <='" + QString::fromStdString((*out)[0].date) + "'"
//               " order by rowid desc limit 3");

	q.exec("select date_, time_, close_,"
				 " dayrangehigh_1day, dayrangehigh_2day, dayrangehigh_3day,"
				 " dayrangelow_1day, dayrangelow_2day, dayrangelow_3day"
				 " from bardata where completed=1 order by rowid desc");

	for (int i = 0; i < out_size; ++i) {

		// current time
		// prev time

//    // first bar after closing time (special handling)
//    if (closing_time != "" && last_parent_time > closing_time && first_parent_time == closing_time) {
//      dayrange_high[0] = price_high - price_close;
//      dayrange_high[1] = std::max(last_dayrange_high[0] + last_close, price_high) - price_close;
//      dayrange_high[2] = std::max(last_dayrange_high[1] + last_close, price_high) - price_close;

//      dayrange_low[0] = price_close - price_low;
//      dayrange_low[1] = price_close - std::min(last_close - last_dayrange_low[0], price_low);
//      dayrange_low[2] = price_close - std::min(last_close - last_dayrange_low[1], price_low);
//    }
//    // normal case
//    else {
//      dayrange_high[0] = std::max(last_dayrange_high[0] + last_close, price_high) - price_close;
//      dayrange_high[1] = std::max(last_dayrange_high[1] + last_close, price_high) - price_close;
//      dayrange_high[2] = std::max(last_dayrange_high[2] + last_close, price_high) - price_close;

//      dayrange_low[0] = price_close - std::min(last_close - last_dayrange_low[0], price_low);
//      dayrange_low[1] = price_close - std::min(last_close - last_dayrange_low[1], price_low);
//      dayrange_low[2] = price_close - std::min(last_close - last_dayrange_low[2], price_low);
//    }

	}
}*/

// TODO: require { dayrange atr } in m_cache
/*int BardataCalculator::update_dayrange_rank_approx_v2(std::vector<BardataTuple> *out, const QSqlDatabase &database) {
	QSqlQuery q(database);
	QString _rowid, dayrange_atr ;
	int out_length = static_cast<int>(out->size());
	int errcode = 0;

	q.setForwardOnly(true);

	for (int i = 0; i < out_length; ++i) {
		_rowid = QString::number((*out)[i].rowid);

		// High 1
		dayrange_atr = QString::fromStdString((*out)[i].get_data_string (bardata::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY));

		q.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY + " from bardata"
					 " where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + " is not null and "
										 + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + "<=" + dayrange_atr +
					 " and rowid <=" + _rowid +
					 " order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "DayRangeRank H1");

		(*out)[i].set_data (bardata::COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY, q.next() ?
												q.value(0).toString().toStdString() : "0");

		// High 2
		dayrange_atr = QString::fromStdString((*out)[i].get_data_string (bardata::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY));

		q.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY + " from bardata"
					 " where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + " is not null and "
										 + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + "<=" + dayrange_atr +
					 " and rowid <=" + _rowid +
					 " order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "DayRangeRank H2");

		(*out)[i].set_data (bardata::COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY, q.next() ?
												q.value(0).toString().toStdString() : "0");

		// High 3
		dayrange_atr = QString::fromStdString((*out)[i].get_data_string (bardata::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY));

		q.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY + " from bardata"
					 " where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + " is not null and "
										 + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + "<=" + dayrange_atr +
					 " and rowid <=" + _rowid +
					 " order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "DayRangeRank H3");

		(*out)[i].set_data (bardata::COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY, q.next() ?
												q.value(0).toString().toStdString() : "0");

		// Low 1
		dayrange_atr = QString::fromStdString((*out)[i].get_data_string (bardata::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY));

		q.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY + " from bardata"
					 " where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + " is not null and "
										 + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + "<=" + dayrange_atr +
					 " and rowid <=" + _rowid +
					 " order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "DayRangeRank L1");

		(*out)[i].set_data (bardata::COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY, q.next() ?
												q.value(0).toString().toStdString() : "0");

		// Low 2
		dayrange_atr = QString::fromStdString((*out)[i].get_data_string (bardata::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY));

		q.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY + " from bardata"
					 " where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + "> 0 and "
										 + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + "<=" + dayrange_atr +
					 " and rowid <=" + _rowid +
					 " order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "DayRangeRank L2");

		(*out)[i].set_data (bardata::COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY, q.next() ?
												q.value(0).toString().toStdString() : "0");

		// Low 3
		dayrange_atr = QString::fromStdString((*out)[i].get_data_string (bardata::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY));

		q.exec("select " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY + " from bardata"
					 " where " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY + "> 0 and "
										 + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY + "<=" + dayrange_atr +
					 " and rowid <=" + _rowid +
					 " order by " + SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY + " desc limit 1");

		QUERY_ERROR_CODE (q, errcode, "DayRangeRank L3");

		(*out)[i].set_data (bardata::COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY, q.next() ?
												q.value(0).toString().toStdString() : "0");
	}

	return errcode;
}*/

/*bool BardataCalculator::update_zone_v2(std::vector<BardataTuple> *out, const QSqlDatabase &database, const QString &database_path, const QString &symbol, const IntervalWeight base_interval, int start_rowid)
{
	QSqlQuery q (database);
	q.setForwardOnly (true);

	QSqlQuery q_update(database);
	QStringList database_alias;
	QStringList table_alias;
	QStringList projection;
	QString sql_select = "";
	QString _rowid = QString::number(start_rowid);
	IntervalWeight w = Globals::getParentInterval(base_interval);
	double m_open, m_high, m_low, m_close, m_fast, m_slow;
	int index;
	bool result_code = true;

//  // Init dummy field name { Monthly, Weekly, Daily, 60min, 5min }
//  for (int i = 0; i < 5; ++i)
//  {
//    projection.push_back("0"); // Fast
//    projection.push_back("0"); // Slow
//  }

//  while (w != WEIGHT_INVALID)
//  {
//    table_alias.push_back ("t_" + Globals::intervalWeightToString(w));
//    database_alias.push_back ("db" + table_alias.last());
//    q.exec ("attach database '" + database_path + "/" + SQLiteHandler::getDatabaseName(symbol, w) + "' AS " + database_alias.last());

//    switch (w)
//    {
//      case WEIGHT_MONTHLY: index = 0; break;
//      case WEIGHT_WEEKLY: index = 2; break;
//      case WEIGHT_DAILY: index = 4; break;
//      case WEIGHT_60MIN: index = 6; break;
//      case WEIGHT_5MIN: index = 8; break;
//      default: index = -1;
//    }

//    projection[index] = table_alias.last() + ".fastavg";
//    projection[index+1] = table_alias.last() + ".slowavg";
//    w = Globals::getParentInterval(w);
//  }

//  sql_select +=
//    "select A.rowid, A.open_, A.high_, A.low_, A.close_, A.fastavg, A.slowavg," + projection.join(",") +
//    " from bardata A";

//  if (database_alias.size() > 0)
//  {
//    QString prev_table_alias = table_alias[0];

//    sql_select +=
//      " LEFT JOIN " + database_alias[0] + ".bardata " + table_alias[0] +
//      " ON A._parent=" + table_alias[0] + ".rowid";

//    for (int i = 1; i < database_alias.size(); ++i)
//    {
//      sql_select +=
//        " LEFT JOIN " + database_alias[i] + ".bardata " + table_alias[i] +
//        " ON " + prev_table_alias + "._parent=" + table_alias[i] + ".rowid";

//      prev_table_alias = table_alias[i];
//    }
//  }

//  sql_select += " WHERE A.rowid >" + _rowid;
//  q.exec (sql_select);
//  QUERY_LAST_ERROR (q,"update zone");

//  BEGIN_TRANSACTION (q_update);
//  q_update.prepare ("update bardata set " +
//                      SQLiteHandler::COLUMN_NAME_OPEN_ZONE +"=?," +
//                      SQLiteHandler::COLUMN_NAME_OPEN_ZONE_MONTHLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_OPEN_ZONE_WEEKLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_OPEN_ZONE_DAILY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_OPEN_ZONE_60MIN + "=?," +
//                      SQLiteHandler::COLUMN_NAME_HIGH_ZONE +"=?," +
//                      SQLiteHandler::COLUMN_NAME_HIGH_ZONE_MONTHLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_HIGH_ZONE_WEEKLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_HIGH_ZONE_DAILY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_HIGH_ZONE_60MIN + "=?," +
//                      SQLiteHandler::COLUMN_NAME_LOW_ZONE +"=?," +
//                      SQLiteHandler::COLUMN_NAME_LOW_ZONE_MONTHLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_LOW_ZONE_WEEKLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_LOW_ZONE_DAILY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_LOW_ZONE_60MIN + "=?," +
//                      SQLiteHandler::COLUMN_NAME_CLOSE_ZONE +"=?," +
//                      SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_MONTHLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_WEEKLY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_DAILY + "=?," +
//                      SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_60MIN + "=?" +
//                    " where rowid=?");
//  QUERY_ERROR (q_update,"update zone");

	while (q.next())
	{
		m_open  = q.value(1).toDouble();
		m_high  = q.value(2).toDouble();
		m_low   = q.value(3).toDouble();
		m_close = q.value(4).toDouble();
		m_fast  = q.value(5).toDouble();
		m_slow  = q.value(6).toDouble();

//    q_update.bindValue (0, check_zone (m_open, m_fast, m_slow));
//    q_update.bindValue (1, check_zone (m_open, q.value(7).toDouble(), q.value(8).toDouble()));
//    q_update.bindValue (2, check_zone (m_open, q.value(9).toDouble(), q.value(10).toDouble()));
//    q_update.bindValue (3, check_zone (m_open, q.value(11).toDouble(), q.value(12).toDouble()));
//    q_update.bindValue (4, check_zone (m_open, q.value(13).toDouble(), q.value(14).toDouble()));
//    q_update.bindValue (5, check_zone (m_high, m_fast, m_slow));
//    q_update.bindValue (6, check_zone (m_high, q.value(7).toDouble(), q.value(8).toDouble()));
//    q_update.bindValue (7, check_zone (m_high, q.value(9).toDouble(), q.value(10).toDouble()));
//    q_update.bindValue (8, check_zone (m_high, q.value(11).toDouble(), q.value(12).toDouble()));
//    q_update.bindValue (9, check_zone (m_high, q.value(13).toDouble(), q.value(14).toDouble()));
//    q_update.bindValue (10, check_zone (m_low, m_fast, m_slow));
//    q_update.bindValue (11, check_zone (m_low, q.value(7).toDouble(), q.value(8).toDouble()));
//    q_update.bindValue (12, check_zone (m_low, q.value(9).toDouble(), q.value(10).toDouble()));
//    q_update.bindValue (13, check_zone (m_low, q.value(11).toDouble(), q.value(12).toDouble()));
//    q_update.bindValue (14, check_zone (m_low, q.value(13).toDouble(), q.value(14).toDouble()));
//    q_update.bindValue (15, check_zone (m_close, m_fast, m_slow));
//    q_update.bindValue (16, check_zone (m_close, q.value(7).toDouble(), q.value(8).toDouble()));
//    q_update.bindValue (17, check_zone (m_close, q.value(9).toDouble(), q.value(10).toDouble()));
//    q_update.bindValue (18, check_zone (m_close, q.value(11).toDouble(), q.value(12).toDouble()));
//    q_update.bindValue (19, check_zone (m_close, q.value(13).toDouble(), q.value(14).toDouble()));
//    q_update.bindValue (20, q.value(0));
//    q_update.exec();
	}

//  END_TRANSACTION (q_update);
	return result_code;
}*/

/*void BardataCalculator::update_candle_approx_v3(const QSqlDatabase &database) {
	QSqlQuery qUpdate(database);
	QSqlQuery qSelect(database);
	QSqlQuery q(database);
	q.setForwardOnly(true);

	QString update_bardata;
	QString select_bardata;
	QString _rowid;
	const int row_count = static_cast<int>(out->size());

	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("begin transaction;");

	update_bardata =
		"update bardata set " +
			SQLiteHandler::COLUMN_NAME_N_UPTAIL + "=?," +
			SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL_RANK  + "=?," +
			SQLiteHandler::COLUMN_NAME_N_DOWNTAIL  + "=?," +
			SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL_RANK  + "=?," +
			SQLiteHandler::COLUMN_NAME_N_TOTALLENGTH  + "=?," +
			SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK  + "=?," +
			SQLiteHandler::COLUMN_NAME_N_BODY  + "=?," +
			SQLiteHandler::COLUMN_NAME_CANDLE_BODY_RANK  + "=?" +
		" where rowid=?";

	for (int i = 0; i < row_count; ++i) {
		_rowid = std::to_string((*out)[i].rowid);

		select_bardata =
			"select " + bardata::COLUMN_NAME_N_UPTAIL + "," + bardata::COLUMN_NAME_CANDLE_UPTAIL_RANK +
			" from bardata" +
			" where " + bardata::COLUMN_NAME_CANDLE_UPTAIL + "<=" + QString::number((*out)[i].candle_uptail).toStdString() + " and " +
									bardata::COLUMN_NAME_ROWID + "<" + _rowid +
			" order by " + bardata::COLUMN_NAME_CANDLE_UPTAIL + " desc limit 1";

		q.exec(QString::fromStdString(select_bardata));

		if ( q.next() ) {
			(*out)[i].candle_uptail_normal = q.value(0).toDouble();
			(*out)[i].candle_uptail_rank = q.value(1).toDouble();
		}

		select_bardata =
			"select " + bardata::COLUMN_NAME_N_DOWNTAIL + "," + bardata::COLUMN_NAME_CANDLE_DOWNTAIL_RANK +
			" from bardata"
			" where " + bardata::COLUMN_NAME_CANDLE_DOWNTAIL + "<=" + QString::number((*out)[i].candle_downtail).toStdString() + " and " +
									bardata::COLUMN_NAME_ROWID + "<" + _rowid +
			" order by " + bardata::COLUMN_NAME_CANDLE_DOWNTAIL + " desc limit 1";

		q.exec(QString::fromStdString(select_bardata));
		if ( q.next() ) {
			(*out)[i].candle_downtail_normal = q.value(0).toDouble();
			(*out)[i].candle_downtail_rank = q.value(1).toDouble();
		}

		select_bardata =
			"select " + bardata::COLUMN_NAME_N_BODY + "," + bardata::COLUMN_NAME_CANDLE_BODY_RANK +
			" from bardata"
			" where " + bardata::COLUMN_NAME_CANDLE_BODY + "<=" + QString::number((*out)[i].candle_body).toStdString() + " and " +
									bardata::COLUMN_NAME_ROWID + "<" + _rowid +
			" order by " + bardata::COLUMN_NAME_CANDLE_BODY + " desc limit 1";

		q.exec(QString::fromStdString(select_bardata));
		if ( q.next() ) {
			(*out)[i].candle_body_normal = q.value(0).toDouble();
			(*out)[i].candle_body_rank = q.value(1).toDouble();
		}

		select_bardata =
			"select " + bardata::COLUMN_NAME_N_TOTALLENGTH + "," + bardata::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK +
			" from bardata"
			" where " + bardata::COLUMN_NAME_CANDLE_TOTALLENGTH + "<=" + QString::number((*out)[i].candle_totallength).toStdString() + " and " +
									bardata::COLUMN_NAME_ROWID + "<" + _rowid +
			" order by " + bardata::COLUMN_NAME_CANDLE_TOTALLENGTH + " desc limit 1";

		q.exec(QString::fromStdString(select_bardata));
		if ( q.next() ) {
			(*out)[i].candle_totallength_normal = q.value(0).toDouble();
			(*out)[i].candle_totallength_rank = q.value(1).toDouble();
		}
	}

	qUpdate.exec("COMMIT");
}*/

/*void BardataCalculator::update_macd_rank_approx(const QSqlDatabase &database, const int &start_rowid) {
	QSqlQuery qUpdate(database);
	QSqlQuery query(database);
	QSqlQuery qSelect(database);
	QString _start_rowid = QString::number(start_rowid);
	long double _macd;

	qSelect.setForwardOnly(true);
	query.setForwardOnly(true);
	query.exec("select rowid, MACD from bardata where rowid>" + _start_rowid);

	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");
	qUpdate.prepare("update bardata set " + SQLiteHandler::COLUMN_NAME_MACD_RANK + "=? where rowid =?");

	while (query.next()) {
		_macd = query.value(1).toDouble();

		if (_macd > 0) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_MACD_RANK + " from bardata"
									 " where MACD>0 and MACD<=" + query.value(1).toString() +
									 " and rowid<=" + _start_rowid +
									 " order by MACD desc limit 1");
			qUpdate.bindValue(0, qSelect.next()? qSelect.value(0).toDouble() : 0);
		}
		else if (_macd < 0) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_MACD_RANK + " from bardata"
									 " where MACD<0 and MACD>=" + query.value(1).toString() +
									 " and rowid<=" + _start_rowid +
									 " order by MACD asc limit 1");
			qUpdate.bindValue(0, qSelect.next()? qSelect.value(0).toDouble() : 0);
		} else {
			qUpdate.bindValue(0, 0);
		}

		qUpdate.bindValue(1, query.value(0).toInt());
		qUpdate.exec();
	}

	qUpdate.exec("COMMIT;");
}*/

/*void BardataCalculator::update_rank_distfs_distcc_atr_approx(const QSqlDatabase &database, const int &start_rowid) {
	QSqlQuery qUpdate(database);
	QSqlQuery qSelect(database);
	QSqlQuery query_1(database);
	QString _rowid = QString::number(start_rowid);
	long double dist_fscross;
	long double dist_fscross_rank = 0;
	long double dist_fs_rank = 0;
	long double atr_rank = 0;

	query_1.setForwardOnly(true);
	qSelect.setForwardOnly(true);

	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");
	qUpdate.prepare(
		"update bardata set " +
			SQLiteHandler::COLUMN_NAME_DISTFS_RANK + "=?," +
			SQLiteHandler::COLUMN_NAME_ATR_RANK + "=?," +
			SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS_RANK + "=?" +
		" where rowid=?");

	query_1.exec("select rowid, ATR, N_DistFS, DistCC_FSCross_ATR from bardata where rowid>" + _rowid);

	while (query_1.next()) {
		qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_ATR_RANK + " from bardata"
								 " where ATR<=" + query_1.value(1).toString() + " and rowid<=" + _rowid +
								 " order by ATR desc limit 1");

		atr_rank = qSelect.next() ? qSelect.value(0).toDouble() : 0;

		qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_DISTFS_RANK + " from bardata"
								 " where N_DistFS<=" + query_1.value(2).toString() + " and rowid<=" + _rowid +
								 " order by N_DistFS desc limit 1");

		dist_fs_rank = qSelect.next() ? qSelect.value(0).toDouble() : 0;

		dist_fscross = query_1.value(3).toDouble();

		if (dist_fscross >= 0) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS_RANK + " from bardata"
									 " where DistCC_FSCross_ATR >= 0 and rowid<" + _rowid + " and "
													"DistCC_FSCross_ATR<=" + query_1.value(3).toString() +
									 " order by DistCC_FSCross_ATR desc limit 1");
		} else {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS_RANK + " from bardata"
									 " where DistCC_FSCross_ATR < 0 and rowid<" + _rowid + " and "
													"DistCC_FSCross_ATR>=" + query_1.value(3).toString() +
									 " order by DistCC_FSCross_ATR asc limit 1");
		}

		dist_fscross_rank = qSelect.next() ? qSelect.value(0).toDouble() : 0;

		qUpdate.bindValue(0, QString::number(dist_fs_rank,'f',4));
		qUpdate.bindValue(1, QString::number(atr_rank,'f',5));
		qUpdate.bindValue(2, QString::number(dist_fscross_rank,'f',4));
		qUpdate.bindValue(3, query_1.value(0));
		qUpdate.exec();
	}

	qUpdate.exec("COMMIT;");
}*/

/*void BardataCalculator::update_fgs_fls_rank_approx(const QSqlDatabase &database, const int &start_rowid) {
	QSqlQuery query(database);
	QSqlQuery qUpdate(database);
	QSqlQuery qSelect(database);
	QString _start_rowid = QString::number(start_rowid);
	long double _fgs_rank, _fls_rank;
	int _fgs, _fls;

	qSelect.setForwardOnly(true);
	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");
	qUpdate.prepare(
		"update bardata set " +
			SQLiteHandler::COLUMN_NAME_FGS_RANK + "=?," +
			SQLiteHandler::COLUMN_NAME_FLS_RANK + "=?" +
		" where rowid=?");

	query.setForwardOnly(true);
	query.exec("select rowid, FGS, FLS from bardata where rowid>" + _start_rowid);

	while (query.next()) {
		_fgs = query.value(1).toInt();
		_fls = query.value(2).toInt();

		if (_fgs > 0) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_FGS_RANK +
									 " from bardata where fgs>0 and fgs<=" + QString::number(_fgs) +
									 " and rowid<=" + _start_rowid +
									 " order by fgs desc limit 1");

			_fgs_rank = qSelect.next() ? qSelect.value(0).toDouble() : 0;
			qUpdate.bindValue(0, QString::number(_fgs_rank,'f',4));
		} else {
			qUpdate.bindValue(0, QVariant());
		}

		if (_fls > 0) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_FLS_RANK +
									 " from bardata where fls>0 and fls<=" + QString::number(_fls) +
									 " and rowid<=" + _start_rowid +
									 " order by fls desc limit 1");
			_fls_rank = qSelect.next() ? qSelect.value(0).toDouble() : 0;
			qUpdate.bindValue(1, QString::number(_fls_rank,'f',4));
		} else {
			qUpdate.bindValue(1, QVariant());
		}

		qUpdate.bindValue(2, query.value(0).toInt());
		qUpdate.exec();
	}

	qUpdate.exec("COMMIT;");
}*/

/*void BardataCalculator::update_candle(const string &database_name) {
	sqlite3 *db = 0;
	sqlite3_stmt *stmt = 0;
	const char* select_count = "select count(1) from bardata";
	string _rowcount;
	string sql_select;
	int rc;

	rc = sqlite3_open(database_name.c_str(), &db);

	if (rc != SQLITE_OK) {
		sqlite3_close(db);
		return;
	}

	rc = sqlite3_prepare_v2(db, select_count, static_cast<int>(strlen(select_count)), &stmt, 0);

	if (rc == SQLITE_OK) {
		sqlite3_step(stmt);
		_rowcount = std::to_string(sqlite3_column_int(stmt, 0));
	}

	sqlite3_finalize(stmt);

	sql_select =
		"select"
		" round(sum(Candle_Uptail)/" + _rowcount + ",8),"
		" round(sum(Candle_Downtail)/" + _rowcount + ",8),"
		" round(sum(Candle_Body)/" + _rowcount + ",8),"
		" round(sum(Candle_TotalLength)/" + _rowcount + ",8)"
		" from bardata";

	sqlite3_prepare_v2(db, sql_select.c_str(), static_cast<int>(sql_select.length()), &stmt, 0);
	sqlite3_step(stmt);

	double avg_uptail = sqlite3_column_double(stmt, 0);
	double avg_downtail = sqlite3_column_double(stmt, 1);
	double avg_body = sqlite3_column_double(stmt, 2);
	double avg_totallength = sqlite3_column_double(stmt, 3);

	sqlite3_finalize(stmt);

	sql_select =
		"select"
		" round(sum((candle_uptail-" + std::to_string(avg_uptail) + ")*"
							 "(candle_uptail-" + std::to_string(avg_uptail) + "))/" + _rowcount + ",8),"
		" round(sum((candle_downtail-" + std::to_string(avg_downtail) + ")*"
							 "(candle_downtail-" + std::to_string(avg_downtail) + "))/" + _rowcount + ",8),"
		" round(sum((candle_body-" + std::to_string(avg_body) + ")*"
							 "(candle_body-" + std::to_string(avg_body) + "))/" + _rowcount + ",8),"
		" round(sum((candle_totallength-" + std::to_string(avg_totallength) + ")*"
							 "(candle_totallength-" + std::to_string(avg_totallength) + "))/" + _rowcount + ",8)"
		" from bardata order by rowid asc";

	sqlite3_prepare_v2(db, sql_select.c_str(), static_cast<int>(sql_select.length()), &stmt, 0);
	sqlite3_step(stmt);

	double stddev_uptail = std::sqrt(sqlite3_column_double(stmt,0));
	double stddev_downtail = std::sqrt(sqlite3_column_double(stmt,1));
	double stddev_body = std::sqrt(sqlite3_column_double(stmt,2));
	double stddev_totallength = std::sqrt(sqlite3_column_double(stmt,3));

	sqlite3_finalize(stmt);

	sql_select =
		"update bardata set"
		" n_uptail=round((candle_uptail-" + std::to_string(avg_uptail) + ")/" + std::to_string(stddev_uptail) + ",4),"
		" n_downtail=round((candle_downtail-" + std::to_string(avg_downtail) + ")/" + std::to_string(stddev_downtail) + ",4),"
		" n_body=round((candle_body-" + std::to_string(avg_body) + ")/" + std::to_string(stddev_body) + ",4),"
		" n_totallength=round((candle_totallength-" + std::to_string(avg_totallength) + ")/" + std::to_string(stddev_totallength) + ",4)";

	const char *begin_transaction =
		"PRAGMA journal_mode = OFF;"
		"PRAGMA synchronous = OFF;"
		"PRAGMA cache_size = 20000;"
		"BEGIN TRANSACTION;";

	sqlite3_exec(db, begin_transaction, 0, 0, 0);
	sqlite3_exec(db, sql_select.c_str(), 0, 0, 0);
	sqlite3_exec(db, "COMMIT;", 0, 0, 0);
	sqlite3_close(db);
}*/

/*void BardataCalculator::update_candle_rank_approx(const QSqlDatabase &database, const int &start_rowid) {
	QSqlQuery qUpdate(database);
	QSqlQuery qSelect(database);
	QSqlQuery query_1(database);
	QString _rowid = QString::number(start_rowid);
	long int sum_1 = 0;
	long int sum_2 = 0;
	long int sum_3 = 0;
	long int sum_4 = 0;

	qSelect.setForwardOnly(true);
	query_1.setForwardOnly(true);
	query_1.exec("select count(1) from bardata");
	double _count = query_1.next()? query_1.value(0).toDouble() : 0;

	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");
	qUpdate.prepare("update bardata set "
									"candle_uptail_rank=?, "
									"candle_downtail_rank=?,"
									"candle_body_rank=?,"
									"candle_totallength_rank=?"
									" where rowid=?");

	query_1.exec("select rowid, n_uptail, n_downtail, n_body, n_totallength from bardata where rowid>" + _rowid);

	while (query_1.next()) {
		qSelect.exec("select count(1) from bardata where n_uptail<=" + query_1.value(1).toString());
		sum_1 = qSelect.next()? qSelect.value(0).toInt() : 0;

		qSelect.exec("select count(1) from bardata where n_downtail<=" + query_1.value(2).toString());
		sum_2 = qSelect.next()? qSelect.value(0).toInt() : 0;

		qSelect.exec("select count(1) from bardata where n_body<=" + query_1.value(3).toString());
		sum_3 = qSelect.next()? qSelect.value(0).toInt() : 0;

		qSelect.exec("select count(1) from bardata where n_totallength<=" + query_1.value(4).toString());
		sum_4 = qSelect.next()? qSelect.value(0).toInt() : 0;

		qUpdate.bindValue(0, QString::number(sum_1 / _count,'f',4));
		qUpdate.bindValue(1, QString::number(sum_2 / _count,'f',4));
		qUpdate.bindValue(2, QString::number(sum_3 / _count,'f',4));
		qUpdate.bindValue(3, QString::number(sum_4 / _count,'f',4));
		qUpdate.bindValue(4, query_1.value(0));
		qUpdate.exec();
	}

	qUpdate.exec("COMMIT");
}*/

/*void BardataCalculator::update_distOHLC_approx(const QSqlDatabase &database, const QString &column_dist, const QString &column_n_dist, int year, int start_rowid) {
	QSqlQuery qUpdate(database);
	QSqlQuery qSelect(database);
	QSqlQuery query_1(database);
	QSqlQuery query_2(database);
	QString _rowid = QString::number(start_rowid);
	QString _year = QString::number(year);
	QString sql_select_1;
	QString sql_select_2;
	double count_1, count_2;
	long int _counter = 1;
	bool b1, b2;

	qSelect.setForwardOnly(true);
	query_1.setForwardOnly(true);
	query_2.setForwardOnly(true);

	// date_>='"+ _year + "-01-01'
	query_1.exec("select count(1) from bardata where date_>='"+ _year + "-01-01' and " + column_dist + ">0");
	count_1 = query_1.next()? query_1.value(0).toDouble() : 0;

	query_1.exec("select count(1) from bardata where date_>='"+ _year + "-01-01' and " + column_dist + "<0");
	count_2 = query_1.next()? query_1.value(0).toDouble() : 0;

	sql_select_1 = "select rowid," + column_n_dist + " from bardata"
								 " where rowid>"+ _rowid + " and " + column_dist + ">0"
								 " order by " + column_n_dist + " asc";

	sql_select_2 = "select rowid," + column_n_dist + " from bardata"
								 " where rowid>"+ _rowid + " and " + column_dist + "<0"
								 " order by " + column_n_dist + " desc";

	query_1.exec(sql_select_1);
	query_2.exec(sql_select_2);

	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");
	qUpdate.prepare("update bardata set " + column_dist + "_rank=? where rowid=?");

	while (true) {
		b1 = query_1.next();
		b2 = query_2.next();
		if (!b1 && !b2) break;

		if (b1) {
			qSelect.exec("select count(1) from bardata"
									 " where date_>='"+ _year + "-01-01' and " +
													 column_n_dist + ">0 and " + column_n_dist + "<=" + query_1.value(1).toString());
			_counter = qSelect.next()? qSelect.value(0).toInt() : 0;
			qUpdate.bindValue(0, QString::number(_counter / count_1, 'f', 4));
			qUpdate.bindValue(1, query_1.value(0));
			qUpdate.exec();
		}

		if (b2) {
			qSelect.exec("select count(1) from bardata"
									 " where date_>='"+ _year + "-01-01' and " +
													 column_n_dist + "<0 and " + column_n_dist + ">=" + query_2.value(1).toString());
			_counter = qSelect.next()? qSelect.value(0).toInt() : 0;
			qUpdate.bindValue(0, QString::number(_counter / count_2, 'f', 4));
			qUpdate.bindValue(1, query_2.value(0));
			qUpdate.exec();
		}
	}

	qUpdate.exec("COMMIT;");
}*/

/*void BardataCalculator::update_close_threshold_rank_approx(const QSqlDatabase &database, const int &id_threshold, const int start_rowid) {
	QSqlQuery qUpdate(database);
	QSqlQuery qSelect(database);
	QSqlQuery query_1(database);
	QSqlQuery query_2(database);
	QSqlQuery query_3(database);
	QSqlQuery query_4(database);
	QString tid = "_" + QString::number(id_threshold); // zero-based
	QString CGF = SQLiteHandler::COLUMN_NAME_CGF + tid;
	QString CLF = SQLiteHandler::COLUMN_NAME_CLF + tid;
	QString CGS = SQLiteHandler::COLUMN_NAME_CGS + tid;
	QString CLS = SQLiteHandler::COLUMN_NAME_CLS + tid;
	QString m_rowid = QString::number(start_rowid);
	QString _rank;
	bool b[4] = {false, false, false, false};

	qSelect.setForwardOnly(true);
	query_1.setForwardOnly(true);
	query_2.setForwardOnly(true);
	query_3.setForwardOnly(true);
	query_4.setForwardOnly(true);

	query_1.exec("select rowid," + CGF + " from bardata where " + CGF + ">0 and rowid>" + m_rowid);
	query_2.exec("select rowid," + CLF + " from bardata where " + CLF + ">0 and rowid>" + m_rowid);
	query_3.exec("select rowid," + CGS + " from bardata where " + CGS + ">0 and rowid>" + m_rowid);
	query_4.exec("select rowid," + CLS + " from bardata where " + CLS + ">0 and rowid>" + m_rowid);

	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");

	while (true) {
		b[0] = query_1.next();
		b[1] = query_2.next();
		b[2] = query_3.next();
		b[3] = query_4.next();
		if (!b[0] && !b[1] && !b[2] && !b[3]) break;

		if (b[0]) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_CGF_RANK + "," + CGF + " from bardata"
									 " where " + CGF + "<=" + query_1.value(1).toString() +
									 " order by " + CGF + " desc limit 1");
			_rank = qSelect.next()? qSelect.value(0).toString() : "0";
			qUpdate.exec("update bardata set " +
										SQLiteHandler::COLUMN_NAME_CGF_RANK + tid + "=" + _rank +
									" where rowid =" + query_1.value(0).toString());
		}

		if (b[1]) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_CLF_RANK + "," + CLF + " from bardata"
									 " where " + CLF + "<=" + query_2.value(1).toString() +
									 " order by " + CLF + " desc limit 1");
			_rank = qSelect.next()? qSelect.value(0).toString() : "0";
			qUpdate.exec("update bardata set " +
										SQLiteHandler::COLUMN_NAME_CLF_RANK + tid + "=" + _rank +
									" where rowid =" + query_2.value(0).toString());
		}

		if (b[2]) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_CGS_RANK + " from bardata"
									 " where " + CGS + "<=" + query_3.value(1).toString() +
									 " order by " + CGS + " desc limit 1");
			_rank = qSelect.next()? qSelect.value(0).toString() : "0";
			qUpdate.exec("update bardata set " +
										SQLiteHandler::COLUMN_NAME_CGS_RANK + tid + "=" + _rank +
									" where rowid =" + query_3.value(0).toString());
		}

		if (b[3]) {
			qSelect.exec("select " + SQLiteHandler::COLUMN_NAME_CLS_RANK + " from bardata"
									 " where " + CLS + "<=" + query_4.value(1).toString() +
									 " order by " + CLS + " desc limit 1");
			_rank = qSelect.next()? qSelect.value(0).toString() : "0";
			qUpdate.exec("update bardata set " +
										SQLiteHandler::COLUMN_NAME_CLS_RANK + tid + "=" + _rank +
									" where rowid =" + query_4.value(0).toString());
		}
	}

	qUpdate.exec("COMMIT;");
}*/

/*void BardataCalculator::update_close_threshold_v2(const QSqlDatabase &database, const QString &column_name, const int &id_threshold, const double &t_value, const int &start_rowid) {
	QSqlQuery qUpdate(database);
	QSqlQuery query(database);
	QString _prev_rowid = QString::number(start_rowid - 1);
	QString m_column_name = column_name + "_" + QString::number(id_threshold); // zero-based
	double m_close, m_fastavg, m_slowavg;
	int _counter = 0;
	bool is_greater_sign = (column_name == SQLiteHandler::COLUMN_NAME_CGF ||
													column_name == SQLiteHandler::COLUMN_NAME_CGS);
	bool is_collate_fast = (column_name == SQLiteHandler::COLUMN_NAME_CGF ||
													column_name == SQLiteHandler::COLUMN_NAME_CLF);

	query.setForwardOnly(true);
	query.exec("select rowid, close_, fastavg, slowavg," + m_column_name +
						 " from bardata where rowid>" + _prev_rowid +
						 " order by rowid asc");

	// get previous row information
	if (query.next()) {
		_counter = query.value(4).toInt();
	}

	qUpdate.exec("PRAGMA journal_mode = OFF;");
	qUpdate.exec("PRAGMA synchronous = OFF;");
	qUpdate.exec("PRAGMA cache_size = 20000;");
	qUpdate.exec("BEGIN TRANSACTION;");
	qUpdate.prepare("update bardata set " + m_column_name + "=? where rowid=?");

	// iterate from latest row
	while (query.next()) {
		m_close = query.value(1).toDouble();
		m_fastavg = query.value(2).toDouble();
		m_slowavg = query.value(3).toDouble();

		if (is_greater_sign) {
			if (is_collate_fast) {
				_counter = (m_close > m_fastavg - t_value)? _counter + 1 : 0;
			} else {
				_counter = (m_close > m_slowavg - t_value)? _counter + 1 : 0;
			}
		}
		else {
			if (is_collate_fast) {
				_counter = (m_close < m_fastavg + t_value)? _counter + 1 : 0;
			} else {
				_counter = (m_close < m_slowavg + t_value)? _counter + 1 : 0;
			}
		}

		if (_counter > 0) {
			qUpdate.bindValue(0, _counter);
			qUpdate.bindValue(1, query.value(0));
			qUpdate.exec();
		}
	}

	qUpdate.exec("COMMIT");
}*/

/*bool BardataCalculator::update_dist_resistance_approx_v2(const QSqlDatabase &database, const QString &database_path,
																													const QString &symbol, const IntervalWeight base_interval,
																													const int &id_threshold, const int &start_rowid)
{
	QTime time; time.start();
	QSqlQuery q (database);
	QSqlQuery q_select (database);
	QSqlQuery q_update (database);

	q.setForwardOnly (true);
	q_select.setForwardOnly (true);
	q_select.exec ("PRAGMA temp_store = FILE;");
	q_select.exec ("PRAGMA soft_heap_limit = 10000;");

	QStringList detach_stmt;
	QString db_path_2 = database_path + "/" + SQLiteHandler::getDatabaseName (symbol, WEIGHT_60MIN);
	QString db_path_3 = database_path + "/" + SQLiteHandler::getDatabaseName (symbol, WEIGHT_5MIN);
	QString db_daily = base_interval < WEIGHT_DAILY? "dbdaily." : "";
	bool result_code = true;

	const QString _start_rowid = QString::number (start_rowid);
	const QString _id_threshold = QString::number (id_threshold);
	const QString _id = "_" + QString::number (id_threshold + 1); // non-zero based
	const QString res_value = SQLiteHandler::COLUMN_NAME_RES + _id;
	const QString distres = SQLiteHandler::COLUMN_NAME_DISTRES + _id;
	const QString distres_atr = SQLiteHandler::COLUMN_NAME_DISTRES_ATR + _id;
	const QString distres_rank = SQLiteHandler::COLUMN_NAME_DISTRES_RANK + _id;
	const QString last_reactdate = SQLiteHandler::COLUMN_NAME_RES_LASTREACTDATE + _id;

	QString detach_temp = "";
	QString select_displacement = "";
	// original
	QString select_min_resistance =
		"select date_, c.react_date"
		" from (select b.rowid as xid, b.date_, min(resistance), resistance_count"
			" from " + db_daily + "resistancedata b"
			" where resistance_count >= 3 and id_threshold=" + _id_threshold +
			" group by b.date_) X join " + db_daily + "resistancereactdate c on x.xid=c.rid";

//  QString select_min_resistance =
//    "select date_, c.react_date"
//    " from (select b.rowid as xid, b.date_, min(resistance), resistance_count"
//      " from " + db_daily + "resistancedata b"
//      " where resistance_count >= 3 and id_threshold=" + _id_threshold +
//      " and b.date_ >='" + lookback_daily_date (symbol, database_path, 250) + "'"
//      " group by b.date_) X join " + db_daily + "resistancereactdate c on x.xid=c.rid";

	if (base_interval < WEIGHT_DAILY)
	{
		attach_memory_database (database_path, symbol, WEIGHT_DAILY, id_threshold, true, &q, &detach_temp);
		detach_stmt.push_back (detach_temp);

		// ## new mod
		// subselect base database 'a' to reduce the data
		select_displacement +=
			"select rowid, max(resistance), round((max(resistance)-close_),4), round((max(resistance)-close_)/atr,6), react_date"
			" from (select a.rowid, b.resistance, a.close_, y.atr, c.react_date from "
					"(select rowid, close_, _parent, _parent_prev from bardata where rowid >" + _start_rowid + ") a";

		// original
//    select_displacement +=
//      "select rowid, max(resistance), round((max(resistance)-close_),4), round((max(resistance)-close_)/atr,6), react_date"
//      " from (select a.rowid, b.resistance, a.close_, y.atr, c.react_date from bardata a";

		// 5min
		if (base_interval < WEIGHT_60MIN)
		{
			q.exec ("ATTACH DATABASE '" + db_path_2 + "' AS db60min");
			detach_stmt.push_back ("DETACH DATABASE db60min");
			select_displacement += " join db60min.bardata x on a._parent=x.rowid";
			select_displacement += " join dbdaily.bardata y on x._parent_prev=y.rowid";
		}
		// 1min
		else if (base_interval < WEIGHT_5MIN)
		{
			q.exec ("ATTACH DATABASE '" + db_path_2 + "' AS db60min");
			q.exec ("ATTACH DATABASE '" + db_path_3 + "' AS db5min");
			detach_stmt.push_back ("DETACH DATABASE db60min");
			detach_stmt.push_back ("DETACH DATABASE db5min");
			select_displacement += " join db5min.bardata z on a._parent=z.rowid";
			select_displacement += " join db60min.bardata x on z._parent=x.rowid";
			select_displacement += " join dbdaily.bardata y on x._parent_prev=y.rowid";
		}
		// 60min
		else
			select_displacement += " join dbdaily.bardata y on a._parent_prev=y.rowid";

		select_displacement +=
			" join dbdaily.resistancedata b on y.date_=b.date_ and y.time_=b.time_"
			" join dbdaily.resistancereactdate c on b.rowid=c.rid"
			" join (" + select_min_resistance + ") d on y.date_=d.date_ and c.react_date=d.react_date"
//      " where a.rowid >" + _start_rowid + " and " // original
//      " where b.resistance_count >= 3 and id_threshold=" + _id_threshold +
			" order by a.rowid, b.resistance desc, c.react_date desc"
			") group by rowid";
	}
	else
	{
		select_displacement =
			"select rowid, max(resistance), round((max(resistance)-close_),4), round((max(resistance)-close_)/atr,6), react_date"
			" from (select a.rowid, b.resistance, a.close_, a.ATR, c.react_date from bardata a"
			" join resistancedata b on a.date_=b.date_"
			" join resistancereactdate c on b.rowid=c.rid"
			" join (" + select_min_resistance + ") d on a.date_=d.date_ and c.react_date=d.react_date"
			" where a.rowid >" + _start_rowid +
			" and b.resistance_count>=3 and id_threshold=" + _id_threshold +
			" order by a.rowid, b.resistance desc, c.react_date desc"
			") group by rowid";
	}

	qDebug() << "elapsed 1 - prepare sql string:" << (time.elapsed()/1000.0) << "secs";
	qDebug() << select_displacement;

	q.exec (select_displacement);

	qDebug() << "elapsed 2 - after select:" << (time.elapsed()/1000.0) << "secs";

	QUERY_LAST_ERROR (q,"dist resistance approx");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare ("update bardata set " +
											res_value + "=?," +
											distres + "=?," +
											distres_atr + "=?," +
											last_reactdate +"=?"
										" where rowid=?");\

	while (q.next())
	{
		q_update.bindValue (0, q.value(1));
		q_update.bindValue (1, q.value(2));
		q_update.bindValue (2, q.value(3));
		q_update.bindValue (3, q.value(4));
		q_update.bindValue (4, q.value(0));
		q_update.exec();
	}

	END_TRANSACTION (q_update);

	qDebug() << "elapsed 3 - after update:" << (time.elapsed()/1000.0) << "secs";

	// detach database
	for (int i = 0; i < detach_stmt.size(); ++i)
		q.exec (detach_stmt[i]);

	// update distance resistance rank (positive value only)
	q.exec ("select rowid," + distres_atr + " from bardata where " + distres_atr + "> 0 and rowid >" + _start_rowid);
	QUERY_LAST_ERROR (q, "dist resistance approx (rank)");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare ("update bardata set " + distres_rank + "=? where rowid=?");

	while (q.next())
	{
		q_select.exec ("select " + distres_rank + " from bardata"
									 " where " + distres_atr + ">0 and " + distres_atr + "<=" + q.value(1).toString() +
									 " and rowid <=" + _start_rowid + " order by " + distres_atr + " desc limit 1");
		q_update.bindValue (0, QString::number (q_select.next() ? q_select.value(0).toDouble() : 0, 'f', 4));
		q_update.bindValue (1, q.value(0));
		q_update.exec ();
	}

	END_TRANSACTION (q_update);

	qDebug() << "elapsed 4 - after histogram:" << (time.elapsed()/1000.0) << "secs";

	return result_code;
}*/

/*bool BardataCalculator::update_dist_support_approx_v2(const QSqlDatabase &database, const QString &database_path,
																											 const QString &symbol, const IntervalWeight &interval,
																											 const int &id_threshold, const int &start_rowid)
{
	QSqlQuery q (database);
	QSqlQuery q_select (database);
	QSqlQuery q_update (database);

	q.setForwardOnly (true);
	q_select.setForwardOnly (true);
	q_select.exec ("PRAGMA temp_store = FILE;");
	q_select.exec ("PRAGMA soft_heap_limit = 10000;");

	QStringList detach_stmt;
	QString db60min_path = database_path + "/" + SQLiteHandler::getDatabaseName (symbol, WEIGHT_60MIN);
	QString db5min_path = database_path + "/" + SQLiteHandler::getDatabaseName (symbol, WEIGHT_5MIN);
	QString db_daily = interval < WEIGHT_DAILY ? "dbdaily." : "";
	bool result_code = true;

	const QString _start_rowid = QString::number (start_rowid);
	const QString _idthreshold = QString::number (id_threshold);
	const QString _id = "_" + QString::number (id_threshold + 1); // non-zero based
	const QString sup_value = SQLiteHandler::COLUMN_NAME_SUP + _id;
	const QString distsup = SQLiteHandler::COLUMN_NAME_DISTSUP + _id;
	const QString distsup_atr = SQLiteHandler::COLUMN_NAME_DISTSUP_ATR + _id;
	const QString distsup_rank = SQLiteHandler::COLUMN_NAME_DISTSUP_RANK + _id;
	const QString last_reactdate = SQLiteHandler::COLUMN_NAME_SUP_LASTREACTDATE + _id;

	QString detach_temp = "";
	QString select_displacement = "";
	QString select_max_support =
		"select date_, c.react_date"
		" from (select b.rowid as xid, b.date_, max(support), support_count"
				" from " + db_daily + "supportdata b"
				" where support_count >= 3 and id_threshold=" + _idthreshold +
				" group by b.date_) X join " + db_daily + "supportreactdate c on x.xid=c.rid";


	if (interval < WEIGHT_DAILY)
	{
		attach_memory_database (database_path, symbol, WEIGHT_DAILY, id_threshold, false, &q, &detach_temp);
		detach_stmt.push_back (detach_temp);

		// ## new mod
		// subselect base database 'a' to reduce the data
		select_displacement +=
			"select rowid, min(support), round((close_-min(support)),4), round((close_-min(support))/atr,6), react_date"
			" from (select a.rowid, b.support, a.close_, y.atr, c.react_date from  "
					"(select rowid, close_, _parent, _parent_prev from bardata where rowid >" + _start_rowid + ") a";

		// original
//    select_displacement +=
//      "select rowid, min(support), round((close_-min(support)),4), round((close_-min(support))/atr,6), react_date"
//      " from (select a.rowid, b.support, a.close_, y.atr, c.react_date from bardata a";

		// 5min
		if (interval < WEIGHT_60MIN)
		{
			q.exec ("attach database '" + db60min_path + "' AS db60min");
			detach_stmt.push_back ("DETACH DATABASE db60min");

			select_displacement +=
				" join db60min.bardata x on a._parent=x.rowid"
				" join dbDaily.bardata y on x._parent_prev=y.rowid";
		}
		// 1min
		else if (interval < WEIGHT_5MIN)
		{
			q.exec ("ATTACH DATABASE '" + db60min_path + "' AS db60min");
			q.exec ("ATTACH DATABASE '" + db5min_path + "' AS db5min");
			detach_stmt.push_back ("DETACH DATABASE db60min");
			detach_stmt.push_back ("DETACH DATABASE db5min");

			select_displacement +=
				" join db5min.bardata z on a._parent=x.rowid"
				" join db60min.bardata x on z._parent=x.rowid"
				" join dbDaily.bardata y on x._parent_prev=y.rowid";
		}
		// 60min
		else
			select_displacement += " join dbDaily.bardata y on a._parent_prev=y.rowid";

		select_displacement +=
			" join dbdaily.supportdata b on y.date_=b.date_ and y.time_=b.time_"
			" join dbdaily.supportreactdate c on b.rowid=c.rid"
			" join (" + select_max_support + ") d on y.date_=d.date_ and c.react_date=d.react_date"
//      " where a.rowid >" + _start_rowid +
//      " and support_count>=3 and id_threshold=" + _idthreshold +
			" order by a.rowid, b.support asc, c.react_date desc"
			") group by rowid";
	}
	else
	{
		// daily, weekly, monthly
		select_displacement =
			"select rowid, min(support), round((close_-min(support)),4), round((close_-min(support))/atr,6), react_date"
			" from(select a.rowid, b.support, a.close_, a.atr, c.react_date from bardata a"
			" join supportdata b on a.date_=b.date_"
			" join supportreactdate c on b.rowid=c.rid"
			" join (" + select_max_support + ") d on a.date_=d.date_ and c.react_date=d.react_date"
			" where a.rowid >" + _start_rowid +
			" and support_count>=3 and id_threshold=" + _idthreshold +
			" order by a.rowid, b.support asc, c.react_date desc"
			") group by rowid";
	}

//  qDebug() << "support displacement" << select_displacement;

	q.exec (select_displacement);
	QUERY_LAST_ERROR (q, "dist support approx");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare ("update bardata set " +
											sup_value + "=?," +
											distsup + "=?," +
											distsup_atr + "=?," +
											last_reactdate + "=?" +
										" where rowid=?");

	while (q.next())
	{
		q_update.bindValue (0, q.value(1));
		q_update.bindValue (1, q.value(2));
		q_update.bindValue (2, q.value(3));
		q_update.bindValue (3, q.value(4));
		q_update.bindValue (4, q.value(0));
		q_update.exec();
	}

	END_TRANSACTION (q_update);

	// detach database
	for (int i = 0; i < detach_stmt.size(); ++i)
		q.exec (detach_stmt[i]);

	// update distance support rank (positive value only)
	q.exec ("select rowid," + distsup_atr + " from bardata where " + distsup_atr + "> 0 and rowid >" + _start_rowid);
	QUERY_LAST_ERROR (q,"dist support approx (rank)");

	BEGIN_TRANSACTION (q_update);
	q_update.prepare ("update bardata set " + distsup_rank + "=? where rowid=?");

	while (q.next())
	{
		q_select.exec ("select " + distsup_rank + " from bardata"
									 " where " + distsup_atr + ">0 and " + distsup_atr + "<=" + q.value(1).toString() +
									 " and rowid <=" + _start_rowid + " order by " + distsup_atr + " desc limit 1");
		q_update.bindValue (0, QString::number (q_select.next() ? q_select.value(0).toDouble() : 0,'f',4));
		q_update.bindValue (1, q.value(0));
		q_update.exec();
	}

	END_TRANSACTION (q_update);

	return result_code;
}*/

/*void BardataCalculator::load_database(QSqlDatabase *out, const QString &filename) {
	QString connstring = "f_slope + QString::number(++connection_id);

	// don't destroy previous result
	if ( out->isOpen() ) return;

	// Open in-memory database
	*out = QSqlDatabase::addDatabase("QSQLITE", connstring);
	out->setDatabaseName(":memory:");
//  out->setDatabaseName("file:memdb1?mode=memory&cache=shared");

	if (!out->open()) {
		std::cout << "Database not opened: " << out->lastError().text().toStdString() << "\n";
		return;
	}

	QSqlQuery q(*out);
	q.exec("PRAGMA page_size = 65536;");
	q.exec("PRAGMA cache_size = 20000;");
	q.exec(SQLiteHandler::SQL_CREATE_TABLE_BARDATA_V2);
	q.exec(SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_V1);
	q.exec(SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_DATE_V1);
	q.exec(SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_V1);
	q.exec(SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_DATE_V1);
	q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT);
	q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV);
	q.exec(SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PREVDATETIME);
	q.exec("attach database '" + filename + "' as dbtempcopy;");
	q.exec("PRAGMA synchronous = OFF;");
	q.exec("PRAGMA journal_mode = OFF;");
	q.exec("begin transaction;");
	q.exec("insert into bardata select * from dbtempcopy.bardata");
	q.exec("insert into resistancedata select * from dbtempcopy.resistancedata");
	q.exec("insert into supportdata select * from dbtempcopy.supportdata");
	q.exec("insert into resistancereactdate select * from dbtempcopy.resistancereactdate");
	q.exec("insert into supportreactdate select * from dbtempcopy.supportreactdate");
	q.exec("commit;");
	q.exec("detach database dbtempcopy;");
}*/

/*void attach_memory_database(const QString &database_path, const QString &symbol, const IntervalWeight target_interval,
														 const int &id_threshold, const bool &is_resistance, QSqlQuery *out, QString *detach_stmt)
{
	// tailored specific for DistResistance and DistSupport calculation

	const QString _id = QString::number (id_threshold);
	const QString database_name = database_path + "/" + SQLiteHandler::getDatabaseName (symbol, target_interval);
	const QString interval_name = Globals::intervalWeightToString (target_interval);
	const QString mdb_alias = "mdbtemp"; // base database
	QString db_alias = "db" + interval_name; // in-memory database
	string s_db_alias = "db" + interval_name.toStdString();
	QString temp_bardata = db_alias + ".bardata";
	int lookback_bars = 0;

	if (target_interval >= WEIGHT_DAILY) lookback_bars = 250; else
	if (target_interval >= WEIGHT_60MIN) lookback_bars = 250 * 20; else
	if (target_interval >= WEIGHT_5MIN) lookback_bars = 250 * 20 * 12; else
	if (target_interval >= WEIGHT_1MIN) lookback_bars = 250 * 20 * 12 * 5;

	out->exec ("PRAGMA temp_store = FILE;");
//  out->exec ("PRAGMA soft_heap_limit = 20000;");
	out->exec ("PRAGMA cache_size = -32768;");
	out->exec ("PRAGMA page_size = 65536;");
	out->exec ("DETACH DATABASE " + QString::fromStdString (s_db_alias));
	out->exec ("ATTACH DATABASE ':memory:' AS " + QString::fromStdString (s_db_alias));
	out->exec ("ATTACH DATABASE '" + database_name + "' as " + mdb_alias);
	out->exec (QString::fromStdString (bardata::SQL_CREATE_TABLE_BARDATA_LITE_V2 (s_db_alias)));

	if (is_resistance)
	{
		out->exec (QString::fromStdString (bardata::SQL_CREATE_TABLE_RESISTANCE_LITE_V1 (s_db_alias)));
		out->exec (QString::fromStdString (bardata::SQL_CREATE_TABLE_RESISTANCE_DATE_V1 (s_db_alias)));
	}
	else
	{
		out->exec (QString::fromStdString (bardata::SQL_CREATE_TABLE_SUPPORT_LITE_V1 (s_db_alias)));
		out->exec (QString::fromStdString (bardata::SQL_CREATE_TABLE_SUPPORT_DATE_V1 (s_db_alias)));
	}

	out->exec ("select max(rowid) from " + mdb_alias + ".bardata");
	int rowid = (out->next() ? out->value(0).toInt() : 0) - lookback_bars;

	out->exec ("PRAGMA temp_store = FILE;");
	out->exec ("PRAGMA journal_mode = OFF;");
	out->exec ("PRAGMA synchronous = OFF;");
	out->exec ("BEGIN TRANSACTION;");
	out->exec ("insert into " + db_alias + ".bardata(rowid, date_, time_, atr, _parent, _parent_prev)"
						 "select rowid, date_, time_, atr, _parent, _parent_prev "
						 "from " + mdb_alias + ".bardata where rowid >" + QString::number(rowid));

#ifdef DEBUG_MODE
	if (out->lastError().isValid()) qDebug() << out->lastError();
#endif

	if (target_interval >= WEIGHT_DAILY)
	{
		// get start date from bardata
		out->exec ("select min(date_) from " + db_alias + ".bardata");
		QString first_date = out->next() ? out->value(0).toString() : "";

		if (is_resistance)
		{
			// ResistanceData
			out->exec ("insert into " + db_alias + ".resistancedata"
								 "(rowid, date_, time_, react_date, react_time, resistance_count, resistance, id_threshold)"
								 "select rowid, date_, time_, react_date, react_time, resistance_count, resistance, id_threshold"
								 " from " + mdb_alias + ".resistancedata where date_ >='" + first_date + "' and "
								 " id_threshold=" + _id + " and resistance_count >= 3");

#ifdef DEBUG_MODE
			if (out->lastError().isValid()) qDebug() << out->lastError();
#endif

			// Resistance ReactDate
			out->exec ("select min(rowid), max(rowid) from " + db_alias + ".resistancedata");

			if (out->next())
			{
				out->exec ("insert into " + db_alias + ".resistancereactdate "
									 "select * from " + mdb_alias + ".resistancereactdate"
									 " where rid >=" + out->value(0).toString() + " and rid <=" + out->value(1).toString());

#ifdef DEBUG_MODE
				if (out->lastError().isValid()) qDebug() << out->lastError();
#endif
			}
		}
		else
		{
			// SupportData
			out->exec ("insert into " + db_alias + ".supportdata"
								 "(rowid, date_, time_, react_date, react_time, support_count, support, id_threshold)"
								 "select rowid, date_, time_, react_date, react_time, support_count, support, id_threshold"
								 " from " + mdb_alias + ".supportdata where date_ >='" + first_date + "'"
								 " and id_threshold=" + _id + " and support_count >= 3");

#ifdef DEBUG_MODE
			if (out->lastError().isValid()) qDebug() << out->lastError();
#endif

			// Support ReactDate
			out->exec("select min(rowid), max(rowid) from " + db_alias + ".SupportData");

			if (out->next())
			{
				out->exec ("insert into " + db_alias + ".supportreactdate "
									 "select * from " + mdb_alias + ".supportreactdate"
									 " where rid>=" + out->value(0).toString() + " and rid <=" + out->value(1).toString());

#ifdef DEBUG_MODE
				if (out->lastError().isValid()) qDebug() << out->lastError();
#endif
			}
		}
	}

	out->exec ("COMMIT;");
	out->exec ("DETACH DATABASE " + mdb_alias);
	out->exec ("CREATE INDEX _temp_parent on " + temp_bardata + "(_parent);");
	out->exec ("CREATE INDEX _temp_parent_prev on " + temp_bardata + "(_parent_prev);");

	*detach_stmt = "DETACH DATABASE " + db_alias;
}*/
