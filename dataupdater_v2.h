#ifndef DATAUPDATER_V2_H
#define DATAUPDATER_V2_H

#include "searchbar/globals.h"
#include "bardatatuple.h"
#include "logdefs.h"

#include <QRunnable>
#include <QSqlDatabase>

class DataUpdaterV2 : public QRunnable {
	public:
		DataUpdaterV2(const QString &symbol, const QString &input_dir, const QString &database_dir, Logger logger);

		~DataUpdaterV2();

		bool is_found_new_data() const;

	private:
		Logger _logger;
		QString _symbol;
		QString _database_dir;
		QString _input_dir;
		QString _update_stmt;
		IntervalWeight recalculate_from_interval;
		int  recalculate_parent_rowid;
		bool recalculate_flag;
		bool is_found_new;

		void create_database(IntervalWeight i, QSqlDatabase *d);

		bool is_new_data_available(const QString &filename, const QString &last_datetime);

//		bool file_load(const QString &filename, const QString& database_name, IntervalWeight interval, std::vector<BardataTuple> *data);

		void recalculate(IntervalWeight interval, int start_parent_rowid);

		void save_result(const QSqlDatabase& database, std::vector<BardataTuple> *data);

		bool update(QSqlDatabase &p_db, IntervalWeight interval, int start_rowid, std::vector<BardataTuple> *data, bool new_data_flag);

		void run() override;
};

#endif // DATAUPDATER_V2_H
