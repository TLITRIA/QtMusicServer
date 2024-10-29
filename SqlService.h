#ifndef SQLSERVICE_H
#define SQLSERVICE_H

#include <QObject>

#include "common.h"

struct SQLTestPrivate;
using namespace S;


class SqlService : public QObject
{
    Q_OBJECT

private:
    static SqlService instance;
    SqlService(QObject *parent = nullptr);
    ~SqlService();

public:
    static SqlService& getInstance();
    SqlService(SqlService const&) = delete;
    void operator=(SqlService const&) = delete;

    // 初始化
    bool doInit(QCStrRef host, QCStrRef database, QCStrRef user, QCStrRef pwd, int port = 3306);
    bool doCheckSource(); // account source
    bool updateCollect(); // 更新source playlist收藏人数
    bool doGenerateXML_source();// 生成音乐资源

    // Task
    bool doGenerateXML_private(QCStrRef ID); //
    bool createFile(QCStrRef filePath); // 注意线程安全性
    bool doLogin(QString* ID, QCStrRef pwd, bool ifRempwd, QString* username, QString* headIconFileName, int* ERRORTYPE); // ret成功失败，传出错误码
    bool doRegister(QCStrRef userName, QCStrRef pwd, bool ifRempwd,
                    int* ERRORTYPE, QString* ID_out);
    bool doStop();

    // 获取信息
    QString doGetHeadIconFileName(QCStrRef ID);
    bool getSingleMusic(QCStrRef ID, QString* audio, QString* lcr, QString* post); // 获取单个音乐的资源
    void printErrorMessage(); // 打印sql错误信息
    
    // 客户端请求
    bool updateLike(QCStrRef m_id, QCStrRef u_id, bool val);
    bool updateCollection(QCStrRef name, QCStrRef post, quint64 creteTime, QCStrRef info, quint64 c_id, QCStrRef u_id, quint64* return_id);
    bool updateCollectionContain(quint64 c_id, QCStrRef u_id, const QStringList& m_ids);
    bool deleteCollection(QCStrRef c_id, QCStrRef u_id);
    bool updateAlbum(QCStrRef a_id, QCStrRef u_id, bool val);
    bool changeUserIcon(QCStrRef u_id, QCStrRef icon);
    bool changeUserName(QCStrRef u_id, QCStrRef name);

private:

    SQLTestPrivate *p;
};

#endif // SQLSERVICE_H
