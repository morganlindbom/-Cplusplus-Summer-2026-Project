// debug_contract_tests.cpp
#include "systems/mainwindow/debug/debug_column2/DebugColumn2.hpp"
#include "systems/mainwindow/debug/debug_column3/DebugColumn3.hpp"
#include "systems/debug/MiProtocol.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

#include <iostream>

namespace
{
bool require(bool condition, const char* message)
{
    /**Reports one debug-contract assertion and returns its pass state.

    The test executable deliberately avoids a third-party test framework so it can
    run in the existing CTest environment with only Qt dependencies.
    */
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
} // namespace

int main(int argc, char* argv[])
{
    /**Validates stable debug UI automation IDs and safe invalid-configuration behavior.

    These checks protect the GUI certification contract without requiring a physical
    CMSIS-DAP probe or launching OpenOCD/GDB during CI.
    */
    QApplication application(argc, argv);
    bool ok = true;

    const auto result = pvd::mi::parseRecord(R"(105^done,value="1")");
    ok &= require(result.kind == pvd::mi::MiRecord::Kind::Result && result.token == 105 && result.resultClass == "done",
                  "MI result record must parse with its decimal token");
    ok &= require(pvd::mi::parseRecord(R"(=thread-selected,id="1")").kind == pvd::mi::MiRecord::Kind::NotifyAsync,
                  "MI notify records must not be evidence results");
    ok &= require(pvd::mi::parseRecord(R"(*stopped,reason="signal-received")").kind == pvd::mi::MiRecord::Kind::ExecAsync,
                  "MI exec async records must parse structurally");
    ok &= require(pvd::mi::parseRecord(R"(~"console, text\n")").kind == pvd::mi::MiRecord::Kind::Console,
                  "MI console streams must preserve commas inside C-strings");
    ok &= require(pvd::mi::parseRecord(R"(101^error,msg="No symbol, \"x\"")").kind == pvd::mi::MiRecord::Kind::Result,
                  "MI error result C-string escaping must parse");
    ok &= require(pvd::mi::parseRecord(R"(102^done,value={a="1",b=["x",{c="y"}]})").kind == pvd::mi::MiRecord::Kind::Result,
                  "MI nested tuple/list results must parse");
    ok &= require(pvd::mi::parseRecord("(gdb)\r").kind == pvd::mi::MiRecord::Kind::Prompt,
                  "MI prompt must be a delimiter, not a parse failure");
    ok &= require(pvd::mi::parseRecord("105^do").kind == pvd::mi::MiRecord::Kind::Malformed,
                  "Incomplete MI records must remain incomplete at the record parser boundary");

    const QString runtimeRoot = QStringLiteral(PVD_RUNTIME_ROOT);
    const QString column2Db =
        QDir(runtimeRoot).filePath("src/systems/mainwindow/debug/debug_column2/debug_column2.sqlite");
    const QString column3Db =
        QDir(runtimeRoot).filePath("src/systems/mainwindow/debug/debug_column3/debug_column3.sqlite");

    pvd::DebugColumn2 configuration(column2Db);
    ok &= require(configuration.findChild<QLineEdit*>("debug_openocd") != nullptr,
                  "DebugColumn2 must expose debug_openocd");
    ok &= require(configuration.findChild<QLineEdit*>("debug_gdb") != nullptr, "DebugColumn2 must expose debug_gdb");
    ok &= require(configuration.findChild<QLineEdit*>("debug_interface") != nullptr,
                  "DebugColumn2 must expose debug_interface");
    ok &= require(configuration.findChild<QLineEdit*>("debug_target") != nullptr,
                  "DebugColumn2 must expose debug_target");
    ok &=
        require(configuration.findChild<QComboBox*>("debug_speed") != nullptr, "DebugColumn2 must expose debug_speed");

    configuration.applySetting("reset_method", "halt reset");
    ok &= require(configuration.resetMethod() == "halt reset", "reset_method must be applied deterministically");
    auto* speed = configuration.findChild<QComboBox*>("debug_speed");
    if (speed)
    {
        speed->setEditText(QString{});
        ok &= require(configuration.speed() == 5000, "invalid adapter speed must fall back to 5000 kHz");
    }

    pvd::DebugColumn3 session(column3Db);
    ok &= require(session.findChild<QLabel*>("debug_status") != nullptr, "DebugColumn3 must expose debug_status");
    auto* coreSelector = session.findChild<QComboBox*>("debug_core_selector");
    ok &= require(coreSelector != nullptr, "DebugColumn3 must expose debug_core_selector");
    ok &= require(coreSelector && coreSelector->count() == 1 && coreSelector->currentText() == "Core 0",
                  "Debug core selector must default safely to Core 0");
    ok &=
        require(coreSelector && !coreSelector->isEnabled(), "Debug core selector must be disabled before GDB connects");
    session.setAvailableCores(true);
    ok &= require(coreSelector && coreSelector->count() == 2 && coreSelector->itemText(0) == "Core 0" &&
                      coreSelector->itemData(0).toInt() == 0 && coreSelector->itemText(1) == "Core 1" &&
                      coreSelector->itemData(1).toInt() == 1,
                  "Multicore configuration must expose Core 0 and Core 1");
    ok &= require(coreSelector && !coreSelector->isEnabled(),
                  "Multicore selector must remain disabled until GDB mapping is resolved");
    session.setAvailableCores(false);
    ok &= require(coreSelector && coreSelector->count() == 1 && coreSelector->currentText() == "Core 0",
                  "Single-core configuration must not expose Core 1");
    ok &= require(session.findChild<QPushButton*>("debug_start") != nullptr, "DebugColumn3 must expose debug_start");
    ok &= require(session.findChild<QPushButton*>("debug_readback_start") != nullptr, "DebugColumn3 must expose debug_readback_start");
    ok &= require(session.findChild<QPushButton*>("debug_stop") != nullptr, "DebugColumn3 must expose debug_stop");
    auto* startButton = session.findChild<QPushButton*>("debug_start");
    auto* stopButton = session.findChild<QPushButton*>("debug_stop");
    ok &= require(startButton && startButton->isEnabled(), "Start OCD must be enabled before explicit startup");
    ok &= require(stopButton && !stopButton->isEnabled(), "Stop OCD must be disabled before explicit startup");
    ok &= require(session.findChild<QPushButton*>("debug_halt") != nullptr, "DebugColumn3 must expose debug_halt");
    ok &= require(session.findChild<QPushButton*>("debug_continue") != nullptr,
                  "DebugColumn3 must expose debug_continue");
    ok &= require(session.findChild<QPushButton*>("debug_step") != nullptr, "DebugColumn3 must expose debug_step");
    ok &= require(session.findChild<QPushButton*>("debug_next") != nullptr, "DebugColumn3 must expose debug_next");
    ok &= require(session.findChild<QPushButton*>("debug_backtrace") != nullptr,
                  "DebugColumn3 must expose debug_backtrace");
    ok &= require(session.findChild<QPushButton*>("debug_registers") != nullptr,
                  "DebugColumn3 must expose debug_registers");
    ok &= require(session.findChild<QLineEdit*>("debug_command") != nullptr, "DebugColumn3 must expose debug_command");
    ok &= require(session.findChild<QPushButton*>("debug_send_command") != nullptr,
                  "DebugColumn3 must expose debug_send_command");
    ok &= require(session.findChild<QPlainTextEdit*>("debug_log") != nullptr, "DebugColumn3 must expose debug_log");

    session.configure("definitely-missing-openocd.exe", "definitely-missing-gdb.exe", "interface/cmsis-dap.cfg",
                      "target/rp2350.cfg", "definitely-missing.elf", 5000, "halt reset", true);
    if (startButton)
        startButton->click();
    auto* status = session.findChild<QLabel*>("debug_status");
    ok &= require(status && status->text() == "Debug configuration invalid",
                  "invalid debug configuration must fail before launching hardware tools");

    if (ok)
        std::cout << "PASS: debug UI and validation contract\n";
    return ok ? 0 : 1;
}
