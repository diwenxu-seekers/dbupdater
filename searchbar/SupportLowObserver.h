#ifndef SUPPORTLOWOBSERVER
#define SUPPORTLOWOBSERVER

#include <QDate>
#include <QHash>
#include <QVariant>
#include <QVariantList>
#include <QVector>

#include "abstractstream.h"
#include "supportquerystream.h"
#include "sqlitehandler.h"

class SupportLowObserver {
  public:
    SupportLowObserver() {}

    ~SupportLowObserver() {
      flush();
    }

    void bind(AbstractStream *baseStream, AbstractStream *supportStream) {
      mStream = baseStream;
      mSupportStream = supportStream;
      if (supportStream != 0) {
        static_cast<SupportQueryStream*>(supportStream)->set_date_dictionary(&dateDict);
      }
    }

    void update() {
      if (mStream == 0 || mSupportStream == 0) return;

      // store historical Low price in descending order
      histDate.push_front(mStream->getDate());
      histLow.push_front(mStream->getLow());

      // align date of baseStream and resistanceStream
      while (!mSupportStream->atEnd() && mSupportStream->getDate() < mStream->getDate()) {
        mSupportStream->readLine();
      }

      // SupportStream should implement these columns:
      // { Rowid, FirstReactDate, Support, LastReactDate }

      QVector<long> supRowid;
      QVector<double> supPrice;
      QVector<QString> supDateString;
      QVector<QDate> supDate;
      QVector<QString> supLastReactDateString;
      QVector<QStringList> supReactDateList;
      QVector<int> supCount;
      QVector<int> supConsecT;

      // process resistance line
      while (!mSupportStream->atEnd() &&
             mSupportStream->getDate().toString("yyyy-MM-dd") ==
             mStream->getDate().toString("yyyy-MM-dd")) {

        supRowid.push_back(mSupportStream->getRowid());
        supPrice.push_back(mSupportStream->getSupportPrice());
        supDate.push_back(mSupportStream->getDate());
        supDateString.push_back(mSupportStream->getDate().toString("yyyy-MM-dd"));
        supLastReactDateString.push_back(mSupportStream->getLastReactDate().toString("yyyy-MM-dd"));
        supReactDateList.push_back(static_cast<SupportQueryStream*>(mSupportStream)->getReactDateList());
        supCount.push_back(0);
        supConsecT.push_back(0);
        mSupportStream->readLine();
      }

      QString day_before_current;
      QString two_days_before_current;
      QString n_days_before_current;
      int count = supRowid.size();
      int consec_t;

      // Consecutive O(N)
      for (int j = 0; j < count; ++j) {

        // PRE -- current date is not equal last react date
        if (supLastReactDateString[j] != supDateString[j]) {
          day_before_current = dateDict[supDateString[j]];
        }
        // POST -- current date is equal to last react date
        else {
          day_before_current = supDateString[j];
        }

        consec_t = 0;
        two_days_before_current = dateDict[day_before_current];
        n_days_before_current = day_before_current;

        // Consecutive_T
        if (supLastReactDateString[j] == day_before_current) {

          ++consec_t;

          if (supReactDateList[j].contains(two_days_before_current)) {

            ++consec_t;
            n_days_before_current = dateDict[two_days_before_current];

            // counting until it broke streak consecutive days
            while (supReactDateList[j].contains(n_days_before_current)) {
              n_days_before_current = dateDict[n_days_before_current];

              if (n_days_before_current == dateDict[n_days_before_current]) break;

              ++consec_t;
            }
          }
        }

        if (consec_t >= 2) supConsecT[j] = consec_t;
      }

      count = histDate.size();

      // Count LowBars O(N^2)
      for (int i = 0; i < count; ++i) {

        // if all resistance line has been processed then quit
        if (supRowid.isEmpty()) break;

        // check foreach resistance level
        for (int j = 0; j < supRowid.size(); ) {
          if (histDate[i] <= supDate[j]) {
            if (histLow[i] >= supPrice[j] || std::fabs(histLow[i]-supPrice[j]) < 0.00001) {
              ++supCount[j];
            }
            else if (supCount[j] > 0) {
              vRowid.push_back(supRowid[j]);
              vCount.push_back(supCount[j]);
              vConsec_T.push_back(supConsecT[j]);

              supRowid.removeAt(j);
              supPrice.removeAt(j);
              supDate.removeAt(j);
              supDateString.removeAt(j);
              supLastReactDateString.removeAt(j);
              supReactDateList.removeAt(j);
              supCount.removeAt(j);
              supConsecT.removeAt(j);
              continue;
            }
          }

          ++j;
        }
      }

      // push remaining data
      for (int i = 0; i < supRowid.size(); ++i) {
        vRowid.push_back(supRowid[i]);
        vCount.push_back(supCount[i]);
        vConsec_T.push_back(supConsecT[i]);
      }

      if (vRowid.size() >= 50000) saveList();
    }

    void flush() {
      if (vRowid.size() > 0) saveList();
    }

  private:
    AbstractStream *mStream = 0;
    AbstractStream *mSupportStream = 0;
    QVariantList vRowid;
    QVariantList vCount;
    QVariantList vConsec_T;
    QVariantList vConsec_N;
    QVector<QDate> histDate;
    QVector<double> histLow;
    QHash<QString,QString> dateDict;

    void clearList() {
      vRowid.clear();
      vCount.clear();
      vConsec_T.clear();
      vConsec_N.clear();
    }

    void saveList() {
      // build column projection
      QStringList projection;
      projection.push_back(SQLiteHandler::COLUMN_NAME_LOWBARS);
      projection.push_back(SQLiteHandler::COLUMN_NAME_CONSEC_T);

      // save updated calculated HighBars in stream
      mSupportStream->save(projection, projection.size(), vCount, vConsec_T, vRowid);

      // free resource
      clearList();
    }
};

#endif // SUPPORTLOWOBSERVER
