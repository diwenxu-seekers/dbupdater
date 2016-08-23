#include "fileloader_v2.h"

#include "bardatadefinition.h"
#include "datetimehelper.h"
#include "searchbar/sqlitehandler.h"

#pragma warning(push,3)
#include <QStringList>
#include <QTextStream>
#pragma warning(pop)

#define LOG_ERROR(...) try{_logger->error(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_WARN(...) try{_logger->warn(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_INFO(...) try{_logger->info(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}

inline int get_moving_avg_state(double fast, double slow)
{
	return (fast > slow ? 1 : (fast < slow ? 2 : 3));
}

inline QString parse_date(const QString &mm_dd_yyyy)
{
	// convert MM/dd/yyyy into yyyy-MM-dd
	return mm_dd_yyyy.mid(6,4) + "-" + mm_dd_yyyy.mid(0,2) + "-" + mm_dd_yyyy.mid(3,2);
}

inline void parse_line_datetime(const QString &line, BardataTuple *results)
{
	results->date = parse_date(line.mid(0, 10)).toStdString();
	results->time = line.mid(11, 5).toStdString();
}

bool parse_line_all(const QString &line, BardataTuple *results)
{
	QStringList columns = line.split(",", QString::KeepEmptyParts);

	if (line.length() < 20) return false;

	results->date = parse_date(columns[0]).toStdString();
	results->time = columns[1].toStdString();
	results->open      = columns[2].toDouble();
	results->high      = columns[3].toDouble();
	results->low       = columns[4].toDouble();
	results->close     = columns[5].toDouble();
	results->volume    = columns[6].toInt();
	results->macd      = columns[7].toDouble();
	results->macd_avg  = columns[8].toDouble();
	results->macd_diff = columns[9].toDouble();
	results->rsi       = columns[10].toDouble();
	results->slow_k    = columns[11].toDouble();
	results->slow_d    = columns[12].toDouble();
	results->fast_avg  = columns[13].toDouble();
	results->slow_avg  = columns[14].toDouble();
	results->dist_f    = columns[15].toDouble();
	results->dist_s    = columns[16].toDouble();
	results->fgs       = columns[17].toInt();
	results->fls       = columns[18].toInt();
	results->openbar   = columns[19].toInt();
	results->set_data(bardata::COLUMN_NAME_MACD, columns[7].toStdString());
	results->set_data(bardata::COLUMN_NAME_RSI, columns[10].toStdString());
	results->set_data(bardata::COLUMN_NAME_SLOWK, columns[11].toStdString());
	results->set_data(bardata::COLUMN_NAME_SLOWD, columns[12].toStdString());

	return true;
}

void bind_and_exec_query(QSqlQuery &q, const BardataTuple &row)
{
	q.bindValue(0, row.date.c_str());
	q.bindValue(1, row.time.c_str());
	q.bindValue(2, row.open);
	q.bindValue(3, row.high);
	q.bindValue(4, row.low);
	q.bindValue(5, row.close);
	q.bindValue(6, row.volume);
	q.bindValue(7, row.macd);
	q.bindValue(8, row.macd_avg);
	q.bindValue(9, row.macd_diff);
	q.bindValue(10, row.rsi);
	q.bindValue(11, row.slow_k);
	q.bindValue(12, row.slow_d);
	q.bindValue(13, row.fast_avg);
	q.bindValue(14, row.slow_avg);
	q.bindValue(15, row.dist_f);
	q.bindValue(16, row.dist_s);
	q.bindValue(17, row.fgs);
	q.bindValue(18, row.fls);
	q.bindValue(19, row.openbar);
	q.bindValue(20, QString::fromStdString(row.prevdate));
	q.bindValue(21, QString::fromStdString(row.prevtime));
	q.bindValue(22, QString::fromStdString(row.prevbarcolor));
	q.bindValue(23, Globals::round_decimals(row.fastavg_slope, 4));
	q.bindValue(24, Globals::round_decimals(row.slowavg_slope, 4));
	q.bindValue(25, row.f_cross);
	q.bindValue(26, row.s_cross);
	q.bindValue(27, Globals::round_decimals(row.atr, 5));
	q.bindValue(28, Globals::round_decimals(row.candle_uptail, 5));
	q.bindValue(29, Globals::round_decimals(row.candle_downtail, 5));
	q.bindValue(30, Globals::round_decimals(row.candle_body, 5));
	q.bindValue(31, Globals::round_decimals(row.candle_totallength, 5));
	q.bindValue(32, Globals::round_decimals(row.dist_cc_fscross, 5));
	q.bindValue(33, Globals::round_decimals(row.dist_cc_fscross_atr, 5));
	q.bindValue(34, Globals::round_decimals(row.dist_fs, 5));
	q.bindValue(35, Globals::round_decimals(row.n_dist_fs, 8));
	q.bindValue(36, Globals::round_decimals(row.dist_of, 5));
	q.bindValue(37, Globals::round_decimals(row.n_dist_of, 8));
	q.bindValue(38, Globals::round_decimals(row.dist_os, 5));
	q.bindValue(39, Globals::round_decimals(row.n_dist_os, 8));
	q.bindValue(40, Globals::round_decimals(row.dist_hf, 5));
	q.bindValue(41, Globals::round_decimals(row.n_dist_hf, 8));
	q.bindValue(42, Globals::round_decimals(row.dist_hs, 5));
	q.bindValue(43, Globals::round_decimals(row.n_dist_hs, 8));
	q.bindValue(44, Globals::round_decimals(row.dist_lf, 5));
	q.bindValue(45, Globals::round_decimals(row.n_dist_lf, 8));
	q.bindValue(46, Globals::round_decimals(row.dist_ls, 5));
	q.bindValue(47, Globals::round_decimals(row.n_dist_ls, 8));
	q.bindValue(48, Globals::round_decimals(row.dist_cf, 5));
	q.bindValue(49, Globals::round_decimals(row.n_dist_cf, 8));
	q.bindValue(50, Globals::round_decimals(row.dist_cs, 5));
	q.bindValue(51, Globals::round_decimals(row.n_dist_cs, 8));
	q.bindValue(52, row.hlf);
	q.bindValue(53, row.hls);
	q.bindValue(54, row.lgf);
	q.bindValue(55, row.lgs);
	q.bindValue(56, row.fs_cross);
	q.bindValue(57, row._parent);
	q.bindValue(58, row._parent_prev);
	q.bindValue(59, row._parent_monthly);
	q.bindValue(60, row._parent_prev_monthly);
	q.bindValue(61, row.completed);
	q.bindValue(62, Globals::round_decimals(row.bar_range, 5));
	q.bindValue(63, Globals::round_decimals(row.intraday_high, 5));
	q.bindValue(64, Globals::round_decimals(row.intraday_low, 5));
	q.bindValue(65, Globals::round_decimals(row.intraday_range, 5));
	q.exec();
}

