#ifndef RESISTANCEHIGHOBSERVER
#define RESISTANCEHIGHOBSERVER

#include <QDate>
#include <QHash>
#include <QVariant>
#include <QVariantList>
#include <QVector>

#include "abstractstream.h"
#include "resistancequerystream.h"
#include "sqlitehandler.h"

class ResistanceHighObserver {
  public:
    ResistanceHighObserver() {}

    ~ResistanceHighObserver() {
      flush();
    }

    void bind(AbstractStream *baseStream, AbstractStream *resistanceStream) {
      mStream = baseStream;
      mResistanceStream = resistanceStream;
      if (resistanceStream != 0) {
        static_cast<ResistanceQueryStream*>(resistanceStream)->set_date_dictionary(&dateDict);
      }
    }

    void update() {
      if (mStream == 0 || mResistanceStream == 0) return;

      // store historical High price in descending order
      histDate.push_front(mStream->getDate());
      histHigh.push_front(mStream->getHigh());

      // align date of baseStream and resistanceStream
      while (!mResistanceStream->atEnd() && mResistanceStream->getDate() < mStream->getDate()) {
        mResistanceStream->readLine();
      }

      // ResistanceStream should implement these columns:
      // { Rowid, FirstReactDate, Resistance, LastReactDate }

      QVector<long> resRowid;
      QVector<double> resPrice;
      QVector<QString> resDateString;
      QVector<QDate> resDate;
      QVector<QString> resLastReactDateString;
      QVector<QStringList> resReactDateList;
      QVector<int> resCount;
      QVector<int> resConsecT;

      // process resistance line
      while (!mResistanceStream->atEnd() &&
              mResistanceStream->getDate().toString("yyyy-MM-dd") ==
              mStream->getDate().toString("yyyy-MM-dd")) {
        resRowid.push_back(mResistanceStream->getRowid());
        resPrice.push_back(mResistanceStream->getResistancePrice());
        resDate.push_back(mResistanceStream->getDate());
        resDateString.push_back(mResistanceStream->getDate().toString("yyyy-MM-dd"));
        resLastReactDateString.push_back(mResistanceStream->getLastReactDate().toString("yyyy-MM-dd"));
        resReactDateList.push_back(static_cast<ResistanceQueryStream*>(mResistanceStream)->getReactDateList());
        resCount.push_back(0);
        resConsecT.push_back(0);
        mResistanceStream->readLine();
      }

      QString day_before_current;
      QString two_days_before_current;
      QString n_days_before_current;
      int count = resRowid.size();
      int consec_t;

      // Consecutive O(N)
      for (int j = 0; j < count; ++j) {

        // PRE -- current date is not equal last react date
        if (resLastReactDateString[j] != resDateString[j]) {
          day_before_current = dateDict[resDateString[j]];
        }
        // POST -- current date is equal to last react date
        else {
          day_before_current = resDateString[j];
        }

        consec_t = 0;
        two_days_before_current = dateDict[day_before_current];
        n_days_before_current = day_before_current;

        // Consecutive_T
        if (resLastReactDateString[j] == day_before_current) {

          ++consec_t;

          if (resReactDateList[j].contains(two_days_before_current)) {

            ++consec_t;
            n_days_before_current = dateDict[two_days_before_current];

            // counting until it broke streak consecutive days
            while (resReactDateList[j].contains(n_days_before_current)) {
              n_days_before_current = dateDict[n_days_before_current];

              if (n_days_before_current == dateDict[n_days_before_current]) break;

              ++consec_t;
            }
          }
        }

        // eliminate less than 2 consecutive
        if (consec_t >= 2) resConsecT[j] = consec_t;
      }

      count = histDate.size();

      // Count HighBars O(N^2)
      for (int i = 0; i < count; ++i) {

        // if all resistance line has been processed then quit
        if (resRowid.isEmpty()) break;

        // check foreach resistance level
        for (int j = 0; j < resRowid.size(); ) {

          if (histDate[i] <= resDate[j]) {
            if (histHigh[i] <= resPrice[j] || std::fabs(histHigh[i]-resPrice[j]) < 0.00001) {
              ++resCount[j];
            }
            else if (resCount[j] > 0) {
              vRowid.push_back(resRowid[j]);
              vCount.push_back(resCount[j]);
              vConsec_T.push_back(resConsecT[j]);

              resRowid.removeAt(j);
              resPrice.removeAt(j);
              resDate.removeAt(j);
              resDateString.removeAt(j);
              resLastReactDateString.removeAt(j);
              resReactDateList.removeAt(j);
              resCount.removeAt(j);
              resConsecT.removeAt(j);
              continue;
            }
          }

          ++j;
        }
      }

      // push remaining data
      for (int i = 0; i < resRowid.size(); ++i) {
        vRowid.push_back(resRowid[i]);
        vCount.push_back(resCount[i]);
        vConsec_T.push_back(resConsecT[i]);
      }

      if (vRowid.size() >= 50000) saveList();
    }

    void flush() {
      if (vRowid.size() > 0) saveList();
    }

  private:
    AbstractStream *mStream = 0;
    AbstractStream *mResistanceStream = 0;
    QVariantList vRowid;
    QVariantList vCount;
    QVariantList vConsec_T;
//    QVariantList vConsec_N;
    QVector<QDate> histDate;
    QVector<double> histHigh;
    QHash<QString,QString> dateDict;

    void clearList() {
      vCount.clear();
      vConsec_T.clear();
//      vConsec_N.clear();
      vRowid.clear();
    }

    void saveList() {
      // build column projection
      QStringList projection;
      projection.push_back(SQLiteHandler::COLUMN_NAME_HIGHBARS);
      projection.push_back(SQLiteHandler::COLUMN_NAME_CONSEC_T);

      // save updated calculated HighBars in stream
      mResistanceStream->save(projection, projection.size(), vCount, vConsec_T, vRowid);

      // free resource
      clearList();
    }
};

#endif // RESISTANCEHIGHOBSERVER
