#include "SqlService.h"
#include "ConfigXml.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>

#include <QFile>
#include <QDir>
#include <QtXml>
#include <QDomDocument>


struct SQLTestPrivate
{
    QSqlDatabase db;
    QSqlQuery query;
    QFile file;
    QString curPath;
    ConfigXml xml;
};


SqlService::SqlService(QObject *parent)
    : QObject{parent}
    , p(new SQLTestPrivate)
{
    // QStringList drivers = QSqlDatabase::drivers();
    // MYLOG << "Available database drivers:";
    // foreach(QString driver,drivers)
    //     MYLOG<<driver;

    p->curPath = ServerSetting::getInstance().getCurDirPath();

    doInit(sqlServer, sqlTable, sqlUsr, sqlPwd);
    if(!p->db.isOpen())
    {
        MYLOG <<"数据库未打开";
        return;
    }
}

SqlService::~SqlService()
{
    delete p;
}

SqlService &SqlService::getInstance()
{
    static SqlService instance;
    return instance;
}

bool SqlService::doInit(QCStrRef host, QCStrRef database, QCStrRef user, QCStrRef pwd, int port)
{
    p->db = QSqlDatabase::addDatabase("QMYSQL");
    p->db.setHostName(host);
    p->db.setPort(port);
    p->db.setDatabaseName(database);
    p->db.setUserName(user);
    p->db.setPassword(pwd);
    if(!p->db.open())
    {
        MYLOG<<p->db.lastError().text();
        return false;
    }
    p->curPath = ServerSetting::getInstance().getCurDirPath();
    p->query = QSqlQuery(p->db);

    // 初始化
    bool ret1 = doCheckSource();
    bool ret2 = updateCollect();
    bool ret3 = doGenerateXML_source();
    return (ret1 && ret2 && ret3);
}

bool SqlService::doCheckSource()
{
    if(!p->db.isOpen())
    {
        MYLOG <<"数据库未打开";
        return false;
    }
    bool ret = true; // 检查结果

    // 1.source 检查音频字幕海报文件资源完整性
    p->query.exec("SELECT * FROM source;");
    if(p->query.size() == -1)
    {
        MYLOG <<"读取异常";
        return false;
    }
    else if(p->query.size() == 0)
    {
        MYLOG <<"没有数据";
        return false;
    }
    else
    {
        QFile tmpFile;
        QSet<QString> missFiles;
        while(p->query.next())
        {
            tmpFile.setFileName(p->curPath + Path[PathType::Audio] + "/" + p->query.value(SQL_SOURCE::AUDIO).toString());
            if(!tmpFile.fileName().isEmpty()&&!tmpFile.exists())
                missFiles.insert(tmpFile.fileName());
            tmpFile.setFileName(p->curPath + Path[PathType::Post] + "/" + p->query.value(SQL_SOURCE::POST).toString());
            if(!tmpFile.fileName().isEmpty()&&!tmpFile.exists())
                missFiles.insert(tmpFile.fileName());
            tmpFile.setFileName(p->curPath + Path[PathType::Lcr] + "/" + p->query.value(SQL_SOURCE::LCR).toString());
            if(!tmpFile.fileName().isEmpty()&&!tmpFile.exists())
                missFiles.insert(tmpFile.fileName());
        }
        if(!missFiles.empty())
        {
            MYLOG<<"source缺失文件数"<<missFiles.size();
            for(auto ite = missFiles.begin();
                 ite != missFiles.end();ite++)
            {
                // MYLOG<<*ite;
            }
            ret=false;
        }
    }

    // 2.account 用户头像
    p->query.exec("SELECT * FROM account;");
    if(p->query.size() == -1)
    {
        MYLOG <<"读取异常";
        return false;
    }
    else if(p->query.size() == 0)
    {
        MYLOG <<"没有数据";
        return false;
    }
    else
    {
        QFile tmpFile;
        QSet<QString> missFiles;
        while(p->query.next())
        {
            tmpFile.setFileName(p->curPath + Path[PathType::HeadIcon] + "/" + p->query.value(SQL_ACCOUNT::HEADICON).toString());
            if(!tmpFile.fileName().isEmpty()&&!tmpFile.exists())
                missFiles.insert(tmpFile.fileName());
        }
        if(!missFiles.empty())
        {
            MYLOG<<ServerSetting::getInstance().getCurDirPath();
            for(auto ite = missFiles.begin();
                 ite != missFiles.end();ite++)
            {
                MYLOG<<*ite;
            }
            ret=false;
        }
    }

    return ret;
}

