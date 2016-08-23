#include <algorithm>
#include <QDebug>
#include <QSqlRecord>
#include "barbuffer.h"
#include "globals.h"
#include "sqlitehandler.h"

using namespace std;

void insert_react_date_detail (QSqlQuery *q, int last_rowid, const QVariantList &rdate, const QVector <QVariantList> &react_list, bool is_resistance)
{
  QVariantList rid;
  QVariantList react_list_1;
  int M;
  int N = rdate.size();
  int m_rid = last_rowid - rdate.size() + 1;

  for (int i = 0; i < N; ++i)
  {
    M = react_list[i].size();
    for (int j = 0; j < M; ++j)
    {
      rid.push_back (m_rid);
      react_list_1.push_back (react_list[i][j]);
    }
    ++m_rid;
  }

  q->prepare ("insert or ignore into " +
                  (is_resistance ? SQLiteHandler::TABLE_NAME_RESISTANCE_DATE : SQLiteHandler::TABLE_NAME_SUPPORT_DATE) +
              "(rid, react_date) values(?,?)");
  q->addBindValue (rid);
  q->addBindValue (react_list_1);
  q->execBatch();
}

bool desc_compare (const QVariant &a, const QVariant &b)
{
  return a.toDouble() > b.toDouble();
}

void sort_descending_list (QVariantList *v)
{
  std::sort (v->begin(), v->end(), desc_compare);
}

inline int BarBuffer::get_size()
{
  return _date.size();
}

void BarBuffer::initialize (int size)
{
  cache_size = size;
  _datetime.resize (size);
  _date.resize (size);
  _time.resize (size);
  _open.resize (size);
  _high.resize (size);
  _low.resize (size);
  _close.resize (size);
}

void BarBuffer::clear()
{
  _datetime.clear();
  _date.clear();
  _time.clear();
  _open.clear();
  _high.clear();
  _low.clear();
  _close.clear();
}

/*void BarBuffer::ResistanceChecking(const int &index, const double &b, const double &r, const int &scope, const int &threshold,
  std::vector<int> *Doutput,std::vector<int> *NumberOfHit, std::vector<double> *duration) {
  QVector<double> maoc;

  for (int i = 0; i < open.size(); ++i) {
    maoc.push_back(open[i]>close[i]? open[i] : close[i]);
  }

  for (int i = index; (i <= index+scope && i < this->getSize()); ++i) {
    double H_0 = high[i];
    int hits = 0;
    std::vector<int> hd;
    bool breaked = false;
    for (int j = index; j <= i; ++j)
    {
      if (b+H_0-maoc[j] < 0)
        //if(maoc[j]-b-H_0>0)
      {
        breaked = true;
        break;
      }

      //cout<<H_0-r<<" "<<high[j]<<" "<<H_0-r-high[j]<<'\n';

      //if((H_0-r-high[j])==0) cout<<"x"<<'\n';

      if (high[j] > H_0-r) {
        ++hits;
        hd.push_back(j);
      }
      else if((H_0-r-high[j]) == 0) {
        ++hits;
        hd.push_back(j);
      }
    }

    if (hits>=threshold && (!breaked)) {
      bool duplicate = false;
      for (int m = 0; m < Doutput->size(); ++m) {
        if (this->high[i] == this->high[(*Doutput)[m]]) {
          if (hits >= (*NumberOfHit)[m]) {
            (*Doutput)[m] = i;
            (*NumberOfHit)[m] = hits;
            (*duration)[m] = i-hd[0]+1;
            duplicate = true;
          }
        }
      }
      if (!duplicate) {
        Doutput->push_back(i);
        NumberOfHit->push_back(hits);
        duration->push_back(i-hd[0]+1);
      }
           //for(int k=0; k<hd.size(); k++)HittingDate->push_back(hd[k]);


//           if(*(react->end()-1))
//           {
//               qDebug()<<"y";
//           }
//           else
//           {
//               qDebug()<<"n";
//           }

       }
    }
}*/

