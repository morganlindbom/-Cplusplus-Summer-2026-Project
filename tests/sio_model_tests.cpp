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
    const auto outputForSettings = sio("pin2", 1, "Output");
    settings.showSelection(outputForSettings);
    QCoreApplication::processEvents();
    auto* pullEditor = settings.findChild<QComboBox*>("setting_pull");
    check(pullEditor != nullptr, "SIO-024 SIO pull editor disappeared from the settings form");
    settings.showSelection(input);
    QCoreApplication::processEvents();
    pullEditor = settings.findChild<QComboBox*>("setting_pull");
    check(pullEditor != nullptr, "SIO-025 pull editor missing for SIO input");
    if (pullEditor)
    {
        int pullTextSignals = 0;
        int pullSettingSignals = 0;
        QObject::connect(pullEditor, &QComboBox::currentTextChanged, [&pullTextSignals](const QString&) { ++pullTextSignals; });
        QObject::connect(&settings, &SettingsColumn3::settingChanged,
                         [&pullSettingSignals](const QString&, const QString& key, const QString&) { if (key == "pull") ++pullSettingSignals; });
        for (const auto& pull : {QStringLiteral("Pull-up"), QStringLiteral("Pull-down"), QStringLiteral("None")})
        {
            const int target = pullEditor->findText(pull);
            check(target >= 0, "SIO-026 Pull item missing from the real QComboBox");
            pullEditor->setCurrentIndex(target);
            QCoreApplication::processEvents();
            check(pullEditor->currentText() == pull && pullTextSignals > 0 && pullSettingSignals > 0 &&
                      lifecycle.selections["pin1"].settings.value("pull") == pull,
                  "SIO-026 real QComboBox keyboard interaction did not update Pull GUI/signal/model");
        }
    }
    settings.showSelection(outputForSettings);
    QCoreApplication::processEvents();
    auto* driveEditor = settings.findChild<QComboBox*>("setting_drive_strength");
    auto* slewEditor = settings.findChild<QComboBox*>("setting_slew_rate");
    check(driveEditor != nullptr && slewEditor != nullptr, "SIO-027 output pad editors missing");
    if (driveEditor && slewEditor)
    {
        int driveTextSignals = 0;
        int driveSettingSignals = 0;
        QObject::connect(driveEditor, &QComboBox::currentTextChanged, [&driveTextSignals](const QString&) { ++driveTextSignals; });
        QObject::connect(&settings, &SettingsColumn3::settingChanged,
                         [&driveSettingSignals](const QString&, const QString& key, const QString&) { if (key == "drive_strength") ++driveSettingSignals; });
        for (const auto& strength : {QStringLiteral("2"), QStringLiteral("4"), QStringLiteral("8"), QStringLiteral("12")})
        {
            const int target = driveEditor->findText(strength);
            driveEditor->setCurrentIndex(target);
            QCoreApplication::processEvents();
            check(driveEditor->currentText() == strength && driveTextSignals > 0 && driveSettingSignals > 0 &&
                      lifecycle.selections["pin2"].settings.value("drive_strength") == strength,
                  "SIO-028 real QComboBox keyboard interaction did not update drive-strength GUI/signal/model");
        }
        int slewTextSignals = 0;
        int slewSettingSignals = 0;
        QObject::connect(slewEditor, &QComboBox::currentTextChanged, [&slewTextSignals](const QString&) { ++slewTextSignals; });
        QObject::connect(&settings, &SettingsColumn3::settingChanged,
                         [&slewSettingSignals](const QString&, const QString& key, const QString&) { if (key == "slew_rate") ++slewSettingSignals; });
        for (const auto& slew : {QStringLiteral("Slow"), QStringLiteral("Fast")})
        {
            const int target = slewEditor->findText(slew);
            slewEditor->setCurrentIndex(target);
            QCoreApplication::processEvents();
            check(slewEditor->currentText() == slew && slewTextSignals > 0 && slewSettingSignals > 0 &&
                      lifecycle.selections["pin2"].settings.value("slew_rate") == slew,
                  "SIO-029 real QComboBox keyboard interaction did not update slew GUI/signal/model");
        }
    }

    // Settings must remain attached to their owning selection when the live
    // form is rebuilt for another pin.  This exercises the same production
    // settingChanged -> ApplicationState path as the GUI.
    auto isolatedInput = sio("isolated_input", 2, "Input");
    auto isolatedOutput = sio("isolated_output", 3, "Output");
    isolatedOutput.settings.insert("drive_strength", "4");
    isolatedOutput.settings.insert("slew_rate", "Slow");
    lifecycle.selections.insert(isolatedInput.componentId, isolatedInput);
    lifecycle.selections.insert(isolatedOutput.componentId, isolatedOutput);
    settings.showSelection(isolatedInput);
    QCoreApplication::processEvents();
    auto* isolatedPull = settings.findChild<QComboBox*>("setting_pull");
    check(isolatedPull != nullptr, "SIO-030 isolated input pull editor missing");
    if (isolatedPull)
    {
        isolatedPull->setCurrentText("Pull-up");
        QCoreApplication::processEvents();
    }
    settings.showSelection(isolatedOutput);
    QCoreApplication::processEvents();
    auto* isolatedDrive = settings.findChild<QComboBox*>("setting_drive_strength");
    auto* isolatedSlew = settings.findChild<QComboBox*>("setting_slew_rate");
    check(isolatedDrive != nullptr && isolatedSlew != nullptr, "SIO-031 isolated output editors missing");
    if (isolatedDrive && isolatedSlew)
    {
        isolatedDrive->setCurrentText("12");
        isolatedSlew->setCurrentText("Fast");
        QCoreApplication::processEvents();
    }
    settings.showSelection(lifecycle.selections.value(isolatedInput.componentId));
    QCoreApplication::processEvents();
    check(settings.findChild<QComboBox*>("setting_pull") != nullptr &&
              settings.findChild<QComboBox*>("setting_pull")->currentText() == "Pull-up" &&
              lifecycle.selections[isolatedInput.componentId].settings.value("pull") == "Pull-up",
          "SIO-032 input pull value bled across selection owner");
    settings.showSelection(lifecycle.selections.value(isolatedOutput.componentId));
    QCoreApplication::processEvents();
    check(settings.findChild<QComboBox*>("setting_drive_strength") != nullptr &&
              settings.findChild<QComboBox*>("setting_drive_strength")->currentText() == "12" &&
              settings.findChild<QComboBox*>("setting_slew_rate") != nullptr &&
              settings.findChild<QComboBox*>("setting_slew_rate")->currentText() == "Fast" &&
              lifecycle.selections[isolatedOutput.componentId].settings.value("drive_strength") == "12" &&
              lifecycle.selections[isolatedOutput.componentId].settings.value("slew_rate") == "Fast",
          "SIO-033 output pad values bled across selection owner");

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
    persistedPin.settings.insert("blink_enabled", "true");
    persistedPin.settings.insert("blink_interval_ms", "100");
    persistedPin.settings.insert("drive_strength", "12");
    persistedPin.settings.insert("slew_rate", "Slow");
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
    check(reloaded.selections.value("pin_1").settings.value("blink_enabled") == "true" &&
              reloaded.selections.value("pin_1").settings.value("blink_interval_ms") == "100" &&
              reloaded.selections.value("pin_1").settings.value("drive_strength") == "12" &&
              reloaded.selections.value("pin_1").settings.value("slew_rate") == "Slow",
          "SIO-CRASH-008 SIO output settings were not preserved after reload");

    QTemporaryDir inputPersistenceDirectory;
    ApplicationState persistedInput;
    persistedInput.projectName = "SIO_PULL_RELOAD";
    persistedInput.projectPath = inputPersistenceDirectory.path();
    persistedInput.product = "Raspberry Pi Pico 2";
    persistedInput.language = "C++";
    persistedInput.state = "Testing";
    auto persistedInputPin = sio("pin_input", 0, "Input");
    persistedInputPin.settings.insert("pull", "Pull-up");
    persistedInput.selections.insert(persistedInputPin.componentId, persistedInputPin);
    check(ProjectStore::save(persistedInput, &persistenceError), "SIO-CRASH-009 input save failed");
    ApplicationState reloadedInput;
    check(ProjectStore::load(inputPersistenceDirectory.path(), &reloadedInput, &persistenceError),
          "SIO-CRASH-010 input reload failed");
    SettingsColumn3 reloadedInputSettings(QString(), &catalog);
    reloadedInputSettings.showSelection(reloadedInput.selections.value("pin_input"));
    QCoreApplication::processEvents();
    auto* reloadedPull = reloadedInputSettings.findChild<QComboBox*>("setting_pull");
    check(reloadedInput.selections.value("pin_input").settings.value("pull") == "Pull-up" &&
              reloadedPull != nullptr && reloadedPull->currentText() == "Pull-up",
          "SIO-CRASH-011 input Pull-up was not preserved in model and live UI after reload");
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
    for (const auto& pull : {QStringLiteral("None"), QStringLiteral("Pull-up"), QStringLiteral("Pull-down")})
    {
        auto configuredInput = input;
        configuredInput.settings.insert("pull", pull);
        QTemporaryDir pullDirectory;
        const auto pullSource = generated(configuredInput, pullDirectory);
        const auto expected = pull == "Pull-up" ? "gpio_set_pulls(0, true, false)"
                           : pull == "Pull-down" ? "gpio_set_pulls(0, false, true)"
                                                  : "gpio_set_pulls(0, false, false)";
        check(pullSource.contains(expected), "SIO-018 pull enum did not map to the SDK call");
    }
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
          "SIO-019 output initialization and pad generation is incorrect");
    for (const auto& strength : {QStringLiteral("2"), QStringLiteral("4"), QStringLiteral("8"), QStringLiteral("12")})
    {
        auto configuredOutput = output;
        configuredOutput.settings.insert("drive_strength", strength);
        QTemporaryDir strengthDirectory;
        check(generated(configuredOutput, strengthDirectory).contains("GPIO_DRIVE_STRENGTH_" + strength + "MA"),
              "SIO-020 drive-strength enum did not map to the SDK call");
    }
    for (const auto& slew : {QStringLiteral("Slow"), QStringLiteral("Fast")})
    {
        auto configuredOutput = output;
        configuredOutput.settings.insert("slew_rate", slew);
        QTemporaryDir slewDirectory;
        check(generated(configuredOutput, slewDirectory).contains("GPIO_SLEW_RATE_" + slew.toUpper()),
              "SIO-021 slew-rate enum did not map to the SDK call");
    }
    auto blinkingOutput = output;
    blinkingOutput.settings.insert("initial_state", "Low");
    blinkingOutput.settings.insert("blink_enabled", "true");
    blinkingOutput.settings.insert("blink_interval_ms", "100");
    QTemporaryDir blinkDirectory;
    const auto blinkSource = generated(blinkingOutput, blinkDirectory);
    check(blinkSource.contains("now_us - last_toggle_us >= 100000u") &&
              blinkSource.contains("gpio_state = !gpio_state"),
          "SIO-022 blink interval/runtime mapping is incorrect");
    auto debounceInput = input;
    debounceInput.settings.insert("debounce_ms", "250");
    QTemporaryDir debounceDirectory;
    const auto debounceSource = generated(debounceInput, debounceDirectory);
    check(!debounceSource.contains("debounce_ms") && !debounceSource.contains("250"),
          "SIO-023 debounce unexpectedly affected the SIO input runtime");

    if (failures == 0)
        std::cout << "All SIO model tests passed.\n";
    return failures == 0 ? 0 : 1;
}
