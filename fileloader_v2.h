#ifndef FILELOADER_V2_H
#define FILELOADER_V2_H

#include "bardatatuple.h"
#include "indicators/atr.h"
#include "indicators/distfscross.h"
#include "searchbar/globals.h"
#include "logdefs.h"

enum class eFlagNewData { UNKNOWN, AVAILABLE, NOT_AVAILABLE };

class FileLoaderV2 {
	public:
		FileLoaderV2(const QString &filename, const QString &databasedir, const QString &inputdir,
								 const QString &symbol, IntervalWeight interval, Logger logger);

		void disable_incomplete_bar(bool b);

		int load_input(std::vector<BardataTuple> *result = nullptr, eFlagNewData new_data_flag = eFlagNewData::UNKNOWN);

		bool request_recalculate() const;

		int recalculate_from_parent_rowid() const;

	private:
		Logger _logger;
		QString _database_name;
		QString _database_dir;
		QString _filename;
		QString _input_dir;
		QString _lastdatetime;
		QString _symbol;
		IntervalWeight _interval;

		ATR _atr;
		DistFSCross _distfscross;
		double fscross_close = 0.0;
		int prev_fs_state = 0;
		int _recalculate_parent_rowid;
		bool _enable_incomplete_bar = true;
		bool _recalculate_flag = false;

		bool get_last_complete(BardataTuple *t);
		bool get_last_incomplete(BardataTuple *t);
		bool get_first_incomplete(BardataTuple *t);
		QString get_daily_time();

		void calculate_basic_indicators(const BardataTuple &prev_row, BardataTuple *new_row, Globals::dbupdater_info* data,
																		const QString& daily_time, bool is_intraday);
		bool create_incomplete_bar(const QString& input_dir, BardataTuple* result, const QString& last_datetime);
		void delete_incomplete_bar(const QSqlDatabase &database);
};

#endif // FILELOADER_V2_H
