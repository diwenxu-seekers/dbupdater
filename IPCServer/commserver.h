#ifndef COMMSERVER_H
#define COMMSERVER_H

#include "session.h"
#include <QObject>
#include <QList>
#include <QLocalServer>


namespace server
{
class CommServer : public QObject
{
    Q_OBJECT
public:
    explicit CommServer(const QString& name, QObject *parent = 0);

    bool start();
    void stop();

    bool isRunning() { return _server.isListening(); }

    void notifyUpdate();

signals:
    void newConnection();
    void disconnected();

public slots:
    void onNewConnection();
    void onDisconnected(Session* session);

private:
    QString _name;
    QLocalServer _server;

    QList<Session*> _sessions;      // All incoming connections
};

} // namespace server

#endif // COMMSERVER_H
