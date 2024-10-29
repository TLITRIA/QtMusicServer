#ifndef SERVERCONTROL_H
#define SERVERCONTROL_H

#include <QTcpServer>

struct ServerControlPrivate;


class ServerControl : public QTcpServer
{
    Q_OBJECT
public:
    ServerControl(QTcpServer *parent = nullptr);
    ~ServerControl();

    bool startServer(int port);
    void incomingConnection(qintptr socketDescriptor);

private:
    ServerControlPrivate *p;
};

#endif // SERVERCONTROL_H
