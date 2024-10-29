#include "ClientHandler.h"
#include <QTcpSocket>
#include <QDebug>
#include <memory>

#include "../Common/common.h"

struct ClientHandlerPrivate
{
    std::shared_ptr<QTcpSocket> socket;
};


ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QObject(parent)
    , p (new ClientHandlerPrivate)
{
    p->socket = std::make_shared<QTcpSocket>();

    if (p->socket->setSocketDescriptor(socketDescriptor) == false)
    {
        qDebug()<<"该描述符已失效";
        return;
    }

    connect(p->socket.get(), &QTcpSocket::disconnected, this, &ClientHandler::DisConnected);

}

ClientHandler::~ClientHandler()
{
    delete p;
}

void ClientHandler::Process()
{
    connect(p->socket.get(), &QTcpSocket::readyRead, this, &ClientHandler::HandlerMsg);
}

void ClientHandler::HandlerMsg()
{
    auto Content = p->socket->readAll();
    // json 处理逻辑
    MYLOG<< Content.toStdString().data();
}
