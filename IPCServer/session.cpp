#include "session.h"

Session::Session(QLocalSocket* socket, QObject *parent) : QObject(parent), _socket(socket)
{
    connect(_socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), _socket, SIGNAL(disconnected()));
}

void Session::onDisconnected()
{
    emit disconnected(this);
}
