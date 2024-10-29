#include "ClientHandler.h"
#include "SqlService.h"
#include "ConfigXml.h"

#include <QTextCodec>

struct ClientHandlerPrivate
{
    QTcpSocket *socket;
    SqlService *sql;
    ConfigXml* xml;
    QDomElement root;
    QString Cli_id;
    bool isLogin = false;
    qintptr descriptor;
};

struct TransPrivate
{
    QFile recvFile;
    qint64 fileSize;
    qint64 recvSize;
    QTextCodec *codec;
};

ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    :p(new ClientHandlerPrivate)
    ,trans(new TransPrivate)
{
    MYLOG;
    p->sql = &SqlService::getInstance();
    p->xml = new ConfigXml();
    p->descriptor = socketDescriptor;

    p->socket = new QTcpSocket;
    if(!p->socket->setSocketDescriptor(p->descriptor))
    {
        MYLOG<<p->socket->errorString()<<p->descriptor;
        return;
    }
    p->socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(p->socket, &QTcpSocket::disconnected, this, &ClientHandler::disconnected);
    connect(p->socket, &QTcpSocket::readyRead, this, &ClientHandler::readyRead);
}

ClientHandler::~ClientHandler()
{
    emit finished();
    MYLOG;
    delete p;
    delete trans;
}


