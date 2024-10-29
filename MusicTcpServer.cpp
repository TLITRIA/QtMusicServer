#include "MusicTcpServer.h"
#include "MusicServer.h"

#include "../Common/common.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QThreadPool>
#include <QMap>
#include <QTimer>
#include <QDateTime>


struct MusicTcpServerPrivate
{
    MusicServer *server;
};



MusicTcpServer::MusicTcpServer(QObject *parent)
    : QObject{parent}
    , p(new MusicTcpServerPrivate)
{
    p->server = new MusicServer;
}

MusicTcpServer::~MusicTcpServer()
{
    delete p;
}

bool MusicTcpServer::isListening()
{
    return p->server->isListening();
}


// void MusicTcpServer::HeartCheck()
// {
//     // 1.断开长时间未执行登录操作的连接
//     quint64 currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

//     for(int i = 0; i < p->OnlineList.size(); i++)
//     {
//         if(currentTime - p->OnlineList[i]->connectTime > 10 * 1000)
//         {
//             MYLOG<<"连接超时："<<currentTime<<p->OnlineList[i]->connectTime;
//         }
//     }


//     // 输出信息
//     // MYLOG<<"当前连接数"<<p->OnlineList.size();
//     int countLogin = 0;

//     for(int i = 0; i < p->OnlineList.size(); i++)
//         if(p->OnlineList[i]->ifLogin)
//             countLogin++;
//     // MYLOG<<"当前已登陆用户数"<<countLogin;
// }






