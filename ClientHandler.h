#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include "common.h"
using namespace S;
#include <QTcpSocket>

struct ClientHandlerPrivate;
struct TransPrivate;

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    ClientHandler(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientHandler();
public:
    void readByteArray(QTcpSocket *socket);
    void sendData(char type, const QByteArray& content, QTcpSocket *socket);
    qint64 sendPage(qint64 page, const QByteArray &content, QTcpSocket *socket);
    void transFile_path(QCStrRef filePath, int type, QTcpSocket *socket);     // 按照文件路径传输指定类型文件
    void transFile_filename(QCStrRef fileName, int type, QTcpSocket *socket); // 按照文件名传输指定类型文件
    void updateCode(int type);
private slots:

public slots:
    void disconnected();
    void readyRead();
    void SYNC();
signals:
    void LOGIN_SUCCESS(QString c_id);
    void Init(qintptr descriptor);
    void multisync();
    void finished();
private:
    ClientHandlerPrivate *p;

    TransPrivate* trans;
    QByteArray recvByteArray;

    char frameHead[7] = {0x0f, (char)0xF0, 0x00, (char)0xFF, 0x00, 0x00, 0x00};
    char frameTail[2] = {0x0D, 0x0A}; // 即/r/n
    char framePage[3] = {0x00, 0x00, 0x00};
};

#endif // CLIENTHANDLER_H
