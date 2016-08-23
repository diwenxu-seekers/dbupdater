/* DataUpdater.h
 * -------------
 * Thread to maintain update database for each symbol.
 * Connection String Format: {Symbol}.{className}.{functionName}
 */
#ifndef DATAUPDATER_H
#define DATAUPDATER_H

#include "searchbar/globals.h"
#include "bardatatuple.h"
#include "logdefs.h"

#include <QRunnable>
#include <QSqlDatabase>

#include <string>
#include <vector>

class DataUpdater : public QRunnable {
	public:
		DataUpdater(const std::string &symbol, const std::string &m_input_path, const std::string &m_database_path, Logger logger);
		~DataUpdater();

		void set_symbol(const std::string &symbol);
		void set_database_path(const std::string &path);
		void set_input_path(const std::string &path);
		bool is_found_new_data() const;
		int count_new_data() const;

	protected:
		void run();

	private:
		static int connection_id;
		Logger _logger;

		// config threshold
		QVector<t_sr_threshold> m_threshold;

		std::vector<BardataTuple> m_cache;
		std::string sql_update_bardata = "";
		std::string m_symbol = "";
		std::string m_database_path = "";
		std::string m_input_path = "";
		int _count_new_data;
		bool _found_new;

		bool create_database(IntervalWeight _interval, QSqlDatabase *out, bool enable_create_table = true, bool enable_preset_sqlite = true);
		void init_update_statement();
		void save_result(const char *m_database_path, const std::vector<BardataTuple> &data);
		void save_result_v2(const QString &m_database_path, const std::vector<BardataTuple> &data);
		bool check_new_data(const std::string &file_path, const std::string &last_datetime);
		bool check_new_data_v2(const QString &file_path, const QString &last_datetime);
};

#endif // DATAUPDATER_H
