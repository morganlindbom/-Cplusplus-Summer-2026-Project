// persistence_contract_tests.cpp
#include "systems/project/ProjectDirtyState.hpp"
#include "systems/project/ProjectStore.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
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

FunctionSelection selection(const QString& component, const QString& function, int gpio)
{
    FunctionSelection value;
    value.componentId = component;
    value.displayName = component;
    value.physicalPin = gpio + 1;
    value.gpio = gpio;
    value.functionId = function;
    value.functionName = function;
    return value;
}

ApplicationState populatedState(const QString& path, const QString& name)
{
    ApplicationState state;
    state.projectName = name;
    state.projectPath = path;
    state.product = "Raspberry Pi Pico 2";
    state.language = "C";
    state.state = "Testing";
    state.debugSessionTools = false;
    state.runtimeDiagnostics = false;
    state.verboseBuildEvidence = true;
    auto first = selection("pin_1", "sio", 0);
    first.settings = {{"direction", "Output"}, {"pull", "Pull-up"}};
    auto second = selection("pin_2", "pwm0b", 1);
    second.settings = {{"frequency_hz", "1000"}, {"duty_percent", "25"}};
    state.selections.insert(first.componentId, first);
    state.selections.insert(second.componentId, second);
    return state;
}

bool equalPersistent(const ApplicationState& left, const ApplicationState& right)
{
    if (left.projectName != right.projectName || left.product != right.product || left.language != right.language ||
        left.state != right.state || left.debugSessionTools != right.debugSessionTools ||
        left.runtimeDiagnostics != right.runtimeDiagnostics ||
        left.verboseBuildEvidence != right.verboseBuildEvidence || left.selections.size() != right.selections.size())
        return false;
    for (auto it = left.selections.cbegin(); it != left.selections.cend(); ++it)
    {
        if (!right.selections.contains(it.key()))
            return false;
        const auto& a = it.value();
        const auto& b = right.selections[it.key()];
        if (a.componentId != b.componentId || a.displayName != b.displayName || a.physicalPin != b.physicalPin ||
            a.gpio != b.gpio || a.functionId != b.functionId || a.functionName != b.functionName ||
            a.settings != b.settings)
            return false;
    }
    return true;
}

