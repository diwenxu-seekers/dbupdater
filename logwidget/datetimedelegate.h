#ifndef DATETIMEDELEGATE_H
#define DATETIMEDELEGATE_H

#include <QItemDelegate>


class DateTimeDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit DateTimeDelegate(QString format = "d/M/yyyy hh:mm:ss a", QWidget *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QString _format;

signals:

public slots:
};

#endif // DATETIMEDELEGATE_H
