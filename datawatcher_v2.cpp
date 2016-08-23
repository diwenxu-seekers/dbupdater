#include "datawatcher_v2.h"

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

inline bool contains_symbol(const QVector<symbol_updater_v2> &v, const QString &symbol)
{
	for (int i = 0; i < v.size(); ++i)
	{
		if (v[i].symbol == symbol) return true;
	}
	return false;
}

DataWatcherV2::DataWatcherV2(QObject *parent, const QString& input_dir, const QString& database_dir, Logger logger):
	QObject(parent), _input_dir(input_dir), _database_dir(database_dir), _logger(logger),
	_server(XmlConfigHandler::get_instance()->get_ipc_name())
{
	qDebug() << "DataWatcherV2::" << QThread::currentThread();

	_symbol_list = XmlConfigHandler::get_instance()->get_list_symbol();

	add_symbol(_symbol_list);

	Globals::initialize_database_lastdatetime();

	connect(this, SIGNAL(finish_update(double)), this, SLOT(onFinishUpdate(double)));
	connect(&_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
	connect(&_server, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
}

DataWatcherV2::~DataWatcherV2()
{
	qDebug("~DataWatcherV2() called");
	clear_symbol();
}

void DataWatcherV2::clear_symbol()
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

void DataWatcherV2::start_watch(int msec)
{
	t_msec = msec;
	do_update = true;

	QTimer::singleShot(100, this, SLOT(doWork()));

	_server.start();

	// @relocate to mainwindow
//  LOG_INFO ("DBUpdater started");
}

void DataWatcherV2::stop_watch()
{
	_server.stop();

	do_update = false;

	// @relocate to mainwindow
//  LOG_INFO ("DBUpdater stopped");
}

void DataWatcherV2::detect_symbol()
{
	QDir dir = _input_dir;
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
			QString _symbol = _s[0];

			if (!contains_symbol(m_updater, _symbol))
			{
				add_symbol(_symbol);
			}
		}
	}
}

// add symbol during realtime / on-the-fly (detect symbol)
void DataWatcherV2::add_symbol(const QString& symbol)
{
	symbol_updater_v2 t_updater;
	t_updater.symbol = symbol;
	t_updater.updater = new DataUpdaterV2(symbol, _input_dir, _database_dir, _logger);
	m_updater.push_back(t_updater);

	IntervalWeight interval = WEIGHT_MONTHLY;
	QString interval_name;
	QString database_name;

	while (interval != WEIGHT_INVALID)
	{
		interval_name = Globals::intervalWeightToString(interval);
		database_name = symbol + "_" + interval_name + ".db";

		if (QFile::exists(_database_dir + "/" + database_name))
		{
			Globals::initialize_last_indicator_dbupdater(_database_dir + "/" + database_name, database_name.toStdString());
		}

		interval = Globals::getChildInterval(interval);
	}
}

// initialize symbol from config.xml
void DataWatcherV2::add_symbol(const QStringList &list_symbol)
{
	symbol_updater_v2 t_updater;
	IntervalWeight interval_i;
	QString interval_name;
	QString filename;
	int count = list_symbol.size();
	bool is_symbol_exists;
	bool file_exists;

	for (int i = 0; i < count; ++i)
	{
		interval_i = WEIGHT_LARGEST;

		if (contains_symbol(m_updater, list_symbol[i]))
		{
			continue;
		}

		is_symbol_exists = false;

		while (interval_i != WEIGHT_INVALID)
		{
			interval_name = Globals::intervalWeightToString(interval_i);
			interval_i = Globals::getChildInterval(interval_i);
			filename = _input_dir + "/" + list_symbol[i] + "_" + interval_name + ".txt";
			file_exists = QFile::exists(filename);

			if (!is_symbol_exists && file_exists)
			{
				is_symbol_exists = true;
			}

			if (file_exists)
			{
				QString databasename = list_symbol[i] + "_" + interval_name + ".db";
				Globals::initialize_last_indicator_dbupdater(_database_dir + "/" + databasename, databasename.toStdString());
			}
		}

		// only create updater for exists symbol
		if (is_symbol_exists)
		{
			t_updater.symbol = list_symbol[i];
			t_updater.updater = new DataUpdaterV2(t_updater.symbol, _input_dir, _database_dir, _logger);
			m_updater.push_back(t_updater);
		}
	}
}

void DataWatcherV2::onNewConnection()
{
	LOG_INFO("A new client connected");
}

void DataWatcherV2::onDisconnected()
{
	LOG_INFO("A client disconnected");
}

void DataWatcherV2::onFinishUpdate(double /*duration*/)
{
//	if (duration > 0.0)
//	{
//		_logger->flush();
//		LOG_INFO("Database updated in {} sec(s)", duration);
//		LOG_INFO("Clients notified");
//	}

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

void DataWatcherV2::doWork()
{
	const time_point<high_resolution_clock> _time = high_resolution_clock::now();
	double update_duration = 0.0;

	if (do_update)
	{
		qDebug() << "DataWatcherV2 Thread (doWork) ::" << QThread::currentThread();

		is_running = true;

		// try to detect for new symbol on-the-fly
		detect_symbol();

		// first, update currency rate before doing any update
		BardataCalculator::update_incomplete_bar_ccy_v2("USDHKD", _input_dir, _database_dir);
		BardataCalculator::update_incomplete_bar_ccy_v2("USDJPY", _input_dir, _database_dir);
		BardataCalculator::update_incomplete_bar_ccy_v2("USDCNH", _input_dir, _database_dir);

		int count = static_cast<int>(m_updater.size());

		// set max thread = (default= 1 thread)
		QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount() * 2);

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

				update_duration = std::chrono::duration_cast<std::chrono::milliseconds>(high_resolution_clock::now()-_time).count()/1000.0;
				LOG_INFO("Database updated in {} sec(s)", update_duration);
				LOG_INFO("Clients notified");
				break;
			}
		}

		// @debug
//    Globals::debug_lastdatetime_rowid();
	}

	emit finish_update(update_duration);
}
