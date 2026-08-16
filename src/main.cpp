// main.cpp
#include "systems/System.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
    /**Starts Qt and delegates the entire application lifecycle to System.*/
    QApplication app(argc, argv);
    pvd::System system;
    system.run();
    return app.exec();
}
