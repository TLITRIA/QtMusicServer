#ifndef CLIENTWORK_H
#define CLIENTWORK_H

#include "SqlService.h"
#include "ConfigXml.h"

#include <QRunnable>
#include <QObject>
#include <QTcpSocket>

#include "../Common/common.h"

using namespace S;

struct TransPrivate;


class ClientWork : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ClientWork(QObject *parent = nullptr);

    // void init(const qintptr& descriptor, bool ifLogin, QCStrRef ID);


signals:
    void Result(int type); // 处理结果，（无结果、登陆、登出）
    void LOGIN_SUCCESS(QCStrRef ID); //

protected:
    void run();

private:
    QTcpSocket *socket;
    qintptr descriptor;

    bool isLogin = false;
    QString ID;
};

#endif // CLIENTWORK_H
