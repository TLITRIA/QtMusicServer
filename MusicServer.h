#ifndef MUSICSERVER_H
#define MUSICSERVER_H

#include <QTcpServer>
#include <QTimer>


#include "common.h"
using namespace S;

struct ClientMessage;

class MusicServer : public QTcpServer
{
public:
    void initServer();
    void incomingConnection(qintptr handle) override;

    bool startServer(const QString &server_address, quint16 server_port);
    void stopServer();

    void HeartCheck(); // 心跳检测
private:
    QTimer* heartTimer;
    QVector<ClientMessage*> OnlineList;
};




#endif // MUSICSERVER_H
