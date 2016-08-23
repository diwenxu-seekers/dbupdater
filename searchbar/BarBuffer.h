#ifndef BARBUFFER_H
#define BARBUFFER_H

#include <queue>
#include <vector>
#include <QDate>
#include <QDateTime>
#include <QSqlQuery>
#include <QTime>
#include <QVariant>
#include <QVariantList>
#include <QVector>

class BarBuffer {
  public:
    void clear();

    int get_size();

    void initialize (int size);

    void support_checking_v2 (int index, double breakThreshold, double reactThreshold, int scope, int testNumber,
                              std::vector<int> *dOutput, std::vector<int> *numberOfHit, std::vector<double> *duration,
                              std::vector<std::vector<int>> *reactDate, std::vector<int> *tagLine);

    void resistance_checking_v2 (int index, double breakThreshold, double reactThreshold, int scope, int testNumber,
                                 std::vector<int> *dOutput, std::vector<int> *numberOfHit, std::vector<double> *duration,
                                 std::vector<std::vector<int>> *reactDate, std::vector<int> *tagLine);

    int max_react_support (int index, double breakThreshold, double reactThreshold, int scope, int threshold,
                           QVariantList *v_value, QVariantList *v_date, QVariantList *v_time, QVariantList *v_rcount, QVariantList *v_duration,
                           QVector <QVariantList>* reactList, QVariantList* lastDuration, QVariantList *firstDuration, QVariantList *tagLine);

    int max_react_resistance (int index, double breakThreshhold, double reactThreshold, int scope, int threshold,
                              QVariantList *v_value, QVariantList *v_date, QVariantList *v_time, QVariantList *v_rcount, QVariantList *v_duration,
                              QVector<QVariantList>* reactList, QVariantList* lastDuration, QVariantList *firstDuration, QVariantList *tagLine);

    void resistance_support_gen (const QSqlDatabase &db, double breakThreshold, double reactThreshold,
                                 int testNumber, int start_rowid_from, int id_threshold);

    // since dbupdater v1.4.1
    // original function may conflict old support/resistance due to incorrect start rowid position
    // may distrupt reactdate list insertion thus calculation of dist resistance and dist support may incorrect
    // this new function aim to resolve the issue
    void resistance_support_gen_dbupdater (const QSqlDatabase &db, double break_threshold, double react_threshold,
                                           int testNumber, int start_rowid, int id_threshold);

    void resistance_support_recalculate (const QSqlDatabase &database, double b, double r, int threshold, int id_threshold);

    void insert_resistance_detail (QSqlQuery *query, const QDate &date_, const QTime &time_,
                                   const QVariantList &rdate, const QVariantList &rtime, const QVariantList &v_value,
                                   const QVariantList &v_rcount, const QVariantList &v_duration, const QVariantList &v_last_duration,
                                   const QVariantList &v_first_duration, int id_threshold);

    void insert_support_detail (QSqlQuery *query, const QDate &date_, const QTime &time_,
                                const QVariantList &reactDate, const QVariantList &reactTime, const QVariantList &v_value,
                                const QVariantList &reactCount, const QVariantList &duration, const QVariantList &lastDuration,
                                const QVariantList &firstDuration, int id_threshold);

  private:
    QVector<QDateTime> _datetime;
    QVector<QDate> _date;
    QVector<QTime> _time;
    QVector<double> _open;
    QVector<double> _close;
    QVector<double> _high;
    QVector<double> _low;
    int cache_size;
};

#endif // BARBUFFER_H