QString get_insert_string()
{
	QStringList projection;
	projection.push_back(SQLiteHandler::COLUMN_NAME_DATE);
	projection.push_back(SQLiteHandler::COLUMN_NAME_TIME);
	projection.push_back(SQLiteHandler::COLUMN_NAME_OPEN);
	projection.push_back(SQLiteHandler::COLUMN_NAME_HIGH);
	projection.push_back(SQLiteHandler::COLUMN_NAME_LOW);
	projection.push_back(SQLiteHandler::COLUMN_NAME_CLOSE);
	projection.push_back(SQLiteHandler::COLUMN_NAME_VOLUME);
	projection.push_back(SQLiteHandler::COLUMN_NAME_MACD);
	projection.push_back(SQLiteHandler::COLUMN_NAME_MACDAVG);
	projection.push_back(SQLiteHandler::COLUMN_NAME_MACDDIFF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_RSI);
	projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWK);
	projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWD);
	projection.push_back(SQLiteHandler::COLUMN_NAME_FASTAVG);
	projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWAVG);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_FGS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_FLS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_OPENBAR);
	projection.push_back(SQLiteHandler::COLUMN_NAME_PREV_DATE);
	projection.push_back(SQLiteHandler::COLUMN_NAME_PREV_TIME);
	projection.push_back(SQLiteHandler::COLUMN_NAME_PREV_BARCOLOR);
	projection.push_back(SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE);
	projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE);
	projection.push_back(SQLiteHandler::COLUMN_NAME_FCROSS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_SCROSS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_ATR);
	projection.push_back(SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL);
	projection.push_back(SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL);
	projection.push_back(SQLiteHandler::COLUMN_NAME_CANDLE_BODY);
	projection.push_back(SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS_ATR);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTFS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTFS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTOF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTOF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTOS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTOS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTHF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTHF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTHS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTHS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTLF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTLF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTLS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTLS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTCF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTCF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_DISTCS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_N_DISTCS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_HLF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_HLS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_LGF);
	projection.push_back(SQLiteHandler::COLUMN_NAME_LGS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_FSCROSS);
	projection.push_back(SQLiteHandler::COLUMN_NAME_IDPARENT);
	projection.push_back(SQLiteHandler::COLUMN_NAME_IDPARENT_PREV);
	projection.push_back(SQLiteHandler::COLUMN_NAME_IDPARENT_MONTHLY);
	projection.push_back(SQLiteHandler::COLUMN_NAME_IDPARENT_PREV_MONTHLY);
	projection.push_back(SQLiteHandler::COLUMN_NAME_COMPLETED);
	projection.push_back(SQLiteHandler::COLUMN_NAME_BAR_RANGE);
	projection.push_back(SQLiteHandler::COLUMN_NAME_INTRADAY_HIGH);
	projection.push_back(SQLiteHandler::COLUMN_NAME_INTRADAY_LOW);
	projection.push_back(SQLiteHandler::COLUMN_NAME_INTRADAY_RANGE);

	QString columns = projection[0];
	QString argv = "?";
	int count = projection.length();

	for (int i = 1; i < count; ++i)
	{
		columns += "," + projection[i];
		argv += ",?";
	}

	// or ignore
	return ("insert into bardata(" + columns + ") values(" + argv + ")");
}

QString read_file_for_incomplete_bar(QString filename, BardataTuple *data, const QString &start_datetime, const QString &end_datetime, IntervalWeight interval)
{
	// generate incomplete bar from "filename" into "data"
	// and search within "start_datetime" and "end_datetime"
	// this way we allow flexiblity and accurate way to create incomplete bar
	// if end_datetime is an empty string, then it will read until the end of file

	if (data != NULL && start_datetime != "")
	{
		QFile file(filename);
		QString src_datetime = "";

		if (file.open(QFile::ReadOnly))
		{
			QStringList parseline;
			QTextStream stream(&file);
			QString datetime = "";
			QString datetime_upperbound = end_datetime;
			QString line = "";
			bool is_first_time = true;

			stream.readLineInto(NULL);

			while (!stream.atEnd() && stream.readLineInto(&line))
			{
				parseline = line.split(",", QString::KeepEmptyParts);

				if (parseline.length() < 6)
				{
					break;
				}

				datetime = parse_date(parseline[0]) + " " + parseline[1];

				if (!is_first_time && datetime_upperbound != "" && datetime > datetime_upperbound)
				{
					break;
				}

				// && (!end_datetime_not_empty || (end_datetime_not_empty && datetime <= end_datetime))
				if (datetime > start_datetime)
				{
					if (is_first_time)
					{
						// override upper bound, if realign needed
						if (end_datetime != "" && datetime > end_datetime)
						{
							std::string _timestring = end_datetime.toStdString();
							std::string base_datetime = datetime.toStdString();

							while (_timestring.length() >= 16 && base_datetime > _timestring)
							{
								_timestring = DateTimeHelper::next_intraday_datetime(_timestring, interval);
							}

							datetime_upperbound = QString::fromStdString(_timestring);

//							qDebug() << Globals::intervalWeightToString(interval) << "datetime upperbound" << datetime_upperbound;

							if (datetime_upperbound == "")
							{
								break;
							}
						}

						is_first_time = false;
					}

					if (datetime_upperbound == "" || (datetime_upperbound != "" && datetime <= datetime_upperbound))
					{
						if (data->open == 0.0) data->open = parseline[2].toDouble();
						data->high = std::fmax(parseline[3].toDouble(), data->high);
						data->low = std::fmin(parseline[4].toDouble(), data->low);
						data->close = parseline[5].toDouble();
						src_datetime = datetime;
					}
				}
			}

			file.close();
		}

		return src_datetime;
	}

	return "";
}


FileLoaderV2::FileLoaderV2(const QString &filename, const QString &databasedir, const QString& inputdir,
													 const QString &symbol, IntervalWeight interval, Logger logger):
	_filename(filename), _database_dir(databasedir), _input_dir(inputdir),
	_symbol(symbol), _interval(interval), _recalculate_flag(false),
	_recalculate_parent_rowid(-1), _logger(logger)
{
	_database_name = _database_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(_interval) + ".db";
}

void FileLoaderV2::disable_incomplete_bar(bool b)
{
	_enable_incomplete_bar = !b;
}

bool FileLoaderV2::request_recalculate() const
{
	return _recalculate_flag;
}

int FileLoaderV2::recalculate_from_parent_rowid() const
{
	return _recalculate_parent_rowid;
}

