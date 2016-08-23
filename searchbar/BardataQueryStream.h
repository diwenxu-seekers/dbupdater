#ifndef BARDATAQUERYSTREAM
#define BARDATAQUERYSTREAM

#include "abstractstream.h"
#include "sqlitehandler.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

#include <cstdarg>

class BardataQueryStream : public AbstractStream {
  public:
    BardataQueryStream()
    {
      mStream = new QSqlQuery();
    }

    BardataQueryStream(const QStringList &sql_statement, const QSqlDatabase &database)
    {
      setQuery(sql_statement, database);
    }

    BardataQueryStream(const QSqlDatabase &database, int startRowid = 0)
    {
      setDatabase(database, startRowid);
    }

    ~BardataQueryStream()
    {
      delete mStream;
      mStream = nullptr;
    }

    void setDatabase(const QSqlDatabase &database, int startRowid = 0)
    {
      projection.clear();
      projection.push_back(SQLiteHandler::COLUMN_NAME_ROWID);
      projection.push_back(SQLiteHandler::COLUMN_NAME_DATE);
      projection.push_back(SQLiteHandler::COLUMN_NAME_TIME);
      projection.push_back(SQLiteHandler::COLUMN_NAME_OPEN);
      projection.push_back(SQLiteHandler::COLUMN_NAME_HIGH);
      projection.push_back(SQLiteHandler::COLUMN_NAME_LOW);
      projection.push_back(SQLiteHandler::COLUMN_NAME_CLOSE);
      projection.push_back(SQLiteHandler::COLUMN_NAME_FASTAVG);
      projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWAVG);
      projection.push_back(SQLiteHandler::COLUMN_NAME_MACD);
      projection.push_back(SQLiteHandler::COLUMN_NAME_RSI);
      projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWK);
      projection.push_back(SQLiteHandler::COLUMN_NAME_SLOWD);
      projection.push_back(SQLiteHandler::COLUMN_NAME_IDPARENT);
      projection.push_back(SQLiteHandler::COLUMN_NAME_IDPARENT_PREV);
      projection.push_back(SQLiteHandler::COLUMN_NAME_ATR);
      projection.push_back(SQLiteHandler::COLUMN_NAME_PREV_DAILY_ATR);

      QString select_bardata = "select " + projection.join(",") + " from bardata";

      if (startRowid > 0)
      {
        select_bardata += " where rowid >=" + QString::number(startRowid);
      }

      select_bardata += " order by rowid asc";

      mDatabase = database;
      mStream = new QSqlQuery(mDatabase);
      mStream->setForwardOnly(true);
      mStream->exec(select_bardata);
      mStream->next();
    }

    void setQuery(const QStringList &sql_statement, const QSqlDatabase &database)
    {
      if (mStream != NULL)
      {
        delete mStream;
      }

      mDatabase = database;
      mStream = new QSqlQuery(mDatabase);
      mStream->setForwardOnly(true);

      int count = sql_statement.size();

      for (int i = 0; i < count; ++i)
      {
        mStream->exec(sql_statement[i]);

        if (mStream->lastError().isValid())
        {
          qDebug("setQueryError:: %s", mStream->lastError().text().toStdString().c_str());
        }
      }

      mStream->next();
    }

    bool readLine()
    {
      return (mStream == NULL) ? false : mStream->next();
    }

    bool atEnd()
    {
      return (mStream == NULL) ? true : !mStream->isValid();
    }

    void save(const QStringList &projection, int N, ...)
    {
      QSqlQuery q(mDatabase);
      q.exec("PRAGMA temp_store = FILE;");
      q.exec("PRAGMA journal_mode = OFF;");
      q.exec("PRAGMA synchronous = OFF;");
      q.exec("PRAGMA cache_size = 50000;");
      q.exec("BEGIN TRANSACTION;");
      q.prepare("update bardata set " + projection.join("=?,") + "=? where rowid=?");

      va_list argsPtr;
      va_start(argsPtr, N + 1);

      for (int i = 0; i < N + 1; ++i)
      {
        q.addBindValue(va_arg(argsPtr, QVariantList));
      }

      va_end(argsPtr);
      q.execBatch();
      q.exec("COMMIT;");
    }

    // getter default (sort by column projection)
    // hardcoded column index to make it faster access

    long int getRowid() override { return mStream->value(0).toLongLong(); }
    QDate getDate() override { return mStream->value(1).toDate(); }
    QTime getTime() override { return mStream->value(2).toTime(); }
    double getOpen() override { return mStream->value(3).toDouble(); }
    double getHigh() override { return mStream->value(4).toDouble(); }
    double getLow() override { return mStream->value(5).toDouble(); }
    double getClose() override { return mStream->value(6).toDouble(); }
    double getMAFast() override { return mStream->value(7).toDouble(); }
    double getMASlow() override { return mStream->value(8).toDouble(); }
    double getMACD() override { return mStream->value(9).toDouble(); }
    double getRSI() override { return mStream->value(10).toDouble(); }
    double getSlowK() override { return mStream->value(11).toDouble(); }
    double getSlowD() override { return mStream->value(12).toDouble(); }
    long int getParent() override { return mStream->value(13).toLongLong(); }
    long int getParentPrev() override { return mStream->value(14).toLongLong(); }
    double getATR() override { return mStream->value(15).toDouble(); }
    double getPrevDailyATR() override { return mStream->value(16).toDouble(); }
    QString getDateString() override { return mStream->value(1).toString(); }
    QString getTimeString() override { return mStream->value(2).toString(); }
    QVariant getValueAt(int index) override { return mStream->value(index); }

  private:
    QSqlQuery *mStream = nullptr;
    QSqlDatabase mDatabase;
    QStringList projection;
};

#endif // BARDATAQUERYSTREAM
