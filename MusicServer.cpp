#include "MusicServer.h"

#include "ClientHandler.h"


struct ClientMessage
{
    bool ifLogin = false;    // 客户连接的状态
    quint64 connectTime = 0; // 连接时间，当连接时间过长且未登录就会自动断开连接
    QString c_id;            // 用户id用于顶号
    ClientHandler* Handler;
    qintptr descriptor; //
};

void MusicServer::initServer()
{
    heartTimer = new QTimer(this);
    connect(heartTimer, &QTimer::timeout, this, &MusicServer::HeartCheck);
    heartTimer->setInterval(1000);
    heartTimer->start();
}

void MusicServer::incomingConnection(qintptr descriptor)
{
    ClientMessage* newCliMess = new ClientMessage;
    OnlineList.push_back(newCliMess);
    newCliMess->connectTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    newCliMess->descriptor = descriptor;
    newCliMess->Handler = new ClientHandler(descriptor);

    connect(newCliMess->Handler, &ClientHandler::finished, [this, newCliMess](){
        for(int i = 0; i < OnlineList.size(); i++)
        {
            if(OnlineList[i] == newCliMess)
            {
                OnlineList.remove(i);
                break;
            }
        }
    });
    connect(newCliMess->Handler, &ClientHandler::LOGIN_SUCCESS, [this, newCliMess](QString c_id){
        newCliMess->c_id = c_id;
    });

    connect(newCliMess->Handler, &ClientHandler::multisync, [this, newCliMess](){
        for(int i = 0; i < OnlineList.size(); i++)
        {
            if(OnlineList[i] != newCliMess && OnlineList[i]->c_id == newCliMess->c_id)
            {
                emit OnlineList[i]->Handler->SYNC();
            }
        }
    });
}

bool MusicServer::startServer(const QString &server_address, quint16 server_port)
{
    if(this->isListening())
        this->stopServer();
    return this->listen(QHostAddress(server_address), server_port);
}

void MusicServer::stopServer()
{
    this->close();
}

void MusicServer::HeartCheck()
{
    MYLOG<<OnlineList.size();
    for(int i = 0; i < OnlineList.size(); i++)
    {
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(OnlineList[i]->connectTime);
        QString formattedDate = dateTime.toString("yyyy-MM-dd HH:mm:ss");
        MYLOG<<formattedDate;
    }
}
