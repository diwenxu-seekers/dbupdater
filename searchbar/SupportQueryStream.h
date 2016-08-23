#ifndef SUPPORTQUERYSTREAM
#define SUPPORTQUERYSTREAM

#include <QSqlDatabase>
#include <QSqlQuery>

#include "abstractstream.h"
#include "sqlitehandler.h"

class SupportQueryStream : public AbstractStream {
  public:
    SupportQueryStream()
    {
      mStream = new QSqlQuery();
    }

    ~SupportQueryStream()
    {
      delete mStream;
      mStream = 0;
    }

    void setDatabase (const QSqlDatabase &database, int id_threshold, QString start_date = "")
    {
      QStringList projection;
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_ROWID);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_DATE);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_TIME);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_REACT_DATE);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_REACT_TIME);
      projection.push_back("MAX(B." + SQLiteHandler::COLUMN_NAME_REACT_DATE + ")");
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_TIME);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_SUPPORT_COUNT);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_SUPPORT_DURATION);
      projection.push_back("A." + SQLiteHandler::COLUMN_NAME_SUPPORT_VALUE);
      projection.push_back("GROUP_CONCAT(B." + SQLiteHandler::COLUMN_NAME_REACT_DATE + ")");

      if (start_date != "")
      {
        start_date = " and A.date_ >'" + start_date + "'";
      }

      mDatabase = database;
      mStream = new QSqlQuery(mDatabase);
      mStream->setForwardOnly(true);
      mStream->exec("select " + projection.join(",") +
                    " from SupportData A"
                    " join SupportReactDate B on A.rowid=B.rid"
                    " where id_threshold=" + QString::number(id_threshold) + start_date +
                    " group by A.rowid"
                    " order by A.rowid desc");

      if (mStream->lastError().isValid()) qDebug() << mStream->lastError();

      mStream->next();
    }

    void setQuery (const QStringList &sqlStatement, const QSqlDatabase &database)
    {
      if (mStream != NULL)
      {
        delete mStream;
      }

      mDatabase = database;
      mStream = new QSqlQuery(mDatabase);
      mStream->setForwardOnly(true);

      int count = sqlStatement.size();

      for (int i = 0; i < count; ++i)
      {
        mStream->exec(sqlStatement[i]);
        if (mStream->lastError().isValid())
        {
          qDebug() << "setQueryError::" << mStream->lastError();
        }
      }

      mStream->next();
    }

    bool readLine()
    {
      return (mStream == 0)? false : mStream->next();
    }

    bool atEnd()
    {
      return (mStream == 0) ? true : !mStream->isValid();
    }

    void save (const QStringList &projection, int N, ...)
    {
      QSqlQuery query (mDatabase);
      query.exec("PRAGMA journal_mode = OFF;");
      query.exec("PRAGMA synchronous = OFF;");
      query.exec("PRAGMA cache_size = 20000;");
      query.exec("BEGIN TRANSACTION;");
      query.prepare("update SupportData set " + projection.join("=?,") + "=? where rowid=?");

      va_list argsPtr;
      va_start(argsPtr, N + 1);

      for (int i = 0; i < N + 1; ++i)
      {
        query.addBindValue(va_arg(argsPtr, QVariantList));
      }

      va_end(argsPtr);
      query.execBatch();
      query.exec("COMMIT;");
    }

    QVariant getValueAt (int index) { return mStream->value(index); }
    long getRowid() { return mStream->value(0).toLongLong(); }
    QDate getDate() { return mStream->value(1).toDate(); }
    QTime getTime() { return mStream->value(2).toTime(); }
    QDate getFirstReactDate() { return mStream->value(3).toDate(); }
    QTime getFirstReactTime() { return mStream->value(4).toTime(); }
    QDate getLastReactDate() { return mStream->value(5).toDate(); }
    QTime getLastReactTime() { return mStream->value(6).toTime(); }
    int getTestNumber() { return mStream->value(7).toInt(); }
    int getDuration() { return mStream->value(8).toInt(); }
    double getSupportPrice() { return mStream->value(9).toDouble(); }
    QStringList getReactDateList() { return mStream->value(10).toString().split(","); }

    // date -> prevdate dictionary to help determine consecutive day
    // since daily data is not large around 3,000 rows then it's feasible to store as local variable
    void set_date_dictionary (QHash <QString, QString> *out)
    {
      if (out == 0) return;

      QSqlQuery q(mDatabase);
      q.setForwardOnly(true);
      q.exec("select date_, prevdate from bardata order by rowid asc");

      while (q.next())
      {
        out->insert(q.value(0).toString(), q.value(1).toString());
      }
    }

  private:
    QSqlQuery *mStream = NULL;
    QSqlDatabase mDatabase;
};

#endif // SUPPORTQUERYSTREAM
