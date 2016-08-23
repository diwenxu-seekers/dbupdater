#include "logleveldelegate.h"
#include "logmsgmodel.h"
#include <QPainter>


LogLevelDelegate::LogLevelDelegate(QWidget *parent) : QItemDelegate(parent)
{
    _vtImages.push_back(QImage(":/resources/log_trace.png"));
    _vtImages.push_back(QImage(":/resources/log_debug.png"));
    _vtImages.push_back(QImage(":/resources/log_info.png"));
    _vtImages.push_back(QImage(":/resources/log_notice.png"));
    _vtImages.push_back(QImage(":/resources/log_warn.png"));
    _vtImages.push_back(QImage(":/resources/log_err.png"));
    _vtImages.push_back(QImage(":/resources/log_critical.png"));
    _vtImages.push_back(QImage(":/resources/log_alert.png"));
    _vtImages.push_back(QImage(":/resources/log_emerg.png"));
}

void LogLevelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data().canConvert<int>())
    {
        LogMsgModel::LogLevel level = static_cast<LogMsgModel::LogLevel>(index.data().value<int>());

        // Highlight background if selected
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        // Centralize the image
        int nOffset = (option.rect.width() - _vtImages[0].width()) / 2;

        painter->save();
        painter->drawImage(option.rect.x() + nOffset, option.rect.y(), _vtImages[level]);
        painter->restore();
    }
    else
    {
        QItemDelegate::paint(painter, option, index);
    }
}