void FileLoaderV2::calculate_basic_indicators(const BardataTuple& prev_row, BardataTuple *new_row, Globals::dbupdater_info *data,
																							const QString &daily_time, bool is_intraday)
{
	if (new_row == NULL)
		return;

	// MACD
	{
		data->macd_m12 = data->macd_m12 + (SF_MACD_FAST * (new_row->close - data->macd_m12));
		data->macd_m26 = data->macd_m26 + (SF_MACD_SLOW * (new_row->close - data->macd_m26));
		double real_macd = data->macd_m12 - data->macd_m26;

		new_row->macd = Globals::round_decimals(real_macd, 6);
		new_row->macd_avg = Globals::round_decimals(data->macd_avg_prev + (SF_MACD_LENGTH * (real_macd - data->macd_avg_prev)), 6);
		new_row->macd_diff = Globals::round_decimals(new_row->macd - new_row->macd_avg, 6);
		data->macd_avg_prev = new_row->macd_avg;
	}

	// RSI
	{
		data->rsi_change = new_row->close - prev_row.close;
		data->rsi_netchgavg = data->rsi_netchgavg + (SF_RSI * (data->rsi_change - data->rsi_netchgavg));
		data->rsi_totchgavg = data->rsi_totchgavg + (SF_RSI * (std::fabs(data->rsi_change) - data->rsi_totchgavg));

		if (data->rsi_totchgavg != 0.0)
		{
			new_row->rsi = Globals::round_decimals(50 * (1 + (data->rsi_netchgavg / data->rsi_totchgavg)), 2);
		}
		else
		{
			new_row->rsi = 50.0;
		}
	}

	// SlowK, SlowD
	{
		if (data->slowk_highest_high.size() + 1 > STOCHASTIC_LENGTH)
		{
			if (!data->slowk_highest_high.empty()) data->slowk_highest_high.pop_front();
			if (!data->slowk_lowest_low.empty()) data->slowk_lowest_low.pop_front();
		}

		data->slowk_highest_high.push_back(new_row->high);
		data->slowk_lowest_low.push_back(new_row->low);

		double _hh = data->slowk_highest_high[0];
		double _ll = data->slowk_lowest_low[0];

		for (int i = 1 ; i < data->slowk_highest_high.size(); ++i)
		{
			_hh = std::fmax(_hh, data->slowk_highest_high[i]);
			_ll = std::fmin(_ll, data->slowk_lowest_low[i]);
		}

		double curr_num1 = (new_row->close - _ll);
		double curr_den1 = (_hh - _ll);
		double real_slowk = (data->slowk_num1_0 + data->slowk_num1_1 + curr_num1) * 100.0 /
				(data->slowk_den1_0 + data->slowk_den1_1 + curr_den1);

		new_row->slow_k = Globals::round_decimals(real_slowk, 2);
		new_row->slow_d = Globals::round_decimals((data->slowk_last_0 + data->slowk_last_1 + real_slowk) / 3.0, 2);

		data->slowk_num1_0 = data->slowk_num1_1;
		data->slowk_num1_1 = curr_num1;

		data->slowk_den1_0 = data->slowk_den1_1;
		data->slowk_den1_1 = curr_den1;

		data->slowk_last_0 = data->slowk_last_1;
		data->slowk_last_1 = real_slowk;
	}

	// MovingAvg
	{
		if (data->close_last50.size()+1 > MA_SLOWLENGTH)
		{
			data->close_last50.pop_front();
		}

		data->close_last50.push_back(new_row->close);

		if (data->close_last50.size() >= MA_FASTLENGTH)
		{
			double sum_close = 0;
			int last_index = data->close_last50.size() - 1;

			for (int i = 0; i < MA_FASTLENGTH; ++i)
			{
				sum_close += data->close_last50[last_index-i];
			}

			new_row->fast_avg = Globals::round_decimals(sum_close / MA_FASTLENGTH, 4);
		}

		if (data->close_last50.size() >= MA_SLOWLENGTH)
		{
			double sum_close = 0;

			for (int i = 0; i < MA_SLOWLENGTH; ++i)
			{
				sum_close += data->close_last50[i];
			}

			new_row->slow_avg = Globals::round_decimals(sum_close / MA_SLOWLENGTH, 4);
		}
	}

	new_row->set_data(bardata::COLUMN_NAME_MACD, QString::number(new_row->macd).toStdString());
	new_row->set_data(bardata::COLUMN_NAME_RSI, QString::number(new_row->rsi).toStdString());
	new_row->set_data(bardata::COLUMN_NAME_SLOWK, QString::number(new_row->slow_k).toStdString());
	new_row->set_data(bardata::COLUMN_NAME_SLOWD, QString::number(new_row->slow_d).toStdString());
	new_row->set_data(bardata::COLUMN_NAME_FASTAVG, QString::number(new_row->fast_avg).toStdString());
	new_row->set_data(bardata::COLUMN_NAME_SLOWAVG, QString::number(new_row->slow_avg).toStdString());

	// BarColor
	std::string barcolor = prev_row.prevbarcolor;
	if (barcolor.size() == 19) barcolor.erase (0, 2);
	if (barcolor.size() > 0) barcolor += ",";

	if (prev_row.close > prev_row.open) barcolor += "G";
	else if (prev_row.close < prev_row.open) barcolor += "R";
	else barcolor += "D";

	new_row->prevbarcolor = barcolor;
	new_row->prevdate = prev_row.date;
	new_row->prevtime = prev_row.time;

	// F-Slope, S-Slope
	{
		if (prev_row.fast_avg > 0.0)
		{
			new_row->fastavg_slope = new_row->fast_avg - prev_row.fast_avg;
			new_row->set_data(bardata::COLUMN_NAME_FASTAVG_SLOPE, std::to_string(new_row->fastavg_slope));
		}

		if (prev_row.slow_avg > 0.0)
		{
			new_row->slowavg_slope = new_row->slow_avg - prev_row.slow_avg;
			new_row->set_data(bardata::COLUMN_NAME_SLOWAVG_SLOPE, std::to_string(new_row->slowavg_slope));
		}
	}

	// BarsAbove and BarsBelow
	{
		double _max_olc = std::max(new_row->open, std::max(new_row->close, new_row->low));
		double _min_ohc = std::min(new_row->open, std::min(new_row->close, new_row->high));

		new_row->lgf = (_max_olc > new_row->fast_avg) ? prev_row.lgf + 1 : 0;
		new_row->lgs = (_max_olc > new_row->slow_avg) ? prev_row.lgs + 1 : 0;
		new_row->hlf = (_min_ohc < new_row->fast_avg) ? prev_row.hlf + 1 : 0;
		new_row->hls = (_min_ohc < new_row->slow_avg) ? prev_row.hls + 1 : 0;
	}

	// ATR
	new_row->atr = _atr.update(new_row->high, new_row->low, new_row->close);
	new_row->dist_cc_fscross = _distfscross.update(new_row->close, new_row->fast_avg, new_row->slow_avg);
	new_row->fs_cross = _distfscross.is_fscross() ? 1 : 0;

	new_row->set_data(bardata::COLUMN_NAME_DISTCC_FSCROSS, std::to_string(new_row->dist_cc_fscross));
	new_row->set_data(bardata::COLUMN_NAME_DISTCC_FSCROSS_ATR, std::to_string(new_row->dist_cc_fscross_atr));

	new_row->dist_of = new_row->open - new_row->fast_avg;
	new_row->dist_os = new_row->open - new_row->slow_avg;
	new_row->dist_hf = new_row->high - new_row->fast_avg;
	new_row->dist_hs = new_row->high - new_row->slow_avg;
	new_row->dist_lf = new_row->low - new_row->fast_avg;
	new_row->dist_ls = new_row->low - new_row->slow_avg;
	new_row->dist_cf = new_row->close - new_row->fast_avg;
	new_row->dist_cs = new_row->close - new_row->slow_avg;
	new_row->dist_fs = new_row->fast_avg - new_row->slow_avg;

	// TODO: use prevdailyatr for intraday
	if (new_row->atr != 0.0)
	{
		new_row->n_dist_of = new_row->dist_of / new_row->atr;
		new_row->n_dist_os = new_row->dist_os / new_row->atr;
		new_row->n_dist_hf = new_row->dist_hf / new_row->atr;
		new_row->n_dist_hs = new_row->dist_hs / new_row->atr;
		new_row->n_dist_lf = new_row->dist_lf / new_row->atr;
		new_row->n_dist_ls = new_row->dist_ls / new_row->atr;
		new_row->n_dist_cf = new_row->dist_cf / new_row->atr;
		new_row->n_dist_cs = new_row->dist_cs / new_row->atr;
		new_row->n_dist_fs = new_row->dist_fs / new_row->atr;
	}

	new_row->candle_uptail = std::fabs(new_row->high - std::max (new_row->open, new_row->close));
	new_row->candle_downtail = std::fabs(std::min (new_row->open, new_row->close) - new_row->low);
	new_row->candle_body = std::fabs(new_row->open - new_row->close);
	new_row->candle_totallength = std::fabs(new_row->high - new_row->low);
	new_row->f_cross = (new_row->fast_avg >= new_row->low && new_row->fast_avg <= new_row->high) ? 1 : 0;
	new_row->s_cross = (new_row->slow_avg >= new_row->low && new_row->slow_avg <= new_row->high) ? 1 : 0;
	new_row->bar_range = new_row->high - new_row->low;

	// for determine openbar in incomplete bar
//	std::string _daily_time = daily_time.toStdString();
//	bool is_openbar = (prev_row.time <= _daily_time &&
//										 (new_row->time > _daily_time || prev_row.date < new_row->date));

	if (!is_intraday)
	{
		if (new_row->atr != 0.0)
		{
			new_row->dist_cc_fscross_atr = new_row->dist_cc_fscross / new_row->atr;
		}
		else
		{
			new_row->dist_cc_fscross_atr = 0.0;
		}
	}
	else
	{
		if (new_row->openbar == 1)
		{
			new_row->intraday_high = new_row->high;
			new_row->intraday_low = new_row->low;
		}
		else
		{
			new_row->intraday_high = std::fmax(new_row->high, prev_row.intraday_high);
			new_row->intraday_low = (prev_row.intraday_low != 0.0) ? std::fmin(new_row->low, prev_row.intraday_low) : new_row->low;
		}

		new_row->intraday_range = new_row->intraday_high - new_row->intraday_low;
		new_row->dist_cc_fscross_atr = 0.0; // calculated later
	}
}

