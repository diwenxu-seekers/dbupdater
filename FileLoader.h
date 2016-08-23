/* FileLoader.h
 * ------------
 * Responsible to convert raw text files into database and calculate basic indicators.
 */
#ifndef FILELOADER_H
#define FILELOADER_H

#include "searchbar/globals.h"
#include "bardatatuple.h"
#include "logdefs.h"
#include "sqlite3/sqlite3.h"

#include <queue>
#include <string>
#include <vector>

class FileLoader {
	public:
		bool load(const std::string &filename, const std::string &database_path,
							const std::string &symbol, IntervalWeight interval,
							std::vector<BardataTuple> *result, std::string *err_msg);

		void set_logger(Logger logger);

	private:
		Logger _logger;
		std::string m_filename = "";
		std::string m_database_path = "";
		std::string m_symbol = "";
		IntervalWeight m_interval;

		// helper for intraday range
		double _intraday_high = 0.0;
		double _intraday_low = 0.0;

		// helper for parent index
		std::string last_parent_date = "";
		std::string last_parent_time = "";
		int last_parent_rowid = 0;

		// helper vars for calculate ATR
		std::queue<double> m_tr;
		double sum_tr = 0.0;

		// helper vars for calculate dist(fscross)
		double fscross_close = 0.0;
		int curr_fs_state = 0;
		int prev_fs_state = 0;

		// helper vars for indexing
		sqlite3 *parent = NULL;
		sqlite3_stmt *parent_stmt = NULL;
		sqlite3_stmt *parent_prev_stmt = NULL;

		bool insert_database(std::vector<BardataTuple> *out = NULL, std::string *err_msg = NULL);

		// getting last information from database to calculate continous indicators
		// examples: ATR, prevbarcolor, CGS, CGF, etc
		bool load_last_state(BardataTuple *_old);

		// get latest incomplete bar to patch with new data
		bool load_last_incomplete(BardataTuple *t);

		// create column projection for insert statement
		std::string get_insert_statement();

		// perform direct calculation that no require any information about previous row
		// examples: Dist(CF), Candle Uptail, Candle Body, F-Cross, S-Cross
		void direct_calculation(BardataTuple *out);

		// perform continous calculation that require information from previous row
		// examples: MACD, RSI, SlowK, SlowD, ATR, prevbarcolor, CGS, CGF
		void continous_calculation(Globals::dbupdater_info *data, const BardataTuple &_old, BardataTuple *_new);

		// open parent database to mapping index to current stream
		void prepare_parent_stream(int start_rowid);

		// close parent database connection
		void finalize_parent_stream();

		// mapping current timeframe to its parent timeframe
		// return parent rowid
		long int indexing_calculation(const std::string &_date, const std::string &_time);

		// (experimental) mapping current timeframe to its parent timeframe
		// return parent rowid
		long int pre_indexing_calculation(const std::string &_date, const std::string &_time);

		// patch _parent index where it still blank waiting for parent to be mapped
		// for examples: intraday bar 06/01/2015 13:00 have to wait 06/01/2015 17:00 to have _parent mapped
		void indexing_calculation_patch();
};

#endif // FILELOADER_H
