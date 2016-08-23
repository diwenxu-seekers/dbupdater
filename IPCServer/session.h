#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <QLocalSocket>


class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(QLocalSocket* socket, QObject *parent = 0);
    ~Session() { _socket->deleteLater(); }

    QLocalSocket* socket() { return _socket; }

signals:
    void disconnected(Session*);

public slots:
    void onDisconnected();

private:
    QLocalSocket* _socket;
};

#endif // SESSION_H
