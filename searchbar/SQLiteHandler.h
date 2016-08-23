#ifndef SQLITEHANDLER_H
#define SQLITEHANDLER_H

#include "globals.h"

#include <QCoreApplication>
#include <QDate>
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlQueryModel>

class SQLiteHandler {
	public:
		// TABLE NAME
		static const QString TABLE_NAME_BARDATA;
		static const QString TABLE_NAME_BARDATA_VIEW; // temp table
		static const QString TABLE_NAME_BARDATA_LOOKAHEAD; // temp table
		static const QString TABLE_NAME_RESISTANCE;
		static const QString TABLE_NAME_RESISTANCE_DATE;
		static const QString TABLE_NAME_SUPPORT;
		static const QString TABLE_NAME_SUPPORT_DATE;

		// COLUMN NAME
		static const QString COLUMN_NAME_ROWID;
		static const QString COLUMN_NAME_COMPLETED;
		static const QString COLUMN_NAME_IDPARENT;
		static const QString COLUMN_NAME_IDPARENT_WEEKLY;
		static const QString COLUMN_NAME_IDPARENT_MONTHLY;
		static const QString COLUMN_NAME_IDPARENT_PREV;
		static const QString COLUMN_NAME_IDPARENT_PREV_WEEKLY;
		static const QString COLUMN_NAME_IDPARENT_PREV_MONTHLY;
		static const QString COLUMN_NAME_IDTHRESHOLD;
		static const QString COLUMN_NAME_DATE;
		static const QString COLUMN_NAME_TIME;
		static const QString COLUMN_NAME_OPEN;
		static const QString COLUMN_NAME_HIGH;
		static const QString COLUMN_NAME_LOW;
		static const QString COLUMN_NAME_CLOSE;
		static const QString COLUMN_NAME_BAR_RANGE;
		static const QString COLUMN_NAME_INTRADAY_HIGH;
		static const QString COLUMN_NAME_INTRADAY_LOW;
		static const QString COLUMN_NAME_INTRADAY_RANGE;
		static const QString COLUMN_NAME_VOLUME;
		static const QString COLUMN_NAME_MACD;
		static const QString COLUMN_NAME_MACD_RANK;
		static const QString COLUMN_NAME_MACDAVG;
		static const QString COLUMN_NAME_MACDDIFF;

		// MACD with threshold
		static const QString COLUMN_NAME_MACD_VALUE1;
		static const QString COLUMN_NAME_MACD_VALUE2;
		static const QString COLUMN_NAME_MACD_RANK1;
		static const QString COLUMN_NAME_MACD_RANK2;

		static const QString COLUMN_NAME_RSI;
		static const QString COLUMN_NAME_RSI_RANK;

		// RSI with threshold
		static const QString COLUMN_NAME_RSI_VALUE1;
		static const QString COLUMN_NAME_RSI_VALUE2;
		static const QString COLUMN_NAME_RSI_RANK1;
		static const QString COLUMN_NAME_RSI_RANK2;

		static const QString COLUMN_NAME_SLOWK;
		static const QString COLUMN_NAME_SLOWK_RANK;

		// SlowK with threshold
		static const QString COLUMN_NAME_SLOWK_VALUE1;
		static const QString COLUMN_NAME_SLOWK_VALUE2;
		static const QString COLUMN_NAME_SLOWK_RANK1;
		static const QString COLUMN_NAME_SLOWK_RANK2;

		static const QString COLUMN_NAME_SLOWD;
		static const QString COLUMN_NAME_SLOWD_RANK;

		// SlowD with threshold
		static const QString COLUMN_NAME_SLOWD_VALUE1;
		static const QString COLUMN_NAME_SLOWD_VALUE2;
		static const QString COLUMN_NAME_SLOWD_RANK1;
		static const QString COLUMN_NAME_SLOWD_RANK2;

		static const QString COLUMN_NAME_FASTAVG;
		static const QString COLUMN_NAME_SLOWAVG;
		static const QString COLUMN_NAME_DAY10;
		static const QString COLUMN_NAME_DAY50;
		static const QString COLUMN_NAME_WEEK10;
		static const QString COLUMN_NAME_WEEK50;
		static const QString COLUMN_NAME_MONTH10;
		static const QString COLUMN_NAME_MONTH50;
		static const QString COLUMN_NAME_FASTAVG_SLOPE;
		static const QString COLUMN_NAME_FASTAVG_SLOPE_RANK;
		static const QString COLUMN_NAME_SLOWAVG_SLOPE;
		static const QString COLUMN_NAME_SLOWAVG_SLOPE_RANK;
		static const QString COLUMN_NAME_FCROSS;
		static const QString COLUMN_NAME_SCROSS;
		static const QString COLUMN_NAME_FGS;
		static const QString COLUMN_NAME_FGS_RANK;
		static const QString COLUMN_NAME_FLS;
		static const QString COLUMN_NAME_FLS_RANK;
		static const QString COLUMN_NAME_DISTF;
		static const QString COLUMN_NAME_DISTS;
		static const QString COLUMN_NAME_DISTCF;
		static const QString COLUMN_NAME_DISTCF_RANK;
		static const QString COLUMN_NAME_N_DISTCF;
		static const QString COLUMN_NAME_DISTCS;
		static const QString COLUMN_NAME_DISTCS_RANK;
		static const QString COLUMN_NAME_N_DISTCS;
		static const QString COLUMN_NAME_DISTFS;
		static const QString COLUMN_NAME_DISTFS_RANK;
		static const QString COLUMN_NAME_N_DISTFS;
		static const QString COLUMN_NAME_FSCROSS;
		static const QString COLUMN_NAME_DISTCC_FSCROSS;
		static const QString COLUMN_NAME_DISTCC_FSCROSS_ATR;
		static const QString COLUMN_NAME_DISTCC_FSCROSS_RANK;
		static const QString COLUMN_NAME_ATR;
		static const QString COLUMN_NAME_ATR_RANK;
		static const QString COLUMN_NAME_PREV_DAILY_ATR;
		static const QString COLUMN_NAME_OPENBAR;
		static const QString COLUMN_NAME_PREV_DATE;
		static const QString COLUMN_NAME_PREV_TIME;
		static const QString COLUMN_NAME_PREV_BARCOLOR;
		static const QString COLUMN_NAME_ROC;
		static const QString COLUMN_NAME_ROC_RANK;
		static const QString COLUMN_NAME_CCY_RATE;

		static const QString COLUMN_NAME_CANDLE_UPTAIL;
		static const QString COLUMN_NAME_CANDLE_DOWNTAIL;
		static const QString COLUMN_NAME_CANDLE_BODY;
		static const QString COLUMN_NAME_CANDLE_TOTALLENGTH;
		static const QString COLUMN_NAME_N_UPTAIL;
		static const QString COLUMN_NAME_N_DOWNTAIL;
		static const QString COLUMN_NAME_N_BODY;
		static const QString COLUMN_NAME_N_TOTALLENGTH;
		static const QString COLUMN_NAME_CANDLE_UPTAIL_RANK;
		static const QString COLUMN_NAME_CANDLE_DOWNTAIL_RANK;
		static const QString COLUMN_NAME_CANDLE_BODY_RANK;
		static const QString COLUMN_NAME_CANDLE_TOTALLENGTH_RANK;

		static const QString COLUMN_NAME_DAYRANGE_HIGH_1DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_2DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_3DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY;
		static const QString COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_1DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_2DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_3DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY;
		static const QString COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY;

		static const QString COLUMN_NAME_CGF;
		static const QString COLUMN_NAME_CGF_RANK;
		static const QString COLUMN_NAME_CLF;
		static const QString COLUMN_NAME_CLF_RANK;
		static const QString COLUMN_NAME_CGS;
		static const QString COLUMN_NAME_CGS_RANK;
		static const QString COLUMN_NAME_CLS;
		static const QString COLUMN_NAME_CLS_RANK;
		static const QString COLUMN_NAME_HLF;
		static const QString COLUMN_NAME_HLS;
		static const QString COLUMN_NAME_LGF;
		static const QString COLUMN_NAME_LGS;

		static const QString COLUMN_NAME_OPEN_ZONE;
		static const QString COLUMN_NAME_OPEN_ZONE_60MIN;
		static const QString COLUMN_NAME_OPEN_ZONE_DAILY;
		static const QString COLUMN_NAME_OPEN_ZONE_WEEKLY;
		static const QString COLUMN_NAME_OPEN_ZONE_MONTHLY;
		static const QString COLUMN_NAME_HIGH_ZONE;
		static const QString COLUMN_NAME_HIGH_ZONE_60MIN;
		static const QString COLUMN_NAME_HIGH_ZONE_DAILY;
		static const QString COLUMN_NAME_HIGH_ZONE_WEEKLY;
		static const QString COLUMN_NAME_HIGH_ZONE_MONTHLY;
		static const QString COLUMN_NAME_LOW_ZONE;
		static const QString COLUMN_NAME_LOW_ZONE_60MIN;
		static const QString COLUMN_NAME_LOW_ZONE_DAILY;
		static const QString COLUMN_NAME_LOW_ZONE_WEEKLY;
		static const QString COLUMN_NAME_LOW_ZONE_MONTHLY;
		static const QString COLUMN_NAME_CLOSE_ZONE;
		static const QString COLUMN_NAME_CLOSE_ZONE_60MIN;
		static const QString COLUMN_NAME_CLOSE_ZONE_DAILY;
		static const QString COLUMN_NAME_CLOSE_ZONE_WEEKLY;
		static const QString COLUMN_NAME_CLOSE_ZONE_MONTHLY;