/*void BarBuffer::SupportChecking(const int &index, const double &b, const double &r, const int &scope, const int &threshold,
  std::vector<int> *Doutput, std::vector<int> *NumberOfHit, std::vector<double> *duration) {
  QVector<double> mioc;

  for (int i = 0; i < open.size(); ++i) {
    mioc.push_back(open[i]<close[i]? open[i] : close[i]);
  }

  for (int i = index;(i <= index+scope && i < this->getSize()); ++i) {
    double L_0 = low[i];
    int hits = 0;
    std::vector<int> hd;
    bool breaked = false;
    for (int j = index; j <= i; ++j) {
      if (L_0-b-mioc[j] > 0){
        breaked = true;
        break;
      }
      if (low[j] < L_0+r){
        ++hits;
        hd.push_back(j);
      }
      else if (L_0+r-low[j] == 0){
        ++hits;
        hd.push_back(j);
      }
    }

    if (hits >= threshold && (!breaked)){
      bool duplicate = false;
      for (int m = 0; m < Doutput->size(); ++m){
        if (this->low[i] == this->low[(*Doutput)[m]]) {
          if (hits >= (*NumberOfHit)[m]){
            (*Doutput)[m] = i;
            (*NumberOfHit)[m] = hits;
            (*duration)[m] = i-hd[0]+1;
            duplicate = true;
          }
        }
      }
      if (!duplicate) {
        Doutput->push_back(i);
        NumberOfHit->push_back(hits);
        duration->push_back(i-hd[0]+1);
      }
    }
  }
}*/

void BarBuffer::resistance_checking_v2 (int index, double breakThreshold, double reactThreshold, int scope, int testNumber,
                                        std::vector <int> *dOutput,
                                        std::vector <int> *numberOfHit,
                                        std::vector <double> *duration,
                                        std::vector <std::vector <int>> *reactDate,
                                        std::vector <int> *tagLine)
{
  std::vector <double> maxPrice; // max(Open,Close)
  int testCount = 0;
  bool breaked;
  bool duplicate;

  for (int i = 0; i < _open.size(); ++i)
  {
    maxPrice.push_back (std::max(_open[i],_close[i]));
  }

  for (int i = index; i <= index + scope && i < get_size(); ++i)
  {
    double firstHigh = _high[i];
    testCount = 0;
    std::vector <int> hd;
    breaked = false;

    for (int j = index; j <= i; ++j)
    {
      // if there's Price that higher than firstHigh then exit loop
      if (breakThreshold + firstHigh < maxPrice[j])
      {
        breaked = true;
        break;
      }

      // TODO: this can be simplified
      if (_high[j] >= firstHigh - reactThreshold)
      {
        ++testCount;
        hd.push_back(j);
      }
      else if (firstHigh - reactThreshold - _high[j] == 0)
      {
        ++testCount;
        hd.push_back(j);
      }
    }

    if (!breaked && testCount >= testNumber)
    {
      duplicate = false;

      for (int m = 0; m < dOutput->size(); ++m)
      {
        if (_high[i] == _high[(*dOutput)[m]] && testCount >= (*numberOfHit)[m])
        {
          (*dOutput)[m] = i;
          (*numberOfHit)[m] = testCount;
          (*reactDate)[m] = hd;
          (*duration)[m] = i - hd[0] + 1;
          duplicate = true;
        }
      }

      if (!duplicate)
      {
        dOutput->push_back(i);
        numberOfHit->push_back(testCount);
        reactDate->push_back(hd);
        duration->push_back(i - hd[0] + 1);
        tagLine->push_back(1);

/*
        // TODO: tagLine for resistace still not really fix
        // if previous reactDate range is subsume of current reactDate range
        // then set previous tagLine to 0

//        if (reactDate->size() > 1) {
//          int previous = reactDate->size() - 2;
//          int current = reactDate->size() - 1;
////          int p_first = (*reactDate)[previous].first();
////          int p_last = (*reactDate)[previous].last();
////          int c_first = (*reactDate)[current].first();
////          int c_last =(*reactDate)[current].last();

//          for (int x = 0; x < (*reactDate)[previous].size(); ++x) {
//            if ((*reactDate)[current].contains((*reactDate)[previous][x])) {
//              (*tagLine)[previous] = 0;
//              break;
//            }
//          }
//        }
*/
      }
    }
  }
}

