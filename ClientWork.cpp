#include "ClientWork.h"

#include <QTextCodec>
#include <QFile>
#include <QFileInfo>






ClientWork::ClientWork(QObject *parent)
    : QObject(parent)
    , trans(new TransPrivate)
{
    MYLOG;

}

void ClientWork::init(const qintptr& descriptor, bool ifLogin, QCStrRef ID)
{
    this->descriptor = descriptor;
    this->isLogin = ifLogin;
    this->ID = ID;
}

void ClientWork::sendData(char type, const QByteArray &content)
{

}

qint64 ClientWork::sendPage(qint64 page, const QByteArray &content)
{

}

void ClientWork::transFile_path(QCStrRef filePath, int type)
{

}

void ClientWork::transFile_filename(QCStrRef fileName, int type)
{

}

void ClientWork::run()
{
    sql = &SqlService::getInstance();
    socket = new QTcpSocket(this);
    socket->setSocketDescriptor(descriptor);
    MYLOG;


    // emit Result(0);
}