		static const QString COLUMN_NAME_DISTOF;
		static const QString COLUMN_NAME_DISTOS;
		static const QString COLUMN_NAME_DISTHF;
		static const QString COLUMN_NAME_DISTHS;
		static const QString COLUMN_NAME_DISTLF;
		static const QString COLUMN_NAME_DISTLS;
		static const QString COLUMN_NAME_N_DISTOF;
		static const QString COLUMN_NAME_N_DISTOS;
		static const QString COLUMN_NAME_N_DISTHF;
		static const QString COLUMN_NAME_N_DISTHS;
		static const QString COLUMN_NAME_N_DISTLF;
		static const QString COLUMN_NAME_N_DISTLS;
		static const QString COLUMN_NAME_DISTOF_RANK;
		static const QString COLUMN_NAME_DISTOS_RANK;
		static const QString COLUMN_NAME_DISTHF_RANK;
		static const QString COLUMN_NAME_DISTHS_RANK;
		static const QString COLUMN_NAME_DISTLF_RANK;
		static const QString COLUMN_NAME_DISTLS_RANK;

		// generic column
		static const QString COLUMN_NAME_RES;
		static const QString COLUMN_NAME_DISTRES;
		static const QString COLUMN_NAME_DISTRES_ATR;
		static const QString COLUMN_NAME_DISTRES_RANK;
		static const QString COLUMN_NAME_RES_LASTREACTDATE;
		static const QString COLUMN_NAME_SUP;
		static const QString COLUMN_NAME_DISTSUP;
		static const QString COLUMN_NAME_DISTSUP_ATR;
		static const QString COLUMN_NAME_DISTSUP_RANK;
		static const QString COLUMN_NAME_SUP_LASTREACTDATE;
		static const QString COLUMN_NAME_DAILY_RLINE;
		static const QString COLUMN_NAME_DAILY_SLINE;
		static const QString COLUMN_NAME_WEEKLY_RLINE;
		static const QString COLUMN_NAME_WEEKLY_SLINE;
		static const QString COLUMN_NAME_MONTHLY_RLINE;
		static const QString COLUMN_NAME_MONTHLY_SLINE;
		static const QString COLUMN_NAME_DOWNVEL_DISTBAR;
		static const QString COLUMN_NAME_DOWNVEL_DISTPOINT;
		static const QString COLUMN_NAME_DOWNVEL_DISTATR;
		static const QString COLUMN_NAME_UPVEL_DISTBAR;
		static const QString COLUMN_NAME_UPVEL_DISTPOINT;
		static const QString COLUMN_NAME_UPVEL_DISTATR;

		// Resistance/Support
		static const QString COLUMN_NAME_REACT_DATE;
		static const QString COLUMN_NAME_REACT_TIME;
		static const QString COLUMN_NAME_DIST_POINT;
		static const QString COLUMN_NAME_DIST_ATR;
		static const QString COLUMN_NAME_RESISTANCE_COUNT;
		static const QString COLUMN_NAME_RESISTANCE_DURATION;
		static const QString COLUMN_NAME_RESISTANCE_VALUE;
		static const QString COLUMN_NAME_SUPPORT_COUNT;
		static const QString COLUMN_NAME_SUPPORT_DURATION;
		static const QString COLUMN_NAME_SUPPORT_VALUE;
		static const QString COLUMN_NAME_TAGLINE;
		static const QString COLUMN_NAME_HIGHBARS;
		static const QString COLUMN_NAME_LOWBARS;
		static const QString COLUMN_NAME_CONSEC_T;
		static const QString COLUMN_NAME_CONSEC_N;

		static const QString COLUMN_NAME_UP_MOM_10;
		static const QString COLUMN_NAME_UP_MOM_10_RANK;
		static const QString COLUMN_NAME_DOWN_MOM_10;
		static const QString COLUMN_NAME_DOWN_MOM_10_RANK;
		static const QString COLUMN_NAME_DISTUD_10;
		static const QString COLUMN_NAME_DISTUD_10_RANK;
		static const QString COLUMN_NAME_DISTDU_10;
		static const QString COLUMN_NAME_DISTDU_10_RANK;
		static const QString COLUMN_NAME_UGD_10; // with threshold
		static const QString COLUMN_NAME_DGU_10; // with threshold
		static const QString COLUMN_NAME_UGD_10_RANK; // with threshold
		static const QString COLUMN_NAME_DGU_10_RANK; // with threshold

		static const QString COLUMN_NAME_UP_MOM_50;
		static const QString COLUMN_NAME_UP_MOM_50_RANK;
		static const QString COLUMN_NAME_DOWN_MOM_50;
		static const QString COLUMN_NAME_DOWN_MOM_50_RANK;
		static const QString COLUMN_NAME_DISTUD_50;
		static const QString COLUMN_NAME_DISTUD_50_RANK;
		static const QString COLUMN_NAME_DISTDU_50;
		static const QString COLUMN_NAME_DISTDU_50_RANK;
		static const QString COLUMN_NAME_UGD_50; // with threshold
		static const QString COLUMN_NAME_DGU_50; // with threshold
		static const QString COLUMN_NAME_UGD_50_RANK; // with threshold
		static const QString COLUMN_NAME_DGU_50_RANK; // with threshold

		// BARDATA
		static const QString SQL_SELECT_BARDATA_DESC;
		static const QString SQL_SELECT_BARDATA_MAX_ROWID;

		// RESISTANCE & SUPPORT
		static const QString SQL_INSERT_RESISTANCE_V1;
		static const QString SQL_INSERT_SUPPORT_V1;

		// CREATE TABLE
		static const QString SQL_CREATE_TABLE_BARDATA_V2;
		static const QString SQL_CREATE_TABLE_RESISTANCE_V1;
		static const QString SQL_CREATE_TABLE_SUPPORT_V1;
		static const QString SQL_CREATE_TABLE_RESISTANCE_DATE_V1;
		static const QString SQL_CREATE_TABLE_SUPPORT_DATE_V1;

		// CREATE INDEX
		static const QString SQL_CREATE_INDEX_BARDATA_PARENT;
		static const QString SQL_CREATE_INDEX_BARDATA_PARENT_PREV;
		static const QString SQL_CREATE_INDEX_BARDATA_PARENT_MONTHLY;
		static const QString SQL_CREATE_INDEX_BARDATA_PARENT_PREV_MONTHLY;
		static const QString SQL_CREATE_INDEX_BARDATA_PREVDATETIME;
		static const QString SQL_CREATE_INDEX_BARDATA_ATR;
		static const QString SQL_CREATE_INDEX_BARDATA_MACD;
		static const QString SQL_CREATE_INDEX_BARDATA_RSI;
		static const QString SQL_CREATE_INDEX_BARDATA_SLOWK;
		static const QString SQL_CREATE_INDEX_BARDATA_SLOWD;
		static const QString SQL_CREATE_INDEX_RESISTANCE_RID;
		static const QString SQL_CREATE_INDEX_SUPPORT_RID;

		// SQL STRING BUILDER
		static QString SQL_SELECT_BARDATA_MAXDATETIME (QString whereArgs);
		static QString SQL_SELECT_BARDATA_MINDATETIME (QString whereArgs);
		static QString SQL_SELECT_BARDATA_LIMIT (const QDate &d, const QTime &t, int limit);
		static QString SQL_SELECT_BARDATA_BUILDER (const QStringList &projection, const QString &where_args);
		static QString SQL_SELECT_BARDATA_BUILDER_JOIN (const QStringList &projection,
																										const QStringList &database_alias,
																										const QString &where_args,
																										const QStringList &SRjoinAlias,
																										const QVector<bool> &SRjoin,
																										const QString &target_alias,
																										bool count_query,
																										const QVector<int> &pricePrevBar,
																										const QVector<IntervalWeight> &pricePrevBarInterval,
																										const IntervalWeight projectionInterval);
		static QStringList SQL_UPDATE_BARDATA_PARENT_INDEX_V2 (const QString &parent, int weight, int start_index);
		static QString SQL_SELECT_DATETIME (const QDate &d, const QTime &t);
		static QString SQL_SELECT_EXISTS (const QString &tableName, const QString &whereArgs);
		static QString SQL_LAST_FROM_ROWID (int start_from_rowid);
		static QString SQL_ROWCOUNT_WHERE (const QString &tableName, const QString &whereArgs);

		SQLiteHandler (int id = 0, QString databasePath = "")
		{
			if (databasePath.isEmpty()) databasePath = application_path;
			application_id = id;
			database_path = databasePath;
		}

		~SQLiteHandler()
		{
			QSqlDatabase *db = nullptr;
			QString connection;

			for (auto it = databasePool.begin(); it != databasePool.constEnd(); ++it)
			{
				db = it.value();
				if (db != NULL)
				{
					connection = db->connectionName();
					db->close();
					delete db;
					db = 0;
					QSqlDatabase::removeDatabase(connection);
				}
			}

			databasePool.clear();
			remove_temp_database();
		}

		void set_application_id (int id)
		{
			application_id = id;
		}

		void set_database_path (const QString &path)
		{
			database_path = path;
		}

		//
		// static clone database
		//
		static QSqlDatabase clone_database (const QSqlDatabase &other)
		{
			static int _id = 1;
			QSqlDatabase database = QSqlDatabase::cloneDatabase(other, "sqlitehandlerclone" + QString::number(++_id));
			database.open();
			return database;
		}

		static bool removeDatabase (QSqlDatabase *database)
		{
			if (database != NULL)
			{
				QString connection = database->connectionName();
				database->close();
				*database = QSqlDatabase();
				QSqlDatabase::removeDatabase(connection);

				if (!QSqlDatabase::contains(connection))
					return true;
			}

			return false;
		}

		static QString getDatabaseName (const QString &symbol, IntervalWeight interval)
		{
			return symbol + "_" + Globals::intervalWeightToString(interval) + ".db";
		}