bool SqlService::updateCollect()  // 更新列表收藏数量等数据
{
    QMap<QString, int> source;
    p->query.exec("SELECT * FROM account_source;");
    while(p->query.next())
        source[p->query.value(1).toString()]++;

    p->query.exec("UPDATE source SET collect = '0';");
    for(auto ite = source.begin();
        ite != source.end(); ite++)
        p->query.exec(QString("UPDATE source SET collect = '%1' WHERE id = '%2';").arg(ite.value()).arg(ite.key()));

    QMap<quint64, int> playlist;
    p->query.exec("SELECT * FROM account_playlist;");
    while(p->query.next())
        playlist[p->query.value(1).toULongLong()]++;

    p->query.exec("UPDATE playlist SET collect = '0';");
    for(auto ite = playlist.begin();
         ite != playlist.end(); ite++)
        p->query.exec(QString("UPDATE playlist SET collect = '%1' WHERE id = '%2';").arg(ite.value()).arg(ite.key()));

    return true;
}

bool SqlService::doGenerateXML_source()
{
    QString tmpFilePath = p->curPath + Path[PathType::Config] + File[FileType::MusicSource];
    createFile(tmpFilePath);

    QDomDocument doc;
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    QDomElement root = doc.createElement("Library");
    doc.appendChild(root);
    // 子节点
    QDomElement MusicSource = doc.createElement("MusicSource");
    root.appendChild(MusicSource);

    // MusicSource
    p->query.exec("SELECT * FROM source;");
    if(p->query.size() == -1)
    {
        MYLOG <<"读取异常";
        p->file.close();
        return false;
    }
    else if(p->query.size() == 0)
    {
        MYLOG <<"没有数据";
        p->file.close();
        return false;
    }
    else
    {
        while(p->query.next())
        {
            // 歌曲信息
            QDomElement music1 = doc.createElement("source");
            music1.setAttribute("ID",p->query.value(SQL_SOURCE::SOURCE_ID).toString());
            music1.setAttribute("NAME",p->query.value(SQL_SOURCE::SOURCE_NAME).toString());
            music1.setAttribute("AUTHOR",p->query.value(SQL_SOURCE::AUTHOR).toString());
            music1.setAttribute("AUDIO",p->query.value(SQL_SOURCE::AUDIO).toString());
            music1.setAttribute("POST",p->query.value(SQL_SOURCE::POST).toString());
            music1.setAttribute("LCR",p->query.value(SQL_SOURCE::LCR).toString());
            music1.setAttribute("TYPE",p->query.value(SQL_SOURCE::TYPE).toString());
            music1.setAttribute("PLAY",p->query.value(SQL_SOURCE::PLAY).toString());
            music1.setAttribute("DOWN",p->query.value(SQL_SOURCE::DOWN).toString());
            music1.setAttribute("COLLECT",p->query.value(SQL_SOURCE::COLLECT).toString());
            MusicSource.appendChild(music1);
        }
    }
    // 集合
    QDomElement Playlist = doc.createElement("Playlist");
    root.appendChild(Playlist);

    QMap<quint64, QStringList> playlist_source;

    p->query.exec("SELECT * FROM playlist_source;");

    while(p->query.next())
    {
        playlist_source[p->query.value(0).toULongLong()].append(p->query.value(1).toString());
    }

    p->query.exec("SELECT * FROM playlist;");
    while(p->query.next())
    {
        QDomElement pl = doc.createElement("pl");
        Playlist.appendChild(pl);

        int playlist_id = p->query.value(0).toInt();
        pl.setAttribute("ID", playlist_id);
        pl.setAttribute("NAME", p->query.value(SQL_PLAYLIST::PLAYLIST_NAME).toString());
        pl.setAttribute("COVER", p->query.value(SQL_PLAYLIST::COVER).toString());
        pl.setAttribute("INFO", p->query.value(SQL_PLAYLIST::INFO).toString());
        pl.setAttribute("COLLECT", p->query.value(SQL_PLAYLIST::PLAYLIST_COLLECT).toString());

        for(auto ite = playlist_source[playlist_id].begin();
               ite != playlist_source[playlist_id].end(); ite++)
        {
            QDomElement S_ID = doc.createElement("S_ID");
            pl.appendChild(S_ID);
            QDomText text = doc.createTextNode(*ite);
            S_ID.appendChild(text);
        }
    }
    // 写入文件
    ConfigXml::WriteToXML(doc, p->file.fileName());

    return true;
}