bool FileLoaderV2::get_last_complete(BardataTuple *t)
{
	QSqlDatabase database;
	bool result_code = false;

	SQLiteHandler::getDatabase_v2(_database_name, &database);

	if (database.isOpen())
	{
		QSqlQuery q(database);
		q.setForwardOnly(true);
		q.exec("PRAGMA temp_store = MEMORY;");
		q.exec("PRAGMA cache_size = 10000;");

		// prepare basic indicators
		{
			q.exec("select rowid, date_, time_"
						 ",open_, high_, low_, close_, fastavg, slowavg, atr"
						 ",_parent, _parent_prev, prevbarcolor"
						 ",hlf, hls, lgf, lgs, intraday_high, intraday_low, openbar"
						 " from bardata where completed=" + QString::number(CompleteStatus::COMPLETE_SUCCESS) +
						 " order by rowid desc limit 1");

			if (q.next())
			{
				t->rowid = q.value(0).toInt();
				t->date = q.value(1).toString().toStdString();
				t->time = q.value(2).toString().toStdString();
				t->open = q.value(3).toDouble();
				t->high = q.value(4).toDouble();
				t->low = q.value(5).toDouble();
				t->close = q.value(6).toDouble();
				t->fast_avg = q.value(7).toDouble();
				t->slow_avg = q.value(8).toDouble();
				t->atr = q.value(9).toDouble();
				t->_parent = q.value(10).toInt();
				t->_parent_prev = q.value(11).toInt();
				t->prevbarcolor = q.value(12).toString().toStdString();
				t->hlf = q.value(13).toInt();
				t->hls = q.value(14).toInt();
				t->lgf = q.value(15).toInt();
				t->lgs = q.value(16).toInt();
				t->intraday_high = q.value(17).toDouble();
				t->intraday_low = q.value(18).toDouble();
				t->openbar = q.value(19).toInt();

				result_code = true;
			}
		}

		// prepare dist-fscross
		{
			q.exec("select close_ from bardata where rowid=("
						 " select rowid from bardata where fs_cross=1"
						 " and rowid<=" + QString::number(t->rowid) +
						 " and completed=" + QString::number(CompleteStatus::COMPLETE_SUCCESS) +
						 " order by rowid desc limit 1"
						 ")-1");

			if (q.next())
			{
				double fs_close = q.value(0).toDouble();
				int fs_state = get_moving_avg_state(t->fast_avg, t->slow_avg);

				_distfscross.initialize(fs_close, t->close, fs_state);
			}
		}

		// prepare atr
		{
			q.exec("select high_, low_, close_ from bardata"
						 " where rowid >" + QString::number(t->rowid - 15) +
						 " and completed=" + QString::number(CompleteStatus::COMPLETE_SUCCESS) +
						 " order by rowid asc");

			if (q.next())
			{
				std::list<double> data;

				// first row -- get close price to use in next iteration
				double prev_close = q.value(2).toDouble();
				double _high, _low;
				double tr1, tr2, tr3;

				while (q.next())
				{
					_high = q.value(0).toDouble();
					_low = q.value(1).toDouble();

					tr1 = _high - _low;
					tr2 = std::fabs(_high - prev_close);
					tr3 = std::fabs(_low - prev_close);

					data.emplace_back(std::fmax(std::fmax(tr1,tr2), tr3));
					prev_close = q.value(2).toDouble();
				}

				_atr.initialize(data, prev_close);
			}
		}
	}

	SQLiteHandler::removeDatabase(&database);

	return result_code;
}

