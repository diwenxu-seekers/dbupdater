#include "logmsgmodel.h"


LogMsgModel::LogMsgModel(QObject* parent) : QAbstractTableModel(parent)
{
}

int LogMsgModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return _lstMsgs.size();
}

int LogMsgModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return LogMsg::fieldCount();
}

QVariant LogMsgModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= _lstMsgs.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        LogMsg msg = _lstMsgs.at(index.row());

        switch (index.column())
        {
            case 0: return msg.level;
            case 1: return msg.time;
            case 2: return msg.msg;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        return _lstMsgs.at(index.row()).msg;
    }

    return QVariant();
}

QVariant LogMsgModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0: return tr("");
            case 1: return tr("Time");
            case 2: return tr("Message");
            default: return QVariant();
        }
    }

    return QVariant();
}

bool LogMsgModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginInsertRows(QModelIndex(), row, row + count - 1);

    for (int nIdx = 0; nIdx < count; ++nIdx)
    {
        LogMsg msg = { LogLevel::off, QDateTime(), "" };
        _lstMsgs.insert(row, msg);
    }

    endInsertRows();

    return true;
}

bool LogMsgModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    for (int nIdx = 0; nIdx < count; ++nIdx)
    {
        _lstMsgs.removeAt(row);
    }

    endRemoveRows();

    return true;
}

bool LogMsgModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        int nRow = index.row();

        LogMsg msg = _lstMsgs.value(nRow);

        switch (index.column())
        {
            case 0: msg.level = static_cast<LogLevel>(value.toInt()); break;
            case 1: msg.time = value.toDateTime(); break;
            case 2: msg.msg = value.toString(); break;
            default: return false;
        }

        _lstMsgs.replace(nRow, msg);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

Qt::ItemFlags LogMsgModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