void BarBuffer::support_checking_v2 (int index, double breakThreshold, double reactThreshold, int scope, int testNumber,
                                     std::vector <int> *dOutput,
                                     std::vector <int> *numberOfHit,
                                     std::vector <double> *duration,
                                     std::vector <std::vector <int>> *reactDate,
                                     std::vector <int> *tagLine)
{
    std::vector <double> minPrice; // min(Open,Close)
//    const double epsilon = 0.0001;
    double firstLow;
    int testCount;
    bool breaked;

    for (int i = 0; i < _open.size(); ++i)
    {
      minPrice.push_back (std::min(_open[i],_close[i])); // XX
    }

    for (int i = index; i <= index + scope && i < get_size(); ++i)
    {
      firstLow = _low[i];
      testCount = 0;
      breaked = false;
      std::vector <int> hd; // reactDate (index)

      for (int j = index; j <= i; ++j)
      {
        // experimental: 2015-07-30
//        if (firstLow - breakThreshold - std::min(open[j],close[j]) > 0) {
//          breaked = true;
//          break;
//        }
//        if (low[j] <= firstLow + reactThreshold ||
//            std::fabs(low[j] - firstLow + reactThreshold) < epsilon) {
//          ++testCount;
//          hd.push_back(j);
//        }

        if (firstLow - breakThreshold - minPrice[j] > 0)
        {
          breaked = true;
          break;
        }

        // TODO: simplified this
        if (_low[j] <= firstLow + reactThreshold)
        {
          ++testCount;
          hd.push_back(j);
        }
        else if (firstLow + reactThreshold - _low[j] == 0)
        {
          ++testCount;
          hd.push_back(j);
        }
      }

      if (testCount >= testNumber && !breaked)
      {
        bool duplicate = false;

        for (int m = 0; m < dOutput->size(); ++m)
        {
          if (_low[i] == _low[ (*dOutput)[m] ])
          {
            if (testCount >= (*numberOfHit)[m])
            {
              (*dOutput)[m] = i;
              (*numberOfHit)[m] = testCount;
              (*reactDate)[m] = hd;
              (*duration)[m] = i - hd[0] + 1;
              duplicate = true;
            }
          }
        }

        if (!duplicate)
        {
          dOutput->push_back(i);
          numberOfHit->push_back(testCount);
          reactDate->push_back(hd);
          duration->push_back(i - hd[0] + 1);
          tagLine->push_back(1);

/*
          // TODO: so far tagLine for support is okay, but need more checking to assure
          // initially, set current as "minimum line"
          // if one of reactdate already exists in previous support line
          // then unset current as "minimum line"

//          if (reactDate->size() > 1) {
//            int previous_index = reactDate->size() - 2;
//            for (int x = 0; x < hd.size(); ++x) {
//              if ((*reactDate)[previous_index].contains(hd[x])) {
//                (*tagLine)[tagLine->size()-1] = 0;
//                break;
//              }
//            }
//          }
*/
        }
      }
    }
}

