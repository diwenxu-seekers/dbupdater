#ifndef BARDATATEXTSTREAM
#define BARDATATEXTSTREAM

#include <cstdarg>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextStream>
#include "AbstractStream.h"
#include "SQLiteHandler.h"

class BardataTextStream : public AbstractStream {
  public:
    BardataTextStream(QTextStream *textStream, const QSqlDatabase &database) {
      bind(textStream, database);
    }

    void bind(QTextStream *textStream, const QSqlDatabase &database) {
      mStream = textStream;
      mDatabase = database;
    }

    bool readLine() {
      if (mStream != NULL && !mStream->atEnd()) {
        QStringList columns;
        columns = mStream->readLine().split(",", QString::KeepEmptyParts);
        mDate = QDate::fromString(columns[0].trimmed(),"MM/dd/yyyy");
        mTime = columns[1].trimmed();
        mOpen = columns[2].trimmed().toDouble();
        mHigh = columns[3].trimmed().toDouble();
        mLow = columns[4].trimmed().toDouble();
        mClose = columns[5].trimmed().toDouble();
        mVolume = columns[6].trimmed().toInt();
        mMACD = columns[7].trimmed().toDouble();
        mMACDAvg = columns[8].trimmed().toDouble();
        mMACDDiff = columns[9].trimmed().toDouble();
        mRSI = columns[10].trimmed().toDouble();
        mSlowK = columns[11].trimmed().toDouble();
        mSlowD = columns[12].trimmed().toDouble();
        mFastAvg = columns[13].trimmed().toDouble();
        mSlowAvg = columns[14].trimmed().toDouble();
        mDistF = columns[15].trimmed().toDouble();
        mDistS = columns[16].trimmed().toDouble();
        mFGS = columns[17].trimmed().toInt();
        mFLS = columns[18].trimmed().toInt();
        mOpenbar = columns[19].trimmed().toInt();
        return true;
      }
      return false;
    }

    void save(const QStringList &projection, int N, ...) {
      QString s = ",?";
      QString sql =
        "INSERT OR IGNORE INTO " + SQLiteHandler::TABLE_NAME_BARDATA +
        "(" + projection.join(",") + ")" +
        "VALUES(?" + s.repeated(projection.size()-1) + ")";

      QSqlQuery query(mDatabase);
      query.exec("PRAGMA journal_mode = OFF;");
      query.exec("PRAGMA synchronous = OFF;");
      query.exec("PRAGMA cache_size = 100000;");
      query.exec("BEGIN TRANSACTION;");
      query.prepare(sql);

      va_list argsPtr;
      va_start(argsPtr, N);

      for (int i = 0; i < N; ++i) {
        query.addBindValue(va_arg(argsPtr, QVariantList));
      }

      va_end(argsPtr);
      query.execBatch();
      if (query.lastError().isValid()) { qDebug()<< sql << "\n" << query.lastError(); }
      query.exec("COMMIT;");
    }

    // getter
    QDate getDate() { return mDate; }
    QTime getTime() { return mTime; }
    double getOpen() { return mOpen; }
    double getHigh() { return mHigh; }
    double getLow() { return mLow; }
    double getClose() { return mClose; }
    long getVolume() { return mVolume; }
    double getMACD() { return mMACD; }
    double getMACDAvg() { return mMACDAvg; }
    double getMACDDiff() { return mMACDDiff; }
    double getRSI() { return mRSI; }
    double getSlowK() { return mSlowK; }
    double getSlowD() { return mSlowD; }
    double getMAFast() { return mFastAvg; }
    double getMASlow() { return mSlowAvg; }
    double getDistF() { return mDistF; }
    double getDistS() { return mDistS; }
    int getFGS() { return mFGS; }
    int getFLS() { return mFLS; }
    int getOpenBar() { return mOpenbar; }
    long getParent() { return mParent; }
    long getParentPrev() { return mParentPrev; }
    long getRowid() { return mRowid; }

  private:
    QTextStream *mStream = NULL;
    QSqlDatabase mDatabase;
    QStringList projection;
    QDate mDate;
    QTime mTime;
    double mOpen;
    double mHigh;
    double mLow;
    double mClose;
    long mVolume;
    double mMACD;
    double mMACDAvg;
    double mMACDDiff;
    double mRSI;
    double mSlowK;
    double mSlowD;
    double mFastAvg;
    double mSlowAvg;
    double mDistF;
    double mDistS;
    int mFGS;
    int mFLS;
    int mOpenbar;
    long mParent;
    long mParentPrev;
    long mRowid;
};

#endif // BARDATATEXTSTREAM
