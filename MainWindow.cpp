#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "bardatacalculator.h"
#include "bardatadefinition.h"
#include "datetimehelper.h"

#include "searchbar/globals.h"
#include "searchbar/sqlitehandler.h"
#include "searchbar/xmlconfighandler.h"
#include "logwidget/tableview_sink.h"
#include "spdlog/spdlog.h"
#include "sqlite3/sqlite3.h"

#include <QtConcurrent/QtConcurrent>
#include <QDockWidget>
#include <QDir>
#include <QFuture>
#include <QLabel>
#include <QMessageBox>
#include <QThread>

#include <chrono>
#include <ctime>
#include <string>
#include <vector>

#define LOG_ERROR(...) try{_logger->error(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_WARN(...) try{_logger->warn(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_INFO(...) try{_logger->info(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}

void create_or_delete_index_db(const std::vector<std::string> &database_name, const bool &is_create, Logger _logger)
{
	sqlite3 *db = NULL;
	QStringList _list;
	std::string dbname;
	const int row_count = static_cast<int>(database_name.size());

	for (int i = 0; i < row_count; ++i)
	{
		if (sqlite3_open(database_name[i].c_str(), &db) == SQLITE_OK)
		{
			_list = QString::fromStdString(database_name[i]).split("/");
			dbname = _list.size() > 1 ? _list.back().toStdString() : database_name[i];

			if (is_create)
			{
				LOG_INFO ("Create index on {}", dbname);
				sqlite3_exec(db, bardata::create_extended_full_index, 0, 0, 0);
			}
			else
			{
				LOG_INFO ("Compact database on {}", dbname);
				sqlite3_exec(db, bardata::drop_extended_index, 0, 0, 0);
				sqlite3_exec(db, "PRAGMA mmap_size=0; VACUUM;", 0, 0, 0);
			}
		}

		sqlite3_close(db);
		db = NULL;
	}
}

void get_database_list(std::vector<std::string> *out)
{
	XmlConfigHandler *config = XmlConfigHandler::get_instance();
	QStringList symbols_list = config->get_list_symbol ();
	std::string database_path = config->get_database_dir().toStdString();
	std::string filename;
	std::string interval_name;
	IntervalWeight _interval;

	for (int i = 0; i < symbols_list.size(); ++i)
	{
		_interval = WEIGHT_MONTHLY;

		while (_interval != WEIGHT_INVALID)
		{
			interval_name = Globals::intervalWeightToStdString(_interval);
			filename = database_path + "/" + symbols_list[i].toStdString() + "_" + interval_name + ".db";

			if (QFile::exists(QString::fromStdString(filename)))
			{
				out->emplace_back(filename);
			}

			_interval = Globals::getChildInterval(_interval);
		}
	}
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow), _infoboxMsg(this)
{
	ui->setupUi(this);

	// Set title
	QString sTitle = QString (tr("%1 v%2")).arg(QCoreApplication::applicationName(), QCoreApplication::applicationVersion());
	this->setWindowTitle(sTitle);

	// Create actions
	actStart = new QAction(QIcon(":/resources/startbtn.png"), tr("Start"), this);
	actStart->setStatusTip(QString(tr("Start %1").arg (QCoreApplication::applicationName())));
	connect (actStart, SIGNAL(triggered()), this, SLOT(start()));

	actStop = new QAction(QIcon(":/resources/stopbtn.png"), tr("Stop"), this);
	actStop->setStatusTip(QString (tr("Stop %1")).arg(QCoreApplication::applicationName()));
	connect (actStop, SIGNAL(triggered()), this, SLOT(stop()));

	_btnStartStop = new QToolButton(this);
	_btnStartStop->setDefaultAction(actStart);

	// Create index on database
	actIndexDB = new QAction(QIcon(":/resources/create_db_index.png"), tr("Indexing database"), this);
	actIndexDB->setStatusTip(QString(tr ("Create indexing on database")));
	connect (actIndexDB, SIGNAL(triggered()), this, SLOT(indexing_database()));

	// Compact database
	actCompactDB = new QAction(QIcon(":/resources/compact_db.png"), tr("Compact Database"), this);
	actCompactDB->setStatusTip(QString(tr("Compact database")));
	connect (actCompactDB, SIGNAL(triggered()), this, SLOT(compact_database()));

	// About dialog
	QAction* actAbout = new QAction(QIcon(":/resources/aboutbtn.png"), tr("About"), this);
	actAbout->setStatusTip(QString(tr("Show %1 Information").arg (QCoreApplication::applicationName())));
	connect (actAbout, SIGNAL(triggered()), this, SLOT(about()));

	// Add buttons to UI main toolbar
	ui->toolBar->addWidget(_btnStartStop);

	// Add a new tool bar for other actions
	QToolBar* tbOther = addToolBar(tr("Other"));
	tbOther->setIconSize(QSize(48, 48));
	tbOther->setLayoutDirection(Qt::RightToLeft);

	// Add to other toolbar
	tbOther->addAction(actAbout);
	tbOther->addSeparator();
	tbOther->addAction(actCompactDB);
	tbOther->addAction(actIndexDB);

	// Initialize logger
	init_logger();

	watcher_thread = new QThread(this);

	// Reading configurations
	XmlConfigHandler *config = XmlConfigHandler::get_instance();
	_timer_interval = static_cast<int>(config->get_timer_interval() * 1000);

#if defined(DBUPDATER_VERSION_1)
	std::string input_path = config->get_input_dir().toStdString();
	std::string database_path = config->get_database_dir().toStdString();
	watcher = new DataWatcher(NULL, input_path, database_path, _logger);
#else
	QString input_path = config->get_input_dir();
	QString database_path = config->get_database_dir();
	watcher = new DataWatcherV2(NULL, input_path, database_path, _logger);
#endif

	watcher->moveToThread(watcher_thread);

	connect(watcher, SIGNAL(finish_watching()), &wait_for_finish, SLOT(quit()));
	connect(watcher_thread, SIGNAL(finished()), watcher_thread, SLOT(deleteLater()));

	watcher_thread->start();
}

