// main.cpp
#include "systems/System.hpp"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char* argv[])
{
    /**Starts Qt and delegates the entire application lifecycle to System.*/
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Pico Visual Designer");
    parser.addHelpOption();
    QCommandLineOption certificationDialogs("certification-dialogs",
                                            "Use deterministic Qt-owned dialogs for GUI certification.");
    parser.addOption(certificationDialogs);
    parser.process(app);
    pvd::System system(parser.isSet(certificationDialogs));
    system.run();
    return app.exec();
}
