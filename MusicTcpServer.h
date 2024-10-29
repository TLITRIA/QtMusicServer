#ifndef MUSICTCPSERVER_H
#define MUSICTCPSERVER_H

#include <QObject>


struct MusicTcpServerPrivate;

struct ClientMessage;

class MusicTcpServer : public QObject
{
    Q_OBJECT
public:
    explicit MusicTcpServer(QObject *parent = nullptr);
    ~MusicTcpServer();

    bool isListening(); // 获取状态


public slots:


public:



private:
    MusicTcpServerPrivate *p;


};

#endif // MUSICTCPSERVER_H
