// main.cpp
#include "systems/System.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    /**Starts Qt and delegates the entire application lifecycle to System.*/
    QApplication app(argc, argv);
    qInfo().noquote() << "PVD executable:" << QCoreApplication::applicationFilePath()
                      << "PID:" << QCoreApplication::applicationPid();
    QCommandLineParser parser;
    parser.setApplicationDescription("Pico Visual Designer");
    parser.addHelpOption();
    QCommandLineOption certificationDialogs("certification-dialogs",
                                            "Use deterministic Qt-owned dialogs for GUI certification.");
    parser.addOption(certificationDialogs);
    QCommandLineOption openProject("open-project",
                                   "Load a project database before showing the application.",
                                   "path");
    parser.addOption(openProject);
    parser.process(app);
    pvd::System system(parser.isSet(certificationDialogs));
    if (parser.isSet(openProject))
    {
        QString error;
        if (!system.loadProjectFromPath(parser.value(openProject), &error))
        {
            qCritical().noquote() << "Project load failed:" << error;
            return 2;
        }
    }
    system.run();
    return app.exec();
}
