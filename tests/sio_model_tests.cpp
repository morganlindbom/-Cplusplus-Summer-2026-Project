// sio_model_tests.cpp
#include "systems/components/FunctionCatalog.hpp"
#include "systems/generation/ProjectGenerator.hpp"
#include "systems/mainwindow/settings/settings_column3/SettingsColumn3.hpp"
#include "systems/project/ProjectStore.hpp"
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSet>
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

FunctionSelection sio(const QString& component, int gpio, const QString& direction)
{
    FunctionSelection value;
    value.componentId = component;
    value.displayName = component;
    value.functionId = "sio";
    value.functionName = "SIO";
    value.gpio = gpio;
    value.physicalPin = gpio + 1;
    value.settings = {{"direction", direction}, {"pull", "None"}};
    return value;
}

QString generated(const FunctionSelection& pin, const QTemporaryDir& directory)
{
    ApplicationState state;
    state.projectName = "SIO_MODEL";
    state.projectPath = directory.path();
    state.product = "Raspberry Pi Pico 2";
    state.language = "C++";
    state.state = "Testing";
    auto rp = sio("rp2350a", -1, "Input");
    rp.functionId = "rp2350a.configure";
    rp.functionName = "Configure RP2350A";
    rp.gpio = -1;
    rp.physicalPin = 0;
    rp.settings = {{"enabled", "true"}, {"stdio_usb", "true"}};
    state.selections.insert(rp.componentId, rp);
    state.selections.insert(pin.componentId, pin);
    QString error;
    check(ProjectGenerator::generate(&state, {}, &error), "SIO generator failed");
    QFile file(QDir(directory.path()).filePath("generated/main.cpp"));
    check(file.open(QIODevice::ReadOnly), "SIO generated source could not be read");
    return QString::fromUtf8(file.readAll());
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QString root = QDir(QCoreApplication::applicationDirPath()).filePath("runtime/src/systems/components/pin_functions");
    FunctionCatalog catalog(root);
    const auto functions = catalog.functionsForGpio(0);
    bool hasSio = false;
    bool hasLegacy = false;
    for (const auto& function : functions)
    {
        hasSio = hasSio || function.id == "sio";
        hasLegacy = hasLegacy || function.id == "gpio.input" || function.id == "gpio.output";
    }
    check(hasSio && !hasLegacy, "SIO-001 function catalog still exposes a legacy GPIO function");

    const QString database = catalog.functionDatabase("sio");
    check(!database.isEmpty(), "SIO database was not discovered");
    const QString connection = "sio_model_metadata";
    auto sql = QSqlDatabase::addDatabase("QSQLITE", connection);
    sql.setDatabaseName(database);
    check(sql.open(), "SIO database could not be opened");
    QSqlQuery query(sql);
    check(query.exec("SELECT setting_key,category,applicable_when FROM settings"), "SIO metadata query failed");
    QSet<QString> keys;
    while (query.next())
        keys.insert(query.value(0).toString());
    check(keys == QSet<QString>{"direction", "pull", "debounce_ms", "initial_state", "blink_enabled",
                                "blink_interval_ms", "drive_strength", "slew_rate"},
          "SIO-002/SIO-003 database does not own the complete setting set");
    check(query.exec("SELECT COUNT(*) FROM settings WHERE setting_key='debounce_ms' AND applicable_when='direction=Input'"),
          "SIO-004 input applicability query failed");
    query.next();
    check(query.value(0).toInt() == 1, "SIO-004 debounce applicability missing");
    check(query.exec("SELECT COUNT(*) FROM settings WHERE setting_key IN ('initial_state','drive_strength','slew_rate') AND applicable_when='direction=Output'"),
          "SIO-005 output applicability query failed");
    query.next();
    check(query.value(0).toInt() == 3, "SIO-005 output applicability missing");
    check(query.exec("SELECT COUNT(*) FROM settings WHERE category='PAD_CONFIGURATION'"), "SIO-006 category query failed");
    query.next();
    check(query.value(0).toInt() == 3, "SIO-006 pad settings are not categorized");
    sql.close();
    QSqlDatabase::removeDatabase(connection);

    SettingsColumn3 settings(QString(), &catalog);
    ApplicationState lifecycle;
    auto lifecyclePin1 = sio("pin_1", 0, "Output");
    auto lifecyclePin2 = sio("pin_2", 1, "Input");
    lifecycle.selections.insert(lifecyclePin1.componentId, lifecyclePin1);
    lifecycle.selections.insert(lifecyclePin2.componentId, lifecyclePin2);
    QObject::connect(&settings, &SettingsColumn3::settingChanged,
                     [&lifecycle](const QString& id, const QString& key, const QString& value)
                     {
                         lifecycle.selections[id].settings[key] = value;
                     });
    QStringList directionChanges;
    QObject::connect(&settings, &SettingsColumn3::settingChanged,
                     [&directionChanges](const QString&, const QString& key, const QString& value)
                     {
                         if (key == "direction")
                             directionChanges.append(value);
                     });
    auto input = sio("pin1", 0, "Input");
    settings.showSelection(input);
    check(settings.findChild<QWidget*>("setting_direction") != nullptr, "SIO-007 direction editor missing");
    check(settings.findChild<QWidget*>("setting_debounce_ms") != nullptr, "SIO-008 input debounce editor missing");
    check(settings.findChild<QWidget*>("setting_initial_state") == nullptr, "SIO-009 output setting was not hidden for input");
    auto* direction = settings.findChild<QComboBox*>("setting_direction");
    check(direction != nullptr, "SIO-010 direction editor is not a combo");
    if (direction)
    {
        direction->setCurrentText("Output");
        QCoreApplication::processEvents();
    }
    check(settings.findChild<QWidget*>("setting_initial_state") != nullptr, "SIO-011 output setting was not rebuilt");
    check(settings.findChild<QWidget*>("setting_debounce_ms") == nullptr, "SIO-012 input setting leaked into output");
    for (int i = 0; i < 20; ++i)
    {
        auto* currentDirection = settings.findChild<QComboBox*>("setting_direction");
        check(currentDirection != nullptr, "SIO-CRASH-003 direction editor disappeared during alternation");
        if (!currentDirection)
            break;
        currentDirection->setCurrentText(i % 2 == 0 ? "Input" : "Output");
        QCoreApplication::processEvents();
        const bool output = (i % 2) != 0;
        check(settings.findChild<QWidget*>(output ? "setting_initial_state" : "setting_debounce_ms") != nullptr,
              "SIO-CRASH-004 mode-specific setting missing after alternation");
        check(settings.findChild<QWidget*>(output ? "setting_debounce_ms" : "setting_initial_state") == nullptr,
              "SIO-CRASH-004 stale mode-specific setting survived alternation");
    }
    check(directionChanges.size() >= 21, "SIO-CRASH-005 direction model changes were not preserved");
    settings.showSelection(lifecyclePin1);
    QCoreApplication::processEvents();
    auto* staleEditor = settings.findChild<QComboBox*>("setting_direction");
    QPointer<QComboBox> stalePointer(staleEditor);
    check(staleEditor != nullptr && staleEditor->currentText() == "Output", "SIO-LIFE-001 pin_1 did not display Output");
    settings.showSelection(lifecyclePin2);
    QCoreApplication::processEvents();
    check(settings.findChild<QComboBox*>("setting_direction")->currentText() == "Input" &&
              lifecycle.selections["pin_1"].settings.value("direction") == "Output" &&
              lifecycle.selections["pin_2"].settings.value("direction") == "Input",
          "SIO-LIFE-002 switching to pin_2 changed pin_1 or displayed the wrong value");
    for (int i = 0; i < 20; ++i)
    {
        const auto& selected = (i % 2 == 0) ? lifecyclePin1 : lifecyclePin2;
        settings.showSelection(selected);
        QCoreApplication::processEvents();
        const QString expected = (i % 2 == 0) ? "Output" : "Input";
        check(settings.findChild<QComboBox*>("setting_direction")->currentText() == expected &&
                  lifecycle.selections["pin_1"].settings.value("direction") == "Output" &&
                  lifecycle.selections["pin_2"].settings.value("direction") == "Input",
              "SIO-LIFE-003 rapid navigation mutated a selection during rebuild");
    }
    QMetaObject::invokeMethod(&settings, [stalePointer]() mutable
                               {
                                   if (!stalePointer.isNull())
                                       stalePointer->activated(stalePointer->findText("Input"));
                               },
                               Qt::QueuedConnection);
    QCoreApplication::processEvents();
    check(lifecycle.selections["pin_1"].settings.value("direction") == "Output" &&
              lifecycle.selections["pin_2"].settings.value("direction") == "Input",
          "SIO-LIFE-004 stale queued editor write was not rejected");
    QTemporaryDir persistenceDirectory;
    ApplicationState persisted;
    persisted.projectName = "SIO_DIRECTION_RELOAD";
    persisted.projectPath = persistenceDirectory.path();
    persisted.product = "Raspberry Pi Pico 2";
    persisted.language = "C++";
    persisted.state = "Testing";
    auto persistedPin = sio("pin_1", 0, "Output");
    persistedPin.settings.insert("initial_state", "Low");
    persisted.selections.insert(persistedPin.componentId, persistedPin);
    QString persistenceError;
    check(ProjectStore::save(persisted, &persistenceError), "SIO-CRASH-007 save failed");
    ApplicationState reloaded;
    check(ProjectStore::load(persistenceDirectory.path(), &reloaded, &persistenceError), "SIO-CRASH-007 reload failed");
    SettingsColumn3 reloadedSettings(QString(), &catalog);
    reloadedSettings.showSelection(reloaded.selections.value("pin_1"));
    QCoreApplication::processEvents();
    auto* reloadedDirection = reloadedSettings.findChild<QComboBox*>("setting_direction");
    check(reloadedDirection != nullptr && reloadedDirection->currentText() == "Output",
          "SIO-CRASH-007 direction was not preserved after reload");
    check(reloadedSettings.findChild<QWidget*>("setting_initial_state") != nullptr &&
              reloadedSettings.findChild<QWidget*>("setting_debounce_ms") == nullptr,
          "SIO-CRASH-007 output form was not rebuilt after reload");
    auto output = sio("pin2", 1, "Output");
    settings.showSelection(output);
    QCoreApplication::processEvents();
    check(settings.findChild<QWidget*>("setting_initial_state") != nullptr, "SIO-013 output settings missing after pin switch");
    check(settings.findChild<QWidget*>("setting_debounce_ms") == nullptr, "SIO-014 pin switch leaked input settings");

    QTemporaryDir inputDirectory;
    const QString inputSource = generated(input, inputDirectory);
    check(inputSource.contains("gpio_init(0)") && inputSource.contains("gpio_set_dir(0, GPIO_IN)"),
          "SIO-015 input generator path is incorrect");
    check(inputSource.contains("gpio_set_pulls(0, false, false)"), "SIO-016 input pull generation is missing");
    QTemporaryDir outputDirectory;
    auto configuredOutput = output;
    configuredOutput.settings.insert("initial_state", "High");
    configuredOutput.settings.insert("drive_strength", "8");
    configuredOutput.settings.insert("slew_rate", "Slow");
    configuredOutput.settings.insert("blink_enabled", "false");
    const QString outputSource = generated(configuredOutput, outputDirectory);
    check(outputSource.contains("gpio_put(1, true)") && outputSource.contains("GPIO_DRIVE_STRENGTH_8MA") &&
              outputSource.contains("GPIO_SLEW_RATE_SLOW") && outputSource.indexOf("gpio_put(1, true)") <
                  outputSource.indexOf("gpio_set_dir(1, GPIO_OUT)"),
          "SIO-017 output initialization and pad generation is incorrect");

    if (failures == 0)
        std::cout << "All SIO model tests passed.\n";
    return failures == 0 ? 0 : 1;
}