bool FileLoaderV2::get_first_incomplete(BardataTuple *t)
{
	if (_interval == WEIGHT_5MIN)
		return false;

	QSqlDatabase database;
	SQLiteHandler::getDatabase_v2(_database_name, &database);

	bool result_code = false;

	if (database.isOpen())
	{
		QSqlQuery q(database);
		q.setForwardOnly(true);
		q.exec("PRAGMA temp_store = MEMORY;");
		q.exec("PRAGMA cache_size = 10000;");
		q.exec("select rowid, date_, time_"
					 " from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE) +
					 " order by rowid asc limit 1");

//		qDebug() << "first incomplete::" << q.lastError();
//		qDebug() << q.lastQuery();

		if (q.next())
		{
			t->rowid = q.value(0).toInt();
			t->date = q.value(1).toString().toStdString();
			t->time = q.value(2).toString().toStdString();
			result_code = true;

//			qDebug() << t->rowid << QString::fromStdString(t->get_datetime());
		}
	}

	SQLiteHandler::removeDatabase(&database);

	return result_code;
}

QString FileLoaderV2::get_daily_time()
{
	QFile file(_input_dir + "/" + _symbol + "_Daily.txt");
	QString result = "";
	BardataTuple tuple;

	if (file.open(QFile::ReadOnly))
	{
		QTextStream stream(&file);
		QString line;

		stream.readLineInto(NULL);

		if (!stream.atEnd() && stream.readLineInto(&line))
		{
			parse_line_datetime(line, &tuple);
			result = QString::fromStdString(tuple.time);
		}

		file.close();
	}

	return result;
}

bool FileLoaderV2::get_last_incomplete(BardataTuple *t)
{
	if (_interval == WEIGHT_5MIN)
		return false;

	QSqlDatabase database;
	SQLiteHandler::getDatabase_v2(_database_name, &database);

	bool result_code = false;

	if (database.isOpen())
	{
		QSqlQuery q(database);
		q.setForwardOnly(true);
		q.exec("PRAGMA temp_store = MEMORY;");
		q.exec("PRAGMA cache_size = 10000;");
		q.exec("select rowid, date_, time_"
					 " from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE) +
					 " order by rowid desc limit 1");

		if (q.next())
		{
			t->rowid = q.value(0).toInt();
			t->date = q.value(1).toString().toStdString();
			t->time = q.value(2).toString().toStdString();
			result_code = true;
		}
	}

	SQLiteHandler::removeDatabase(&database);

	return result_code;
}

