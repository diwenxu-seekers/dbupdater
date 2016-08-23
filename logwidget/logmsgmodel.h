#ifndef LOGMSGMODEL_H
#define LOGMSGMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>


class LogMsgModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // Log level
    typedef enum
    {
        trace = 0,
        debug = 1,
        info = 2,
        notice = 3,
        warn = 4,
        err = 5,
        critical = 6,
        alert = 7,
        emerg = 8,
        off = 9
    } LogLevel;

    // Log message
    typedef struct
    {
        LogLevel level;
        QDateTime time;
        QString msg;

        static int fieldCount() { return 3; }
    } LogMsg;

    LogMsgModel(QObject* parent = 0);

    // The following functions must be implemented for table model
    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

    // The following functions have to be privided for the table model to be editable
    bool insertRows(int row, int count, const QModelIndex &parent) Q_DECL_OVERRIDE;
    bool removeRows(int row, int count, const QModelIndex &parent) Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    QList<LogMsg> _lstMsgs;              // Keep all log messages
};

Q_DECLARE_METATYPE(LogMsgModel::LogLevel)

#endif // LOGMSGMODEL_H
