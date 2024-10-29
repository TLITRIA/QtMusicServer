#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>

struct ClientHandlerPrivate;

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientHandler();

    void Process();
signals:
    void DisConnected();
public slots:
    void HandlerMsg();
private:
    ClientHandlerPrivate *p;
};

#endif // CLIENTHANDLER_H