int BarBuffer::max_react_resistance (int index, double breakThreshhold, double reactThreshold, int scope, int threshold,
                                     QVariantList *v_value, QVariantList *v_date, QVariantList *v_time,
                                     QVariantList *v_rcount, QVariantList *v_duration, QVector <QVariantList>* reactList,
                                     QVariantList *lastDuration, QVariantList *firstDuration, QVariantList *tagLine)
{
  std::vector <int> doutput;
  std::vector <int> number_of_hit;
  std::vector <double> duration;
  std::vector <std::vector <int>> react_date;
  std::vector <int> v_tagLine;

  resistance_checking_v2 (index, breakThreshhold, reactThreshold, scope, threshold, &doutput, &number_of_hit, &duration, &react_date, &v_tagLine);

  int maxReact = 0;
  int doutput_size = static_cast<int> (doutput.size());
  int doutput_idx;
  bool insert_flag = (v_value != 0 && v_date != 0 && v_time != 0);

  for (int i = 0; i < doutput_size; ++i)
  {
    doutput_idx = doutput[i];

    if (insert_flag)
    {
      v_value->push_back (_high[doutput_idx]);
      v_date->push_back (_date[doutput_idx].toString("yyyy-MM-dd"));
      v_time->push_back (_time[doutput_idx].toString("hh:mm"));
      v_rcount->push_back (number_of_hit[i]);
      v_duration->push_back (duration[i]);

      QVariantList rd;
      for (int j = 0; j < react_date[i].size(); ++j)
      {
        rd.push_back (_date[react_date[i][j]]);
      }

      reactList->push_back (rd);
      lastDuration->push_back (react_date[i].front() - index);
      firstDuration->push_back (react_date[i].back() - index);
      tagLine->push_back (v_tagLine[i]);
    }

    if (number_of_hit[i] > maxReact)
    {
      maxReact = number_of_hit[i];
    }
  }

  return maxReact;
}

int BarBuffer::max_react_support (int index, double breakThreshold, double reactThreshold, int scope, int threshold,
                                  QVariantList *v_value, QVariantList *v_date, QVariantList *v_time,
                                  QVariantList *v_rcount, QVariantList *v_duration, QVector <QVariantList>* reactList,
                                  QVariantList *lastDuration, QVariantList *firstDuration, QVariantList *tagLine)
{
  std::vector <int> Doutput;
  std::vector <int> NumberOfHit;
  std::vector <double> duration;
  std::vector <std::vector <int>> React_Date;
  std::vector <int> v_tagLine;

  support_checking_v2 (index, breakThreshold, reactThreshold, scope, threshold, &Doutput, &NumberOfHit, &duration, &React_Date, &v_tagLine);

  int maxReact = 0;
  int doutput_size = static_cast<int> (Doutput.size());
  int doutput_idx;
  bool insert_flag = (v_value != 0 && v_date != 0 && v_time != 0);

  for (int i = 0; i < doutput_size; ++i)
  {
    doutput_idx = Doutput[i];

    if (insert_flag)
    {
      v_value->push_back (_low[doutput_idx]);
      v_date->push_back (_date[doutput_idx].toString("yyyy-MM-dd"));
      v_time->push_back (_time[doutput_idx].toString("hh:mm"));
      v_rcount->push_back (NumberOfHit[i]);
      v_duration->push_back (duration[i]);

      QVariantList rd;
      for (int j = 0; j < React_Date[i].size(); ++j)
      {
        rd.push_back (_date[React_Date[i][j]]);
      }

      reactList->push_back (rd);
      lastDuration->push_back (React_Date[i].front() - index);
      firstDuration->push_back (React_Date[i].back() - index);
      tagLine->push_back (v_tagLine[i]);
    }

    if (NumberOfHit[i] > maxReact)
    {
      maxReact = NumberOfHit[i];
    }
  }

  return maxReact;
}

