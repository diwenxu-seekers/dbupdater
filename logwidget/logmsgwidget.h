#ifndef LOGMSGWIDGET_H
#define LOGMSGWIDGET_H

#include "logmsgmodel.h"
#include <QWidget>
#include <QSortFilterProxyModel>
#include <QEvent>


class LogMsgWidget : public QWidget
{
    Q_OBJECT

public:
    static const QEvent::Type LOG_MSG_EVENT = static_cast<QEvent::Type>(QEvent::User + 1);
    static const QEvent::Type CLEAR_MSG_EVENT = static_cast<QEvent::Type>(QEvent::User + 2);

    class LogMsgEvent : public QEvent
    {
    public:
        LogMsgEvent(const LogMsgModel::LogMsg& msg) : QEvent(LOG_MSG_EVENT), message(msg) { }
        LogMsgModel::LogMsg message;
    };

    explicit LogMsgWidget(QWidget *parent = 0);

    void append(const LogMsgModel::LogMsg& msg);
    void clear();

    void customEvent(QEvent* event);

private:
    LogMsgModel* _data;
    QSortFilterProxyModel* _proxyModel;

    void add(const LogMsgModel::LogMsg& msg);
    void flush();

signals:

public slots:
};

#endif // LOGMSGWIDGET_H
