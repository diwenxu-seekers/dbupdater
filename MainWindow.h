#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aboutdlg.h"
#include "infobox.h"
#include "logdefs.h"

#if defined(DBUPDATER_VERSION_1)
#include "datawatcher.h"
#else
#include "datawatcher_v2.h"
#endif

#include <QFutureWatcher>
#include <QMainWindow>
#include <QToolButton>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = Q_NULLPTR);
		~MainWindow();
		void init_logger();

	private:
		Ui::MainWindow* ui;
		AboutDlg* _aboutDlg = nullptr;
		Logger _logger;
		InfoBox _infoboxMsg;

		QThread *watcher_thread = nullptr;

#if defined(DBUPDATER_VERSION_1)
		DataWatcher *watcher = nullptr;
#else
		DataWatcherV2 *watcher = nullptr;
#endif

		QFutureWatcher<void> indexing_watcher;
		QAction* actStart = nullptr;
		QAction* actStop = nullptr;
		QAction* actCompactDB = nullptr;
		QAction* actIndexDB = nullptr;
		QToolButton* _btnStartStop;
		QEventLoop wait_for_finish;

		int _timer_interval = 3000; // miliseconds

	private slots:
		// toggle to start/stop updater
		void start();
		void stop();

		// start indexing event
		void indexing_database();
		void compact_database();

		// finished indexing event
		void indexing_database_finished();
		void compact_database_finished();

		void about();
};

#endif // MAINWINDOW_H