void BarBuffer::resistance_support_gen (const QSqlDatabase &database, double breakThreshold, double reactThreshold,
                                        int testNumber, int start_rowid_from, int id_threshold)
{
  QSqlQuery q_insert (database);
  QSqlQuery query (database);
  query.setForwardOnly(true);

  QSqlQuery q (database);
  q.setForwardOnly(true);
  q.exec ("select date_, time_, open_, high_, low_, close_ from bardata"
          " where rowid >=" + QString::number(start_rowid_from) +
          " order by date_ desc");

  QVariantList res_value;
  QVariantList res_date;
  QVariantList res_time;
  QVariantList res_reactcount;
  QVariantList res_duration;
  QVariantList res_tagline;
  QVector <QVariantList> res_reactlist;
  QVariantList res_lastduration;
  QVariantList res_firstduration;

  QVariantList sup_value;
  QVariantList sup_date;
  QVariantList sup_time;
  QVariantList sup_reactcount;
  QVariantList sup_duration;
  QVariantList sup_tagline;
  QVector <QVariantList> sup_reactlist;
  QVariantList sup_lastduration;
  QVariantList sup_firstduration;
  QDate d; QTime t;
  int maxreact_support;
  int maxreact_resistance;
  long last_rowid;

  for (int i = 0; i < 250 && q.next(); ++i)
  {
    d = q.value(0).toDate();
    t = q.value(1).toTime();
    _date.push_back(d);
    _time.push_back(t);
    _datetime.push_back(QDateTime(d, t));
    _open.push_back(q.value(2).toDouble());
    _high.push_back(q.value(3).toDouble());
    _low.push_back(q.value(4).toDouble());
    _close.push_back(q.value(5).toDouble());
  }

  q_insert.exec ("PRAGMA temp_store = MEMORY;");
  q_insert.exec ("PRAGMA cache_size = 50000;");
  q_insert.exec ("PRAGMA journal_mode = OFF;");
  q_insert.exec ("PRAGMA synchronous = OFF;");
  q_insert.exec ("BEGIN TRANSACTION;");

  while (_date.size() > 0)
  {
    maxreact_resistance = max_react_resistance
       (0, breakThreshold, reactThreshold, _date.size(), testNumber, &res_value, &res_date, &res_time,
        &res_reactcount, &res_duration, &res_reactlist, &res_lastduration, &res_firstduration, &res_tagline);

    maxreact_support = max_react_support
       (0, breakThreshold, reactThreshold, _date.size(), testNumber, &sup_value, &sup_date, &sup_time,
        &sup_reactcount, &sup_duration, &sup_reactlist, &sup_lastduration, &sup_firstduration, &sup_tagline);

    // resistance
    insert_resistance_detail (&q_insert, _date[0], _time[0], res_date, res_time, res_value, res_reactcount,
                              res_duration, res_lastduration, res_firstduration, id_threshold);

    query.exec ("select rowid from resistancedata order by rowid desc limit 1");
    last_rowid = query.next() ? query.value(0).toInt() : 0;

    insert_react_date_detail (&q_insert, last_rowid, res_date, res_reactlist, true);

    // support
    insert_support_detail (&q_insert, _date[0], _time[0], sup_date, sup_time, sup_value, sup_reactcount,
                           sup_duration, sup_lastduration, sup_firstduration, id_threshold);

    query.exec ("select rowid from supportdata order by rowid desc limit 1");
    last_rowid = query.next() ? query.value(0).toInt() : 0;
    insert_react_date_detail (&q_insert, last_rowid, sup_date, sup_reactlist, false);

    // release resource
    res_value.clear();
    res_date.clear();
    res_time.clear();
    res_reactcount.clear();
    res_duration.clear();
    res_lastduration.clear();
    res_reactlist.clear();
    res_firstduration.clear();
    res_tagline.clear();

    sup_value.clear();
    sup_date.clear();
    sup_time.clear();
    sup_reactcount.clear();
    sup_duration.clear();
    sup_lastduration.clear();
    sup_reactlist.clear();
    sup_firstduration.clear();
    sup_tagline.clear();

    // pop the current bar and load one new bar into memory (rolling data)
    _date.pop_front();
    _time.pop_front();
    _datetime.pop_front();
    _open.pop_front();
    _high.pop_front();
    _low.pop_front();
    _close.pop_front();

    if (q.next())
    {
      d = q.value(0).toDate();
      t = q.value(1).toTime();
      _date.push_back(d);
      _time.push_back(t);
      _datetime.push_back (QDateTime(d, t));
      _open.push_back (q.value(2).toDouble());
      _high.push_back (q.value(3).toDouble());
      _low.push_back (q.value(4).toDouble());
      _close.push_back (q.value(5).toDouble());
    }
  }

  q_insert.exec("COMMIT");
}

