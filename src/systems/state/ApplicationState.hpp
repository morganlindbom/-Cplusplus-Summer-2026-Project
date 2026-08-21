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
    QString product = "Raspberry Pi Pico 2 W";
    QString language = "C++";
    QString state = "Development";
    bool debugSessionTools = true;
    bool runtimeDiagnostics = true;
    bool verboseBuildEvidence = true;
    QHash<QString, FunctionSelection> selections;
    QStringList generatedFiles;
};
}
