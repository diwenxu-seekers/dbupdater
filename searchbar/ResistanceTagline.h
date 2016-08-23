/**
 * ResistanceTagline.h
 * ----------------
 * Alternative to find prime set of ResistanceLine.
 * Because we only perform upto Daily data which is around 4000 rows,
 * We use standard QVector. Later on, if data is large then we change the algorithm.
 **/
#ifndef RESISTANCE_TAGLINE_H
#define RESISTANCE_TAGLINE_H

#include "sqlitehandler.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVector>

class ResistanceTagline {
  public :
    void prepare_data (const QSqlDatabase &database, int start_rowid = 0, int period = 250)
    {
      QSqlQuery q(database);
      QString _rowid_condition = "";

      if (start_rowid > 0)
      {
        _rowid_condition = " where rowid >=" + QString::number(start_rowid - period) + " and completed=1";
      }

      QString sql_select =
        "select date_, time_, open_, high_, low_, close_"
        " from bardata " + _rowid_condition +
        " order by rowid desc";

      q.setForwardOnly(true);
      q.exec(sql_select);

      while (q.next())
      {
       m_date.push_back(QDate::fromString(q.value(0).toString(), "yyyy-MM-dd"));
       m_time.push_back(QTime::fromString(q.value(1).toString(), "hh:mm"));
       m_open.push_back(q.value(2).toDouble());
       m_high.push_back(q.value(3).toDouble());
       m_low.push_back(q.value(4).toDouble());
       m_close.push_back(q.value(5).toDouble());
      }

      this->m_db = database;
    }

    void calculate (int id_threshold, int period = 250, double breakThreshold = 1, double reactThreshold = 1, int testNumber = 2)
    {
      QSqlQuery q(m_db);
      q.exec("PRAGMA journal_mode = OFF;");
      q.exec("PRAGMA synchronous = OFF;");
      q.exec("BEGIN TRANSACTION;");
      q.prepare("update resistancedata set tagline2=1"
                " where date_=? and time_=? and resistance=? and"
                " id_threshold=" + QString::number(id_threshold));

      int N = m_date.length() - period;

      for (int i = 0; i < N; ++i)
      {
        calculate_resistance_tagline (&q, i, period, breakThreshold, reactThreshold, testNumber);
      }

      q.exec("COMMIT;");
    }

  private:
    QSqlDatabase m_db;
    QVector<QDate> m_date;
    QVector<QTime> m_time;
    QVector<double> m_open;
    QVector<double> m_high;
    QVector<double> m_close;
    QVector<double> m_low;

    void calculate_resistance_tagline (QSqlQuery *query, const int &barIndex, int period = 250, double breakThreshold = 1, double reactThreshold = 1, int testNumber = 2)
    {
      QDate d1, d2;
      QTime t1, t2;
      double p1 = 0, p2 = 0; // price
      int bb1 = 0; // bar back
      int bb2 = 0;
      double firstHigh, priceFloor;
      int testCount, lastTestBarBack = 0;
      QVector<QDate> oDate;
      QVector<QTime> oTime;
      QVector<double> oFirstHigh;
      QString lastBarDate = m_date[barIndex].toString("yyyy-MM-dd");
      QString lastBarTime = m_time[barIndex].toString("hh:mm");

//      qDebug() << "Test Date (Resistance):" << lastBarDate << lastBarTime;

      for (int totalBar = period; totalBar >= 1; --totalBar)
      {
        maxprice_among_bar(barIndex, m_date, m_time, m_open, m_close, totalBar, &d1, &t1, &p1, &bb1);
        priceFloor = p1 - breakThreshold;

        first_high_above_price(barIndex, m_date, m_time, m_high, priceFloor, totalBar, &d2, &t2, &p2, &bb2);
        firstHigh = p2;
        testCount = 0;

        for (int barBack = bb2; barBack >= 0; --barBack)
        {
          if (m_high[barIndex + barBack] >= firstHigh - reactThreshold)
          {
            ++testCount;
            lastTestBarBack = barBack;
          }
        }

        if (testCount >= testNumber)
        {
          for (int barBack = bb2; barBack >= 0; --barBack)
          {
            if (m_high[barIndex + barBack] >= firstHigh - reactThreshold)
            {
//              qDebug() << "Result::" << barBack
//                       << m_date[barIndex + barBack]
//                       << m_time[barIndex + barBack]
//                       << firstHigh;

              oDate.push_back(m_date[barIndex + barBack]); // react date
              oTime.push_back(m_time[barIndex + barBack]); // react time
              oFirstHigh.push_back(firstHigh);

              query->bindValue(0, lastBarDate);
              query->bindValue(1, lastBarTime);
              query->bindValue(2, firstHigh);
              query->exec();
            }
          }
        }

//        qDebug() << "-------------------------------totalBar" << totalBar << lastTestBarBack;
        totalBar = lastTestBarBack + 1;
        oDate.clear();
        oTime.clear();
        oFirstHigh.clear();
      }
    }

    void maxprice_among_bar (const int &barIndex,
                             const QVector<QDate> &iDate, const QVector<QTime> &iTime,
                             const QVector<double> &iOpen, const QVector<double> &iClose,
                             const int &iTotalBar,
                             QDate *oDate, QTime *oTime, double *oPrice, int *oBarBack)
    {
      double max_price;
      int vBarBack = iTotalBar - 1;
      int idx = barIndex + vBarBack;
      *oDate = iDate[idx];
      *oTime = iTime[idx];
      *oPrice = std::max(iOpen[idx],iClose[idx]);
      *oBarBack = vBarBack;

      for (vBarBack = iTotalBar - 2; vBarBack >= 0; --vBarBack)
      {
        idx = barIndex + vBarBack;
        max_price = std::max(iOpen[idx],iClose[idx]);

        if (max_price > *oPrice)
        {
          *oDate = iDate[idx];
          *oTime = iTime[idx];
          *oPrice = max_price;
          *oBarBack = vBarBack;
        }
      }
    }

    void first_high_above_price (const int &barIndex,
                                 const QVector<QDate> &iDate, const QVector<QTime> &iTime,
                                 const QVector<double> &iHigh,
                                 const double &iPriceFloor, const int &iTotalBar,
                                 QDate *oDate, QTime *oTime, double *oHigh, int *oBarBack)
    {
      int idx;
      for (int vBarBack = iTotalBar - 1; vBarBack >= 0; --vBarBack)
      {
        idx = barIndex + vBarBack;
        if (iHigh[idx] >= iPriceFloor)
        {
          *oDate = iDate[idx];
          *oTime = iTime[idx];
          *oHigh = iHigh[idx];
          *oBarBack = vBarBack;
          vBarBack = 0;
        }
      }
    }
};

#endif // RESISTANCE_TAGLINE_H