void BarBuffer::resistance_support_gen_dbupdater (const QSqlDatabase &database, double break_threshold, double react_threshold,
                                                  int testNumber, int start_rowid, int id_threshold)
{
  QSqlQuery q_insert(database);
  QSqlQuery q_select(database);
  q_select.setForwardOnly(true);

  QSqlQuery q(database);
  q.setForwardOnly(true);
  q.exec("select rowid, date_, time_, open_, high_, low_, close_ from bardata"
         " where rowid >=" + QString::number(start_rowid - 250) + " and completed in(1,2)"
         " order by date_ desc");

  QVariantList res_value;
  QVariantList res_date;
  QVariantList res_time;
  QVariantList res_reactcount;
  QVariantList res_duration;
  QVariantList res_tagline;
  QVector<QVariantList> res_reactlist;
  QVariantList res_lastduration;
  QVariantList res_firstduration;

  QVariantList sup_value;
  QVariantList sup_date;
  QVariantList sup_time;
  QVariantList sup_reactcount;
  QVariantList sup_duration;
  QVariantList sup_tagline;
  QVector<QVariantList> sup_reactlist;
  QVariantList sup_lastduration;
  QVariantList sup_firstduration;
  QDate d; QTime t;
  int maxreact_support;
  int maxreact_resistance;
  long last_rowid;

  QVector<int> _rowid;

  for (int i = 0; i < 250 && q.next(); ++i)
  {
    _rowid.push_back(q.value(0).toInt());
    d = q.value(1).toDate();
    t = q.value(2).toTime();
    _date.push_back(d);
    _time.push_back(t);
    _datetime.push_back(QDateTime(d, t));
    _open.push_back(q.value(3).toDouble());
    _high.push_back(q.value(4).toDouble());
    _low.push_back(q.value(5).toDouble());
    _close.push_back(q.value(6).toDouble());
  }

  q_insert.exec("PRAGMA temp_store = MEMORY;");
  q_insert.exec("PRAGMA cache_size = 50000;");
  q_insert.exec("PRAGMA journal_mode = OFF;");
  q_insert.exec("PRAGMA synchronous = OFF;");
  q_insert.exec("BEGIN TRANSACTION;");

  while (!_rowid.isEmpty() && _rowid.front() > start_rowid)
  {
    maxreact_resistance = max_react_resistance
       (0, break_threshold, react_threshold, _date.size(), testNumber, &res_value, &res_date, &res_time,
        &res_reactcount, &res_duration, &res_reactlist, &res_lastduration, &res_firstduration, &res_tagline);

    maxreact_support = max_react_support
       (0, break_threshold, react_threshold, _date.size(), testNumber, &sup_value, &sup_date, &sup_time,
        &sup_reactcount, &sup_duration, &sup_reactlist, &sup_lastduration, &sup_firstduration, &sup_tagline);

    // resistance
    insert_resistance_detail (&q_insert, _date[0], _time[0], res_date, res_time, res_value, res_reactcount,
                              res_duration, res_lastduration, res_firstduration, id_threshold);

    q_select.exec("select rowid from resistancedata order by rowid desc limit 1");
    last_rowid = q_select.next() ? q_select.value(0).toInt() : 0;

    insert_react_date_detail (&q_insert, last_rowid, res_date, res_reactlist, true);

    // support
    insert_support_detail (&q_insert, _date[0], _time[0], sup_date, sup_time, sup_value, sup_reactcount,
                           sup_duration, sup_lastduration, sup_firstduration, id_threshold);

    q_select.exec("select rowid from supportdata order by rowid desc limit 1");
    last_rowid = q_select.next() ? q_select.value(0).toInt() : 0;
    insert_react_date_detail (&q_insert, last_rowid, sup_date, sup_reactlist, false);

    // release resource
    res_value.clear();
    res_date.clear();
    res_time.clear();
    res_reactcount.clear();
    res_duration.clear();
    res_lastduration.clear();
    res_reactlist.clear();
    res_firstduration.clear();
    res_tagline.clear();

    sup_value.clear();
    sup_date.clear();
    sup_time.clear();
    sup_reactcount.clear();
    sup_duration.clear();
    sup_lastduration.clear();
    sup_reactlist.clear();
    sup_firstduration.clear();
    sup_tagline.clear();

    // pop the current bar and load one new bar into memory (rolling data)
    _rowid.pop_front();
    _date.pop_front();
    _time.pop_front();
    _datetime.pop_front();
    _open.pop_front();
    _high.pop_front();
    _low.pop_front();
    _close.pop_front();

    if (q.next())
    {
      _rowid.push_back(q.value(0).toInt());
      d = q.value(1).toDate();
      t = q.value(2).toTime();
      _date.push_back(d);
      _time.push_back(t);
      _datetime.push_back(QDateTime(d, t));
      _open.push_back(q.value(3).toDouble());
      _high.push_back(q.value(4).toDouble());
      _low.push_back(q.value(5).toDouble());
      _close.push_back(q.value(6).toDouble());
    }
  }

  q_insert.exec("COMMIT");
}