void ClientHandler::readByteArray(QTcpSocket *socket)
{
    while(socket->bytesAvailable()>0)
    {
        static QByteArray frame_head_compare = QByteArray(frameHead,4);
        recvByteArray+=socket->readAll();
        while(recvByteArray.length()>0)
        {
            while(!recvByteArray.startsWith(frame_head_compare) && recvByteArray.size() > 4)
                recvByteArray.remove(0,1);
            if(recvByteArray.size() < 7+2)
                return;
            const int content_length = uchar(recvByteArray[4])*0x100 + uchar(recvByteArray[5]);
            if(recvByteArray.size() < 7+2 + content_length)
                return;
            if(memcmp(recvByteArray.constData()+7+content_length,frameTail,2) != 0)
            {
                recvByteArray.clear(); // 传输失败
                MYLOG<<"传输失败";
                return;
            }
            switch((char)recvByteArray[6])
            {
            case CODE::TEST:
            {
                MYLOG<<QString(QByteArray(recvByteArray.constData()+7, content_length).data());
                break;
            }
            case CODE::UP_HEAD:
            {
                QString fileName;
                int type;
                UnpackFileMsg(QJsonDocument::fromJson(recvByteArray.mid(7, content_length)).object(),
                              &fileName, &type, &trans->fileSize);
                trans->recvFile.setFileName(ServerSetting::getInstance().getCurDirPath()
                                            +Path[type] + "/" + fileName);
                if(trans->recvFile.exists())
                    trans->recvFile.remove();
                if(!trans->recvFile.open(QIODevice::NewOnly | QIODevice::WriteOnly))
                    sendData(CODE::UP_HEAD, QByteArray("文件创建出错"), socket);
                trans->recvSize = 0;
                trans->recvFile.resize(trans->fileSize);
                MYLOG<<"上传的文件"<<trans->fileSize<<fileName;
                break;
            }
            case CODE::UP_BODY:
            {
                if(!trans->recvFile.isOpen())
                    break;
                trans->recvFile.seek(trans->recvSize);
                int realReadSize = trans->recvFile.write(
                    recvByteArray.sliced(7+3, content_length-3) );
                if(realReadSize != content_length-3)
                    sendData(CODE::UP_BODY, QByteArray("数据写入出错"), socket);
                trans->recvSize+=realReadSize;
                if(trans->recvSize == trans->fileSize)
                    sendData(CODE::UP_BODY, QByteArray("传输完成"), socket);
                break;
            }
            case CODE::DOWN_HEAD:
            {
                sendData(CODE::END, "", socket);
                break;
            }
            case CODE::DOWN_BODY:
            {
                sendData(CODE::END, "", socket);
                break;
            }
            case CODE::END:
            {
                if(trans->recvFile.isOpen())
                {
                    MYLOG;
                    trans->recvFile.close();
                }

                trans->fileSize = 0;
                int resType;
                int pathType;
                QString message;
                if(!UnpackTransEnd(QJsonDocument::fromJson(recvByteArray.sliced(7, content_length)).object()
                                    ,&resType ,&pathType ,&message))
                    break;
                if(resType!=TRANS::RES::SUCCESS)
                {
                    MYLOG<<"传输失败"<<message;
                    break;
                }
                QFileInfo info = QFileInfo(trans->recvFile);



                break;
            }
            case CODE::LOGIN:
            {
                if(p->isLogin == true) // 同一个客户端连续登陆，忽略
                    break;
                // 解析
                QString ID;
                QString username;
                QString pwd;
                bool ifRempwd;
                bool ifAutolog;
                if(!UnpackLoginMsg(QJsonDocument::fromJson(recvByteArray.sliced(7, content_length)).object()
                                    ,&ID ,&username, &pwd, &ifRempwd, &ifAutolog))
                    break;
                // 登陆
                int error_type;
                QString HeadIconPath;
                if(!p->sql->doLogin(&ID, pwd, ifRempwd, &username, &HeadIconPath, &error_type)) // 登陆与错误处理
                {
                    if(error_type == LOGINCODE::ERROR::PWDERROR)
                        sendData(CODE::LOGIN, UserMsg("密码错误",ID,username, "", false, false), socket);
                    else if(error_type == LOGINCODE::ERROR::NOID)
                        sendData(CODE::LOGIN, UserMsg("账户未注册",ID,username, "", false, false), socket);
                    else if(error_type == LOGINCODE::ERROR::INPUTERROR)
                        sendData(CODE::LOGIN, UserMsg("服务器错误",ID,username, "", false, false), socket);
                    socket->disconnectFromHost();
                    break;
                }
                /* 登陆后马上要做的任务如下 */
                /*
1.确认登陆消息有效√
2.发送用户数据√
3.发送音乐库数据√
4.发送头像√
*/
                p->isLogin = true;
                p->Cli_id = ID;
                sendData(CODE::LOGIN, UserMsg("success", ID, username, HeadIconPath,
                                              ifRempwd, ifAutolog), socket);
                emit LOGIN_SUCCESS(ID);

                // 生成用户资源
                {
                    // QString accountXmlPath = S::ServerSetting::getInstance().getCurDirPath()
                    //                   + S::Path[S::PathType::Account]
                    //                   + "/" + ID + ".xml";
                    p->sql->doGenerateXML_private(ID);
                    transFile_filename(ID + ".xml", S::PathType::Account, socket); // 必须是不带前面路径的文件名
                }

                {
                    QString sourceXmlPath = S::ServerSetting::getInstance().getCurDirPath()
                                            + S::Path[S::PathType::Config]
                                            + S::File[S::FileType::MusicSource];
                    transFile_path(sourceXmlPath, S::PathType::Config, socket);
                }
                // 发送头像
                {
                    transFile_filename(p->sql->doGetHeadIconFileName(ID), S::PathType::HeadIcon, socket);
                }
                break;
            }
            case CODE::REGISTER:
            {
                if(p->isLogin == true) // 同一个客户端连续注册，忽略
                    break;
                // 解析
                QString ID;
                QString username;
                QString pwd;
                bool ifRempwd;
                bool ifAutolog;
                if(!UnpackLoginMsg(QJsonDocument::fromJson(recvByteArray.sliced(7, content_length)).object()
                                    ,&ID ,&username, &pwd, &ifRempwd, &ifAutolog))
                    break;

                int error_type;
                if(!p->sql->doRegister(username, pwd, ifRempwd, &error_type, &ID)) // 注册与错误处理
                {
                    MYLOG<<"注册错误处理，错误码："<<error_type;
                    p->sql->printErrorMessage();
                    if(error_type==REGISTERCODE::ERROR::SAMENAME)
                        sendData(CODE::REGISTER, UserMsg("账户已注册",ID,username,"",false,false), socket);
                    else if(error_type==REGISTERCODE::ERROR::OVERWHLEM)
                        sendData(CODE::REGISTER, UserMsg("账户溢出",ID,username,"",false,false), socket);
                    socket->abort();
                    break;
                }

                // 注册成功后要做的事
                p->isLogin = true;
                p->Cli_id = ID;
                sendData(CODE::LOGIN, UserMsg("success",ID,username,"",
                                                 ifRempwd, ifAutolog), socket);
                emit LOGIN_SUCCESS(ID);

                p->sql->doGenerateXML_private(ID);
                transFile_filename(ID + ".xml", S::PathType::Account, socket);
                QString sourceXmlPath = S::ServerSetting::getInstance().getCurDirPath()
                                        + S::Path[S::PathType::Config]
                                        + S::File[S::FileType::MusicSource];
                transFile_path(sourceXmlPath, S::PathType::Config, socket);
                break;
            }
            case CODE::REQUEST: // 客户端请求
            {
                if(p->isLogin == false)
                    break;
                QJsonDocument jsonDoc(QJsonDocument::fromJson(recvByteArray.sliced(7, content_length)).object());
                switch(jsonDoc["type"].toInt())
                {
                case C::REQUEST::DownSingleMusic:
                {
                    QString audio;
                    QString lcr;
                    QString post;
                    p->sql->getSingleMusic(jsonDoc["m_id"].toString(), &audio, &lcr, &post);
                    // 连续下载
                    if(!audio.isEmpty())
                        transFile_filename(audio, S::PathType::Audio, socket);
                    if(!lcr.isEmpty())
                        transFile_filename(lcr, S::PathType::Lcr, socket);
                    if(!post.isEmpty())
                        transFile_filename(post, S::PathType::Post, socket);
                    // 下载结束，更新指令
                    sendData(CODE::UPDATE, Update("", UPDATE::MusiScource), socket);
                    break;
                }
                case C::REQUEST::UpdateLike: // 添加/删除喜爱的歌曲
                {
                    // MYLOG<<jsonDoc["m_id"].toString()<<getID()<<jsonDoc["val"].toBool();
                    p->sql->updateLike(jsonDoc["m_id"].toString(), p->Cli_id, jsonDoc["val"].toBool());
                    emit multisync();
                    break;
                }
                case C::REQUEST::UpdateWholeCollection: // 添加收藏夹或修改整个收藏夹
                {
                    quint64 return_cid;
                    // 对收藏夹本身的修改
                    if(!p->sql->updateCollection(jsonDoc["name"].toString(),
                                               jsonDoc["post"].toString(),
                                               jsonDoc["creatTime"].toString().toULongLong(),
                                               jsonDoc["info"].toString(),
                                               jsonDoc["id"].toString().toULongLong(),
                                               p->Cli_id, &return_cid))
                    {
                        p->sql->printErrorMessage();
                        break;
                    }
                    // 对收藏内容的修改
                    QStringList contains;
                    for(int i = 0; i < jsonDoc["containArray"].toArray().count(); i++)
                        contains.push_back(jsonDoc["containArray"].toArray()[i].toString());

                    if(!p->sql->updateCollectionContain(return_cid, p->Cli_id, contains))
                    {
                        p->sql->printErrorMessage();
                        break;
                    }
                    break;
                }
                case C::REQUEST::DeleteCollection: // 添加或修改整个收藏夹
                {
                    p->sql->deleteCollection(jsonDoc["c_id"].toString(), p->Cli_id);
                    break;
                }
                case C::REQUEST::GetUpdateUserData:
                {
                    p->sql->doGenerateXML_private(p->Cli_id);
                    transFile_filename(p->Cli_id + ".xml", S::PathType::Account, socket);
                    break;
                }
                case C::REQUEST::UpdateAlbum:
                {
                    if(!p->sql->updateAlbum(jsonDoc["a_id"].toString(), p->Cli_id, jsonDoc["val"].toBool()))
                        p->sql->printErrorMessage();
                    p->sql->doGenerateXML_private(p->Cli_id);
                    transFile_filename(p->Cli_id + ".xml", S::PathType::Account, socket);
                    break;
                }
                case C::REQUEST::_ChangeIcon:
                {
                    if(!p->isLogin)
                        break;
                    if(!p->sql->changeUserIcon(jsonDoc["u_id"].toString(), jsonDoc["icon"].toString()))
                    {
                        p->sql->printErrorMessage();
                        break;
                    }
                    p->sql->doGenerateXML_private(p->Cli_id);
                    transFile_filename(p->Cli_id + ".xml", S::PathType::Account, socket);
                    sendData(CODE::UPDATE, S::UserIcon(jsonDoc["icon"].toString()), socket);
                    break;
                }
                case C::REQUEST::_ChangeName:
                {
                    if(!p->sql->changeUserName(jsonDoc["u_id"].toString(), jsonDoc["name"].toString()))
                        p->sql->printErrorMessage();

                    p->sql->doGenerateXML_private(p->Cli_id);
                    transFile_filename(p->Cli_id + ".xml", S::PathType::Account, socket);

                    MYLOG<<jsonDoc["name"].toString();
                    sendData(CODE::UPDATE, S::UserName(jsonDoc["name"].toString()), socket);
                    break;
                }
                default:
                {
                    MYLOG<<"未添加的指令" << jsonDoc["type"].toInt();
                    break;
                }
                }
                break;
            }
            default:
            {
                MYLOG<<"未添加的命令";break;
            }
            }
            recvByteArray.remove(0, 7+2+content_length);
        }
    }
}