MainWindow::~MainWindow()
{
	delete watcher;
	watcher_thread->quit();
	delete ui;
}

void MainWindow::init_logger()
{
//#ifndef _DEBUG
//  spdlog::set_async_mode (1048576, spdlog::async_overflow_policy::block_retry);
//#endif

		std::vector<spdlog::sink_ptr> sinks;

		// UI log message sink
		LogMsgWidget* logMsgWidget = new LogMsgWidget();

		QDockWidget* logMsgdock = new QDockWidget(tr("Log Messages"), this);
		logMsgdock->setAllowedAreas(Qt::BottomDockWidgetArea);
		logMsgdock->setFeatures(QDockWidget::DockWidgetMovable);
		logMsgdock->setWidget(logMsgWidget);

		this->addDockWidget(Qt::BottomDockWidgetArea, logMsgdock);

		auto ui_sink = std::make_shared<spdlog::sinks::tableview_sink_mt>(logMsgWidget);

		// Default log level : ERROR
		ui_sink->set_level(spdlog::level::info);

		sinks.push_back(ui_sink);

		// Rotating file logger
		try
		{
				// Log path
				QString sLogPath = QCoreApplication::applicationDirPath() + tr("/") + QString(LOG_PATH);

				// Make sure the log path exists
				QDir().mkpath(sLogPath);

				// Get application name
				QString sAppName = QCoreApplication::applicationFilePath();
				sAppName = sAppName.mid(sAppName.lastIndexOf('/') + 1);
				sAppName = sAppName.mid(0, sAppName.lastIndexOf('.'));

				// Log base name
				QString sLogFilebasename = sLogPath + tr("/") + sAppName.toLower();

				// Rotating file logger
				auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(sLogFilebasename.toStdString().c_str(), LOG_FILE_EXT,
																																								LOG_FILE_SIZE, LOG_FILE_COUNT, true);

				// Set log level
				file_sink->set_level(spdlog::level::debug);

				// Add logger to sink
				sinks.push_back(file_sink);

				auto combined_logger = spdlog::create("logger", begin(sinks), end(sinks));
				combined_logger->set_pattern(LOG_PATTERN);
		}
		catch (const spdlog::spdlog_ex& ex)
		{
				QMessageBox::warning(this, tr("Unable to create file logger"), QString(ex.what()), QMessageBox::Ok, QMessageBox::Warning);

				auto combined_logger = spdlog::create("logger", begin(sinks), end(sinks));
				combined_logger->set_pattern(LOG_PATTERN);

				combined_logger->critical(ex.what());
		}

		_logger = spdlog::get("logger");
}

