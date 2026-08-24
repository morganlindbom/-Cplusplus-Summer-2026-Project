// dialog_service_tests.cpp
#include "systems/mainwindow/dialog/AutomationFileDialogService.hpp"
#include "systems/mainwindow/dialog/IFileDialogService.hpp"
#include "systems/mainwindow/dialog/NativeFileDialogService.hpp"
#include "systems/mainwindow/project/project_column3/ProjectColumn3.hpp"
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>

using namespace pvd;

namespace
{
int failures = 0;

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

class FakeDialogService final : public IFileDialogService
{
  public:
    QString openResult;
    QString directoryResult;

    /**Returns the configured Open result without owning application state.*/
    QString getOpenFileName(QWidget*, const QString&, const QString&, const QString&) override
    {
        return openResult;
    }

    /**Returns the configured directory result without owning application state.*/
    QString getExistingDirectory(QWidget*, const QString&, const QString&) override
    {
        return directoryResult;
    }
};

QString componentDatabase(const QTemporaryDir& directory)
{
    /**Creates the minimal component database required to construct ProjectColumn3.*/
    const QString path = directory.filePath("project_column3.sqlite");
    const QString connection = "dialog_service_fixture";
    auto database = QSqlDatabase::addDatabase("QSQLITE", connection);
    database.setDatabaseName(path);
    check(database.open(), "DIALOG fixture database could not open");
    QSqlQuery query(database);
    check(query.exec("CREATE TABLE metadata(key TEXT PRIMARY KEY,value TEXT)"),
          "DIALOG fixture metadata table could not be created");
    check(query.exec("INSERT INTO metadata VALUES ('title','Project Management')"),
          "DIALOG fixture metadata could not be inserted");
    database.close();
    QSqlDatabase::removeDatabase(connection);
    return path;
}
} // namespace

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    // DIALOG-001, DIALOG-006, DIALOG-007: explicit service selection.
    check(dynamic_cast<NativeFileDialogService*>(makeFileDialogService(false).get()) != nullptr,
          "DIALOG-001 production mode did not select native implementation");
    check(dynamic_cast<AutomationFileDialogService*>(makeFileDialogService(true).get()) != nullptr,
          "DIALOG-006 certification mode did not select automation implementation");
    check(dynamic_cast<NativeFileDialogService*>(makeFileDialogService(false).get()) != nullptr,
          "DIALOG-007 normal mode did not select native implementation");

    QTemporaryDir fixture;
    const QString database = componentDatabase(fixture);
    FakeDialogService service;
    ProjectColumn3 panel(database, &service);
    panel.show();

    // DIALOG-003, DIALOG-005: the selected path follows openRequested unchanged.
    service.openResult = fixture.filePath("selected.sqlite");
    QString openedPath;
    int openCount = 0;
    QObject::connect(&panel, &ProjectColumn3::openRequested, &panel,
                     [&openedPath, &openCount](const QString& path)
                     {
                         openedPath = path;
                         ++openCount;
                     });
    auto* openButton = panel.findChild<QPushButton*>("project_open");
    check(openButton != nullptr, "DIALOG-003 Open control was not found");
    if (openButton)
        openButton->click();
    check(openCount == 1, "DIALOG-005 Open signal was not emitted exactly once");
    check(openedPath == service.openResult, "DIALOG-003 selected path did not reach openRequested");

    // DIALOG-004, DIALOG-008: cancellation emits no path and cannot load a project.
    service.openResult.clear();
    if (openButton)
        openButton->click();
    check(openCount == 1, "DIALOG-004 cancel emitted openRequested");
    check(openCount == 1, "DIALOG-008 cancellation crossed the backend boundary");

    // DIALOG-002, DIALOG-003: exercise the real Qt-owned non-native dialog and select a file through it.
    const QString selectedFile = fixture.filePath("automation-selected.sqlite");
    QFile selectedFileHandle(selectedFile);
    check(selectedFileHandle.open(QIODevice::WriteOnly), "DIALOG fixture project file could not be created");
    selectedFileHandle.close();
    AutomationFileDialogService automation;
    QTimer::singleShot(0,
                       [&selectedFile]()
                       {
                           for (auto* widget : QApplication::topLevelWidgets())
                           {
                               auto* dialog = qobject_cast<QFileDialog*>(widget);
                               if (dialog && dialog->objectName() == "pvd_automation_file_dialog")
                               {
                                   dialog->selectFile(selectedFile);
                                   QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
                                   return;
                               }
                           }
                       });
    check(automation.getOpenFileName(nullptr, "Open", fixture.path(), "SQLite (*.sqlite)") == selectedFile,
          "DIALOG-002/DIALOG-003 non-native dialog did not return selected file");

    // DIALOG-009, DIALOG-010: existing persistence fixture and Save control remain present.
    check(QFileInfo::exists(database), "DIALOG-009 project fixture was not retained");
    check(panel.findChild<QPushButton*>("project_save") != nullptr, "DIALOG-010 Save control is missing");

    if (failures == 0)
        std::cout << "All dialog service tests passed.\n";
    return failures == 0 ? 0 : 1;
}