void FileLoaderV2::delete_incomplete_bar(const QSqlDatabase &database)
{
	if (database.isOpen())
	{
		QSqlQuery q(database);
//		q.exec("PRAGMA cache_size = 10000;");
		q.exec("PRAGMA journal_mode = OFF;");
		q.exec("PRAGMA synchronous = OFF;");
		q.exec("BEGIN TRANSACTION;");

		// delete incomplete and pending rows
//		q.exec("delete from bardata where completed <>" + QString::number(CompleteStatus::COMPLETE_SUCCESS));

		// delete pending rows
		q.exec("delete from bardata where completed=" + QString::number(CompleteStatus::COMPLETE_PENDING));

		// delete incomplete rows
		q.exec("delete from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE));

		q.exec("COMMIT;");
	}
}

int FileLoaderV2::load_input(std::vector<BardataTuple> *result, eFlagNewData new_data_flag)
{
	QFile file(_filename);
	int _rows = 0;

	_recalculate_flag = false;
	_recalculate_parent_rowid = -1;

	// new implementation in ver2.0:
	//	1. eliminate native sqlite library -> easier to debug
	//	2. parent indexing is performed in here -> easier to maintain
	//	3. we create incomplete bar after load complete bar (optional/could be disabled for testing purpose).
	//			the reason is incomplete bar is useful to ensure completeness of parent indexing
	//			and we also want to update every indicators in incomplete bar, thats why we need to create it upfront
	//	4. using ReadWrite to obtain exclusive read permission, to ensure no other application alter data during read

	// algorithm for incomplete daily bar < 5min datetime
	// 1. keep maintain incomplete daily bar of that and generate new incomplete daily for new 5min datetime
	// 2. when there's new complete bar of that day coming in,
	//		we rollback all previous incomplete bar (revert index of its child and "recalculate")

	if (file.open(QFile::ReadOnly))
	{
		const QString insert_stmt = get_insert_string();
		const IntervalWeight parent_interval = Globals::getParentInterval(_interval);
		const bool is_intraday = (_interval < WEIGHT_DAILY);
		const bool has_parent = (parent_interval != WEIGHT_INVALID);

		QSqlDatabase database;
		QSqlDatabase parent_database;
		QSqlDatabase monthly_database;

		// open database connection
		SQLiteHandler::getDatabase_v2(_database_name, &database);

		if (has_parent)
		{
			QString _parent_database_name = _database_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(parent_interval) + ".db";
			QString _monthly_database_name = _database_dir + "/" + _symbol + "_" + Globals::intervalWeightToString(WEIGHT_MONTHLY) + ".db";

			SQLiteHandler::getDatabase_v2(_parent_database_name, &parent_database);
			SQLiteHandler::getDatabase_v2(_monthly_database_name, &monthly_database);
		}

		QSqlQuery q_monthly(monthly_database);
		QSqlQuery q_parent(parent_database);
		QSqlQuery q_insert(database);

		QString line_string;
		BardataTuple curr_row, prev_row;
		BardataTuple last_incomplete_row;
		BardataTuple first_incomplete_row;
		QString first_incomplete_datetime;

		QString symbol_interval = _symbol + "_" + Globals::intervalWeightToString(_interval);
		QString database_name = symbol_interval + ".db";
		QString last_complete_datetime = QString::fromStdString(Globals::get_database_lastdatetime(symbol_interval.toStdString()));
		QString curr_datetime = "";
		QString daily_time = get_daily_time();
		std::string key_database_name = database_name.toStdString();

		// TODO: will be removed in the future, too many dependency of it
		// get bundle information to calculate { MACD, RSI, SlowK, SlowD }
		Globals::dbupdater_info last_data = Globals::get_last_indicator_for_dbupdater(key_database_name);

		get_last_complete(&prev_row);
		qDebug() << "get_last_incomplete" << get_last_incomplete(&last_incomplete_row);
		qDebug() << "get_first_incomplete" << get_first_incomplete(&first_incomplete_row);

		first_incomplete_datetime = QString::fromStdString(first_incomplete_row.get_datetime());

		qDebug() << "first incomplete datetime" << first_incomplete_datetime;

		if (_interval > WEIGHT_5MIN && new_data_flag == eFlagNewData::AVAILABLE)
		{
			IntervalWeight child_interval = Globals::getChildInterval(_interval);
			std::string child_datetime = Globals::get_database_lastdatetime_v3(_symbol.toStdString(), child_interval);

			if (child_datetime.empty())
			{
				Globals::set_database_lastdatetime_rowid_v3(_database_dir, _symbol.toStdString(), child_interval);
				child_datetime = Globals::get_database_lastdatetime_v3(_symbol.toStdString(), child_interval);
			}

//			qDebug() << "check @CL_60min v1:" << QString::fromStdString(Globals::get_database_lastdatetime("@CL_60min"));
//			qDebug() << "check @CL_60min v2:" << QString::fromStdString(Globals::get_database_lastdatetime_v3("@CL", WEIGHT_60MIN));

			// notify recalculate if child datetime > prev last complete parent

			if (child_datetime > prev_row.get_datetime())
			{
				LOG_DEBUG("Notify RECALCULATE::Incomplete bar: {}", first_incomplete_datetime.toStdString());
				qDebug("Notify RECALCULATE::Incomplete bar: _%s_", first_incomplete_datetime.toStdString().c_str());

				if (first_incomplete_row.date != "")
				{
					_recalculate_flag = true;
					_recalculate_parent_rowid = first_incomplete_row.rowid;
					qDebug() << "recalculate" << _recalculate_flag << _recalculate_parent_rowid;
				}
			}

//			delete_incomplete_bar(database);
		}

		//
		// (1) Read Complete Bar
		//
		QTextStream stream(&file);
		stream.readLineInto(NULL);

		q_parent.setForwardOnly(true);
		q_insert.exec("PRAGMA cache_size = 10000;");
		q_insert.exec("PRAGMA journal_mode = OFF;");
		q_insert.exec("PRAGMA synchronous = OFF;");
		q_insert.exec("BEGIN TRANSACTION;");
		q_insert.exec("delete from bardata where completed="+ QString::number(CompleteStatus::INCOMPLETE));
		q_insert.exec("delete from bardata where completed="+ QString::number(CompleteStatus::COMPLETE_PENDING));
		q_insert.prepare(insert_stmt);

		while (!stream.atEnd() && stream.readLineInto(&line_string))
		{
			if (line_string.isEmpty()) break;
			parse_line_datetime(line_string, &curr_row);
			curr_datetime = QString::fromStdString(curr_row.get_datetime());

			if (curr_datetime > last_complete_datetime)
			{
				if (!parse_line_all(line_string, &curr_row))
				{
					_rows = -1;
					break;
				}

				curr_row.rowid = prev_row.rowid + 1;
				curr_row.completed = CompleteStatus::COMPLETE_PENDING;

				if (has_parent)
				{
					// _parent and _parent_monthly indexing
					QString _date = QString::fromStdString(curr_row.date);
					QString _time = QString::fromStdString(curr_row.time);

					// base
					q_parent.exec("select rowid from bardata where date_ < '" + _date + "'"
												" or (date_ = '" + _date + "' and time_ < '" + _time + "')"
												" order by rowid desc limit 1");

					if (q_parent.next())
					{
						curr_row._parent_prev = q_parent.value(0).toInt();

						// TODO: consider holiday indexing as well
						curr_row._parent = curr_row._parent_prev + 1;
					}

					// monthly
					q_monthly.exec("select rowid from bardata where date_ < '" + _date + "'"
												 " or (date_ = '" + _date + "' and time_ < '" + _time + "')"
												 " order by rowid desc limit 1");

					if (q_monthly.next())
					{
						curr_row._parent_prev_monthly = q_monthly.value(0).toInt();
						curr_row._parent_monthly = curr_row._parent_prev_monthly + 1;
					}
				}

				calculate_basic_indicators(prev_row, &curr_row, &last_data, daily_time, is_intraday);

				bind_and_exec_query(q_insert, curr_row);

				if (q_insert.lastError().isValid())
				{
					_rows = -1;
					LOG_DEBUG ("{}: FileLoaderV2 Error: {} {}", database_name.toStdString(),
										 q_insert.lastError().text().toStdString(), curr_row.get_datetime());
					qDebug(q_insert.lastError().text().toStdString().c_str());
					break;
				}

				if (result != NULL)
				{
					result->push_back(curr_row);
				}

				_lastdatetime = QString::fromStdString(curr_row.get_datetime());
				last_complete_datetime = curr_datetime;
				prev_row = curr_row;
				++_rows;
			}
		}

		q_insert.exec("COMMIT;");

		file.close();

		// Important: set back last data into Globals, to re-use in next iteration
		if (_rows > 0)
		{
			Globals::set_last_indicator_dbupdater(key_database_name, last_data);
		}

		//
		// (2) Create Incomplete Bar
		//
		if (_enable_incomplete_bar && _interval > WEIGHT_5MIN)
		{
			// if (update incomplete only) -- or no new data, get lastdatetime from lastcomplete
			if (_lastdatetime == "")
			{
				_lastdatetime = QString::fromStdString(prev_row.date + " " + prev_row.time);
			}

//			q_insert.exec("select rowid, date_, time_, open_, high_, low_, close_, openbar, prevdate, prevtime"
//										" from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE) +
//										" order by rowid desc limit 1");

//			q_insert.exec("select 1 from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE) +
//										" order by rowid desc limit 1");

			/*if (q_insert.next())
			{
				qDebug("INCOMPLETE BAR EXISTS -- update");
				LOG_DEBUG("{} : incomplete bar (update)", database_name.toStdString());

				BardataTuple incomplete_bar;
				QString prev_incomplete_datetime;

				q_insert.exec("select rowid, date_, time_, open_, high_, low_, close_, openbar, prevdate, prevtime"
											" from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE) +
											" order by rowid asc");

				while (q_insert.next())
				{
					prev_incomplete_datetime = q_insert.value(8).toString() + " " + q_insert.value(9).toString();

					if (create_incomplete_bar(_input_dir, &incomplete_bar, prev_incomplete_datetime))
					{
						calculate_basic_indicators(prev_row, &incomplete_bar, &last_data, is_intraday);
						prev_row = incomplete_bar;
					}
				}

				incomplete_bar.rowid = q_insert.value(0).toInt();
				incomplete_bar.date = q_insert.value(1).toString().toStdString();
				incomplete_bar.time = q_insert.value(2).toString().toStdString();
				incomplete_bar.open = q_insert.value(3).toDouble();
				incomplete_bar.high = q_insert.value(4).toDouble();
				incomplete_bar.low = q_insert.value(5).toDouble();
				incomplete_bar.close = q_insert.value(6).toDouble();
				incomplete_bar.openbar = q_insert.value(7).toInt();
				incomplete_bar.prevdate = q_insert.value(8).toString().toStdString();
				incomplete_bar.prevtime = q_insert.value(9).toString().toStdString();
				incomplete_bar.completed = CompleteStatus::INCOMPLETE;
				prev_incomplete_datetime = QString::fromStdString(incomplete_bar.prevdate + " " + incomplete_bar.prevtime);

				qDebug("EXISTING INCOMPLETE: %d %s %s %lf %lf %lf %lf",
							 incomplete_bar.rowid, incomplete_bar.date.c_str(), incomplete_bar.time.c_str(),
							 incomplete_bar.open, incomplete_bar.high, incomplete_bar.low, incomplete_bar.close);

				qDebug() << "Prev Incomplete Datetime: " << prev_incomplete_datetime;

				// reload OHLC
				if (create_incomplete_bar(_input_dir, &incomplete_bar, prev_incomplete_datetime))
				{
					qDebug() << "incomplete bar:" << QString::fromStdString(incomplete_bar.date) << QString::fromStdString(incomplete_bar.time);

					// TODO: last_data may mis-align here
					calculate_basic_indicators(prev_row, &incomplete_bar, &last_data, is_intraday);

					q_insert.exec("PRAGMA journal_mode = OFF;");
					q_insert.exec("PRAGMA synchronous = OFF;");
					q_insert.exec("BEGIN TRANSACTION;");

					//
					{
						q_insert.exec("update bardata set"
													" open_=" + QString::number(incomplete_bar.open) +
													",high_=" + QString::number(incomplete_bar.high) +
													",low_=" + QString::number(incomplete_bar.low) +
													",close_=" + QString::number(incomplete_bar.close) +
													",bar_range=" + QString::number(incomplete_bar.bar_range) +
													",intraday_high=" + QString::number(incomplete_bar.intraday_high) +
													",intraday_low=" + QString::number(incomplete_bar.intraday_low) +
													",intraday_range=" + QString::number(incomplete_bar.intraday_range) +
													",atr=" + QString::number(incomplete_bar.atr,'f') +
													",macd=" + QString::number(incomplete_bar.macd,'f') +
													",rsi=" + QString::number(incomplete_bar.rsi,'f') +
													",slowk=" + QString::number(incomplete_bar.slow_k,'f') +
													",slowd=" + QString::number(incomplete_bar.slow_d,'f') +
													",fastavg=" + QString::number(incomplete_bar.fast_avg,'f') +
													",slowavg=" + QString::number(incomplete_bar.slow_avg,'f') +
													",fastavgslope=" + QString::number(incomplete_bar.fastavg_slope,'f') +
													",slowavgslope=" + QString::number(incomplete_bar.slowavg_slope,'f') +
													",candle_uptail=" + QString::number(incomplete_bar.candle_uptail,'f') +
													",candle_downtail=" + QString::number(incomplete_bar.candle_downtail,'f') +
													",candle_body=" + QString::number(incomplete_bar.candle_body,'f') +
													",candle_totallength=" + QString::number(incomplete_bar.candle_totallength,'f') +
													",distof=" + QString::number(incomplete_bar.dist_of,'f') +
													",n_distof=" + QString::number(incomplete_bar.dist_of_close,'f') +
													",distos=" + QString::number(incomplete_bar.dist_os,'f') +
													",n_distos=" + QString::number(incomplete_bar.dist_os_close,'f') +
													",disthf=" + QString::number(incomplete_bar.dist_hf,'f') +
													",n_disthf=" + QString::number(incomplete_bar.dist_hf_close,'f') +
													",disths=" + QString::number(incomplete_bar.dist_hs,'f') +
													",n_disths=" + QString::number(incomplete_bar.dist_hs_close,'f') +
													",distlf=" + QString::number(incomplete_bar.dist_lf,'f') +
													",n_distlf=" + QString::number(incomplete_bar.dist_lf_close,'f') +
													",distls=" + QString::number(incomplete_bar.dist_ls,'f') +
													",n_distls=" + QString::number(incomplete_bar.dist_ls_close,'f') +
													",distcf=" + QString::number(incomplete_bar.dist_cf,'f') +
													",n_distcf=" + QString::number(incomplete_bar.dist_cf_close,'f') +
													",distcs=" + QString::number(incomplete_bar.dist_cs,'f') +
													",n_distcs=" + QString::number(incomplete_bar.dist_cs_close,'f') +
													",completed=" + QString::number(CompleteStatus::INCOMPLETE) +
													" where rowid=" + QString::number(incomplete_bar.rowid));
					}

					qDebug() << q_insert.lastQuery();

					if (q_insert.lastError().isValid())
					{
						qDebug() << q_insert.lastError();
					}

					q_insert.exec("COMMIT;");

					if (result != NULL)
					{
						result->push_back(incomplete_bar);
					}
				}
			}
			else
			*/

			// always create
			{
				qDebug("INCOMPLETE BAR NOT EXISTS -- insert at _%s_", _lastdatetime.toStdString().c_str());
				LOG_DEBUG("{}: incomplete bar (insert)", database_name.toStdString());

				BardataTuple incomplete_bar;

				q_insert.exec("PRAGMA journal_mode = OFF;");
				q_insert.exec("PRAGMA synchronous = OFF;");
				q_insert.exec("BEGIN TRANSACTION;");
				q_insert.exec("delete from bardata where completed=" + QString::number(CompleteStatus::INCOMPLETE));
				q_insert.prepare(insert_stmt);

				// allow to create multiple incomplete bar
				while (create_incomplete_bar(_input_dir, &incomplete_bar, _lastdatetime))
				{
					incomplete_bar.completed = CompleteStatus::INCOMPLETE;
					qDebug() << "incomplete bar created:" << QString::fromStdString(incomplete_bar.get_datetime());

					if (has_parent)
					{
						QString _date = QString::fromStdString(incomplete_bar.date);
						QString _time = QString::fromStdString(incomplete_bar.time);

						q_parent.exec("select rowid from bardata where date_ <'" + _date + "'"
													" or (date_ = '" + _date + "' and time_ <'" + _time + "')"
													" order by rowid desc limit 1");

						if (q_parent.next())
						{
							incomplete_bar._parent_prev = q_parent.value(0).toInt();
							incomplete_bar._parent = incomplete_bar._parent_prev + 1;
						}

						q_monthly.exec("select rowid from bardata where date_ <'" + _date + "'"
													 " or (date_ = '" + _date + "' and time_ <'" + _time + "')"
													 " order by rowid desc limit 1");

						if (q_monthly.next())
						{
							incomplete_bar._parent_prev_monthly = q_monthly.value(0).toInt();
							incomplete_bar._parent_monthly = incomplete_bar._parent_prev_monthly + 1;
						}
					}

					calculate_basic_indicators(prev_row, &incomplete_bar, &last_data, daily_time, is_intraday);

					bind_and_exec_query(q_insert, incomplete_bar);

					if (result != NULL)
					{
						incomplete_bar.completed = CompleteStatus::INCOMPLETE;
						result->push_back(incomplete_bar);
					}

					_lastdatetime = QString::fromStdString(incomplete_bar.date + " " + incomplete_bar.time);

					prev_row = incomplete_bar;

					if (q_insert.lastError().isValid())
					{
						qDebug() << q_insert.lastError() << q_insert.lastQuery();
						LOG_DEBUG("{}: insert incomplete error: {}", database_name.toStdString(), q_insert.lastError().text().toStdString());
					}
				}

				q_insert.exec("COMMIT;");
			}
		}

		SQLiteHandler::removeDatabase(&database);
		SQLiteHandler::removeDatabase(&parent_database);
		SQLiteHandler::removeDatabase(&monthly_database);
	}

	return _rows;
}

bool FileLoaderV2::create_incomplete_bar(const QString &input_dir, BardataTuple *result, const QString &last_datetime)
{
	if (_interval <= WEIGHT_5MIN || result == NULL)
		return false;

	QString lastdatetime_5min = "";

	result->date = "";
	result->time= "";
	result->open = 0.0;
	result->high = 0.0;
	result->low = std::numeric_limits<double>::max();
	result->close = 0.0;

	if (_interval >= WEIGHT_WEEKLY)
	{
		if (_interval == WEIGHT_MONTHLY)
		{
			result->date = DateTimeHelper::next_monthly_date(last_datetime.mid(0,10).toStdString(), true);
			result->time = last_datetime.mid(11,5).toStdString();
		}
		else if (_interval == WEIGHT_WEEKLY)
		{
			result->date = DateTimeHelper::next_weekly_date(last_datetime.mid(0,10).toStdString(), true);
			result->time = last_datetime.mid(11,5).toStdString();

			std::string base_curr_date = lastdatetime_5min.mid(0,10).toStdString();
			while (!result->date.empty() && base_curr_date > result->date)
			{
				result->date = DateTimeHelper::next_weekly_date(result->date, true);
			}
		}

		QString end_datetime = QString::fromStdString(result->date + " " + result->time);
		QString daily_filename = input_dir + "/" + _symbol +  "_Daily.txt";
		QString lastdatetime_daily = read_file_for_incomplete_bar(daily_filename, result, last_datetime, end_datetime, WEIGHT_DAILY);

		if (lastdatetime_daily == "") { lastdatetime_daily = last_datetime; }

		lastdatetime_5min = read_file_for_incomplete_bar(input_dir + "/" + _symbol +  "_5min.txt", result, lastdatetime_daily, "", _interval);

//		qDebug() << "create incomplete bar weekly/monthly" << lastdatetime_daily << lastdatetime_5min;
	}
	else if (_interval >= WEIGHT_60MIN)
	{
		if (_interval == WEIGHT_DAILY)
		{
			result->date = DateTimeHelper::next_daily_date(last_datetime.mid(0, 10).toStdString());
			result->time = last_datetime.mid(11, 5).toStdString();

			std::string base_curr_date = lastdatetime_5min.mid(0, 10).toStdString();
			while (!result->date.empty() && base_curr_date > result->date)
			{
				result->date = DateTimeHelper::next_daily_date(result->date);
			}
		}
		else if (_interval == WEIGHT_60MIN)
		{
			std::string _timestring = DateTimeHelper::next_intraday_datetime(last_datetime.toStdString(), _interval);

			if (_timestring.length() >= 16)
			{
				result->date = _timestring.substr(0, 10);
				result->time = _timestring.substr(11, 5);

//				qDebug() << "timestring:" << QString::fromStdString(_timestring) << "lastdatetime 5min" << lastdatetime_5min;

				std::string base_curr_date = lastdatetime_5min.mid(0,10).toStdString();
				std::string base_curr_time = lastdatetime_5min.mid(11,5).toStdString();

				while (!_timestring.empty() &&
							 (base_curr_date > result->date || (base_curr_date == result->date && base_curr_time > result->time)))
				{
					_timestring = DateTimeHelper::next_intraday_datetime(_timestring, _interval);

					if (_timestring.length() >= 16)
					{
						result->date = _timestring.substr(0, 10);
						result->time = _timestring.substr(11, 5);
					}
				}
			}
		}

		QString end_datetime = QString::fromStdString(result->date + " " + result->time);
//		qDebug() << "60min datetime" << last_datetime << end_datetime;

		lastdatetime_5min = read_file_for_incomplete_bar(input_dir + "/" + _symbol +  "_5min.txt", result, last_datetime, end_datetime, _interval);

		if (result->get_datetime() < lastdatetime_5min.toStdString())
		{
			std::string _timestring = result->get_datetime();
			std::string base_curr_date = lastdatetime_5min.mid(0,10).toStdString();
			std::string base_curr_time = lastdatetime_5min.mid(11,5).toStdString();

			while (!_timestring.empty() &&
						 (base_curr_date > result->date || (base_curr_date == result->date && base_curr_time > result->time)))
			{
				_timestring = DateTimeHelper::next_intraday_datetime(_timestring, _interval);

				if (_timestring.length() >= 16)
				{
					result->date = _timestring.substr(0, 10);
					result->time = _timestring.substr(11, 5);
				}
			}
		}
	}

//	qDebug() << "incomplete last5min:" << lastdatetime_5min << last_datetime;

	if (result->date.empty() || result->time.empty() || result->open <= 0.0 || result->high <= 0.0 ||
			result->low <= 0.0 || result->close <= 0.0)
	{
		LOG_DEBUG("{} {}: incomplete bar not created. last:{}, {} {}, {} {} {} {}", _symbol.toStdString(),
							Globals::intervalWeightToStdString(_interval), last_datetime.toStdString(),
							result->date, result->time, result->open, result->high, result->low, result->close);

		qDebug() << "incomplete bar: not created" << QString::fromStdString(result->get_datetime())
						 << result->open << result->high << result->low << result->close;

		return false;
	}

	qDebug("incomplete bar: %s %s %lf %lf %lf %lf", result->date.c_str(),
				 result->time.c_str(), result->open, result->high, result->low, result->close);

	return true;
}
