#include "commserver.h"
#include <QtNetwork>

using namespace server;


CommServer::CommServer(const QString& name, QObject *parent) : QObject(parent), _name(name), _server(this)
{
    connect(&_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

bool CommServer::start()
{
    // Close previously opened socket
    if (_server.isListening())
        _server.close();

    // Remove all previous connections
    _sessions.clear();

    return _server.listen(_name);
}

void CommServer::stop()
{
    // Close the listening socket
    _server.close();

    foreach (Session* session, _sessions)
    {
        QLocalSocket* socket = session->socket();
        if (socket->isValid())
        {
            socket->disconnectFromServer();
        }
    }
}

void CommServer::notifyUpdate()
{
    // Broadcast notification to all connections
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_5);

    out << (quint16)0;
    out << QString("dbupdated");
    out.device()->seek(0);
    out << (quint16)(data.size() - sizeof(quint16));

    foreach (Session* session, _sessions)
    {
        QLocalSocket* socket = session->socket();
        if (socket->isValid() && socket->isWritable())
            socket->write(data);
    }
}

void CommServer::onNewConnection()
{
    // Handling new incoming connections and keep all of them
    QLocalSocket* connection = _server.nextPendingConnection();
    Session* session = new Session(connection, this);

    _sessions.append(session);

    connect(session, SIGNAL(disconnected(Session*)), this, SLOT(onDisconnected(Session*)));

    emit newConnection();
}

void CommServer::onDisconnected(Session* session)
{
    int nIdx = _sessions.indexOf(session);
    if (nIdx != -1)
    {
        _sessions.removeAt(nIdx);
    }

    delete session;

    emit disconnected();
}