bool SqlService::doGenerateXML_private(QCStrRef ID)
{
    QString tmpFilePath = ServerSetting::getInstance().getCurDirPath()
                          + Path[PathType::Account]
                          + QString("/%1.xml").arg(ID);
    createFile(tmpFilePath);

    QDomDocument doc;
    // 文件头部格式
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);

    // root
    QDomElement root = doc.createElement("Account");
    doc.appendChild(root);

    QDomElement MusicSource = doc.createElement("MusicSource"); // 收藏的音乐资源
    root.appendChild(MusicSource);
    p->query.exec(QString("SELECT source_id\
        FROM account, source, account_source\
        WHERE account.id = account_source.`account_id` \
        AND account_source.`source_id` = source.`id`\
        AND account.id = '%1';").arg(ID));
    while(p->query.next())
    {
        QDomElement source = doc.createElement("source");
        MusicSource.appendChild(source);
        QDomText textNode = doc.createTextNode(p->query.value(0).toString());
        source.appendChild(textNode);
    }

    // 播放列表
    QDomElement PlayList = doc.createElement("PlayList"); // 收藏的音乐资源
    root.appendChild(PlayList);
    p->query.exec(QString("SELECT playlist_id FROM account_playlist WHERE account_id = '%1';").arg(ID));
    while(p->query.next())
    {
        QDomElement pl = doc.createElement("pl");
        PlayList.appendChild(pl);
        QDomText textNode = doc.createTextNode(p->query.value(0).toString());
        pl.appendChild(textNode);
    }


    // 收藏关系
    QDomElement Custom = doc.createElement("Custom"); // 收藏的音乐资源
    root.appendChild(Custom);
    QMap<quint64, QStringList> custom_source;
    p->query.exec(QString("SELECT * FROM custom_source;"));
    while(p->query.next())
        custom_source[p->query.value(0).toULongLong()].append(p->query.value(1).toString());
    // MYLOG<<custom_source;


    p->query.exec(QString("SELECT * FROM custom WHERE account_id = '%1';").arg(ID));
    while(p->query.next())
    {
        QDomElement cl = doc.createElement("cl");
        Custom.appendChild(cl);
        quint64 id = p->query.value(SQL_CUSTOM::CUSTOM_ID).toULongLong();
        cl.setAttribute("ID", p->query.value(SQL_CUSTOM::CUSTOM_ID).toString());
        cl.setAttribute("NAME", p->query.value(SQL_CUSTOM::C_NAME).toString());
        cl.setAttribute("POST", p->query.value(SQL_CUSTOM::C_POST).toString());
        cl.setAttribute("CREATETIME", p->query.value(SQL_CUSTOM::C_CREATETIME).toString());
        cl.setAttribute("INFO", p->query.value(SQL_CUSTOM::C_INFO).toString());

        for(int i = 0; i < custom_source[id].size(); i++)
        {
            QDomElement S_ID = doc.createElement("S_ID");
            cl.appendChild(S_ID);
            QDomText textNode = doc.createTextNode(custom_source[id][i]);
            S_ID.appendChild(textNode);
        }
    }
    ConfigXml::WriteToXML(doc, tmpFilePath);
    return true;
}

bool SqlService::createFile(QCStrRef filePath)
{
    p->file.setFileName(filePath);
    if(p->file.exists())
        p->file.remove();
    bool ret = p->file.open(QIODevice::NewOnly);
    p->file.close();
    return ret;
}