void BarBuffer::resistance_support_recalculate (const QSqlDatabase &database, double b, double r, int threshold, int id_threshold)
{
  QSqlQuery qLast (database);
  qLast.setForwardOnly (true);

  // remove old threshold
  qLast.exec ("PRAGMA journal_mode = OFF;");
  qLast.exec ("PRAGMA synchronous = OFF;");
  qLast.exec ("BEGIN TRANSACTION;");
  qLast.exec ("delete from " + SQLiteHandler::TABLE_NAME_RESISTANCE + " where id_threshold=" + QString::number (id_threshold));
  qLast.exec ("delete from " + SQLiteHandler::TABLE_NAME_SUPPORT + " where id_threshold=" + QString::number (id_threshold));
  qLast.exec ("COMMIT;");

  // if breakpoint is zero, then quit (for delete particular threshold value)
  if (b <= 0) return;

  resistance_support_gen (database, b, r, threshold, 0, id_threshold);
}

void BarBuffer::insert_resistance_detail (QSqlQuery *query, const QDate &date_, const QTime &time_,
                                          const QVariantList &rdate, const QVariantList &rtime,
                                          const QVariantList &v_value, const QVariantList &v_rcount, const QVariantList &v_duration,
                                          const QVariantList &v_last_duration, const QVariantList &v_first_duration, int id_threshold)
{
  QString s_date = date_.toString("yyyy-MM-dd");
  QString s_time = time_.toString("hh:mm");
  QVariantList _date;
  QVariantList _time;
  QVariantList _index;
  const int N = rdate.size();

  for (int i = 0; i < N; ++i)
  {
    _index.push_back (id_threshold);
    _date.push_back (s_date);
    _time.push_back (s_time);
  }

  query->prepare (SQLiteHandler::SQL_INSERT_RESISTANCE_V1);
  query->addBindValue (_date);
  query->addBindValue (_time);
  query->addBindValue (rdate);
  query->addBindValue (rtime);
  query->addBindValue (v_rcount);
  query->addBindValue (v_value);
  query->addBindValue (v_duration);
  query->addBindValue (v_last_duration);
  query->addBindValue (v_first_duration);
  query->addBindValue (_index);
  query->execBatch();
}

void BarBuffer::insert_support_detail (QSqlQuery *query, const QDate &date_, const QTime &time_,
                                       const QVariantList &reactDate, const QVariantList &reactTime,
                                       const QVariantList &v_value, const QVariantList &reactCount, const QVariantList &duration,
                                       const QVariantList &lastDuration, const QVariantList &firstDuration, int id_threshold)
{
  QString s_date = date_.toString("yyyy-MM-dd");
  QString s_time = time_.toString("hh:mm");
  QVariantList _date;
  QVariantList _time;
  QVariantList _index;
  const int N = reactDate.size();

  // repeat date, time, id_threshold for n-rows
  for (int i = 0; i < N; ++i)
  {
    _index.push_back (id_threshold);
    _date.push_back (s_date);
    _time.push_back (s_time);
  }

  query->prepare (SQLiteHandler::SQL_INSERT_SUPPORT_V1);
  query->addBindValue (_date);
  query->addBindValue (_time);
  query->addBindValue (reactDate);
  query->addBindValue (reactTime);
  query->addBindValue (reactCount);
  query->addBindValue (v_value);
  query->addBindValue (duration);
  query->addBindValue (lastDuration);
  query->addBindValue (firstDuration);
  query->addBindValue (_index);
  query->execBatch();
}