void MainWindow::start()
{
	// Change icon to stop
	_btnStartStop->setDefaultAction(actStop);

	// Disable exit action
	this->setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
	this->show();
	this->activateWindow();

	// Disable compact/indexing action
	actCompactDB->setEnabled(false);
	actIndexDB->setEnabled(false);

	// Start worker thread
	watcher->start_watch(_timer_interval);

	LOG_INFO ("DBUpdater started");
}

void MainWindow::stop()
{
	// temporary disable button and show message
	QLabel *msg = new QLabel("Stopping update.. Please wait until current update finish..");
	ui->statusBar->addPermanentWidget(msg);
	_btnStartStop->setEnabled(false);

	// Stop worker thread
	watcher->stop_watch();

	// wait for signal (until update finished)
	wait_for_finish.exec();

	// Change icon to start
	_btnStartStop->setDefaultAction(actStart);
	_btnStartStop->setEnabled(true);

	// Enable exit action
	this->setWindowFlags(this->windowFlags()|Qt::WindowCloseButtonHint);
	this->show();
	this->activateWindow();

	// Enable compact/indexing action
	actCompactDB->setEnabled(true);
	actIndexDB->setEnabled(true);

	// release message
	ui->statusBar->removeWidget(msg);
	delete msg;

	LOG_INFO ("DBUpdater stopped");
}

void MainWindow::indexing_database()
{
	if (QMessageBox::question(this, "Create Index Confirmation", "Do you want to create index on database?\n"
				"Warning: the process may take several minutes to complete.") == QMessageBox::No)
		return;

	// Populate database files
	std::vector<std::string> database_name;
	get_database_list (&database_name);

	QFuture<void> future = QtConcurrent::run(QThreadPool::globalInstance(), create_or_delete_index_db, database_name, true, _logger);

	connect (&indexing_watcher, SIGNAL(finished()), this, SLOT(indexing_database_finished()));
	indexing_watcher.setFuture(future);

	// Show blocking message
	_infoboxMsg.setText("Creating database index...");
	_infoboxMsg.show();
}

void MainWindow::compact_database()
{
	if (QMessageBox::question(this, "Compact Database Confirmation", "Do you want to compact database?\n"
				"Warning: the process may take several minutes to complete.") == QMessageBox::No)
		return;

	// populate database files
	std::vector<std::string> database_name;
	get_database_list (&database_name);

	QFuture<void> future = QtConcurrent::run(QThreadPool::globalInstance(), create_or_delete_index_db, database_name, false, _logger);

	connect (&indexing_watcher, SIGNAL(finished()), this, SLOT(compact_database_finished()));
	indexing_watcher.setFuture(future);

	// Show blocking message
	_infoboxMsg.setText("Compacting database...");
	_infoboxMsg.show();
}

void MainWindow::compact_database_finished()
{
	LOG_INFO ("Finish compacting database");
	indexing_watcher.disconnect();

	// Unblocking UI input
	_infoboxMsg.close();
}

void MainWindow::indexing_database_finished()
{
	LOG_INFO ("Finish creating index");
	indexing_watcher.disconnect();

	// Unblocking UI input
	_infoboxMsg.close();
}

void MainWindow::about()
{
	// If about dialog not created yet, create it
	if (!_aboutDlg)
	{
		_aboutDlg = new AboutDlg(this);
	}

	// Show it
	_aboutDlg->show();
}