void ClientHandler::sendData(char type, const QByteArray &content, QTcpSocket *socket)
{
    frameHead[6] = type;
    const quint64 data_size=content.count();
    frameHead[5]=data_size%0x100;
    frameHead[4]=data_size/0x100;
    socket->write(frameHead, 7);
    socket->write(content);
    socket->write(frameTail, 2);
}

qint64 ClientHandler::sendPage(qint64 page, const QByteArray &content, QTcpSocket *socket)
{
    frameHead[6] = CODE::DOWN_BODY;
    const quint64 data_size=content.count() + 3;
    frameHead[5]=data_size%0x100;
    frameHead[4]=data_size/0x100;

    framePage[0]=page/0x10000%0x100;
    framePage[1]=page/0x100%0x100;
    framePage[2]=page%0x100;

    // MYLOG<<page<<" "<<content.data();
    socket->write(frameHead, 7);
    socket->write(framePage, 3);
    socket->write(content);
    socket->write(frameTail, 2);
    return content.count();
}

void ClientHandler::transFile_path(QCStrRef filePath, int type, QTcpSocket *socket)
{
    // MYLOG<<"传输的文件位于"<<filePath;
    QFileInfo fileInfo(filePath);
    if(!fileInfo.exists())
    {
        MYLOG<<filePath<<"\t!fileInfo.exists()";
        return;
    }

    QFile file(fileInfo.filePath());
    if(!file.open(QIODevice::ReadOnly))
        return;

    sendData(CODE::DOWN_HEAD, fileMsg(fileInfo.fileName(), type, fileInfo.size()), socket);

    qint64 pages = 0;
    while(!file.atEnd())
        sendPage(pages++, file.read(1024), socket);

    file.close();

    sendData(CODE::END, transEnd(TRANS::RES::SUCCESS, type, "传输成功"), socket);
}

void ClientHandler::transFile_filename(QCStrRef fileName, int type, QTcpSocket *socket)
{
    QString filePath = S::ServerSetting::getInstance().getCurDirPath()
                       + S::Path[type] + "/" + fileName;
    transFile_path(filePath, type, socket);
}

void ClientHandler::disconnected()
{
    MYLOG << "Client disconnected";

    this->deleteLater();
}

void ClientHandler::readyRead()
{
    if(p->socket->bytesAvailable()<=0)
        return;
    while(p->socket->bytesAvailable()>0)
        readByteArray(p->socket);
}

void ClientHandler::SYNC()
{
    MYLOG;
    if(!p->Cli_id.isEmpty())
    {
        p->sql->doGenerateXML_private(p->Cli_id);
        transFile_filename(p->Cli_id + ".xml", S::PathType::Account, p->socket);
        QString sourceXmlPath = S::ServerSetting::getInstance().getCurDirPath()
                                + S::Path[S::PathType::Config]
                                + S::File[S::FileType::MusicSource];
        transFile_path(sourceXmlPath, S::PathType::Config, p->socket);
    }
}
