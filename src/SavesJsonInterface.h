#ifndef SAVESJSONINTERFACE_H
#define SAVESJSONINTERFACE_H

class QJsonObject;

class SavesJsonInterface
{
public:
    virtual QJsonObject save2json() = 0;
    virtual bool loadFromJson(const QJsonObject& obj) = 0;
};

#endif // SAVESJSONINTERFACE_H
