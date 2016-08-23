// DataWatcher.h
// Controller class for DataUpdater
#ifndef DATAWATCHERV2_H
#define DATAWATCHERV2_H

#include "dataupdater_v2.h"
#include "logdefs.h"
#include "searchbar/sqlitehandler.h"
#include "searchbar/globals.h"
#include "IPCServer/commserver.h"

typedef struct {
	QString symbol;
	DataUpdaterV2 *updater;
} symbol_updater_v2;

class DataWatcherV2 : public QObject {
	Q_OBJECT

	public:
		DataWatcherV2(QObject *parent, const QString &input_dir, const QString &database_dir, Logger logger);
		~DataWatcherV2();

		void add_symbol(const QString &symbol);
		void add_symbol(const QStringList &list_symbol);
		void start_watch(int msec = 5000);
		void stop_watch();

	private:
		server::CommServer _server;
		Logger _logger;

		QStringList _symbol_list;
		QString _database_dir = "";
		QString _input_dir = "";
		int t_msec = 0;
		bool do_update = false;
		bool is_running = false;

		// Mapping (Symbol) -> (DataUpdater)
		QVector<symbol_updater_v2> m_updater;

		void detect_symbol();
		void clear_symbol();

	public slots:
		void doWork();
		void onNewConnection();
		void onDisconnected();
		void onFinishUpdate(double duration);

	signals:
		void finish_watching();
		void finish_update(double duration);
};

#endif // DATAWATCHERV2_H