bool SqlService::doLogin(QString* ID, QCStrRef pwd, bool ifRempwd, QString* username, QString* headIconFileName, int* ERRORTYPE)
{
    if(ID->isEmpty() && username->isEmpty())
    {
        *ERRORTYPE = LOGINCODE::ERROR::INPUTERROR;
        return false;
    }
    if(ID->isEmpty())
        p->query.exec(QString("SELECT * FROM account WHERE name = '%1';").arg(*username));
    else
        p->query.exec(QString("SELECT * FROM account WHERE id = '%1';").arg(*ID));
    if(p->query.size()!=1)
    {
        *ERRORTYPE = LOGINCODE::ERROR::NOID;
        return false; // 未找到账号
    }

    bool isAutolog; // 服务器记录账户是否是自动登陆，与服务端发来的标志做区分
    while(p->query.next())
    {
        isAutolog = p->query.value(SQL_ACCOUNT::REMPWD).toBool();
        *headIconFileName = p->query.value(SQL_ACCOUNT::HEADICON).toString();
        *username = p->query.value(SQL_ACCOUNT::ACCOUNT_NAME).toString();
        *ID = p->query.value(SQL_ACCOUNT::ACCOUNT_ID).toString();
        if(isAutolog) // 自动登陆
        {
            continue;
        }
        if(p->query.value(SQL_ACCOUNT::PWD).toString() != pwd) // 密码不符
        {
            *ERRORTYPE = LOGINCODE::ERROR::PWDERROR;
            return false;
        }
    }

    if(ifRempwd != isAutolog) // 修改自动登陆状态
    {
        if(p->query.exec(QString("UPDATE account SET isRempwd = '%1' WHERE id = '%2'")
                                .arg(ifRempwd).arg(*ID)))
        {
            MYLOG<<ID<<"自动登陆更新成功";
        }
    }
    return true;
}

bool SqlService::doRegister(QCStrRef userName, QCStrRef pwd, bool ifRempwd, int *ERRORTYPE, QString* ID_out)
{
    p->query.exec(QString("SELECT * FROM account WHERE name = '%1';").arg(userName));
    if(p->query.size()!=0)
    {
        *ERRORTYPE = REGISTERCODE::ERROR::SAMENAME;
        return false; // 已有重名
    }

    p->query.exec(QString("SELECT id FROM account ORDER BY id DESC;"));
    p->query.next();
    QString ID = p->query.value(0).toString();
    if(ID.toInt() >= 999999999)
    {
        *ERRORTYPE = REGISTERCODE::ERROR::OVERWHLEM;
        return false; // 编号溢出
    }
    ID = QString("%1").arg(ID.toInt()+1);
    if(!p->query.exec("INSERT INTO account (id, name, pwd, isRempwd)\
             VALUES ('"+ID+"', '"+userName+"', '"+pwd+"', '"+ (ifRempwd?"1":"0") +"');"))
    {
        *ERRORTYPE = REGISTERCODE::ERROR::UNKNOWN; // 未知错误
        return false;
    }
    *ID_out = ID;
    return true;
}

bool SqlService::doStop()
{
    p->db.close();
    p->db.removeDatabase(QSqlDatabase::defaultConnection);
    return true;
}

QString SqlService::doGetHeadIconFileName(QCStrRef ID)
{
    p->query.exec(QString("SELECT icon FROM account WHERE id = '%1';").arg(ID));
    p->query.next();
    return p->query.value(0).toString();
}

bool SqlService::getSingleMusic(QCStrRef ID, QString *audio, QString *lcr, QString *post)
{
    p->query.exec(QString("SELECT audio, LCR, post FROM source WHERE id = '%1';").arg(ID));
    p->query.next();
    *audio = p->query.value(0).toString();
    *lcr = p->query.value(1).toString();
    *post = p->query.value(2).toString();
    return true;
}

void SqlService::printErrorMessage()
{
    QSqlError error = p->query.lastError();
    qDebug() << "SQL Error:" << error.text();
    qDebug() << "Database Error Text:" << error.databaseText();
    qDebug() << "SQL Query Executed:" << p->query.lastQuery();
}

bool SqlService::updateLike(QCStrRef m_id, QCStrRef u_id, bool val)
{
    if(val)
    {
        return p->query.exec(QString("INSERT INTO account_source(account_id, source_id)\
        VALUES ('%1', '%2');").arg(u_id).arg(m_id));
    }
    else
    {
        return p->query.exec(QString("DELETE FROM account_source\
        WHERE account_id = '%1' AND source_id = '%2';").arg(u_id).arg(m_id));
    }
    return true;
}

