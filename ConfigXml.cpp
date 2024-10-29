#include "ConfigXml.h"
#include "common.h"
#include <QFile>
#include <QDebug>



ConfigXml::ConfigXml(const QString& filePath):filePath(filePath)
{
    hasPath = true;
}

ConfigXml::ConfigXml()
{
    hasPath = false;
}

ConfigXml::~ConfigXml()
{

}

bool ConfigXml::LoadXMLPath(const QString &Path)
{
    QFile file(Path);
    if(file.open(QIODevice::ReadOnly) == false)
    {
        MYLOG << "xml 文件打开失败："<< Path ;
        return false;
    }
    file.close();
    this->filePath = Path;
    hasPath = true;
    return true;
}


QDomElement ConfigXml::ReadFromXML()
{
    QDomElement root;
    if(!hasPath)
    {
        MYLOG<< "未加载文件路径";
        return root;
    }
    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly) == false)
    {
        MYLOG << "xml 文件打开失败："<< filePath ;
        return root;
    }

    auto Content = file.readAll();
    file.close();
    QString errorMsg;
    int row = 0, column = 0;
    // 解析xml文件内容
    if(doc.setContent(Content, &errorMsg, &row, &column) == false)
    {
        MYLOG<<filePath<<"xml 文件解析失败:"<<errorMsg;
        MYLOG<<"row:"<<row<<"column:"<<column;
        return root;
    }
    // MYLOG<<"xml 文件解析成功!";


    root = std::move(doc.documentElement());

    return root;
}

bool ConfigXml::WriteToXML()
{
    QFile file(filePath);
    if(file.open(QIODevice::WriteOnly) == false)
    {
        MYLOG<< "xml 文件打开失败："<< filePath;
        return false;
    }

    if (file.write(doc.toByteArray()) <= 0)
    {
        MYLOG << "Debug message at" << __FILE__ << ":" << __LINE__ <<"xml 文件写入失败!";
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool ConfigXml::createFile(const QString &filePath)
{
    QFile tmpFile;
    tmpFile.setFileName(filePath);
    if(tmpFile.exists())
        tmpFile.remove();
    bool ret = tmpFile.open(QIODevice::NewOnly);
    tmpFile.close();
    return ret;
}

bool ConfigXml::createEmptyPrivateXML(const QString &filePath)
{
    createFile(filePath);
    QFile tmpFile(filePath);

    QDomDocument doc;
    // xml头部格式
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);

    // xml根节点
    QDomElement root = doc.createElement("LoginAccount");
    doc.appendChild(root);

    // 用户属性
    {   // 收藏
        QDomElement Favourite = doc.createElement("Favourite");
        root.appendChild(Favourite);
        QDomElement source = doc.createElement("source");
        Favourite.appendChild(source);
        QDomElement playlist = doc.createElement("playlist");
        Favourite.appendChild(playlist);
        QDomElement customized = doc.createElement("customized");
        Favourite.appendChild(customized);
    }

    { // 设置、信息
    }

    bool ret = WriteToXML(doc, filePath);
    return ret;
}

bool ConfigXml::WriteToXML(const QDomDocument& doc, const QString& filePath)
{
    bool ret = false;
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    if (file.isOpen()) {
        QTextStream stream(&file);
        doc.save(stream, 4);
        ret = !stream.status(); // 检查stream的状态
    }
    file.close();
    return ret;
}
