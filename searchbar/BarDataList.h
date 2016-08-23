/**
 * BardataList.h
 * Used in case when you need to fetch query result into local variable.
 * Typical usage in Matcher application where they need to iterate all records data.
 */
#ifndef BARDATALIST_H
#define BARDATALIST_H

#include <QDate>
#include <QTime>
#include <QString>
#include <QVector>

#define MAX_ELEMENT   5000000
#define PAGE_SIZE     200000

// implement bardata tuple
// please note: column is not ordered as in database schema
typedef struct {
  long rowid;
  QDate date;
  QTime time;
  QString prevbarcolor;
  double open;
  double high;
  double low;
  double close;
  long volume;
  double macd;
  double macdavg;
  double macddiff;
  double rsi;
  double slowk;
  double slowd;
  double fastavg;
  double slowavg;
  double distf;
  double dists;
  double distfs;
  double atr;
  double prev_atr;
  int fgs; // F > S
  int fls; // F < S
  int cgf; // C > F
  int clf; // C < F
  int cgs; // C > S
  int cls; // C < S
  int hlf;
  int hls;
  int lgf;
  int lgs;
  int macd_g0; // MACD > 0
  int macd_l0; // MACD < 0
  int rsi_g70; // RSI > 70
  int rsi_l30; // RSI < 30
  int slowk_g80; // SlowK > 80
  int slowk_l20; // SlowK < 20
  int slowd_g80; // SlowD > 80
  int slowd_l20; // SlowD < 20
  long _parent;
  long _parent_weekly;
  long _parent_monthly;
  long _parent_prev;
  long _parent_prev_weekly;
  long _parent_prev_monthly;
  bool openbar;

  // custom value (use these columns for any other value)
  double value1;
  double value2;
  double value3;
  double value4;
  double value5;
  QString s_date;
  QString s_time;
} t_bardata;

class BarDataList {
  public:
    BarDataList() {
      mylist = new QVector<t_bardata*>(CURRENT_ALLOC);
      for (long i = 0; i < CURRENT_ALLOC; ++i) {
        (*mylist)[i] = new t_bardata;
      }
    }

    ~BarDataList() {
      for (long i = 0; i < CURRENT_ALLOC; ++i) {
        delete (*mylist)[i];
      }
      delete mylist;
    }

    void clear() {
      for (long i = 0; i < CURRENT_ALLOC; ++i) {
        delete (*mylist)[i];
        (*mylist)[i] = 0;
      }
      mylist->clear();
      size = 0;
    }

    long length() const {
      return size;
    }

    void removeAt(const int &index) {
      if (index > -1 && index < size) {
        delete (*mylist)[index];
        mylist->removeAt(index);
        --size;
      }
    }

    // similar usage to cursor in QSqlQuery
    t_bardata* getNextItem() {
      if (size >= CURRENT_ALLOC) {
        // resize the length
        CURRENT_ALLOC += PAGE_SIZE;
        mylist->resize(mylist->size() + PAGE_SIZE);

        // realloc heap
        for (long i = size; i < CURRENT_ALLOC; ++i) {
          (*mylist)[i] = new t_bardata;
        }
      }
      ++size;
      return mylist->at(size-1);
    }

    t_bardata* operator[](const int &index) const {
      if (index < 0 || index > size) return 0;
      return (*mylist)[index];
    }

    // we assume distf in list already sorted in ascending order
    /*long binary_search_distf(const double &distf) {
      int low = 0;
      int mid = 0;
      int high = size;

      while (low < high) {
        mid = (low + high) >> 1;
        if ((*mylist)[mid]->distf > distf) high = mid - 1;
        else if ((*mylist)[mid]->distf < distf) low = mid + 1;
        else break;
      }

//      qDebug() << "distf0" << mid << low << high;
      if ((*mylist)[mid]->distf >= distf) {
        while (mid > 0 && (*mylist)[mid]->distf >= distf)
          --mid;
      }

//      qDebug() << "distf1" << mid << low << high;
      if ((*mylist)[mid]->distf < distf) {
        while (mid < size && (*mylist)[mid]->distf < distf)
          ++mid;
      }

//      qDebug() << "distf" << mid << low << high;
      return mid;
    }*/

    // we assume dists in list already sorted in ascending order
    /*long binary_search_dists(const double &dists) {
      int low = 0;
      int mid = 0;
      int high = size;

      while (low < high) {
        mid = (low + high) >> 1;
        if ((*mylist)[mid]->dists > dists) high = mid - 1;
        else if ((*mylist)[mid]->dists < dists) low = mid + 1;
        else break;
      }

//      qDebug() << "dists0" << mid << low << high;
      if ((*mylist)[mid]->dists >= dists) {
        while (mid > 0 && (*mylist)[mid]->dists >= dists)
          --mid;
      }

//      qDebug() << "dists1" << mid << low << high;
      if ((*mylist)[mid]->dists < dists) {
        while (mid < size && (*mylist)[mid]->dists < dists)
          ++mid;
      }

//      qDebug() << "dists" << mid << low << high;
      return mid;
    }*/

  private:
    QVector<t_bardata *> *mylist = 0;
    long CURRENT_ALLOC = 1000000; // initial allocated space
    long size = 0; // actual size
};

#endif // BARDATALIST_H
