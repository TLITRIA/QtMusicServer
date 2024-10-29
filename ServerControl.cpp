#include "ServerControl.h"
#include "ClientHandler.h"

#include <QHostAddress>
#include <QThread>

#include "../Common/common.h"
struct ServerControlPrivate
{

};

ServerControl::ServerControl(QTcpServer *parent)
    : QTcpServer(parent)
    , p(new ServerControlPrivate)
{
    THLOG;
}

ServerControl::~ServerControl()
{
    delete p;
}

bool ServerControl::startServer(int port)
{
    return this->listen(QHostAddress::Any, port);
}

void ServerControl::incomingConnection(qintptr socketDescriptor)
{
    QThread* t = new QThread;
    ClientHandler *h = new ClientHandler(socketDescriptor);

    h->moveToThread(t);
    t->start();
    // 线程启动，处理客户端逻辑
    connect(t, &QThread::start, h, &ClientHandler::Process);

    // 线程退出
    connect(h, &ClientHandler::DisConnected, t, &QThread::quit); // 断开连接
    connect(t, &QThread::finished, h, &ClientHandler::deleteLater); // 销毁指针
    connect(t, &QThread::finished, t, &QThread::deleteLater); // 销毁指针
    
}
