// ApplicationState.hpp
#pragma once
#include <QHash>
#include <QString>
#include <QStringList>

namespace pvd {
struct FunctionSelection
{
    QString componentId;
    QString displayName;
    int physicalPin = 0;
    int gpio = -1;
    QString functionId;
    QString functionName;
    QHash<QString, QString> settings;
};

class ApplicationState final
{
public:
    QString projectName = "PICO2W";
    QString projectPath;
    QString language = "C++";
    QString state = "Development";
    QHash<QString, FunctionSelection> selections;
    QStringList generatedFiles;
};
}
