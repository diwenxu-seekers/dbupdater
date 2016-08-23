// DataWatcher.h
// Controller class for DataUpdater
#ifndef DATAWATCHER_H
#define DATAWATCHER_H

#include "dataupdater.h"
#include "logdefs.h"
#include "searchbar/sqlitehandler.h"
#include "searchbar/globals.h"
#include "IPCServer/commserver.h"

#include <string>
#include <vector>

typedef struct {
	std::string symbol;
	DataUpdater *updater;
} symbol_updater;

class DataWatcher : public QObject {
	Q_OBJECT

	public:
		DataWatcher(QObject *parent, const std::string &m_input_path, const std::string &m_database_path, Logger logger);
		~DataWatcher();

		void add_symbol(const std::string &symbol);
		void add_symbol(const QStringList &list_symbol);
		void start_watch(int msec = 5000);
		void stop_watch();

	private:
		server::CommServer _server;
		Logger _logger;
		QStringList m_symbol_list;
		std::string m_database_path = "";
		std::string m_input_path = "";
		int t_msec = 0;
		bool do_update = false;
		bool is_running = false;

		// Mapping (Symbol) -> (DataUpdater)
		std::vector<symbol_updater> m_updater;

		void detect_symbol();
		void clear_symbol();

	public slots:
		void doWork();
		void onNewConnection();
		void onDisconnected();
		void onFinishedUpdate();

	signals:
		void finish_watching();
		void finish_update();
};

#endif // DATAWATCHER_H
