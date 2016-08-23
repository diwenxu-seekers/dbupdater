#ifndef SUPPORTELOCITYOBSERVER
#define SUPPORTVELOCITYOBSERVER

#include <QDate>
#include <QVariantList>
#include <QVector>
#include "abstractstream.h"

class SupportVelocityObserver {
  public:
    SupportVelocityObserver() {}

    ~SupportVelocityObserver() {
      flush();
    }

    void bind(AbstractStream *baseStream, AbstractStream *secondStream) {
      mStream = baseStream;
      mSecondStream = secondStream;
    }

    void set_threshold(const int &idThreshold, const double &reactThreshold) {
      mIdThreshold = idThreshold;
      mReactThreshold = reactThreshold;
    }

    void set_intraday_flag(const bool &b) {
      isIntraday = b;
    }

    void update() {
      if (mStream == 0 || mSecondStream == 0) return;

      QDate current_date = mStream->getDate();
      QTime current_time = mStream->getTime();
      QDateTime current_datetime(current_date, current_time);
      double current_low = mStream->getLow();

      while (!mSecondStream->atEnd() && mSecondStream->getValueAt(1).toDate() <= current_date) {
        rowid.push_back(mSecondStream->getRowid());
        startReactDatetime.push_back(QDateTime(mSecondStream->getValueAt(1).toDate(), mSecondStream->getValueAt(2).toTime()));
        endReactDatetime.push_back(QDateTime(mSecondStream->getValueAt(3).toDate(), mSecondStream->getValueAt(4).toTime()));
        supportLevel.push_back(mSecondStream->getValueAt(5).toDouble());
        closePrice.push_back(mSecondStream->getValueAt(6).toDouble());
        dailyATR.push_back(mSecondStream->getValueAt(7).toDouble());
        distbar.push_back(0);
        distpoint.push_back(std::numeric_limits<double>::max());
        mSecondStream->readLine();
      }

      for (int i = 0; i < startReactDatetime.size(); ) {
        if (current_datetime == endReactDatetime[i]) {
          // compare last bar
          distpoint[i] = std::min(distpoint[i], current_low);

          // increment last bar
          ++distbar[i];

          // push into list
          vRowid.push_back(rowid[i]);
          vDistBar.push_back(distbar[i]);
          vDistPoint.push_back(QString::number(closePrice[i] - distpoint[i],'f',4));
          vDistATR.push_back(QString::number((closePrice[i] - distpoint[i]) / dailyATR[i],'f',4));

          // pop out
          rowid.removeAt(i);
          startReactDatetime.removeAt(i);
          endReactDatetime.removeAt(i);
          supportLevel.removeAt(i);
          closePrice.removeAt(i);
          dailyATR.removeAt(i);
          distbar.removeAt(i);
          distpoint.removeAt(i);
          continue;
        }
        else {
          if ( !isIntraday ) {
            if (current_datetime >= startReactDatetime[i] && current_datetime <= endReactDatetime[i]) {
              distpoint[i] = std::min(distpoint[i], current_low);
              ++distbar[i];
            }
          }
          // Intraday
          else {
            if (current_datetime >= startReactDatetime[i] && current_datetime <= endReactDatetime[i]) {

              // just increment if first react has been encountered
              if (distbar[i] > 0) {
                distpoint[i] = std::min(distpoint[i], current_low);
                ++distbar[i];
              }

              // find first react
              else if (current_low >= supportLevel[i] + mReactThreshold ||
                       std::fabs(supportLevel[i] + mReactThreshold - current_low) < 0.00001) {
                distpoint[i] = std::min(distpoint[i], current_low);
                distbar[i] = 1;
              }
            }
          }
        }

        ++i;
      }

      if (vRowid.size() >= 50000) saveList();
    }

    void flush() {
      if (vRowid.size() > 0) saveList();
    }

  private:
    AbstractStream *mStream = 0;
    AbstractStream *mSecondStream = 0;
    QVariantList vRowid;
    QVariantList vDistBar;
    QVariantList vDistPoint;
    QVariantList vDistATR;
    QVector<QDateTime> startReactDatetime;
    QVector<QDateTime> endReactDatetime;
    QVector<double> supportLevel;
    QVector<double> closePrice;
    QVector<double> dailyATR;
    QVector<int> distbar;
    QVector<double> distpoint;
    QVector<int> rowid;
    double mReactThreshold;
    int mIdThreshold;
    bool isIntraday;

    void clearList() {
      vRowid.clear();
      vDistBar.clear();
      vDistPoint.clear();
      vDistATR.clear();
    }

    void saveList() {
      QString mID = "_" + QString::number(mIdThreshold+1);

      // build projection
      QStringList projection;
      projection.push_back(SQLiteHandler::COLUMN_NAME_UPVEL_DISTBAR + mID);
      projection.push_back(SQLiteHandler::COLUMN_NAME_UPVEL_DISTPOINT + mID);
      projection.push_back(SQLiteHandler::COLUMN_NAME_UPVEL_DISTATR + mID);

      // save
      mStream->save(projection, projection.size(), vDistBar, vDistPoint, vDistATR, vRowid);

      // free resources
      clearList();
    }

};

#endif // SUPPORTVELOCITYOBSERVER