		QString getDatabaseNamePath (const QString &symbol, IntervalWeight interval)
		{
			return database_path + "/" + symbol + "_" + Globals::intervalWeightToString(interval) + ".db";
		}

		QSqlDatabase getTempResultDatabase (IntervalWeight interval, int id) const
		{
			QString local_database_path = database_path + "/" + getTempResultDatabaseName(interval, id);
			QString connectionString = create_tempdb_connstring(interval, id);

//      if (tempDatabasePool.contains(connectionString)) {
//        tempDatabasePool[connectionString]->close();
//        delete tempDatabasePool[connectionString];
//        tempDatabasePool.remove(connectionString);
//        QSqlDatabase::removeDatabase(connectionString);
//      }

//      QSqlDatabase *db = new QSqlDatabase();
//      *db = QSqlDatabase::addDatabase("QSQLITE", connectionString);
//      db->setDatabaseName(database_path);
//      db->open();

//      tempDatabasePool.insert(connectionString, db);
//      return *db;

			if (QSqlDatabase::contains(connectionString))
			{
				QSqlDatabase::removeDatabase(connectionString);
			}

			if (!tempdatabase_connection.contains(connectionString))
			{
				tempdatabase_connection.push_back( connectionString );
			}

			QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionString);
			db.setDatabaseName(local_database_path);
			db.open();
			return db;
		}

		static QString getTempResultDatabaseName (IntervalWeight interval, int id)
		{
//      QString appid = QString::number(application_id) + QString::number(id);
//      return tempDatabaseName + appid + "." + Globals::intervalWeightToString(interval);
			return tempDatabaseName + QString::number(id) + "." + Globals::intervalWeightToString(interval);
		}

		void remove_temp_database()
		{
			int N = tempdatabase_connection.size();

			for (int i = 0; i < N; ++i)
			{
				QSqlDatabase::removeDatabase(tempdatabase_connection[i]);
				QFile::remove(database_path + "/" + tempdatabase_connection[i].split("#")[1]);
			}

			tempdatabase_connection.clear();
		}

		//
		// internal clone database
		//
		QSqlDatabase getCloneDatabase (const QString &symbol, IntervalWeight interval)
		{
			return getCloneDatabase(symbol + "_" + Globals::intervalWeightToString(interval) + ".db");
		}

		QSqlDatabase getCloneDatabase (const QString &databaseName)
		{
			QSqlDatabase other = getDatabase(databaseName);
			QString appid = QString::number(application_id) + QString::number(++connection_id);
			QString connection = other.connectionName() + appid + QTime::currentTime().toString("zzz");
			QSqlDatabase database = QSqlDatabase::cloneDatabase(other,connection);
			database.open();
			return database;
		}

		//
		// GetDatabase
		//
		const QSqlDatabase& getDatabase (const QString &symbol, IntervalWeight interval)
		{
			return getDatabase(symbol + "_" + Globals::intervalWeightToString(interval) + ".db");
		}

		const QSqlDatabase& getDatabase (const QString &symbol, const QString &interval)
		{
			return getDatabase(symbol + "_" + interval + ".db");
		}

		const QSqlDatabase& getDatabase (const QString &databaseName)
		{
			if (!databasePool.contains(databaseName))
			{
				QString connection = QCoreApplication::applicationName() + "." + "sqlitehandler." + databaseName + QString::number(application_id);
				QString database_name = database_path + "/" + databaseName;
				QSqlDatabase *db = new QSqlDatabase();
				*db = QSqlDatabase::addDatabase("QSQLITE", connection);
				db->setDatabaseName(database_name);
				db->open();

				QSqlQuery q(*db);
//        QString page_size;
//        if (databaseName.contains("Monthly")) page_size = "4096";
//        else if (databaseName.contains("Weekly") || databaseName.contains("Daily")) page_size = "16384";
//        else page_size = "65536";

				q.exec("PRAGMA page_size = 65536;");
				q.exec("PRAGMA cache_size = 50000;");
				q.exec("PRAGMA journal_mode = OFF;");
				q.exec("PRAGMA synchronous = OFF;");
				q.exec(SQL_CREATE_TABLE_BARDATA_V2);
				q.exec(SQL_CREATE_TABLE_RESISTANCE_V1);
				q.exec(SQL_CREATE_TABLE_SUPPORT_V1);
				q.exec(SQL_CREATE_TABLE_RESISTANCE_DATE_V1);
				q.exec(SQL_CREATE_TABLE_SUPPORT_DATE_V1);
				q.exec(SQL_CREATE_INDEX_BARDATA_PARENT);
				q.exec(SQL_CREATE_INDEX_BARDATA_PARENT_PREV);
				q.exec(SQL_CREATE_INDEX_BARDATA_PARENT_MONTHLY);
				q.exec(SQL_CREATE_INDEX_BARDATA_PREVDATETIME);

				// store opened database instance
				databasePool.insert(databaseName, db);
				return *db;
			}

			// Always try reopen database
			QSqlDatabase *db = databasePool.value(databaseName);
			if (!db->isOpen()) db->open();
			return *db;
		}

		// @deprecated
		void removeDatabase (const QString &databaseName)
		{
			if (databasePool.contains(databaseName))
			{
				QSqlDatabase *db = databasePool.take(databaseName);
				if (db != NULL)
				{
					QSqlDatabase *temp = db;
					QString connection = db->connectionName();
					db->close();
					*db = QSqlDatabase();
					delete temp;
					QSqlDatabase::removeDatabase(connection);
				}
			}
		}

		static void getDatabase_v2 (const QString &database_name, QSqlDatabase *out)
		{
			QMutexLocker locker(&_mutex);

			QString connstring = "getDatabasev2." + QCoreApplication::applicationName() + "." + QString::number(++connection_id);
			if (out->isOpen())
			{
				removeDatabase(out);
			}

			*out = QSqlDatabase::addDatabase("QSQLITE", connstring);
			out->setDatabaseName(database_name);
			out->open();

			QSqlQuery q(*out);
			q.exec("PRAGMA temp_store = FILE;");
			q.exec("PRAGMA page_size = 65536;");
			q.exec("PRAGMA journal_mode = OFF;");
			q.exec("PRAGMA synchronous = OFF;");
			q.exec("PRAGMA cache_size = 50000;");
			q.exec(SQL_CREATE_TABLE_BARDATA_V2);
			q.exec(SQL_CREATE_TABLE_RESISTANCE_V1);
			q.exec(SQL_CREATE_TABLE_SUPPORT_V1);
			q.exec(SQL_CREATE_TABLE_RESISTANCE_DATE_V1);
			q.exec(SQL_CREATE_TABLE_SUPPORT_DATE_V1);
		}

		static void getDatabase_v3 (const QString &database_name, QSqlDatabase *out, int busy_timeout = 2000, QString journal_mode = "OFF")
		{
			QString connstring = "getDatabasev3." + QCoreApplication::applicationName() + "." + QString::number(++connection_id);
			if (out->isOpen()) removeDatabase(out);

			*out = QSqlDatabase::addDatabase("QSQLITE", connstring);
			out->setDatabaseName(database_name);
			out->open();

			QSqlQuery q(*out);
			q.exec("PRAGMA busy_timeout = " + QString::number(busy_timeout) +";");
			q.exec("PRAGMA journal_mode =" + journal_mode + ";");
			q.exec("PRAGMA synchronous = OFF;");
			q.exec("PRAGMA cache_size = 50000;");
			q.exec("PRAGMA page_size = 65536;");
			q.exec(SQL_CREATE_TABLE_BARDATA_V2);
			q.exec(SQL_CREATE_TABLE_RESISTANCE_V1);
			q.exec(SQL_CREATE_TABLE_SUPPORT_V1);
			q.exec(SQL_CREATE_TABLE_RESISTANCE_DATE_V1);
			q.exec(SQL_CREATE_TABLE_SUPPORT_DATE_V1);
		}

		static void getDatabase_readonly (const QString &database_name, QSqlDatabase *out, int busy_timeout = 2000)
		{
			QString connstring = "getDatabasereadonly." + QCoreApplication::applicationName() + "." + QString::number(++connection_id);
			if (out->isOpen()) removeDatabase(out);

			*out = QSqlDatabase::addDatabase("QSQLITE", connstring);
			out->setDatabaseName(database_name);
			out->open();

			QSqlQuery q(*out);
			q.exec("PRAGMA busy_timeout = " + QString::number(busy_timeout) +";");
			q.exec("PRAGMA query_only = true;");
			q.exec("PRAGMA read_uncommitted = true;");
			q.exec("PRAGMA journal_mode = OFF;");
			q.exec("PRAGMA synchronous = OFF;");
			q.exec("PRAGMA cache_size = 50000;");
			q.exec("PRAGMA page_size = 65536;");
			q.exec(SQL_CREATE_TABLE_BARDATA_V2);
			q.exec(SQL_CREATE_TABLE_RESISTANCE_V1);
			q.exec(SQL_CREATE_TABLE_SUPPORT_V1);
			q.exec(SQL_CREATE_TABLE_RESISTANCE_DATE_V1);
			q.exec(SQL_CREATE_TABLE_SUPPORT_DATE_V1);
		}

		static void reopen_database (QSqlDatabase *database)
		{
			// useful for release resource but maintain the database connection

			QString database_name = database->databaseName();
			SQLiteHandler::removeDatabase(database);
			getDatabase_v2(database_name, database);

			QSqlQuery q(*database);
			q.exec("PRAGMA temp_store = FILE;");
			q.exec("PRAGMA cache_size = 50000;");
			q.exec("PRAGMA journal_mode = OFF;");
			q.exec("PRAGMA synchronous = OFF;");
		}

		QStringList getColumnNames (const QString &databaseName, const QString &tableName)
		{
			QSqlRecord rec = getDatabase(databaseName).record(tableName);
			QStringList result;

			for (int i = 0; i < rec.count(); ++i)
			{
				result.push_back(rec.fieldName(i));
			}

			return result;
		}


		//
		// Temporary Result helper
		//
		QSqlQuery* getSqlQuery (const QSqlDatabase &main_database, const QString &symbol,
														const IntervalWeight base_interval, const IntervalWeight projection_interval,
														int id_threshold,
														// TODO: merge these into one, attacth_stmt
														const QStringList &databaseName,
														const QStringList &databaseAlias,
														const QString &sql_select,
														const QStringList &projectionNames,
														int /*prevbar_count*/,
														int result_id,
														QString *errorString = nullptr,
														QString *temp_database_path = nullptr,
														bool is_append = false)
		{
			QSqlQuery *query = NULL;
			QSqlQuery query_1(main_database);
			query_1.setForwardOnly(true);
			query_1.exec("PRAGMA synchronous = OFF;");
			query_1.exec("PRAGMA cache_size = 50000;");
			query_1.exec("PRAGMA temp_store = MEMORY;");

			int resistance_column_count = 0;
			int support_column_count = 0;

			for (int i = 1; i < databaseName.length(); ++i)
			{
				query_1.exec("attach database '" + databaseName[i] + "' AS " + databaseAlias[i]);

				// check database locked
				if (query_1.lastError().isValid())
				{
					if (errorString != NULL)
					{
						*errorString += databaseName[i] + ":" + query_1.lastError().text() + "\n";
						return NULL;
					}
				}
			}

			if (projectionNames.length() > 0)
			{
				QMap<QString,int> check_duplicate_projection;

				int column_count = projectionNames.length();
				QStringList intersectProjection;

				QString tempDatabasePath = database_path + "/" + SQLiteHandler::getTempResultDatabaseName(projection_interval, result_id);
				QString tempDatabaseAlias = "tempResult" + Globals::intervalWeightToString(projection_interval);
				QString tempTableName = SQLiteHandler::TABLE_NAME_BARDATA_VIEW;

				QString sql_attach_temp = "ATTACH DATABASE '" + tempDatabasePath + "' AS " + tempDatabaseAlias + ";";
				QString sql_create_table = "CREATE TABLE IF NOT EXISTS " + tempDatabaseAlias + "." + tempTableName + "(";
				QString column_names = "";

				if (temp_database_path != NULL)
				{
					*temp_database_path = tempDatabasePath;
				}

				for (int i = 0; i < column_count; ++i)
				{
					// for insert - select
					if (i > 0) column_names += ",";

					column_names += "'" + projectionNames[i] + "'";

					// for create table
					if (!check_duplicate_projection.contains(projectionNames[i]))
					{
						check_duplicate_projection.insert(projectionNames[i], 0);

						if (i > 0) sql_create_table += ",";

						sql_create_table += "'" + projectionNames[i] + "' TEXT";

						if (projectionNames[i].startsWith("Min_") || projectionNames[i].startsWith("Max_"))
							intersectProjection.push_back(projectionNames[i]);
					}
				}

				sql_create_table += ",PRIMARY KEY(date_ ASC, time_ ASC));";

				query_1.exec(sql_attach_temp);
				query_1.exec("PRAGMA page_size = 65536;");
				query_1.exec("PRAGMA cache_size = -16384;");

				if (!is_append)
				{
					query_1.exec("DROP TABLE IF EXISTS " + tempDatabaseAlias + "." + tempTableName + ";");
					query_1.exec(sql_create_table);

					if (query_1.lastError().isValid())
					{
						if (errorString != NULL) *errorString += "create temp table:" + query_1.lastError().text() + "\n";
						return NULL;
					}
				}
				else
				{
					// query table schema for determine max number of Support/Resistance
					query_1.exec("select * from " + SQLiteHandler::TABLE_NAME_BARDATA_VIEW + " limit 1");

					QSqlRecord rec = query_1.record();
					QString fname;
					int _index = rec.indexOf("delete_flag") + 1;

					while (_index < rec.count())
					{
						fname = rec.fieldName(_index);
						if (fname.contains("Resistance")) ++resistance_column_count; else
						if (fname.contains("Support")) ++support_column_count;
						++_index;
					}
				}

				query_1.exec("PRAGMA synchronous = OFF;");
				query_1.exec("BEGIN TRANSACTION;");
				query_1.exec("INSERT OR IGNORE INTO " + tempDatabaseAlias + "." + tempTableName + "(" + column_names + ") " + sql_select);

				if (query_1.lastError().isValid())
				{
					if (errorString != NULL)
					{
						*errorString += "insert temp table:" + query_1.lastError().text() + "\n";
					}

					return NULL;
				}

				query_1.exec("commit;");
				query_1.exec("alter table " + tempDatabaseAlias + "." + tempTableName + " add column delete_flag INTEGER default 0;");

				// @deprecated: Bypass intersect deletion if zero
				/*if (prevbar_count > 0)
				{
					if (projection_interval == base_interval)
					{
						support_resistance_check_previous_bar_v2(&query_1, intersectProjection, base_interval, prevbar_count);
					}
					else
					{
						QString baseDatabasePath = database_path + "/" + SQLiteHandler::getTempResultDatabaseName(base_interval, result_id);
						QString baseDatabaseAlias = "baseResult" + Globals::intervalWeightToString(base_interval);
						query_1.exec("ATTACH DATABASE '" + baseDatabasePath + "' AS " + baseDatabaseAlias + ";");
						query_1.exec("PRAGMA synchronous = OFF;");
						query_1.exec("PRAGMA cache_size = 50000;");
						query_1.exec("BEGIN TRANSACTION;");
						query_1.exec("update " + tempDatabaseAlias + "." + tempTableName +
												 " set delete_flag=1 where rowid in(select rowid from "+ baseDatabaseAlias + ".bardataview where delete_flag=1)");
						query_1.exec("COMMIT;");
						query_1.exec("DETACH DATABASE " + baseDatabaseAlias);
					}
				}*/

				QStringList reprojection = projectionNames;

				reprojection[1] = "strftime('%m/%d/%Y',date_) as Date";
				reprojection[2] = "time_ as Time";

				for (int i = 3; i < column_count; ++i)
				{
					reprojection[i] = "a.'" + reprojection[i] + "'";
				}

				// extend support/resistance detail
				if (projection_interval >= WEIGHT_DAILY)
				{
					extend_view_support_resistance(&query_1, &reprojection, id_threshold, base_interval,
																				 projection_interval, projection_interval,
																				 resistance_column_count, support_column_count, errorString);
				}

				// BardataView (remove unnecessary columns)
				reprojection.removeAt(reprojection.indexOf("rowid"));
				reprojection.removeAt(reprojection.indexOf("a.'open_'"));
				reprojection.removeAt(reprojection.indexOf("a.'high_'"));
				reprojection.removeAt(reprojection.indexOf("a.'low_'"));
				reprojection.removeAt(reprojection.indexOf("a.'close_'"));
				reprojection.removeAt(reprojection.indexOf("a.'_parent'"));
				reprojection.removeAt(reprojection.indexOf("a.'_parent_monthly'"));
				reprojection.removeAt(reprojection.indexOf("a.'_parent_prev'"));
				reprojection.removeAt(reprojection.indexOf("a.'_parent_prev_monthly'"));
				reprojection.removeAt(reprojection.indexOf("a.'prevdate'"));
				reprojection.removeAt(reprojection.indexOf("a.'prevtime'"));
				reprojection.removeAt(reprojection.indexOf("a.'Rowid60min'"));
				reprojection.removeAt(reprojection.indexOf("a.'RowidDaily'"));
				reprojection.removeAt(reprojection.indexOf("a.'RowidWeekly'"));
				reprojection.removeAt(reprojection.indexOf("a.'RowidMonthly'"));

				for (int i = 0; i < intersectProjection.length(); ++i)
				{
					reprojection.removeAt(reprojection.indexOf("a.'" + intersectProjection[i] + "'"));
				}

				for (int i = 1; i < databaseName.length(); ++i)
				{
					query_1.exec("detach database " + databaseAlias[i]);
				}

				query_1.exec("detach database " + tempDatabaseAlias);
				query_1.clear();

				QSqlDatabase tempdb = getTempResultDatabase(projection_interval, result_id);
				query = new QSqlQuery(tempdb);

				// extend look ahead bar
				QVector<int> lookahead;
				lookahead.push_back(1);
				lookahead.push_back(3);
				lookahead.push_back(5);
				lookahead.push_back(10);
				lookahead.push_back(20);
				lookahead.push_back(50);
				extend_view_lookahead_bars(tempdb, lookahead, databaseName[0], getDatabaseNamePath (symbol, WEIGHT_DAILY) /*, base_interval*/);

				query->setForwardOnly(false);
				query->exec("select " + reprojection.join(",") + " from " + tempTableName + " a "
										" where delete_flag=0 order by a.date_ asc, a.time_ asc");
			}
			// for export result to Tradestation
			else
			{
				query = new QSqlQuery(main_database);
				query->setForwardOnly(true);
				query->exec(sql_select);
			}

			// catch query error
			if (query != NULL && query->lastError().isValid())
			{
				if (errorString != NULL)
				{
					*errorString += query->lastError().text() + "\n";
				}

				query->clear();
				delete query;
				return NULL;
			}

			return query;
		}

		void extend_view_support_resistance (QSqlQuery *query,
																				 QStringList *out_projection,
																				 int id_threshold,
																				 const IntervalWeight base_weight,
																				 const IntervalWeight parent_weight,
																				 const IntervalWeight projection_weight,
																				 int resistance_column_count = 0,
																				 int support_column_count = 0,
																				 QString *errorString = NULL) const
		{
			QStringList res_date, res_time, res_count, res_duration, res_value;
			QStringList sup_date, sup_time, sup_count, sup_duration, sup_value;
			QStringList cc;
			QStringList con;
			QStringList dur;
			const QString resistance_table = SQLiteHandler::TABLE_NAME_RESISTANCE;
			const QString support_table = SQLiteHandler::TABLE_NAME_SUPPORT;
			const QString temp_table = SQLiteHandler::TABLE_NAME_BARDATA_VIEW;
			const QString tempDatabaseAlias = "tempResult" + Globals::intervalWeightToString(projection_weight);
			const QString interval = Globals::intervalWeightToString(parent_weight);
			QString sql_alter_table;
			QString sql_select_detail;
			QString sql_update;
			QString set_columns;
			QString column_name;
			QString child_alias = "a";
			QString parent_database = "";
			QString left_join_clause = "";
			QString group_by_clause = "";
			int max_column;

			if (base_weight < parent_weight)
			{
				IntervalWeight w = base_weight;
				QString parent_bardata;
				QString parent_rowid;
				QString child_idparent;

				while (w != WEIGHT_INVALID && w != parent_weight)
				{
					w = Globals::getParentInterval(w);
					parent_database = "db" + Globals::intervalWeightToString(w);
					parent_bardata = parent_database + "." + TABLE_NAME_BARDATA;
					parent_rowid = parent_bardata + "." + COLUMN_NAME_ROWID;

					if (w == WEIGHT_MONTHLY) {
						child_idparent = "a." + COLUMN_NAME_IDPARENT_PREV_MONTHLY;
					} else {
						child_idparent = child_alias + "." + COLUMN_NAME_IDPARENT_PREV;
					}

					left_join_clause += " left join " + parent_bardata + " on " + parent_rowid + "=" + child_idparent;
					if (!group_by_clause.isEmpty()) group_by_clause += ",";
					group_by_clause += child_alias + ".date_," + child_alias + ".time_";
					child_alias = parent_bardata;
				}

				if (!parent_database.isEmpty())
					parent_database += ".";
			}
			else if (projection_weight > base_weight)
			{
				parent_database = "db" + Globals::intervalWeightToString(projection_weight) + ".";
			}

			if (!group_by_clause.isEmpty()) group_by_clause += ",";
			group_by_clause += child_alias + ".date_," + child_alias + ".time_";

			query->exec("PRAGMA synchronous = OFF;");
			query->exec("PRAGMA cache_size = 50000;");
			query->exec("BEGIN TRANSACTION;");

			// Alter column Resistance
			query->exec ("select max(cast(" + temp_table + ".'Resistance (#)' as int)) from " + tempDatabaseAlias + "." + temp_table);

			if (query->lastError().isValid())
			{
				if (errorString != NULL)
				{
					*errorString += "select max column (res): " + query->lastError().text() + "\n";
				}
			}

			max_column = query->next()? query->value(0).toInt() : 0;

			for (int i = 0; i < max_column; ++i)
			{
				column_name = interval + "Resistance" + QString::number(i);
				out_projection->push_back(column_name + " as '" + interval + " Resistance'");

				if (i >= resistance_column_count)
				{
					sql_alter_table = "alter table " + tempDatabaseAlias + "." + temp_table + " add column '" + column_name + "' TEXT;";
					query->exec(sql_alter_table);
				}
			}

			// Alter column Support
			query->exec ("select max(cast(" + temp_table + ".'Support (#)' as int)) from " + tempDatabaseAlias + "." + temp_table);

			if (query->lastError().isValid())
			{
				if (errorString != NULL)
				{
					*errorString += "select max count (sup):" + query->lastError().text() + "\n";
				}
			}

			max_column = query->next()? query->value(0).toInt() : 0;

			for (int i = 0; i < max_column; ++i)
			{
				column_name = interval + "Support" + QString::number(i);
				out_projection->push_back(column_name + " as '" + interval + " Support'");

				if (i >= support_column_count)
				{
					sql_alter_table = "alter table " + tempDatabaseAlias + "." + temp_table + " add column '" + column_name + "' TEXT;";
					query->exec(sql_alter_table);
				}
			}

			query->exec("COMMIT;");

			//
			// Insert resistance detail
			//
			if (parent_database != "")
			{
				sql_select_detail =
						"select date_,time_,group_concat(resistance),group_concat(resistance_count),group_concat(resistance_duration) from"
						"(select a.date_,a.time_,b.resistance,b.resistance_count,b.resistance_duration"
						" from " + tempDatabaseAlias + "." + temp_table + " a " + left_join_clause +
						" join " + parent_database + resistance_table + " b on b.date_=" + child_alias + ".date_ and b.time_=" + child_alias + ".time_" +
						" where delete_flag=0 and id_threshold=" + QString::number(id_threshold) + " and" +
						" b.resistance >= CAST(a.low_ as Real) and b.resistance <= CAST(a.high_ as Real)" +
						" order by b.resistance desc)"
						" group by date_,time_";
			}
			else
			{
				sql_select_detail =
						"select date_,time_,group_concat(resistance),group_concat(resistance_count),group_concat(resistance_duration) from"
						"(select a.date_,a.time_,b.resistance,b.resistance_count,b.resistance_duration"
						" from " + tempDatabaseAlias + "." + temp_table + " a " + left_join_clause +
						" join " + parent_database + resistance_table + " b on b.date_=" + child_alias + ".PrevDate and b.time_=" + child_alias + ".PrevTime" +
						" where delete_flag=0 and id_threshold=" + QString::number(id_threshold) + " and" +
						" b.resistance >= CAST(a.low_ as Real) and b.resistance <= CAST(a.high_ as Real)" +
						" order by b.resistance desc)"
						" group by date_,time_";
			}

			query->exec (sql_select_detail);

			if (query->lastError().isValid())
			{
				if (errorString != NULL)
				{
					*errorString += query->lastError().text() + "\n";
				}
			}

			while (query->next())
			{
				res_date.push_back(query->value(0).toString());
				res_time.push_back(query->value(1).toString());
				res_value.push_back(query->value(2).toString());
				res_count.push_back(query->value(3).toString());
				res_duration.push_back(query->value(4).toString());
			}

			//
			// Insert support detail
			//
			if (parent_database != "")
			{
				sql_select_detail =
					"select date_,time_,group_concat(support),group_concat(support_count),group_concat(support_duration) from"
					"(select a.rowid, a.date_,a.time_,b.support,b.support_count,b.support_duration"
					" from " + tempDatabaseAlias + "." + temp_table + " a " + left_join_clause +
					" join " + parent_database + support_table + " b on b.date_=" + child_alias + ".date_ and b.time_=" + child_alias + ".time_" +
					" where delete_flag=0 and id_threshold=" + QString::number(id_threshold) + " and " +
					 "b.support >= CAST(a.low_ as Real) and b.support <= CAST(a.high_ as Real)" +
					" order by b.support desc)"
					" group by date_,time_";
			}
			else
			{
				sql_select_detail =
					"select date_,time_,group_concat(support),group_concat(support_count),group_concat(support_duration) from"
					"(select a.rowid, a.date_,a.time_,b.support,b.support_count,b.support_duration"
					" from " + tempDatabaseAlias + "." + temp_table + " a " + left_join_clause +
					" join " + parent_database + support_table + " b on b.date_=" + child_alias + ".PrevDate and b.time_=" + child_alias + ".PrevTime" +
					" where delete_flag=0 and id_threshold=" + QString::number(id_threshold) + " and " +
					 "b.support >= CAST(a.low_ as Real) and b.support <= CAST(a.high_ as Real) " +
					" order by b.support desc)"
					" group by date_,time_";
			}

			query->exec(sql_select_detail);

			if (query->lastError().isValid())
			{
				if (errorString != NULL)
				{
					*errorString += query->lastError().text() + "\n";
				}
			}

			while (query->next())
			{
				sup_date.push_back(query->value(0).toString());
				sup_time.push_back(query->value(1).toString());
				sup_value.push_back(query->value(2).toString());
				sup_count.push_back(query->value(3).toString());
				sup_duration.push_back(query->value(4).toString());
			}

			//
			// Update temp table
			//
			query->exec("PRAGMA synchronous = OFF;");
			query->exec("BEGIN TRANSACTION;");

			int N = res_date.length();

			for (int i = 0; i < N; ++i)
			{
				cc = res_value[i].split(",");
				con = res_count[i].split(",");
				dur = res_duration[i].split(",");
				set_columns = "";
				max_column = cc.length();

				for (int j = 0; j < max_column; ++j)
				{
					if (j > 0) set_columns += ",";
					set_columns += interval + "Resistance" + QString::number(j) + "='" + cc[j] + " (" + con[j] + "," + dur[j] + ")'";
				}

				sql_update =
					"update " + tempDatabaseAlias + "." + temp_table + " set "+ set_columns +
					" where date_='" + res_date[i] + "' and time_='" + res_time[i] + "'";

				query->exec(sql_update);
			}

			N = sup_date.length();

			for (int i = 0; i < N; ++i)
			{
				cc = sup_value[i].split(",");
				con = sup_count[i].split(",");
				dur = sup_duration[i].split(",");
				set_columns = "";
				max_column = cc.length();

				for (int j = 0; j < max_column; ++j)
				{
					if (j > 0) set_columns += ",";
					set_columns += interval + "Support" + QString::number(j) + "='" + cc[j] +" (" + con[j]+"," + dur[j] + ")'";
				}

				sql_update =
					"update " + tempDatabaseAlias + "." + temp_table + " set "+ set_columns +
					" where date_='" + sup_date[i] + "' and time_='" + sup_time[i] + "'";

				query->exec(sql_update);
			}

			query->exec("COMMIT;");
		}

		void extend_view_lookahead_bars (const QSqlDatabase &database, const QVector<int> &lookahead_thres, const QString &base_databasename,
																		 const QString &daily_databasename /*, const IntervalWeight &base_interval*/)
		{
			const QString dbbase_alias = "dbbase";
//      const QString prev_atr = base_interval < WEIGHT_DAILY ? SQLiteHandler::COLUMN_NAME_PREV_DAILY_ATR : SQLiteHandler::COLUMN_NAME_ATR;
			const int lookahead_size = lookahead_thres.size();
			std::vector <QSqlQuery> vec_query (lookahead_size, QSqlQuery (database));
			std::vector <int> pa_lookahead (lookahead_size, 0);
			std::vector <int> pb_lookahead (lookahead_size, 0);
			long int row_count = 0;
			int _idx;

			QSqlQuery q_update (database);
			QStringList column_name;
			QString s_number;
			QString last_daily_atr = "";

			QString create_table =
				"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_BARDATA_LOOKAHEAD +
				"(Date TEXT, Time TEXT, Open REAL, High REAL, Low REAL, Close REAL";

			column_name.push_back ("Date");
			column_name.push_back ("Time");
			column_name.push_back ("Open");
			column_name.push_back ("High");
			column_name.push_back ("Low");
			column_name.push_back ("Close");

			q_update.exec ("attach database '" + daily_databasename + "' as dbdaily");
			q_update.exec ("select atr from bardata where completed = 1 order by rowid desc limit 1");
			last_daily_atr = q_update.next() ? q_update.value(0).toString() : "";
			q_update.exec ("detach database dbdaily");

			for (int i = 0; i < lookahead_size; ++i)
			{
				s_number = QString::number (lookahead_thres[i]);
				create_table += ",'Open (" + s_number + ")' REAL,";
				create_table += "'High (" + s_number + ")' REAL,";
				create_table += "'Low (" + s_number + ")' REAL,";
				create_table += "'Close (" + s_number + ")' REAL,";
				create_table += "'C-Higher (" + s_number + ")' INT,";
				create_table += "'C-Lower (" + s_number + ")' INT,";
				create_table += "'C-Point (" + s_number + ")' REAL,"; // NEW
				create_table += "'C-ATR (" + s_number + ")' REAL"; // NEW

				column_name.push_back ("'Open (" + s_number + ")'");
				column_name.push_back ("'High (" + s_number + ")'");
				column_name.push_back ("'Low (" + s_number + ")'");
				column_name.push_back ("'Close (" + s_number + ")'");
				column_name.push_back ("'C-Higher (" + s_number + ")'");
				column_name.push_back ("'C-Lower (" + s_number + ")'");
				column_name.push_back ("'C-Point (" + s_number + ")'"); // NEW
				column_name.push_back ("'C-ATR (" + s_number + ")'"); // NEW

				vec_query[i].setForwardOnly (true);
				vec_query[i].exec ("PRAGMA cache_size = -16384;");
				vec_query[i].exec ("ATTACH DATABASE '" + database_path + "/" + base_databasename + "' as " + dbbase_alias);
				vec_query[i].exec ("select strftime('%m/%d/%Y',a.date_), a.time_,"
													 "a.open_, a.high_, a.low_, a.close_,"
													 "b.open_, b.high_, b.low_, b.close_,"
													 "case when a.close_<b.close_ then 1 else 0 end,"
													 "case when a.close_>b.close_ then 1 else 0 end,"
													 "(b.close_-a.close_),"
													 "round((b.close_-a.close_)/" + last_daily_atr + ",4)"
													 " from " + SQLiteHandler::TABLE_NAME_BARDATA_VIEW + " a"
													 " left join " + dbbase_alias +".bardata b on b.rowid=a.rowid+" + QString::number (lookahead_thres[i]));
			}

			create_table += ");";
			QString s_ = ",?";

			q_update.exec ("PRAGMA journal_mode = OFF;");
			q_update.exec ("PRAGMA synchronous = OFF;");
			q_update.exec ("PRAGMA cache_size = 50000;");
			q_update.exec ("PRAGMA page_size = 65536;");
			q_update.exec ("PRAGMA temp_store = MEMORY;");
			q_update.exec (create_table);
			q_update.exec ("BEGIN TRANSACTION;");
			q_update.exec ("DELETE FROM " + SQLiteHandler::TABLE_NAME_BARDATA_LOOKAHEAD);
			q_update.prepare ("INSERT INTO " + SQLiteHandler::TABLE_NAME_BARDATA_LOOKAHEAD +
												" VALUES(?" + s_.repeated(column_name.size() - 1) + ")");

			while (vec_query[0].next())
			{
				for (int i = 0; i <= 13; ++i)
				{
					q_update.bindValue (i, vec_query[0].value(i));
				}

				if (vec_query[0].value(10).toInt() == 1) ++pa_lookahead[0];
				if (vec_query[0].value(11).toInt() == 1) ++pb_lookahead[0];
				_idx = 13;

				for (int i = 1; i < lookahead_size; ++i)
				{
					vec_query[i].next();
					q_update.bindValue (++_idx, vec_query[i].value(6)); // Open
					q_update.bindValue (++_idx, vec_query[i].value(7)); // High
					q_update.bindValue (++_idx, vec_query[i].value(8)); // Low
					q_update.bindValue (++_idx, vec_query[i].value(9)); // Close
					q_update.bindValue (++_idx, vec_query[i].value(10)); // C-Higher
					q_update.bindValue (++_idx, vec_query[i].value(11)); // C-Lower
					q_update.bindValue (++_idx, vec_query[i].value(12)); // C-Point
					q_update.bindValue (++_idx, vec_query[i].value(13)); // C-ATR
					if (vec_query[i].value(10).toInt() == 1) ++pa_lookahead[i];
					if (vec_query[i].value(11).toInt() == 1) ++pb_lookahead[i];
				}

				q_update.exec();
				++row_count;
			}

			// extended row: number of occurence
			q_update.bindValue (0, "Occur");

			for (int i = 1; i <= 5; ++i)
				q_update.bindValue (i, QVariant());

			_idx = 5;

			for (int i = 0; i < lookahead_size; ++i)
			{
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, pa_lookahead[i]);
				q_update.bindValue (++_idx, pb_lookahead[i]);
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
			}

			q_update.exec ();

			// extended row: probability
			q_update.bindValue (0, "Prob");

			for (int i = 1; i <= 5; ++i)
				q_update.bindValue (i, QVariant());

			_idx = 5;

			for (int i = 0; i < lookahead_size; ++i)
			{
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QString::number (pa_lookahead[i] / static_cast<double> (row_count),'f',4));
				q_update.bindValue (++_idx, QString::number (pb_lookahead[i] / static_cast<double> (row_count),'f',4));
				q_update.bindValue (++_idx, QVariant());
				q_update.bindValue (++_idx, QVariant());
			}

			q_update.exec ();
			q_update.exec ("COMMIT;");

			for (int i = 0; i < lookahead_size; ++i)
				vec_query[i].exec ("DETACH DATABASE " + dbbase_alias +";");
		}

		// with realtime deletion (@deprecated -- rarely use)
		void support_resistance_check_previous_bar_v2 (QSqlQuery *query, const QStringList &intersect_projection,
																									 const IntervalWeight base_interval, int prevbar_count) const
		{
			QString sql_update;
			QString sql_select;
			QString base_interval_name = Globals::intervalWeightToString(base_interval);
			QString database_alias;
			QString interval_name;
			QString column_name;
			QString rowid_column;
			QString having_clause;
			QStringList split_str;
			int N = intersect_projection.length();

			for (int i = 0; i < N; ++i) {
				split_str = intersect_projection[i].split("_");
				interval_name = split_str[1];
				column_name = split_str[2];

				if (base_interval_name != interval_name)
				{
					database_alias = "db" + interval_name + ".";
					rowid_column = "Rowid" + interval_name;
				}
				else
				{
					database_alias = "";
					rowid_column = "Rowid";
				}

				if (column_name == "Resistance")
				{
					having_clause = "HAVING A."+intersect_projection[i]+"<=MAX(T."+intersect_projection[i]+")";
				}
				else
				{
					having_clause = "HAVING A."+intersect_projection[i]+">=MIN(T."+intersect_projection[i]+")";
				}

				// Candidate of deletion
				sql_select =
					"SELECT A.rowid FROM bardataview A"
					" JOIN bardataview T ON T.rowid>=(A.rowid-" + QString::number(prevbar_count) + ") AND T.rowid<A.rowid"
					" GROUP BY A.rowid " + having_clause;

				query->exec(sql_select);

				if (query->lastError().isValid()) query->lastError();

				QVariantList rowid;
				while ( query->next() )
				{
					rowid.push_back(query->value(0).toString());
				}

//        qDebug() << "#### rowid count:" << rowid.size();

				query->exec("PRAGMA journal_mode = OFF;");
				query->exec("PRAGMA synchronous = OFF;");
				query->exec("PRAGMA cache_size = 50000;");
				query->exec("PRAGMA page_size = 65536;");
				query->exec("BEGIN TRANSACTION;");

//        sql_update =
//          "UPDATE bardataview SET delete_flag=1 WHERE rowid IN("
//          "SELECT A.rowid FROM bardataview A"
//          " JOIN bardataview T ON T.rowid>=(A.rowid-" + QString::number(PrevBarCount) + ") AND T.rowid<A.rowid"
//          " WHERE A.rowid=? AND T.delete_flag=0"
//          " GROUP BY A.rowid " + having_clause + ")";

//        query->prepare(sql_update);
//        query->addBindValue(rowid);
//        query->execBatch();

				for (int j = 0; j < rowid.size(); ++j) {
					sql_update =
						"UPDATE bardataview SET delete_flag=1 WHERE rowid IN("
						"SELECT A.rowid FROM bardataview A"
						" JOIN bardataview T ON T.rowid>=(A.rowid-" + QString::number(prevbar_count) + ") AND T.rowid<A.rowid"
						" WHERE A.rowid=" + rowid[j].toString() + " AND T.delete_flag=0"
						" GROUP BY A.rowid " + having_clause + ")";
//          qDebug() << sql_update;
					query->exec(sql_update);
//          if (query->lastError().isValid()) qDebug() << query->lastError();
				}

				query->exec("COMMIT;");
			}
		}

		// w/o realtime deletion (@deprecated)
		/*void support_resistance_check_previous_bar(QSqlQuery *query,
																							 const QStringList &intersectProjection,
																							 const IntervalWeight base_interval,
																							 const int &PrevBarCount) {
			QString sql_update;
			QString base_interval_name = Globals::intervalWeightToString(base_interval);
			QString database_alias;
			QString interval_name;
			QString column_name;
			QString rowid_column;
			QString having_clause;
			QStringList split_str;
			int N = intersectProjection.length();

			query->exec("BEGIN TRANSACTION;");
			for (int i = 0; i < N; ++i) {
				split_str = intersectProjection[i].split("_");
				interval_name = split_str[1];
				column_name = split_str[2];

				if (base_interval_name != interval_name) {
					database_alias = "db" + interval_name + ".";
					rowid_column = "Rowid" + interval_name;
				} else {
					database_alias = "";
					rowid_column = "Rowid";
				}

				if (column_name == "Resistance") {
					having_clause = "HAVING A."+intersectProjection[i]+"<=MAX(T."+intersectProjection[i]+"))";
				} else {
					having_clause = "HAVING A."+intersectProjection[i]+">=MIN(T."+intersectProjection[i]+"))";
				}

				sql_update =
					"UPDATE bardataview SET delete_flag=1 WHERE rowid IN("
					"SELECT A.rowid FROM bardataview A"
					" JOIN bardataview T ON T.rowid>=(A.rowid-" + QString::number(PrevBarCount) + ") AND T.rowid<A.rowid "
					" GROUP BY A.rowid " + having_clause;

				qDebug() << sql_update;
				query->exec(sql_update);
				if (query->lastError().isValid()) qDebug() << query->lastError();
			}
			query->exec("COMMIT;");
		}*/

		/*QSqlQueryModel* getSqlModel(const QString &databaseName, const QString &sql) {
			QSqlQueryModel *model = new QSqlQueryModel();
			model->setQuery(sql, getDatabase(databaseName));
			if (model->lastError().isValid()) {
				qDebug() << "get_sqlmodel" << model->lastError();
				delete model;
				return NULL;
			}

//      int counter = 0;
//      while (model->canFetchMore() && ++counter <= 500) {
//        model->fetchMore();
//      }
			return model;
		}*/

		/*QSqlQueryModel* getSqlModel(const QSqlQuery &query) {
			QSqlQueryModel *model = new QSqlQueryModel();
			model->setQuery(query);
			if (model->lastError().isValid()) {
				qDebug() << "getSqlModel" << model->lastError();
				delete model;
				return NULL;
			}
			return model;
		}*/

		/*int selectRowCount(const QString &databaseName, const QString &whereArgs) {
			QSqlDatabase db = getDatabase(databaseName);
			QSqlQuery query(db);
			QString sql = SQLiteHandler::SQL_ROWCOUNT_WHERE(SQLiteHandler::TABLE_NAME_BARDATA, whereArgs);
			query.setForwardOnly(true);
			if (query.exec(sql) && query.next()) {
				return query.value(0).toInt();
			}
			return -1;
		}*/


		// for merging
		QSqlQuery *GetStream (const QSqlDatabase &main_database, const IntervalWeight base_weight, const QStringList &databaseName, const QStringList &databaseAlias, int startRowid)
		{
			QSqlQuery *query = new QSqlQuery(main_database);
			query->setForwardOnly(true);

			for (int i = 0; i < databaseName.length(); ++i)
			{
				query->exec("ATTACH DATABASE '" + databaseName[i] + "' AS " + databaseAlias[i]);
			}

			query->exec ("PRAGMA journal_mode = OFF;");
			query->exec ("PRAGMA synchronous = OFF;");
			query->exec ("PRAGMA cache_size = -16384;");

			QString sql = "select A.rowid, A.date_, A.time_, A.open_, A.high_, A.low_, A.close_, A.Zone, 0, A.Zone60min, A.ZoneDaily, A.ZoneWeekly, A.ZoneMonthly ";

			if (base_weight<=WEIGHT_60MIN)
			{
				sql+=", R1.ATR ";
			}
			else sql+=", A.ATR ";

			for (int i = 0; i < databaseAlias.length(); ++i)
			{
				if (databaseAlias[i] == "db5min")
					sql += ",R5.rowid as Rowid5M,R5.date_ as Date5M,R5.time_ as Time5M,R5.FastAvg as Fast5M, R5.SlowAvg as Slow5M, R5.close_ as Close5M, R5.open_ as Open5M, R5.high_ as High5M, R5.low_ as Low5M, R5.ATR as ATR5M  ";

				else if (databaseAlias[i] == "db60min")
					sql += ",R4.rowid as Rowid4M,R4.date_ as Date60M,R4.time_ as Time60M,R4.FastAvg as Fast60M, R4.SlowAvg as Slow60M, R4.close_ as Close60M, R4.open_ as Open60M, R4.high_ as High60M, R4.low_ as Low60M, R4.ATR as ATR60M  ";

				else if (databaseAlias[i] == "dbDaily")
					sql += ",R1.rowid as RowidDaily,R1.date_ as DateDaily,R1.time_ as TimeDaily,R1.FastAvg as FastDaily, R1.SlowAvg as SlowDaily, R1.close_ as CloseDaily, R1.open_ as OpenDaily, R1.high_ as HighDaily, R1.low_ as LowDaily, R1.ATR as ATRDaily  ";

				else if (databaseAlias[i] == "dbWeekly")
					sql += ",R2.rowid as RowidWeekly,R2.date_ as DateWeekly,R2.time_ as TimeWeekly,R2.FastAvg as FastWeekly, R2.SlowAvg as SlowWeekly, R2.close_ as CloseWeekly, R2.open_ as OpenWeekly, R2.high_ as HighWeekly, R2.low_ as LowWeekly, R2.ATR as ATRWeekly ";

				else if (databaseAlias[i] == "dbMonthly")
					sql += ",R3.rowid as RowidMonthly,R3.time_ as TimeMonthly,R3.FastAvg as FastMonthly, R3.SlowAvg as SlowMonthly, R3.close_ as CloseMonthly, R3.close_ as OpenMonthly, R3.high_ as HighMonthly, R3.low_ as LowMonthly, R3.ATR as ATRMonthly   ";
			}

			if (base_weight == WEIGHT_MONTHLY)
				sql += ",A.rowid as RowidMonthly,A.date_ as DateMonthly, A.time_ as TimeMonthly ,A.FastAvg as FastMonthly,A.SlowAvg as SlowMonthly,A.close_ as CloseMonthly,A.open_ as OpenMonthly, A.high_ as HighMonthly, A.low_ as LowMonthly,A.ATR as ATRMonthly ";

			else if (base_weight == WEIGHT_WEEKLY)
				sql += ",A.rowid as RowidWeekly,A.date_ as DateWeekly, A.time_ as TimeWeekly ,A.FastAvg as FastWeekly,A.SlowAvg as SlowWeekly,A.close_ as CloseWeekly,A.open_ as OpenWeekly, A.high_ as HighWeekly, A.low_ as LowWeekly,A.ATR as ATRWeekly   ";

			else if (base_weight == WEIGHT_DAILY)
				sql += ",A.rowid as RowidDaily,A.date_ as DateDaily, A.time_ as TimeDaily ,A.FastAvg as FastDaily,A.SlowAvg as SlowDaily,A.close_ as CloseDaily,A.open_ as OpenDaily, A.high_ as HighDaily, A.low_ as LowDaily,A.ATR as ATRDaily      ";

			else if (base_weight == WEIGHT_60MIN)
				sql += ",A.rowid as Rowid60M,A.date_ as Date60M, A.time_ as Time60M ,A.FastAvg as Fast60M,A.SlowAvg as Slow60M,A.close_ as Close60M,A.open_ as Open60M, A.high_ as High60M, A.low_ as Low60M,A.ATR as ATR60M ";

			else if (base_weight == WEIGHT_5MIN)
				sql += ",A.rowid as Rowid5M,A.date_ as Date5M, A.time_ as Time5M ,A.FastAvg as Fast5M,A.SlowAvg as Slow5M,A.close_ as Close5M,A.open_ as Open5M, A.high_ as High5M, A.low_ as Low5M,A.ATR as ATR5M  ";

			int start_idx = 0;
			sql += " from " + TABLE_NAME_BARDATA + " A";

			QString curr_table_name;
			QString prev_table_name;
			QString prev_alias = "A";

			for (int i = databaseAlias.length()-1; i >= 0 ; --i)
			{
				curr_table_name = databaseAlias[i] + "." + TABLE_NAME_BARDATA;

				if (databaseAlias[start_idx].isEmpty())
				{
					prev_table_name = "A";
				}
				else
				{
					prev_table_name = databaseAlias[start_idx] + "." + TABLE_NAME_BARDATA;
				}

				prev_table_name = prev_alias;

				if (curr_table_name.contains("Daily")) prev_alias = "R1";
				else if (curr_table_name.contains("Weekly")) prev_alias = "R2";
				else if (curr_table_name.contains("Monthly")) prev_alias = "R3";
				else if (curr_table_name.contains("60min")) prev_alias = "R4";
				else if (curr_table_name.contains("5min")) prev_alias = "R5";

				if (curr_table_name.contains("Monthly"))
				{
					sql += " LEFT JOIN " + curr_table_name + " " + prev_alias + " ON " +
							prev_alias + "." + COLUMN_NAME_ROWID + "=A." + COLUMN_NAME_IDPARENT_MONTHLY;
				}
				else
				{
					sql += " LEFT JOIN " + curr_table_name + " " + prev_alias + " ON " +
							prev_alias + "." + COLUMN_NAME_ROWID + "=" + prev_table_name + "." + COLUMN_NAME_IDPARENT;
				}

				start_idx = i;
			}

			sql += " where A.rowid>="+QString::number(startRowid)+ " order by A.rowid asc";
			query->exec(sql);

			return query;
		}

		QSqlQuery *GetStream (const QSqlDatabase &main_database, const IntervalWeight base_weight, const QStringList &databaseName, const QStringList &databaseAlias, QDateTime startTime, QDateTime endTime)
		{
			QSqlQuery *query = new QSqlQuery(main_database);
			query->setForwardOnly(true);

			for (int i = 0; i < databaseName.length(); ++i)
			{
				query->exec("ATTACH DATABASE '" + databaseName[i] + "' AS " + databaseAlias[i]);
			}

			query->exec ("PRAGMA journal_mode = OFF;");
			query->exec ("PRAGMA synchronous = OFF;");
			query->exec ("PRAGMA cache_size = 50000;");

			QString sql = "select A.rowid, A.date_, A.time_, A.open_, A.high_, A.low_, A.close_ ";

			sql += (base_weight <= WEIGHT_60MIN) ? ", R1.ATR " : ", A.ATR ";

			for (int i = 0; i<databaseAlias.length(); i++)
			{
				if (databaseAlias[i] == "db5min") sql+=",R5.rowid as Rowid5M,R5.date_ as Date5M,R5.time_ as Time5M,R5.FastAvg as Fast5M, R5.SlowAvg as Slow5M, R5.close_ as Close5M, R5.open_ as Open5M, R5.high_ as High5M, R5.low_ as Low5M, R5.ATR as ATR5M  ";
				else if (databaseAlias[i] == "db60min") sql+=",R4.rowid as Rowid60M,R4.date_ as Date60M,R4.time_ as Time60M,R4.FastAvg as Fast60M, R4.SlowAvg as Slow60M, R4.close_ as Close60M, R4.open_ as Open60M, R4.high_ as High60M, R4.low_ as Low60M, R4.ATR as ATR60M  ";
				else if (databaseAlias[i] == "dbDaily") sql+=",R1.rowid as RowidDaily,R1.date_ as DateDaily,R1.time_ as TimeDaily,R1.FastAvg as FastDaily, R1.SlowAvg as SlowDaily, R1.close_ as CloseDaily, R1.open_ as OpenDaily, R1.high_ as HighDaily, R1.low_ as LowDaily, R1.ATR as ATRDaily  ";
				else if (databaseAlias[i] == "dbWeekly") sql+=",R2.rowid as RowidWeekly,R2.date_ as DateWeekly,R2.time_ as TimeWeekly,R2.FastAvg as FastWeekly, R2.SlowAvg as SlowWeekly, R2.close_ as CloseWeekly, R2.open_ as OpenWeekly, R2.high_ as HighWeekly, R2.low_ as LowWeekly, R2.ATR as ATRWeekly ";
				else if (databaseAlias[i] == "dbMonthly") sql+=",R3.rowid as RowidMonthly,R3.time_ as TimeMonthly,R3.FastAvg as FastMonthly, R3.SlowAvg as SlowMonthly, R3.close_ as CloseMonthly, R3.close_ as OpenMonthly, R3.high_ as HighMonthly, R3.low_ as LowMonthly, R3.ATR as ATRMonthly   ";
			}

			if (base_weight==WEIGHT_MONTHLY) sql += ",A.rowid as RowidMonthly,A.date_ as DateMonthly, A.time_ as TimeMonthly ,A.FastAvg as FastMonthly,A.SlowAvg as SlowMonthly,A.close_ as CloseMonthly,A.open_ as OpenMonthly, A.high_ as HighMonthly, A.low_ as LowMonthly,A.ATR as ATRMonthly ";
			else if (base_weight==WEIGHT_WEEKLY) sql += ",A.rowid as RowidWeekly,A.date_ as DateWeekly, A.time_ as TimeWeekly ,A.FastAvg as FastWeekly,A.SlowAvg as SlowWeekly,A.close_ as CloseWeekly,A.open_ as OpenWeekly, A.high_ as HighWeekly, A.low_ as LowWeekly,A.ATR as ATRWeekly   ";
			else if (base_weight==WEIGHT_DAILY) sql += ",A.rowid as RowidDaily,A.date_ as DateDaily, A.time_ as TimeDaily ,A.FastAvg as FastDaily,A.SlowAvg as SlowDaily,A.close_ as CloseDaily,A.open_ as OpenDaily, A.high_ as HighDaily, A.low_ as LowDaily,A.ATR as ATRDaily      ";
			else if (base_weight==WEIGHT_60MIN) sql += ",A.rowid as Rowid60M,A.date_ as Date60M, A.time_ as Time60M ,A.FastAvg as Fast60M,A.SlowAvg as Slow60M,A.close_ as Close60M,A.open_ as Open60M, A.high_ as High60M, A.low_ as Low60M,A.ATR as ATR60M ";
			else if (base_weight==WEIGHT_5MIN) sql += ",A.rowid as Rowid5M,A.date_ as Date5M, A.time_ as Time5M ,A.FastAvg as Fast5M,A.SlowAvg as Slow5M,A.close_ as Close5M,A.open_ as Open5M, A.high_ as High5M, A.low_ as Low5M,A.ATR as ATR5M  ";

			int start_idx = 0;
			sql += " from " + TABLE_NAME_BARDATA + " A";

			QString curr_table_name;
			QString prev_table_name;
			QString prev_alias = "A";

			for (int i = databaseAlias.length()-1; i >= 0 ; --i)
			{
				curr_table_name = databaseAlias[i] + "." + TABLE_NAME_BARDATA;

				if (databaseAlias[start_idx].isEmpty()) {
					prev_table_name = "A";
				} else {
					prev_table_name = databaseAlias[start_idx] + "." + TABLE_NAME_BARDATA;
				}


				prev_table_name = prev_alias;

				if (curr_table_name.contains("Daily")) prev_alias = "R1";
				else if (curr_table_name.contains("Weekly")) prev_alias = "R2";
				else if (curr_table_name.contains("Monthly")) prev_alias = "R3";
				else if (curr_table_name.contains("60min")) prev_alias = "R4";
				else if (curr_table_name.contains("5min")) prev_alias = "R5";

				if (curr_table_name.contains("Monthly")) {
					sql += " LEFT JOIN " + curr_table_name + " " + prev_alias + " ON " +
							prev_alias + "." + COLUMN_NAME_ROWID + "=A." + COLUMN_NAME_IDPARENT_MONTHLY;
				} else {
					sql += " LEFT JOIN " + curr_table_name + " " + prev_alias + " ON " +
							prev_alias + "." + COLUMN_NAME_ROWID + "=" + prev_table_name + "." + COLUMN_NAME_IDPARENT;
				}

				start_idx = i;
			}

			QString Sdate= "'" + startTime.date().toString("yyyy-MM-dd") + "'";
			QString Stime = "'" + startTime.time().toString("HH:mm") + "'";
			QString Edate= "'" + endTime.date().toString("yyyy-MM-dd") + "'";
			QString Etime = "'" + endTime.addDays(1).time().toString("HH:mm") + "'";

			sql+=" where ( A.date_>" + Sdate
					+ " OR ( A.date_=" + Sdate + " AND A.time_>=" + Stime
					+ "))"
					+ " and A.date_<"+Edate+
					" order by A.rowid asc";

			query->exec(sql);

			return query;
		}

	private:
		static QMutex _mutex;
		static const QString tempDatabaseName;
		static int connection_id;
		mutable QStringList tempdatabase_connection;

		QHash<QString,QSqlDatabase*> databasePool;
		QHash<QString,QSqlDatabase*> tempDatabasePool;
		QString application_path = QCoreApplication::applicationDirPath();
		QString database_path;
		int application_id;

		QString create_tempdb_connstring (IntervalWeight interval, int id) const
		{
			return "tempdb#" + getTempResultDatabaseName (interval, id);
		}

		/*void dumpSql (QString &databaseName, QString &tableName) {
			QString sql_select = "SELECT * FROM " + tableName;
			QSqlDatabase database = getDatabase(databaseName);
			QSqlQuery query(database);
			query.setForwardOnly(true);
			if (query.exec(sql_select)) {
				QString filename = databaseName + "." + tableName + ".sql";
				QStringList columns = getColumnNames(databaseName, tableName);
				QString column_names = columns.join(",");
				QString sql_insert = "";
				QString values;
				int N = columns.length();
				while (query.next()) {
					values = "";
					for (int i = 0; i < N; ++i) {
						if (i > 0) values += ",";
						values += query.value(i).toString();
					}
					sql_insert += "INSERT INTO " + tableName + "(" + column_names + ") VALUES(" + values + ");";
				}
			}
		}*/
};

#endif // SQLITEHANDLER_H
