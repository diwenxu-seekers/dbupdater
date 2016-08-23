#include "logmsgwidget.h"
#include "datetimedelegate.h"
#include "logleveldelegate.h"
#include <QTableView>
#include <QHeaderView>
#include <QLayout>
#include <QApplication>


#define DATETIME_FORMAT     "MMM d yyyy HH:mm:ss.zzz"
#define COL_LEVEL_WIDTH     30
#define COL_DATETIME_WIDTH  140

LogMsgWidget::LogMsgWidget(QWidget *parent) : QWidget(parent)
{
    // Set the data model
    _data = new LogMsgModel(this);

    _proxyModel = new QSortFilterProxyModel(this);
    _proxyModel->setSourceModel(_data);

    QTableView* tableView = new QTableView();
    tableView->setModel(_proxyModel);

    // Set display behaviors
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->verticalHeader()->hide();
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Default sorting by time
    tableView->setSortingEnabled(true);
    tableView->sortByColumn(0, Qt::DescendingOrder);
    tableView->horizontalHeader()->setSortIndicator(1, Qt::DescendingOrder);

    // Set column width
    tableView->setColumnWidth(0, COL_LEVEL_WIDTH);
    tableView->setColumnWidth(1, COL_DATETIME_WIDTH);

    // Set default row height
    tableView->verticalHeader()->setDefaultSectionSize(24);
    tableView->horizontalHeader()->setStretchLastSection(true);

    tableView->setItemDelegateForColumn(0, new LogLevelDelegate(this));
    tableView->setItemDelegateForColumn(1, new DateTimeDelegate(DATETIME_FORMAT, this));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(tableView);

    setLayout(layout);
}

void LogMsgWidget::append(const LogMsgModel::LogMsg& msg)
{
    QApplication::postEvent(this, new LogMsgEvent(msg));
}

void LogMsgWidget::clear()
{
    QApplication::postEvent(this, new QEvent(CLEAR_MSG_EVENT));
}

void LogMsgWidget::customEvent(QEvent* event)
{
    switch (event->type())
    {
        case LOG_MSG_EVENT:
            add(static_cast<LogMsgEvent*>(event)->message);
            break;

        case CLEAR_MSG_EVENT:
            flush();
            break;
    }
}

void LogMsgWidget::add(const LogMsgModel::LogMsg& msg)
{
    // Append log message to viewer
    _data->insertRows(0, 1, QModelIndex());

    QModelIndex nIdx = _data->index(0, 0, QModelIndex());
    _data->setData(nIdx, msg.level, Qt::EditRole);

    nIdx = _data->index(0, 1, QModelIndex());
    _data->setData(nIdx, msg.time, Qt::EditRole);

    nIdx = _data->index(0, 2, QModelIndex());
    _data->setData(nIdx, msg.msg, Qt::EditRole);
}

void LogMsgWidget::flush()
{
    if (_data->rowCount(QModelIndex()) > 0)
    {
        _data->removeRows(0, _data->rowCount(QModelIndex()), QModelIndex());
    }
}