bool SqlService::updateCollection(QCStrRef name, QCStrRef post, quint64 creteTime, QCStrRef info, quint64 c_id, QCStrRef u_id, quint64* return_id)
{
    bool ret;
    if(c_id == 0) // 新建收藏夹
    {
        p->query.exec("SELECT id FROM custom ORDER BY id;");
        bool ifContinuous = true;
        quint64 index = 1; // 序列从1开始，取0意味着新建文件夹
        while(p->query.next())
        {
            if(p->query.value(0).toULongLong() != index)
            {
                ifContinuous = false;
                break;
            }
            index++;
        }
        if(ifContinuous) // 如果没有空缺，序列号顺延下一位
            index++;
        if(p->query.size() == 0)
            index = 1; // 如果无数据序列号取0
        ret = p->query.exec("INSERT INTO custom(id, account_id, c_name, post, createTime, info)\
                    VALUES('" + QString::number(index) + "', '"+ u_id + "', '"
                    + name + "', '" + post + "','" + QString::number(creteTime) + "', '" + info + "');");
        if(return_id != nullptr)
            *return_id = index;
    }
    else // 更新收藏夹
    {
        if(return_id != nullptr)
            *return_id = c_id;
        ret = p->query.exec("UPDATE custom\
                SET \
                c_name = '" + name + "', \
                post = '" + post + "', \
                createTime = '" + QString::number(creteTime) + "', \
                info = '" + info + "' \
                WHERE id = '" + QString::number(c_id) + "' AND account_id = '"+ u_id + "';");
    }
    return ret;
}

bool SqlService::updateCollectionContain(quint64 c_id, QCStrRef u_id, const QStringList &m_ids)
{
    p->query.exec("SELECT * FROM custom WHERE id = '" + QString::number(c_id) + "' AND account_id = '" + u_id + "';");
    if(p->query.size()<1)
    {
        MYLOG<<"未找到用户的收藏夹";
        return false;
    }

    p->query.exec("SELECT * FROM custom_source WHERE custom_id = '" + QString::number(c_id) + "';");
    if(p->query.size()>0) // 全部更新要删除原有的对应关系
        p->query.exec("DELETE FROM custom_source WHERE custom_id = '" + QString::number(c_id) + "';");

    for(int i = 0; i < m_ids.size(); i++)
        p->query.exec("INSERT INTO custom_source(custom_id, source_id) VALUES('" + QString::number(c_id) + "', '" + m_ids[i] + "');");
    return true;
}

bool SqlService::deleteCollection(QCStrRef c_id, QCStrRef u_id)
{
    p->query.exec("SELECT * FROM custom WHERE id = '" + c_id + "' AND account_id = '" + u_id + "';");
    if(p->query.size()<1) // 未找到用户的文件夹. size 不包括表头
    {
        this->printErrorMessage();
        MYLOG<<"未找到用户的文件夹";
        return true;
    }

    p->query.exec("DELETE FROM custom_source WHERE custom_id = '" + c_id + "';");
    p->query.exec("DELETE FROM custom WHERE id = '" + c_id + "';");
    return true;
}

bool SqlService::updateAlbum(QCStrRef a_id, QCStrRef u_id, bool val)
{
    if(val)
        return p->query.exec("INSERT INTO account_playlist(playlist_id, account_id) VALUES ('" + a_id + "', '" + u_id + "');");
    else
        return p->query.exec("DELETE FROM account_playlist WHERE playlist_id = '" + a_id + "' AND account_id = '" + u_id + "';");
    return false;
}

bool SqlService::changeUserIcon(QCStrRef u_id, QCStrRef icon)
{
    return p->query.exec("UPDATE account SET icon = '" + icon + "' WHERE id = '" + u_id + "';");
}

bool SqlService::changeUserName(QCStrRef u_id, QCStrRef name)
{
    p->query.exec("SELECT * FROM account WHERE id != '" + u_id + "' AND account.`name` = '" + u_id + "';");
    if(p->query.size()>0)
        return false;
    return p->query.exec("UPDATE account SET account.`name` = '" + name + "' WHERE id = '" + u_id + "';");
}
