#include "MusicServer.h"
#include "common.h"
#include "SqlService.h"



#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MYLOG<<"路径检查："<<S::CheckPath_create();
    SqlService* sql = &SqlService::getInstance();
    if(!sql->doInit(S::sqlServer, S::sqlTable, S::sqlUsr, S::sqlPwd))
    {
        MYLOG<<"数据库无法连接";
        return 0;
    }

    MusicServer s;
    s.initServer(); // 初始化

    bool ret = s.startServer("127.0.0.1", 8081);
    MYLOG<< (ret?"启动成功":"启动失败");
    if(!ret)
        return 0;

    return a.exec();
}
