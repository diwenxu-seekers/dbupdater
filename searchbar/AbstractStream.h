// Base class for input stream in StreamProcessor.h
// Input source could be from Text or SqlQuery.
// This class intended to be interface (pure abstract class) for Stream class.

#ifndef ABSTRACTSTREAM
#define ABSTRACTSTREAM

#include <QDate>
#include <QTime>
#include <QStringList>
#include <QVariant>

#include <cstdarg>

class AbstractStream {
  public:
    virtual bool readLine() = 0;
    virtual bool atEnd() = 0;
    virtual void save(const QStringList &projection, int N, ...) = 0;

    // getter shared attribute
    virtual QVariant getValueAt(int index) = 0;
    virtual QDate getDate() = 0;
    virtual QTime getTime() = 0;
    virtual long int getRowid() = 0;
    virtual QString getDateString() { return ""; }
    virtual QString getTimeString() { return ""; }

    // getter for Bardata
    virtual double getOpen() { return 0; }
    virtual double getHigh() { return 0; }
    virtual double getLow() { return 0; }
    virtual double getClose() { return 0; }
    virtual double getMAFast() { return 0; }
    virtual double getMASlow() { return 0; }
    virtual double getMACD() { return 0; }
    virtual double getRSI() { return 0; }
    virtual double getSlowK() { return 0; }
    virtual double getSlowD() { return 0; }
    virtual double getATR() { return 0; }
    virtual double getPrevDailyATR() { return 0; }
    virtual long int getParent() { return 0; }
    virtual long int getParentPrev() { return 0; }

    // getter for Support/Resistance
    virtual double getResistancePrice() { return 0; }
    virtual double getSupportPrice() { return 0; }
    virtual QDate getFirstReactDate() { return QDate(); }
    virtual QTime getFirstReactTime() { return QTime(); }
    virtual QDate getLastReactDate() { return QDate(); }
    virtual QTime getLastReactTime() { return QTime(); }
    virtual int getTestNumber() { return 0; }
    virtual int getDuration() { return 0; }
    virtual double getDistPoint() { return 0; }
    virtual double getDistATR() { return 0; }
};

#endif // ABSTRACTSTREAM
