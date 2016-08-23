#include "datetimedelegate.h"
#include <QDateTime>
#include <QPainter>


DateTimeDelegate::DateTimeDelegate(QString format, QWidget *parent) : QItemDelegate(parent), _format(format)
{
}

void DateTimeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data().canConvert<QDateTime>())
    {
        // Highlight background if selected
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        QString sDateTime = index.data().toDateTime().toString(_format);
        this->drawDisplay(painter, option, option.rect, sDateTime);
    }
    else
    {
        QItemDelegate::paint(painter, option, index);
    }
}