bool execute(const QString& database, const QString& sql)
{
    const QString connection = "persistence_fixture_" + QString::number(reinterpret_cast<quintptr>(&database));
    auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
    db.setDatabaseName(database);
    const bool opened = db.open();
    QSqlQuery query(db);
    const bool result = opened && query.exec(sql);
    db.close();
    QSqlDatabase::removeDatabase(connection);
    return result;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    {
        QTemporaryDir directory;
        const ApplicationState expected = populatedState(directory.path(), "ROUND_TRIP");
        QString error;
        check(ProjectStore::save(expected, &error), "PERSIST-001 save failed");
        ApplicationState actual;
        check(ProjectStore::load(directory.path(), &actual, &error), "PERSIST-001 load failed");
        check(equalPersistent(expected, actual), "PERSIST-001 persistent fields differ");
    }
    {
        QTemporaryDir directory;
        ApplicationState expected;
        expected.projectName = "SIO_INSTANCE_ISOLATION";
        expected.projectPath = directory.path();
        expected.product = "Raspberry Pi Pico 2";
        expected.language = "C++";
        expected.state = "Testing";
        auto pin1 = selection("pin_1", "sio", 0);
        auto pin2 = selection("pin_2", "sio", 1);
        auto pin3 = selection("pin_3", "sio", 2);
        pin1.settings.insert("direction", "Output");
        pin3.settings.insert("direction", "Output");
        expected.selections.insert(pin1.componentId, pin1);
        expected.selections.insert(pin2.componentId, pin2);
        expected.selections.insert(pin3.componentId, pin3);
        QString error;
        check(ProjectStore::save(expected, &error), "PERSIST-SIO-001 same-function save failed");
        ApplicationState actual;
        check(ProjectStore::load(directory.path(), &actual, &error), "PERSIST-SIO-003 same-function reload failed");
        check(actual.selections.value("pin_1").settings.value("direction") == "Output" &&
                  actual.selections.value("pin_2").settings.value("direction") == "Input" &&
                  actual.selections.value("pin_3").settings.value("direction") == "Output",
              "PERSIST-SIO-004 missing SIO default or same-function directions collided after reload");
        actual.selections["pin_1"].settings["direction"] = "Input";
        actual.selections["pin_2"].settings["direction"] = "Output";
        actual.selections["pin_3"].settings["direction"] = "Input";
        check(ProjectStore::save(actual, &error), "PERSIST-SIO-005 reverse same-function save failed");
        ApplicationState reversed;
        check(ProjectStore::load(directory.path(), &reversed, &error), "PERSIST-SIO-006 reverse reload failed");
        check(reversed.selections.value("pin_1").settings.value("direction") == "Input" &&
                  reversed.selections.value("pin_2").settings.value("direction") == "Output" &&
                  reversed.selections.value("pin_3").settings.value("direction") == "Input",
              "PERSIST-SIO-007 reverse same-function directions collided after reload");
    }
    {
        ProjectDirtyState dirty;
        dirty.markDirty();
        check(dirty.isDirty(), "PERSIST-002 persistent mutation did not mark dirty");
    }
    {
        QTemporaryDir directory;
        auto state = populatedState(directory.path(), "SAVE_CLEAR");
        ProjectDirtyState dirty;
        dirty.markDirty();
        QString error;
        check(ProjectStore::save(state, &error), "PERSIST-003 save failed");
        dirty.markSaved();
        check(!dirty.isDirty(), "PERSIST-003 successful save did not clear dirty");
    }
    {
        QTemporaryDir directory;
        const QString blocked = directory.filePath("blocked");
        QFile file(blocked);
        check(file.open(QIODevice::WriteOnly), "PERSIST-004 fixture file could not be created");
        auto state = populatedState(blocked, "SAVE_FAILURE");
        const ApplicationState before = state;
        ProjectDirtyState dirty;
        dirty.markDirty();
        QString error;
        check(!ProjectStore::save(state, &error), "PERSIST-004 invalid save unexpectedly succeeded");
        check(dirty.isDirty(), "PERSIST-004 failed save cleared dirty");
        check(equalPersistent(before, state), "PERSIST-004 failed save changed state");
    }
    {
        QTemporaryDir firstDirectory;
        QTemporaryDir secondDirectory;
        auto expected = populatedState(secondDirectory.path(), "VALID_B");
        QString error;
        check(ProjectStore::save(expected, &error), "PERSIST-005 fixture save failed");
        auto actual = populatedState(firstDirectory.path(), "VALID_A");
        check(ProjectStore::load(secondDirectory.path(), &actual, &error), "PERSIST-005 valid load failed");
        check(equalPersistent(expected, actual), "PERSIST-005 state transition mixed A and B");
    }
    {
        QTemporaryDir directory;
        auto state = populatedState(directory.path(), "SELECTION_FAILURE");
        QString error;
        check(ProjectStore::save(state, &error), "PERSIST-006 fixture save failed");
        const ApplicationState before = state;
        const QString database = directory.filePath("SELECTION_FAILURE.sqlite");
        check(execute(database, "DROP TABLE selections"), "PERSIST-006 fixture corruption failed");
        check(!ProjectStore::load(database, &state, &error), "PERSIST-006 load unexpectedly succeeded");
        check(equalPersistent(before, state), "PERSIST-006 live state changed after selection failure");
    }
    {
        QTemporaryDir directory;
        auto state = populatedState(directory.path(), "SETTINGS_FAILURE");
        QString error;
        check(ProjectStore::save(state, &error), "PERSIST-007 fixture save failed");
        const ApplicationState before = state;
        const QString database = directory.filePath("SETTINGS_FAILURE.sqlite");
        check(execute(database, "DROP TABLE settings"), "PERSIST-007 fixture corruption failed");
        check(!ProjectStore::load(database, &state, &error), "PERSIST-007 load unexpectedly succeeded");
        check(equalPersistent(before, state), "PERSIST-007 live state changed after settings failure");
    }
    {
        QTemporaryDir directory;
        const QString database = directory.filePath("CORRUPT.sqlite");
        check(execute(database, "CREATE TABLE project(key TEXT PRIMARY KEY,value TEXT)"),
              "PERSIST-008 fixture creation failed");
        ApplicationState state = populatedState(directory.path(), "UNCHANGED");
        const ApplicationState before = state;
        QString error;
        check(!ProjectStore::load(database, &state, &error), "PERSIST-008 incomplete database accepted");
        check(equalPersistent(before, state), "PERSIST-008 live state changed after corrupt load");
        check(!error.isEmpty(), "PERSIST-008 did not report a useful error");
    }
    {
        QTemporaryDir directory;
        auto state = populatedState(directory.path(), "DERIVED_FILES");
        QString error;
        check(ProjectStore::save(state, &error), "PERSIST-009 fixture save failed");
        QDir generated(directory.filePath("generated"));
        check(generated.mkpath("."), "PERSIST-009 generated directory creation failed");
        QFile generatedFile(generated.filePath("main.cpp"));
        check(generatedFile.open(QIODevice::WriteOnly), "PERSIST-009 generated file creation failed");
        generatedFile.write("generated");
        generatedFile.close();
        ApplicationState reopened;
        check(ProjectStore::load(directory.path(), &reopened, &error), "PERSIST-009 reload failed");
        check(reopened.generatedFiles == QStringList{generated.absoluteFilePath("main.cpp")},
              "PERSIST-009 generatedFiles was not reconstructed");
    }
    {
        QTemporaryDir directory;
        const auto expected = populatedState(directory.path(), "OPTIONS");
        QString error;
        check(ProjectStore::save(expected, &error), "PERSIST-010 save failed");
        ApplicationState actual;
        check(ProjectStore::load(directory.path(), &actual, &error), "PERSIST-010 load failed");
        check(expected.product == actual.product && expected.language == actual.language &&
                  expected.state == actual.state && expected.debugSessionTools == actual.debugSessionTools &&
                  expected.runtimeDiagnostics == actual.runtimeDiagnostics &&
                  expected.verboseBuildEvidence == actual.verboseBuildEvidence,
              "PERSIST-010 project-level options were not restored");
    }
    {
        QTemporaryDir directory;
        auto expected = populatedState(directory.path(), "LEGACY_INPUT");
        expected.selections["pin_1"].functionId = "gpio.input";
        expected.selections["pin_1"].functionName = "GPIO Input";
        expected.selections["pin_1"].settings = {{"pull", "Pull-down"}, {"debounce_ms", "25"}};
        QString error;
        check(ProjectStore::save(expected, &error), "SIO-MIG-001 legacy input fixture save failed");
        const QString database = directory.filePath("LEGACY_INPUT.sqlite");
        check(
            execute(
                database,
                "UPDATE selections SET function_id='gpio.input',function_name='GPIO Input' WHERE component_id='pin_1'"),
            "SIO-MIG-001 legacy input mutation failed");
        ApplicationState actual;
        check(ProjectStore::load(directory.path(), &actual, &error), "SIO-MIG-001 legacy input load failed");
        check(actual.selections["pin_1"].functionId == "sio" &&
                  actual.selections["pin_1"].settings.value("direction") == "Input" &&
                  actual.selections["pin_1"].settings.value("pull") == "Pull-down" &&
                  actual.selections["pin_1"].settings.value("debounce_ms") == "25",
              "SIO-MIG-001/003 input was not canonicalized with settings preserved");
        check(ProjectStore::save(actual, &error), "SIO-MIG-005 canonical input save failed");
        ApplicationState reloaded;
        check(ProjectStore::load(directory.path(), &reloaded, &error), "SIO-MIG-007 canonical input reload failed");
        check(reloaded.selections["pin_1"].functionId == "sio" &&
                  reloaded.selections["pin_1"].settings.value("direction") == "Input",
              "SIO-MIG-005/007 canonical input did not survive save/reload");
    }
    {
        QTemporaryDir directory;
        auto expected = populatedState(directory.path(), "LEGACY_OUTPUT");
        expected.selections["pin_1"].functionId = "gpio.output";
        expected.selections["pin_1"].functionName = "GPIO Output";
        expected.selections["pin_1"].settings = {{"initial_state", "High"},
                                                 {"blink_enabled", "true"},
                                                 {"blink_interval_ms", "750"},
                                                 {"drive_strength", "8"},
                                                 {"slew_rate", "Slow"}};
        QString error;
        check(ProjectStore::save(expected, &error), "SIO-MIG-002 legacy output fixture save failed");
        const QString database = directory.filePath("LEGACY_OUTPUT.sqlite");
        check(execute(database, "UPDATE selections SET function_id='gpio.output',function_name='GPIO Output' WHERE "
                                "component_id='pin_1'"),
              "SIO-MIG-002 legacy output mutation failed");
        ApplicationState actual;
        check(ProjectStore::load(directory.path(), &actual, &error), "SIO-MIG-002 legacy output load failed");
        const auto& migrated = actual.selections["pin_1"];
        check(migrated.functionId == "sio" && migrated.settings.value("direction") == "Output" &&
                  migrated.settings.value("initial_state") == "High" &&
                  migrated.settings.value("blink_enabled") == "true" &&
                  migrated.settings.value("blink_interval_ms") == "750" &&
                  migrated.settings.value("drive_strength") == "8" && migrated.settings.value("slew_rate") == "Slow",
              "SIO-MIG-002/004 output was not canonicalized with settings preserved");
        check(ProjectStore::save(actual, &error), "SIO-MIG-006 canonical output save failed");
        ApplicationState reloaded;
        check(ProjectStore::load(directory.path(), &reloaded, &error), "SIO-MIG-007 canonical output reload failed");
        check(reloaded.selections["pin_1"].functionId == "sio" &&
                  reloaded.selections["pin_1"].settings.value("direction") == "Output",
              "SIO-MIG-006/007 canonical output did not survive save/reload");
    }
    if (failures == 0)
        std::cout << "All persistence contract tests passed.\n";
    return failures == 0 ? 0 : 1;
}
