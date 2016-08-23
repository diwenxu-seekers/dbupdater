/**
 * SupportTagline.h
 * ----------------
 * Alternative to find prime set of SupportLine.
 * Because we only perform upto Daily data which is around 4000 rows,
 * We use standard QVector. Later on, if data is large then we change the algorithm.
 **/
#ifndef SUPPORT_TAGLINE_H
#define SUPPORT_TAGLINE_H

#include "sqlitehandler.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVector>

class SupportTagline {
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

      m_db = database;
    }

    void calculate (int id_threshold, int period = 250, double breakThreshold = 1, double reactThreshold = 1, int testNumber = 2)
    {
      QSqlQuery q(m_db);
      QString update_db =
        "update supportdata set tagline2=1"
        " where date_=? and time_=? and support=? and id_threshold=" + QString::number(id_threshold);

      q.exec("PRAGMA journal_mode = OFF;");
      q.exec("PRAGMA synchronous = OFF;");
      q.exec("BEGIN TRANSACTION;");
      q.prepare(update_db);
      int rowcount = m_date.length() - period;

      for (int i = 0; i < rowcount; ++i)
      {
        calculate_support_tagline (&q, i, period, breakThreshold, reactThreshold, testNumber);
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

    void calculate_support_tagline (QSqlQuery *query, const int &barIndex, int period = 250, double breakThreshold = 1, double reactThreshold = 1, int testNumber = 2)
    {
      QDate d1, d2;
      QTime t1, t2;
      double firstLow, priceCap;
      double p1 = 0, p2 = 0; // price
      int bb1 = 0; // bar back
      int bb2 = 0;
      int testCount, lastTestBarBack = 0;
      QVector<QDate> oDate;
      QVector<QTime> oTime;
      QVector<double> oFirstLow;
      QString lastBarDate = m_date[barIndex].toString("yyyy-MM-dd");
      QString lastBarTime = m_time[barIndex].toString("hh:mm");

//      qDebug() << "Test Date (Support):" << lastBarDate << lastBarTime;

      for (int totalBar = period; totalBar >= 1; --totalBar)
      {
        minprice_among_bar(barIndex, m_date, m_time, m_open, m_close, totalBar, &d1, &t1, &p1, &bb1);
        priceCap = p1 + breakThreshold;

        first_low_below_price(barIndex, m_date, m_time, m_low, priceCap, totalBar, &d2, &t2, &p2, &bb2);
        firstLow = p2;
        testCount = 0;

//        if (barIndex == 2966)
//          qDebug() << "priceCap" << priceCap
//                   << "firstLow" << firstLow
//                   << "barBack" << bb2
//                   << "(" << totalBar << ")";

        for (int barBack = bb2; barBack >= 0; --barBack)
        {
          if (m_low[barIndex + barBack] <= firstLow + reactThreshold)
          {
            ++testCount;
            lastTestBarBack = barBack;
          }
        }

        if (testCount >= testNumber)
        {
          for (int barBack = bb2; barBack >= 0; --barBack)
          {
            if (m_low[barIndex + barBack] <= firstLow + reactThreshold)
            {
//              if (barIndex == 2966)
//              qDebug() << "Results::" << barBack
//                       << m_date[barIndex + barBack]
//                       << m_time[barIndex + barBack]
//                       << firstLow;

              oDate.push_back(m_date[barIndex + barBack]); // react date
              oTime.push_back(m_time[barIndex + barBack]); // react time
              oFirstLow.push_back(firstLow);

              query->bindValue(0, lastBarDate);
              query->bindValue(1, lastBarTime);
              query->bindValue(2, firstLow);
              query->exec();
            }
          }
        }

//        if (barIndex == 2966) {
//          qDebug() << "-------------------------------totalBar" << totalBar << lastTestBarBack;
//        }

        totalBar = (totalBar != lastTestBarBack)? lastTestBarBack + 1 : lastTestBarBack;
        oDate.clear();
        oTime.clear();
        oFirstLow.clear();
      }
    }

    void minprice_among_bar (const int &barIndex,
                             const QVector<QDate> &iDate, const QVector<QTime> &iTime,
                             const QVector<double> &iOpen, const QVector<double> &iClose,
                             const int &iTotalBar,
                             QDate *oDate, QTime *oTime, double *oPrice, int *oBarBack)
    {
      double minPrice;
      int vBarBack = iTotalBar - 1;
      int idx = barIndex + vBarBack;
      *oDate = iDate[idx];
      *oTime = iTime[idx];
      *oPrice = std::fmin(iOpen[idx], iClose[idx]);
      *oBarBack = vBarBack;

      for (vBarBack = iTotalBar - 2; vBarBack >= 0; --vBarBack)
      {
        idx = barIndex + vBarBack;
        minPrice = std::fmin(iOpen[idx], iClose[idx]);

        if (minPrice < *oPrice)
        {
          *oDate = iDate[idx];
          *oTime = iTime[idx];
          *oPrice = minPrice;
          *oBarBack = vBarBack;
        }
      }
    }

    void first_low_below_price (const int &barIndex,
                                const QVector<QDate> &iDate, const QVector<QTime> &iTime,
                                const QVector<double> &iLow,
                                const double &iPriceCap, const int &iTotalBar,
                                QDate *oDate, QTime *oTime, double *oLow, int *oBarBack)
    {
      int idx;
      for (int vBarBack = iTotalBar - 1; vBarBack >= 0; --vBarBack)
      {
        idx = barIndex + vBarBack;
        if (iLow[idx] <= iPriceCap)
        {
          *oDate = iDate[idx];
          *oTime = iTime[idx];
          *oLow = iLow[idx];
          *oBarBack = vBarBack;
          vBarBack = 0;
        }
      }
    }
};

#endif // SUPPORT_TAGLINE_H
