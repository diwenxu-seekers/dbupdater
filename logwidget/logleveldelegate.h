#ifndef LOGLEVELDELEGATE_H
#define LOGLEVELDELEGATE_H

#include <QItemDelegate>
#include <QVector>


class LogLevelDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit LogLevelDelegate(QWidget *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected:
    QVector<QImage> _vtImages;

signals:

public slots:
};

#endif // LOGLEVELDELEGATE_H
