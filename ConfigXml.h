#ifndef CONFIGXML_H
#define CONFIGXML_H

#include <QObject>
#include <QtXml>
/*
专门处理xml
*/
class ConfigXml : public QObject
{
    Q_OBJECT
public:
    ConfigXml(const QString& filePath);
    ConfigXml();
    ~ConfigXml();

    bool LoadXMLPath(const QString& Path);
    QDomElement ReadFromXML();
    bool WriteToXML();

    static bool createFile(const QString& filePath);
    static bool createEmptyPrivateXML(const QString& filePath);
    static bool WriteToXML(const QDomDocument& doc, const QString& filePath);

private:
    bool hasPath;
    QString filePath;
    QDomDocument doc;
};



#endif // CONFIGXML_H
