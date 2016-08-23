#include "datawatcher.h"

#include "searchbar/globals.h"
#include "searchbar/xmlconfighandler.h"
#include "bardatacalculator.h"

#include <QDir>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

#include <chrono>

#define LOG_ERROR(...) try{_logger->error(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_WARN(...) try{_logger->warn(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_INFO(...) try{_logger->info(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}
#define LOG_DEBUG(...) try{_logger->debug(__VA_ARGS__);}catch(spdlog::spdlog_ex &){}

using namespace std::chrono;

inline bool contains_symbol(const std::vector<symbol_updater> &v, const std::string &symbol)
{
	for (int i = 0; i < v.size(); ++i)
	{
		if (v[i].symbol == symbol) return true;
	}
	return false;
}

DataWatcher::DataWatcher(QObject *parent, const std::string &input_path, const std::string &database_path, Logger logger):
	QObject(parent), _logger(logger), _server(XmlConfigHandler::get_instance()->get_ipc_name())
{
	qDebug() << "DataWatcher::" << QThread::currentThread();

	m_input_path = input_path;
	m_database_path = database_path;
	m_symbol_list = XmlConfigHandler::get_instance()->get_list_symbol();
	add_symbol(m_symbol_list);

	Globals::initialize_database_lastdatetime();

	connect(this, SIGNAL(finish_update()), this, SLOT(onFinishedUpdate()));
	connect(&_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
	connect(&_server, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
}

DataWatcher::~DataWatcher()
{
	qDebug("~DataWatcher() called");
	clear_symbol();
}

void DataWatcher::clear_symbol()
{
	QThreadPool::globalInstance()->waitForDone(-1);
	const int count = static_cast<int>(m_updater.size());

	for (int i = 0; i < count; ++i)
	{
		delete m_updater[i].updater;
		m_updater[i].updater = nullptr;
	}

	m_updater.clear();
}

void DataWatcher::start_watch(int msec)
{
	t_msec = msec;
	do_update = true;

	QTimer::singleShot(100, this, SLOT(doWork()));

	_server.start();

	// @relocate to mainwindow
//  LOG_INFO ("DBUpdater started");
}

void DataWatcher::stop_watch()
{
	_server.stop();

	do_update = false;

	// @relocate to mainwindow
//  LOG_INFO ("DBUpdater stopped");
}

void DataWatcher::detect_symbol()
{
	QDir dir = QString::fromStdString(m_input_path);
	QStringList nameFilter("*.txt");
	QStringList files = dir.entryList(nameFilter);
	QStringList _s;

	// exclude CCY file from update
	QStringList excluded_files;
	excluded_files.push_back("USDHKD");
	excluded_files.push_back("USDJPY");
	excluded_files.push_back("USDCNH");
	excluded_files.push_back("EURUSD");

	for (int i = 0; i < files.size(); ++i)
	{
		_s = files[i].split("_");

		if (_s.size() > 1 && !excluded_files.contains(_s[0]))
		{
			std::string _symbol = _s[0].toStdString();

			if (!contains_symbol(m_updater, _symbol))
			{
				add_symbol(_symbol);
			}
		}
	}
}

// add symbol during realtime / on-the-fly (detect symbol)
void DataWatcher::add_symbol(const std::string &symbol)
{
	symbol_updater t_updater;
	t_updater.symbol = symbol;
	t_updater.updater = new DataUpdater(symbol, m_input_path, m_database_path, _logger);
	m_updater.emplace_back(t_updater);

	IntervalWeight interval = WEIGHT_MONTHLY;
	QString interval_name;
	QString database_name;

	while (interval != WEIGHT_INVALID)
	{
		interval_name = Globals::intervalWeightToString(interval);
		database_name = QString::fromStdString(symbol) + "_" + interval_name + ".db";

		if (Globals::is_file_exists(m_database_path + "/" + database_name.toStdString()))
		{
			Globals::initialize_last_indicator_dbupdater
				(QString::fromStdString(m_database_path) + "/" + database_name, database_name.toStdString());
		}

		interval = Globals::getChildInterval(interval);
	}
}

// initialize symbol from config.xml
void DataWatcher::add_symbol(const QStringList &list_symbol)
{
	symbol_updater t_updater;
	IntervalWeight interval_i;
	std::string interval_name;
	int count = list_symbol.size();
	bool is_symbol_exists;
	bool file_exists;

	for (int i = 0; i < count; ++i)
	{
		interval_i = WEIGHT_LARGEST;

		if (contains_symbol(m_updater, list_symbol[i].toStdString()))
		{
			continue;
		}

		is_symbol_exists = false;

		while (interval_i != WEIGHT_INVALID)
		{
			interval_name = Globals::intervalWeightToStdString(interval_i);
			interval_i = Globals::getChildInterval(interval_i);
			file_exists = Globals::is_file_exists(m_input_path + "/" + list_symbol[i].toStdString() + "_" + interval_name + ".txt");

			if (!is_symbol_exists && file_exists)
			{
				is_symbol_exists = true;
			}

			if (file_exists)
			{
				Globals::initialize_last_indicator_dbupdater (QString::fromStdString(m_database_path) + "/" +
					list_symbol[i] + "_" + QString::fromStdString(interval_name) + ".db",
					list_symbol[i].toStdString() + "_" + interval_name + ".db");
			}
		}

		// only create updater for exists symbol
		if (is_symbol_exists)
		{
			t_updater.symbol = list_symbol[i].toStdString();
			t_updater.updater = new DataUpdater(t_updater.symbol, m_input_path, m_database_path, _logger);
			m_updater.emplace_back(t_updater);
		}
	}
}

void DataWatcher::onNewConnection()
{
	LOG_INFO("A new client connected");
}

void DataWatcher::onDisconnected()
{
	LOG_INFO("A client disconnected");
}

void DataWatcher::onFinishedUpdate()
{
	if (do_update)
	{
		QTimer::singleShot(t_msec, this, SLOT(doWork()));
	}
	else
	{
		is_running = false;
		emit finish_watching();
	}
}

void DataWatcher::doWork()
{
	if (do_update)
	{
		qDebug() << "DataWatcher Thread (doWork) ::" << QThread::currentThread();

		is_running = true;

		const time_point<high_resolution_clock> _time = high_resolution_clock::now();

		// try to detect for new symbol on-the-fly
		detect_symbol();

		// first, we need to update currency rate before doing any update
		BardataCalculator::update_incomplete_bar_ccy(QString::fromStdString(m_input_path) + "/USDHKD_5min.txt",
																								 QString::fromStdString(m_database_path) + "/USDHKD_Daily.db");

		BardataCalculator::update_incomplete_bar_ccy(QString::fromStdString(m_input_path) + "/USDJPY_5min.txt",
																								 QString::fromStdString(m_database_path) + "/USDJPY_Daily.db");

		BardataCalculator::update_incomplete_bar_ccy(QString::fromStdString(m_input_path) + "/USDCNH_5min.txt",
																								 QString::fromStdString(m_database_path) + "/USDCNH_Daily.db");

		int count = static_cast<int>(m_updater.size());

		// set max thread = (default)
		QThreadPool::globalInstance()->setMaxThreadCount(1);

		// set no timeout (wait until all processes finished)
		QThreadPool::globalInstance()->setExpiryTimeout(-1);

		// begin update
		for (int i = 0; i < count; ++i)
		{
			if (m_updater[i].updater != NULL)
			{
				QThreadPool::globalInstance()->start(m_updater[i].updater, QThread::NormalPriority);
			}
		}

		// wait until all process done
		QThreadPool::globalInstance()->waitForDone(-1);

		// notify if new data exists
		for (int i = 0; i < count; ++i)
		{
			if (m_updater[i].updater != NULL && m_updater[i].updater->is_found_new_data())
			{
				if (_server.isRunning())
				{
					_server.notifyUpdate();
				}
				LOG_INFO ("Database updated in {} sec(s)", std::chrono::duration_cast<std::chrono::milliseconds>(high_resolution_clock::now()-_time).count()/1000.0);
				LOG_INFO ("Clients notified");
				break;
			}
		}

		// @debug
//    Globals::debug_lastdatetime_rowid();
	}

	emit finish_update();

//	if (do_update)
//	{
//		QTimer::singleShot(t_msec, this, SLOT(doWork()));
//	}
//	else
//	{
//		is_running = false;
//		emit finish_watching();
//	}
}
