// build_transfer_contract_tests.cpp
#include "systems/mainwindow/build/build_column3/BuildColumn3.hpp"
#include "systems/mainwindow/transfer/transfer_column2/TransferColumn2.hpp"
#include "systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFile>

#include <iostream>

namespace
{
bool require(bool condition, const char* message)
{
    /**Reports one build/transfer contract assertion and returns its pass state.

    The executable uses only Qt and the existing CTest runner so the GUI contract
    remains testable without a physical Pico, BOOTSEL volume, or debug probe.
    */
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
} // namespace

int main(int argc, char* argv[])
{
    /**Validates stable Build/Transfer automation IDs and safe precondition failures.

    The test deliberately stops before external tools or hardware are launched; it
    protects the observable lifecycle boundaries used by Windows GUI certification.
    */
    QApplication application(argc, argv);
    bool ok = true;

    const QString runtimeRoot = QStringLiteral(PVD_RUNTIME_ROOT);
    const QString buildDb =
        QDir(runtimeRoot).filePath("src/systems/mainwindow/build/build_column3/build_column3.sqlite");
    const QString transfer2Db =
        QDir(runtimeRoot).filePath("src/systems/mainwindow/transfer/transfer_column2/transfer_column2.sqlite");
    const QString transfer3Db =
        QDir(runtimeRoot).filePath("src/systems/mainwindow/transfer/transfer_column3/transfer_column3.sqlite");

    pvd::BuildColumn3 build(buildDb);
    ok &= require(build.findChild<QPushButton*>("configure_project") != nullptr,
                  "BuildColumn3 must expose configure_project");
    ok &= require(build.findChild<QPushButton*>("build_project") != nullptr,
                  "BuildColumn3 must expose build_project");
    ok &= require(build.findChild<QLabel*>("build_status") != nullptr, "BuildColumn3 must expose build_status");
    ok &= require(build.findChild<QPlainTextEdit*>("build_log") != nullptr, "BuildColumn3 must expose build_log");

    auto* buildStatus = build.findChild<QLabel*>("build_status");
    ok &= require(buildStatus && buildStatus->text() == "Idle",
                  "Build status must begin in the visible Idle state");

    bool buildCompletionObserved = false;
    bool buildCompletionOk = true;
    QObject::connect(&build, &pvd::BuildColumn3::buildCompleted, &application,
                     [&buildCompletionObserved, &buildCompletionOk](bool result, const QString&)
                     {
                         /**Captures the synchronous missing-path build failure.

                         A failed precondition must complete the build contract rather
                         than leaving callers waiting for an impossible process result.
                         */
                         buildCompletionObserved = true;
                         buildCompletionOk = result;
                     });
    build.build();
    ok &= require(buildCompletionObserved && !buildCompletionOk,
                  "Build without configured paths must emit one failed completion");
    ok &= require(buildStatus && buildStatus->text() == "Build failed",
                  "Build must expose the failed state for missing paths");

    pvd::TransferColumn2 transferTarget(transfer2Db);
    ok &= require(transferTarget.findChild<QLineEdit*>("transfer_artifact") != nullptr,
                  "TransferColumn2 must expose transfer_artifact");
    ok &= require(transferTarget.findChild<QComboBox*>("transfer_method") != nullptr,
                  "TransferColumn2 must expose transfer_method");

    pvd::TransferColumn3 transfer(transfer3Db);
    ok &= require(pvd::TransferColumn3::postProgramStartStatesValid("running", "running"),
                  "Both running RP2350 cores must pass post-program start");
    ok &= require(!pvd::TransferColumn3::postProgramStartStatesValid("running", "halted"),
                  "A halted Core 1 must fail post-program start");
    ok &= require(!pvd::TransferColumn3::postProgramStartStatesValid("halted", "running"),
                  "A halted Core 0 must fail post-program start");
    ok &= require(!pvd::TransferColumn3::postProgramStartStatesValid("unknown", "running"),
                  "An unknown core state must fail closed");
    QFile transferSource(QDir(QStringLiteral(PVD_SOURCE_ROOT))
                             .filePath("src/systems/mainwindow/transfer/transfer_column3/TransferColumn3.cpp"));
    ok &= require(transferSource.open(QIODevice::ReadOnly | QIODevice::Text),
                  "Transfer source must be available for lifecycle contract checks");
    const QString sourceText = QString::fromUtf8(transferSource.readAll());
    ok &= require(sourceText.contains("program \\\"%1\\\" verify"),
                  "Transfer must program and verify without implicit reset/exit");
    ok &= require(sourceText.contains("<< \"-c\" << \"reset run\""),
                  "Transfer must issue explicit reset run");
    ok &= require(sourceText.contains("<< \"-c\" << \"targets\""),
                  "Transfer must query target state");
    ok &= require(sourceText.contains("<< \"-c\" << \"shutdown\""),
                  "Transfer must shut down OpenOCD explicitly");
    ok &= require(!sourceText.contains("verify reset exit"),
                  "Legacy combined program verify reset exit contract must not return");
    ok &= require(transfer.findChild<QLineEdit*>("transfer_drive") != nullptr,
                  "TransferColumn3 must expose transfer_drive");
    ok &= require(transfer.findChild<QLabel*>("transfer_status") != nullptr,
                  "TransferColumn3 must expose transfer_status");
    ok &= require(transfer.findChild<QPushButton*>("transfer_firmware") != nullptr,
                  "TransferColumn3 must expose transfer_firmware");
    ok &= require(transfer.findChild<QPlainTextEdit*>("transfer_log") != nullptr,
                  "TransferColumn3 must expose transfer_log");

    transfer.configure(QString{}, "OpenOCD probe", QString{}, QString{}, "PICO2W", false);
    auto* transferButton = transfer.findChild<QPushButton*>("transfer_firmware");
    if (transferButton)
        transferButton->click();
    auto* transferStatus = transfer.findChild<QLabel*>("transfer_status");
    ok &= require(transferStatus && transferStatus->text() == "Transfer blocked",
                  "Transfer without a current artifact must be blocked before hardware access");

    if (ok)
        std::cout << "PASS: build and transfer GUI contracts\n";
    return ok ? 0 : 1;
}