// @deprecated: denormalize version
/*void BarBuffer::sqlite_insert_resistance_wtf(QSqlQuery *query, const int &column_count,
    const QDate &date_, const QTime &time_, const QVariantList &v_value, const int &threshold_index) {

  // if values is empty then exit
  if (v_value.length() == 0 || query == NULL) {
    return;
  }

  const QString s_index = QString::number(threshold_index);
  const QString s_date = date_.toString("yyyy-MM-dd");
  const QString s_time = time_.toString("hh:mm");
  const int N = v_value.length();

  // alter column
  if (column_count < N) {
    int last_id = column_count-1; // to zero-based index
    int new_column_number = N - column_count;
    for (int i = 1; i <= new_column_number; ++i) {
      query->exec("alter table " + SQLiteHandler::TABLE_NAME_RESISTANCE_TAB +
                 " add column v" + QString::number(last_id+i) + " REAL;");
    }
  }

  // build column projection
  QString join_column = "";
  QString join_value = "";
  for (int i = 0; i < N; ++i) {
    if (i > 0) {
      join_column += ",";
      join_value += ",";
    }
    join_column += "v" + QString::number(i);
    join_value += v_value[i].toString();
  }

  QString sql_insert =
    "insert into " + SQLiteHandler::TABLE_NAME_RESISTANCE_TAB + "(index_,date_,time_,column_count," + join_column + ")" +
    " values(" + s_index + ",'" + s_date + "','" + s_time + "'," + QString::number(N) + "," + join_value + ")";

  // execute insert, commit later
  query->exec(sql_insert);

  if (query->lastError().isValid()) {
    qDebug() << query->lastError();
    qDebug() << sql_insert;
  }
}*/

// @deprecated: denormalize version
/*void BarBuffer::sqlite_insert_support_wtf(QSqlQuery *query, const int &column_count,
    const QDate &date_, const QTime &time_, const QVariantList &v_value, const int &threshold_index) {

  // if values is empty then exit
  if (v_value.length() == 0 || query == NULL) {
    return;
  }

  const QString s_index = QString::number(threshold_index);
  const QString s_date = date_.toString("yyyy-MM-dd");
  const QString s_time = time_.toString("hh:mm");
  const int N = v_value.length();

  // alter column
  if (column_count < N) {
    int last_id = column_count-1; // to zero-based index
    int new_column_number = N - column_count;
    for (int i = 1; i <= new_column_number; ++i) {
      query->exec("alter table " + SQLiteHandler::TABLE_NAME_SUPPORT_TAB +
                 " add column v" + QString::number(last_id+i) + " REAL;");
    }
  }

  // build column projection
  QString join_column = "";
  QString join_value = "";
  for (int i = 0; i < N; ++i) {
    if (i > 0) {
      join_column += ",";
      join_value += ",";
    }
    join_column += "v" + QString::number(i);
    join_value += v_value[i].toString();
  }

  QString sql_insert =
    "insert into " + SQLiteHandler::TABLE_NAME_SUPPORT_TAB + "(index_,date_,time_,column_count," + join_column + ")" +
    " values(" + s_index + ",'" + s_date + "','" + s_time + "'," + QString::number(N) + "," + join_value + ")";

  // execute insert, commit later
  query->exec(sql_insert);

  if (query->lastError().isValid()) {
    qDebug() << query->lastError();
    qDebug() << sql_insert;
  }
}*/
