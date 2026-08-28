#include "systems/mainwindow/debug/debug_column3/DebugColumn3.hpp"
#include "systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTimer>

#include <iostream>

namespace
{
bool writeRequest(const QString& path, const QString& runId, const QString& result, const QString& log,
                  const QString& ack, const QString& elf)
{
    QJsonObject request;
    request["runId"] = runId;
    request["resultPath"] = result;
    request["rawLogPath"] = log;
    request["ackPath"] = ack;
    request["artifactElfPath"] = elf;
    request["mode"] = "CertificationReadback";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.write(QJsonDocument(request).toJson(QJsonDocument::Indented));
    file.flush();
    return true;
}

bool waitForStatus(QApplication& app, QLabel* status, const QString& expected, int timeoutMs)
{
    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    while (QDateTime::currentMSecsSinceEpoch() < deadline)
    {
        app.processEvents(QEventLoop::AllEvents, 100);
        if (status && status->text() == expected)
            return true;
    }
    return status && status->text() == expected;
}
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (argc != 6)
    {
        std::cerr << "usage: post_program_hardware_diagnostic <elf> <uf2> <project-root> <result> <log>\n";
        return 2;
    }

    const QString elf = QFileInfo(argv[1]).absoluteFilePath();
    const QString uf2 = QFileInfo(argv[2]).absoluteFilePath();
    const QString projectRoot = QFileInfo(argv[3]).absoluteFilePath();
    const QString resultPath = QFileInfo(argv[4]).absoluteFilePath();
    const QString logPath = QFileInfo(argv[5]).absoluteFilePath();
    const QString runId = QStringLiteral("post-program-readback-%1")
                              .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssfffZ"));
    const QString requestPath = QDir(projectRoot).filePath("validation/pin_certification/requests/active-readback-request.json");
    const QString ackPath = QDir(QFileInfo(resultPath).absolutePath()).filePath(runId + ".trigger-ack.json");
    QDir().mkpath(QFileInfo(resultPath).absolutePath());
    QDir().mkpath(QFileInfo(logPath).absolutePath());
    QDir().mkpath(QFileInfo(requestPath).absolutePath());
    QFile::remove(resultPath);
    QFile::remove(logPath);
    QFile::remove(ackPath);
    if (!writeRequest(requestPath, runId, resultPath, logPath, ackPath, elf))
        return 3;
    qputenv("PVD_READBACK_REQUEST_PATH", requestPath.toUtf8());

    const QString repositoryRoot = QDir::currentPath();
    const QString transferDb = QDir(repositoryRoot).filePath("src/systems/mainwindow/transfer/transfer_column3/transfer_column3.sqlite");
    pvd::TransferColumn3 transfer(transferDb);
    transfer.configure(uf2, QStringLiteral("OpenOCD probe"), QDir(QFileInfo(elf).dir()).filePath("generated"),
                       QFileInfo(elf).dir().absolutePath(), QFileInfo(elf).completeBaseName(), true);
    auto* transferButton = transfer.findChild<QPushButton*>("transfer_firmware");
    auto* transferStatus = transfer.findChild<QLabel*>("transfer_status");
    if (!transferButton || !transferStatus)
        return 4;
    transferButton->click();
    const bool transferPass = waitForStatus(app, transferStatus, QStringLiteral("Transfer completed"), 120000);
    std::cout << "TRANSFER_STATUS=" << transferStatus->text().toStdString() << "\n";
    std::cout << transfer.findChild<QPlainTextEdit*>("transfer_log")->toPlainText().toStdString() << "\n";
    if (!transferPass)
        return 10;

    // OpenOCD has shut down; allow the flashed target to execute autonomously.
    QElapsedTimer autonomous;
    autonomous.start();
    while (autonomous.elapsed() < 500)
        app.processEvents(QEventLoop::AllEvents, 25);

    const QString debugDb = QDir(repositoryRoot).filePath("src/systems/mainwindow/debug/debug_column3/debug_column3.sqlite");
    const QString openocd = qEnvironmentVariable("PICO_OPENOCD_PATH");
    const QString gdb = qEnvironmentVariable("PICO_GDB_PATH");
    const QString scripts = QFileInfo(openocd).dir().filePath("scripts");
    pvd::DebugColumn3 debug(debugDb);
    debug.configure(openocd, gdb, QDir(scripts).filePath("interface/cmsis-dap.cfg"),
                    QDir(scripts).filePath("target/rp2350.cfg"), elf, 5000, QStringLiteral("none"));
    auto* readbackButton = debug.findChild<QPushButton*>("debug_readback_start");
    if (!readbackButton)
        return 11;
    readbackButton->click();
    const qint64 resultDeadline = QDateTime::currentMSecsSinceEpoch() + 180000;
    while (QDateTime::currentMSecsSinceEpoch() < resultDeadline)
    {
        app.processEvents(QEventLoop::AllEvents, 100);
        if (QFileInfo::exists(resultPath))
        {
            QFile result(resultPath);
            if (result.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QJsonParseError error;
                const QJsonDocument document = QJsonDocument::fromJson(result.readAll(), &error);
                if (error.error == QJsonParseError::NoError && document.isObject() &&
                    document.object().value("runId").toString() == runId)
                    break;
            }
        }
    }
    debug.stopServer();
    app.processEvents(QEventLoop::AllEvents, 1000);
    std::cout << "RUN_ID=" << runId.toStdString() << "\n";
    std::cout << "RESULT_PATH=" << resultPath.toStdString() << "\n";
    std::cout << "LOG_PATH=" << logPath.toStdString() << "\n";
    return QFileInfo::exists(resultPath) && QFileInfo::exists(logPath) ? 0 : 12;
}
