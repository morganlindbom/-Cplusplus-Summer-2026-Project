// DebugColumn3.cpp
#include "systems/mainwindow/debug/debug_column3/DebugColumn3.hpp"
#include "systems/debug/MiProtocol.hpp"

#include "systems/database/SqliteUtil.hpp"
#include "systems/mainwindow/PanelUtil.hpp"

#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTcpSocket>
#include <QTimer>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
using MiRecord = pvd::mi::MiRecord;

bool commandNeedsFileValidation(const QString& command)
{
    /**Determines whether a configured command is a filesystem path.

    Bare executable names are intentionally left to PATH resolution while absolute
    paths and explicit relative paths are validated before a debug process is started.
    */
    return QFileInfo(command).isAbsolute() || command.contains('/') || command.contains('\\');
}
} // namespace

namespace pvd
{
DebugColumn3::DebugColumn3(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds an interactive OpenOCD plus GDB debugging surface.

    OpenOCD owns the transport to the RP2350 while GDB provides the user-facing
    halt, continue, step, breakpoint, register, and memory command channel.
    */
    auto* layout = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Debug"));

    status_ = new QLabel("Not configured", this);
    status_->setObjectName("debug_status");
    status_->setWordWrap(true);
    layout->addWidget(status_);
    readbackStatus_ = new QLabel("Idle", this);
    readbackStatus_->setObjectName("debug_readback_status");
    readbackStatus_->setAccessibleName("Debug readback status");
    layout->addWidget(readbackStatus_);
    readbackResultPath_ = new QLabel(this);
    readbackResultPath_->setObjectName("debug_readback_result_path");
    readbackResultPath_->setAccessibleName("Debug readback result path");
    readbackResultPath_->setWordWrap(true);
    layout->addWidget(readbackResultPath_);

    auto* coreRow = new QHBoxLayout();
    coreLabel_ = new QLabel("Debug Core:", this);
    coreLabel_->setObjectName("debug_core_label");
    coreSelector_ = new QComboBox(this);
    coreSelector_->setObjectName("debug_core_selector");
    coreSelector_->setAccessibleName("Debug Core");
    coreSelector_->addItem("Core 0", 0);
    coreSelector_->setCurrentIndex(0);
    coreSelector_->setEnabled(false);
    coreRow->addWidget(coreLabel_);
    coreRow->addWidget(coreSelector_);
    coreRow->addStretch(1);
    layout->addLayout(coreRow);

    auto* coreHelp = new QLabel(
        "Selects the physical RP2350 context for GDB commands. Halt may affect both cores in all-stop mode.", this);
    coreHelp->setObjectName("debug_core_help");
    coreHelp->setWordWrap(true);
    layout->addWidget(coreHelp);

    auto* sessionRow = new QHBoxLayout();
    startButton_ = new QPushButton("Start OCD", this);
    auto* start = startButton_;
    start->setObjectName("debug_start");
    readbackStartButton_ = new QPushButton("Readback Attach", this);
    auto* readbackStart = readbackStartButton_;
    readbackStart->setObjectName("debug_readback_start");
    readbackStart->setAccessibleName("Start readback-only debugger attach");
    stopButton_ = new QPushButton("Stop OCD", this);
    auto* stop = stopButton_;
    stop->setObjectName("debug_stop");
    setSessionButtons(false);
    sessionRow->addWidget(start);
    sessionRow->addWidget(readbackStart);
    sessionRow->addWidget(stop);
    layout->addLayout(sessionRow);

    auto* actionRow = new QHBoxLayout();
    auto* halt = new QPushButton("Halt", this);
    auto* continueButton = new QPushButton("Continue", this);
    auto* step = new QPushButton("Step", this);
    auto* next = new QPushButton("Next", this);
    auto* backtrace = new QPushButton("Backtrace", this);
    auto* registers = new QPushButton("Registers", this);
    halt->setObjectName("debug_halt");
    continueButton->setObjectName("debug_continue");
    step->setObjectName("debug_step");
    next->setObjectName("debug_next");
    backtrace->setObjectName("debug_backtrace");
    registers->setObjectName("debug_registers");
    actionRow->addWidget(halt);
    actionRow->addWidget(continueButton);
    actionRow->addWidget(step);
    actionRow->addWidget(next);
    actionRow->addWidget(backtrace);
    actionRow->addWidget(registers);
    layout->addLayout(actionRow);

    auto* commandRow = new QHBoxLayout();
    command_ = new QLineEdit(this);
    command_->setObjectName("debug_command");
    command_->setPlaceholderText("GDB command, e.g. info registers, break main, continue, next");
    sendCommand_ = new QPushButton("Send", this);
    sendCommand_->setObjectName("debug_send_command");
    commandRow->addWidget(command_, 1);
    commandRow->addWidget(sendCommand_);
    layout->addLayout(commandRow);

    log_ = new QPlainTextEdit(this);
    log_->setObjectName("debug_log");
    log_->setReadOnly(true);
    layout->addWidget(log_, 1);

    openocdProcess_ = new QProcess(this);
    gdbProcess_ = new QProcess(this);
#ifdef Q_OS_WIN
    openocdProcess_->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments* arguments)
        {
            /**Prevents Windows from creating a visible console host for OpenOCD.*/
            arguments->flags |= CREATE_NO_WINDOW | DETACHED_PROCESS;
        });
    gdbProcess_->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments* arguments)
        {
            /**Prevents Windows from creating a visible console host for GDB.*/
            arguments->flags |= CREATE_NO_WINDOW | DETACHED_PROCESS;
        });
#endif
    readinessTimer_ = new QTimer(this);
    readinessTimer_->setInterval(250);
    readbackDeadlineTimer_ = new QTimer(this);
    readbackDeadlineTimer_->setSingleShot(true);
    connect(readbackDeadlineTimer_, &QTimer::timeout, this,
            [this]() { finishReadback(false, QStringLiteral("EVIDENCE_TRANSACTION_TIMEOUT")); });

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
            [this]()
            {
                /**Ensures the owned debug processes stop before Qt tears down the event loop.*/
                appendLog("Application aboutToQuit cleanup entered");
                stopServer();
            });

    connect(openocdProcess_, &QProcess::readyReadStandardOutput, this,
            [this]()
            {
                /**Forwards OpenOCD standard output into the persistent debug transcript.

                Keeping server and debugger messages together makes probe, target, and
                firmware failures diagnosable from one UI surface.
                */
                const QString output = QString::fromLocal8Bit(openocdProcess_->readAllStandardOutput());
                appendLog(output);
                interpretOpenOcdOutput(output);
            });
    connect(openocdProcess_, &QProcess::readyReadStandardError, this,
            [this]()
            {
                /**Forwards OpenOCD diagnostic output into the persistent debug transcript.

                OpenOCD writes much of its normal startup evidence to stderr, therefore
                stderr is treated as diagnostic text rather than an automatic failure.
                */
                const QString output = QString::fromLocal8Bit(openocdProcess_->readAllStandardError());
                appendLog(output);
                interpretOpenOcdOutput(output);
            });
    connect(openocdProcess_, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError)
            {
                /**Reports process-start and runtime failures with Qt's explanatory text.

                The previous numeric-only error made missing executables and permission
                problems unnecessarily difficult to distinguish.
                */
                setStatus("OpenOCD error");
                setSessionButtons(false);
                appendLog("OpenOCD process error: " + openocdProcess_->errorString());
            });
    connect(
        openocdProcess_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            /**Tracks unexpected OpenOCD termination and stops readiness polling.

                A clean user-requested shutdown is reported differently from a server
                exit that occurs while a debug session is expected to remain active.
                */
            readinessTimer_->stop();
            if (!stopping_)
            {
                setSessionButtons(false);
                setStatus("OpenOCD stopped unexpectedly");
                appendLog(
                    QString("OpenOCD exited (code %1, status %2).").arg(exitCode).arg(static_cast<int>(exitStatus)));
            }
            appendLog(QString("OpenOCD finished signal received: pid=%1 exitCode=%2 exitStatus=%3 state=%4")
                          .arg(openocdProcess_->processId())
                          .arg(exitCode)
                          .arg(static_cast<int>(exitStatus))
                          .arg(static_cast<int>(openocdProcess_->state())));
        });

    connect(gdbProcess_, &QProcess::readyReadStandardOutput, this,
            [this]()
            {
                /**Forwards GDB output to the debug transcript.

                This includes breakpoint hits, stack frames, register values, and
                responses to commands entered in the Debug workflow.
                */
                const QString output = QString::fromLocal8Bit(gdbProcess_->readAllStandardOutput());
                appendLog(output);
                if (readbackOnly_ && readbackMiMode_)
                    consumeReadbackMiOutput(output);
                else
                {
                    interpretGdbOutput(output);
                    consumeReadbackOutput(output);
                }
            });
    connect(gdbProcess_, &QProcess::readyReadStandardError, this,
            [this]()
            {
                /**Forwards GDB diagnostic output to the debug transcript.

                Keeping stderr visible is important for architecture mismatches,
                remote-protocol failures, and malformed ELF diagnostics.
                */
                const QString output = QString::fromLocal8Bit(gdbProcess_->readAllStandardError());
                appendLog(output);
                if (!(readbackOnly_ && readbackMiMode_))
                {
                    interpretGdbOutput(output);
                    consumeReadbackOutput(output);
                }
            });
    connect(gdbProcess_, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError)
            {
                /**Reports GDB process failures using a human-readable cause.

                OpenOCD can remain usable even when GDB fails, so the server is not
                automatically destroyed by this handler.
                */
                setStatus("GDB error");
                setSessionButtons(false);
                appendLog("GDB process error: " + gdbProcess_->errorString());
            });
    connect(gdbProcess_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus)
            {
                /**Records debugger-client termination without misclassifying normal Stop.

                An unexpected GDB exit leaves OpenOCD available for diagnosis or a
                subsequent manual restart of the session.
                */
                if (!stopping_)
                {
                    setSessionButtons(false);
                    setStatus("GDB stopped");
                    appendLog(
                        QString("GDB exited (code %1, status %2).").arg(exitCode).arg(static_cast<int>(exitStatus)));
                }
                resetCoreSelection();
                gdbOutputBuffer_.clear();
            });

    connect(readinessTimer_, &QTimer::timeout, this,
            [this]()
            {
                /**Polls OpenOCD's GDB server without blocking the Qt event loop.

                Debugger startup is intentionally sequenced after port 3333 is ready so
                GDB does not race OpenOCD target initialization.
                */
                probeOpenOcdReadiness();
            });
    connect(coreSelector_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                /**Routes semantic core selection through the active GDB mapping.*/
                if (index < 0 || index >= coreSelector_->count())
                    return;
                selectDebugCore(static_cast<DebugCore>(coreSelector_->itemData(index).toInt()));
            });
    connect(start, &QPushButton::clicked, this, &DebugColumn3::startSession);
    connect(readbackStart, &QPushButton::clicked, this, &DebugColumn3::startReadbackSession);
    connect(stop, &QPushButton::clicked, this,
            [this]()
            {
                /**Records the user-originated stop event before cleanup begins.*/
                appendLog("STOP OCD clicked");
                stopServer();
            });
    connect(halt, &QPushButton::clicked, this, &DebugColumn3::haltTarget);
    connect(continueButton, &QPushButton::clicked, this,
            [this]()
            {
                /**Continues target execution through the interactive GDB channel.

                The command is kept identical to normal GDB usage so the resulting
                transcript remains familiar and scriptable.
                */
                sendGdbCommand(QStringLiteral("continue"));
            });
    connect(step, &QPushButton::clicked, this,
            [this]()
            {
                /**Executes one source-level GDB step.

                Source stepping is most reliable when Debug Session Tools requested
                a deoptimized Debug build before the session was opened.
                */
                sendGdbCommand(QStringLiteral("step"));
            });
    connect(next, &QPushButton::clicked, this,
            [this]()
            {
                /**Executes one source-level GDB next operation.

                Unlike step, next keeps function calls at the current source frame
                when debug information permits it.
                */
                sendGdbCommand(QStringLiteral("next"));
            });
    connect(backtrace, &QPushButton::clicked, this,
            [this]()
            {
                /**Requests the current GDB stack backtrace.

                A dedicated action is useful during fault diagnosis because it avoids
                typing while preserving the exact command in the transcript.
                */
                sendGdbCommand(QStringLiteral("backtrace"));
            });
    connect(registers, &QPushButton::clicked, this,
            [this]()
            {
                /**Requests the current processor-register set from GDB.

                Register state is central when diagnosing HardFaults, interrupt state,
                and multicore execution on the RP2350.
                */
                sendGdbCommand(QStringLiteral("info registers"));
            });
    connect(sendCommand_, &QPushButton::clicked, this,
            [this]()
            {
                /**Sends the current interactive command to GDB.

                Empty commands are ignored and successful submissions are cleared so
                repeated hardware-debug operations stay quick to enter.
                */
                const QString command = command_->text().trimmed();
                if (command.isEmpty())
                    return;
                sendGdbCommand(command);
                command_->clear();
            });
    connect(command_, &QLineEdit::returnPressed, sendCommand_, &QPushButton::click);
}

DebugColumn3::~DebugColumn3()
{
    /**Terminates child debugger processes before the Qt widget hierarchy is destroyed.

    This prevents orphaned OpenOCD/GDB processes from retaining the CMSIS-DAP probe
    after Pico Visual Designer exits.
    */
    stopServer();
}

void DebugColumn3::configure(const QString& openocdExe, const QString& gdbExe, const QString& interfaceCfg,
                             const QString& targetCfg, const QString& elfPath, int speed, const QString& resetMethod,
                             bool autoStart)
{
    /**Stores the current project debugger configuration and optionally starts it.

    Entering or refreshing the Debug workflow only updates configuration. Hardware
    startup is reserved for the explicit Start OCD button.
    */
    const QString newOpenOcd = openocdExe.trimmed();
    const QString newGdb = gdbExe.trimmed();
    const QString newInterface = interfaceCfg.trimmed();
    const QString newTarget = targetCfg.trimmed();
    const QString newElf = elfPath.trimmed();
    const int newSpeed = speed > 0 ? speed : 5000;
    const bool configurationChanged = newOpenOcd != openocdExe_ || newGdb != gdbExe_ || newInterface != interface_ ||
                                      newTarget != target_ || newElf != elfPath_ || newSpeed != speed_ ||
                                      resetMethod != resetMethod_;
    const bool sessionRunning =
        openocdProcess_->state() != QProcess::NotRunning || gdbProcess_->state() != QProcess::NotRunning;
    if (sessionRunning && configurationChanged)
        stopServer();

    openocdExe_ = newOpenOcd;
    gdbExe_ = newGdb;
    interface_ = newInterface;
    target_ = newTarget;
    elfPath_ = newElf;
    speed_ = newSpeed;
    resetMethod_ = resetMethod;

    Q_UNUSED(autoStart);
    setStatus("Configured — press Start OCD");
    appendLog(QString("Configured ELF: %1").arg(elfPath_));
}

void DebugColumn3::setAvailableCores(bool core1Enabled)
{
    /**Updates the semantic core choices from the active project's configuration.*/
    if (!coreSelector_)
        return;

    const int expectedCount = core1Enabled ? 2 : 1;
    if (core1Enabled_ == core1Enabled && coreSelector_->count() == expectedCount)
        return;

    core1Enabled_ = core1Enabled;

    const QSignalBlocker blocker(coreSelector_);
    coreSelector_->clear();
    coreSelector_->addItem("Core 0", static_cast<int>(DebugCore::Core0));
    if (core1Enabled_)
        coreSelector_->addItem("Core 1", static_cast<int>(DebugCore::Core1));
    coreSelector_->setCurrentIndex(0);
    selectedCore_ = DebugCore::Core0;
    coreThreadMappingResolved_ = false;
    coreThreadMapping_.clear();
    setCoreSelectorEnabled(false);
}

QString DebugColumn3::coreName(DebugCore core) const
{
    /**Returns the user-facing physical-core label.*/
    return core == DebugCore::Core0 ? QStringLiteral("Core 0") : QStringLiteral("Core 1");
}

QString DebugColumn3::coreTargetName(DebugCore core) const
{
    /**Returns the active OpenOCD target name used in diagnostic messages.*/
    return core == DebugCore::Core0 ? QStringLiteral("rp2350.cm0") : QStringLiteral("rp2350.cm1");
}

int DebugColumn3::threadForCore(DebugCore core) const
{
    /**Resolves a physical core through the mapping discovered in this GDB session.*/
    for (auto it = coreThreadMapping_.cbegin(); it != coreThreadMapping_.cend(); ++it)
    {
        if (it.value() == core)
            return it.key();
    }
    return -1;
}

void DebugColumn3::setCoreSelectorEnabled(bool enabled)
{
    /**Enables core selection only after a live GDB mapping is available.*/
    if (coreSelector_)
        coreSelector_->setEnabled(enabled && coreThreadMappingResolved_);
}

void DebugColumn3::resetCoreSelection()
{
    /**Clears session-scoped thread IDs and returns the UI to its safe default.*/
    coreThreadMapping_.clear();
    coreThreadMappingResolved_ = false;
    selectedCore_ = DebugCore::Core0;
    if (coreSelector_)
    {
        const QSignalBlocker blocker(coreSelector_);
        coreSelector_->setCurrentIndex(0);
    }
    setCoreSelectorEnabled(false);
}

void DebugColumn3::resolveCoreThreadMapping()
{
    /**Requests and resolves physical-core target names from the active GDB session.*/
    if (gdbProcess_->state() == QProcess::NotRunning)
        return;
    sendGdbCommand(QStringLiteral("info threads"), false);
    appendLog("Resolving RP2350 physical core to GDB thread mapping.");
}

void DebugColumn3::selectDebugCore(DebugCore core)
{
    /**Selects a physical core through the session-specific GDB thread mapping.*/
    const int threadId = threadForCore(core);
    if (threadId < 0 || gdbProcess_->state() == QProcess::NotRunning)
    {
        appendLog(QString("Cannot select %1: target mapping or GDB session is unavailable.").arg(coreName(core)));
        const QSignalBlocker blocker(coreSelector_);
        coreSelector_->setCurrentIndex(core == DebugCore::Core1 ? 1 : 0);
        return;
    }

    sendGdbCommand(QString("thread %1").arg(threadId), false);
    selectedCore_ = core;
    const QSignalBlocker blocker(coreSelector_);
    coreSelector_->setCurrentIndex(core == DebugCore::Core1 ? 1 : 0);
    appendLog(
        QString("Debug core selected: %1 (%2), GDB thread %3").arg(coreName(core), coreTargetName(core)).arg(threadId));
}

void DebugColumn3::updateSelectedCoreFromThread(int threadId)
{
    /**Synchronizes the semantic selector when GDB reports a thread switch.*/
    if (!coreThreadMapping_.contains(threadId))
        return;
    selectedCore_ = coreThreadMapping_.value(threadId);
    if (coreSelector_)
    {
        const QSignalBlocker blocker(coreSelector_);
        coreSelector_->setCurrentIndex(selectedCore_ == DebugCore::Core1 ? 1 : 0);
    }
    appendLog(QString("GDB active target: %1 (%2)").arg(coreName(selectedCore_), coreTargetName(selectedCore_)));
}

void DebugColumn3::setSessionButtons(bool running)
{
    /**Updates the explicit OpenOCD lifecycle controls.

    Entering or refreshing the Debug page never changes this state; only an
    explicit start, terminal startup failure, or explicit stop does.
    */
    if (startButton_)
        startButton_->setEnabled(!running);
    if (readbackStartButton_)
        readbackStartButton_->setEnabled(!running);
    if (stopButton_)
        stopButton_->setEnabled(running);
}

void DebugColumn3::startSession()
{
    /**Starts OpenOCD first and attaches GDB only after the server is ready.

    Configuration is validated before touching hardware so missing tools, target
    scripts, and stale build artifacts fail with actionable messages instead of an
    opaque child-process error.
    */
    if (openocdProcess_->state() != QProcess::NotRunning)
    {
        if (gdbProcess_->state() == QProcess::NotRunning)
        {
            appendLog("OpenOCD is already running; checking GDB server readiness.");
            readinessAttempts_ = 0;
            probeOpenOcdReadiness();
        }
        return;
    }

    QString error;
    if (!validateConfiguration(&error))
    {
        setSessionButtons(false);
        setStatus("Debug configuration invalid");
        appendLog("Cannot start debug session: " + error);
        if (readbackOnly_ && readbackRequestAccepted_)
        {
            if (readbackStatus_)
                readbackStatus_->setText("Failed");
            writeReadbackResult(false, QStringLiteral("REQUEST_SETUP: ") + error);
        }
        return;
    }

    stopping_ = false;
    resetCoreSelection();
    setSessionButtons(true);
    startOpenOcd();
}

void DebugColumn3::startReadbackSession()
{
    /**Accepts one explicit runner request before starting certification readback.*/
    // This is intentionally the first operation in the slot. It proves that the
    // Qt clicked signal reached C++ even when request validation later fails.
    const QString suppliedRequestPath = qEnvironmentVariable("PVD_READBACK_REQUEST_PATH");
    const QString appRequestPath = QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("../validation/pin_certification/requests/active-readback-request.json"));
    readbackRequestPath_ = suppliedRequestPath.isEmpty() ? QDir::cleanPath(appRequestPath) : QDir::cleanPath(suppliedRequestPath);
    QFile earlyRequest(readbackRequestPath_);
    if (earlyRequest.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QJsonParseError earlyError;
        const QJsonDocument earlyDocument = QJsonDocument::fromJson(earlyRequest.readAll(), &earlyError);
        earlyRequest.close();
        const QString earlyAckPath = earlyDocument.object().value("ackPath").toString();
        if (earlyError.error == QJsonParseError::NoError && !earlyAckPath.isEmpty())
        {
            readbackAckPath_ = QDir::cleanPath(earlyAckPath);
            QDir().mkpath(QFileInfo(readbackAckPath_).absolutePath());
            QJsonObject ack;
            ack["runId"] = earlyDocument.object().value("runId").toString();
            ack["event"] = "CXX_TRIGGER_RECEIVED";
            ack["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
            ack["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
            QFile ackFile(readbackAckPath_ + ".tmp");
            if (ackFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                ackFile.write(QJsonDocument(ack).toJson(QJsonDocument::Indented));
                ackFile.flush();
                ackFile.close();
                QFile::remove(readbackAckPath_);
                QFile::rename(readbackAckPath_ + ".tmp", readbackAckPath_);
            }
        }
    }
    readbackOnly_ = true;
    QFile requestFile(readbackRequestPath_);
    if (!requestFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (readbackStatus_)
            readbackStatus_->setText("Failed");
        appendLog("READBACK_REQUEST_INVALID: request file unavailable: " + readbackRequestPath_);
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument request = QJsonDocument::fromJson(requestFile.readAll(), &parseError);
    requestFile.close();
    const QJsonObject object = request.object();
    const QString requestedRunId = object.value("runId").toString();
    const QString requestedResultPath = object.value("resultPath").toString();
    const QString requestedLogPath = object.value("rawLogPath").toString();
    readbackAckPath_ = QDir::cleanPath(object.value("ackPath").toString());
    if (parseError.error != QJsonParseError::NoError || requestedRunId.isEmpty() || requestedResultPath.isEmpty() ||
        requestedLogPath.isEmpty() || object.value("mode").toString() != QStringLiteral("CertificationReadback"))
    {
        if (readbackStatus_)
            readbackStatus_->setText("Failed");
        appendLog("READBACK_REQUEST_INVALID: missing runId/resultPath/rawLogPath/mode");
        return;
    }
    readbackRunId_ = requestedRunId;
    readbackResultPathValue_ = QDir::cleanPath(requestedResultPath);
    readbackLogPath_ = QDir::cleanPath(requestedLogPath);
    if (readbackRunId_.isEmpty() || readbackResultPathValue_.isEmpty() || readbackLogPath_.isEmpty())
    {
        if (readbackStatus_)
            readbackStatus_->setText("Failed");
        appendLog("READBACK_REQUEST_INVALID: empty normalized output path");
        return;
    }
    QDir().mkpath(QFileInfo(readbackLogPath_).absolutePath());
    QDir().mkpath(QFileInfo(readbackResultPathValue_).absolutePath());
    readbackRequestAccepted_ = true;
    if (readbackStatus_)
        readbackStatus_->setText("Trigger received");
    appendLog(QString("READBACK_TRIGGER_RECEIVED run_id=%1").arg(readbackRunId_));
    if (readbackResultPath_)
        readbackResultPath_->setText(readbackResultPathValue_);
    if (readbackStatus_)
        readbackStatus_->setText("Request accepted");
    if (!readbackLogPath_.isEmpty())
    {
        QFile log(readbackLogPath_);
        if (log.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            log.write(QString("READBACK_START run_id=%1\n").arg(readbackRunId_).toUtf8());
            log.flush();
        }
    }
    appendLog(QString("READBACK_REQUEST_ACCEPTED run_id=%1 result=%2 log=%3").arg(readbackRunId_, readbackResultPathValue_, readbackLogPath_));
    appendLog("CERTIFICATION READBACK ATTACH requested: reset/load/program/continue forbidden before evidence read.");
    startSession();
}

void DebugColumn3::startReadbackEvidence()
{
    if (!readbackOnly_ || readbackEvidenceActive_ || gdbProcess_->state() == QProcess::NotRunning)
        return;
    readbackEvidenceActive_ = true;
    readbackEvidenceCompleted_ = false;
    readbackCommands_.clear();
    const QString p = QStringLiteral("observer_pwm0a_100hz_50_clean_gpio1");
    readbackCommands_ << QStringLiteral("print (int)%1_complete").arg(p)
                      << QStringLiteral("print (int)%1_pass").arg(p)
                      << QStringLiteral("print (int)%1_sample_count").arg(p)
                      << QStringLiteral("print (int)%1_transition_count").arg(p);
    for (int i = 0; i < 12; ++i)
        readbackCommands_ << QStringLiteral("print (int)%1_transition_state[%2]").arg(p).arg(i)
                          << QStringLiteral("print (unsigned long long)%1_transition_us[%2]").arg(p).arg(i);
    readbackCommands_ << QStringLiteral("print (int)%1_overflow").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_dbgpause").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer_high").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer_low").arg(p)
                      << QStringLiteral("print (unsigned long long)%1_startup_raw_timer_64").arg(p)
                      << QStringLiteral("print (unsigned long long)%1_startup_time_us_64").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_dbgpause").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer_high").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer_low").arg(p)
                      << QStringLiteral("print (unsigned long long)%1_end_raw_timer_64").arg(p)
                      << QStringLiteral("print (unsigned long long)%1_end_time_us_64").arg(p)
                      << QStringLiteral("print (int)%1_timer_progress_valid").arg(p);
    readbackCommands_ << QStringLiteral("print (unsigned int)%1_startup_resets_reset").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_resets_reset_done").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer0_dbgpause").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer0_pause").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer0_source").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer0_timerawh").arg(p)
                      << QStringLiteral("print (unsigned int)%1_startup_timer0_timerawl").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_resets_reset").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_resets_reset_done").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer0_dbgpause").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer0_pause").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer0_source").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer0_timerawh").arg(p)
                      << QStringLiteral("print (unsigned int)%1_end_timer0_timerawl").arg(p)
                      << QStringLiteral("print (int)%1_end_register_snapshot_captured").arg(p);
    for (int i = 0; i < 12; ++i)
        readbackCommands_ << QStringLiteral("print (unsigned int)%1_transition_timer_high[%2]").arg(p).arg(i)
                          << QStringLiteral("print (unsigned int)%1_transition_timer_low[%2]").arg(p).arg(i)
                          << QStringLiteral("print (unsigned long long)%1_transition_raw_timer_64[%2]").arg(p).arg(i);
    readbackResponseBuffer_.clear();
    readbackTranscript_.clear();
    readbackCommandId_ = 0;
    readbackMiFirstEvidenceToken_ = 0;
    if (readbackStatus_)
        readbackStatus_->setText("Reading evidence");
    readbackEvidenceReadStarted_ = true;
    readbackStage_ = QStringLiteral("EVIDENCE_READING");
    appendLog(QString("[READBACK %1] EVIDENCE_READING: C++ session-owned transport; commands=%2")
                  .arg(readbackRunId_).arg(readbackCommands_.size()));
    if (readbackDeadlineTimer_)
        readbackDeadlineTimer_->start(60000);
    sendNextReadbackCommand();
}

void DebugColumn3::startReadbackSynchronization()
{
    if (!readbackOnly_ || readbackSyncActive_ || readbackEvidenceActive_ ||
        gdbProcess_->state() == QProcess::NotRunning)
        return;

    // Synchronize after halt, when all connection/thread notifications and the
    // response to the halt request have had an opportunity to drain.  A unique
    // prompt is the transaction delimiter; generic `(gdb)` is not sufficient.
    const QString safeRunId = readbackRunId_.isEmpty() ? QStringLiteral("unknown") : readbackRunId_;
    readbackPrompt_ = QStringLiteral("__PVD_GDB_%1__").arg(safeRunId);
    readbackSyncToken_ = QStringLiteral("__PVD_SYNC_%1__").arg(safeRunId);
    readbackSyncActive_ = true;
    readbackStage_ = QStringLiteral("GDB_SYNC");
    readbackSyncPromptSeen_ = false;
    readbackSyncTokenSeen_ = false;
    readbackResponseBuffer_.clear();
    appendLog(QString("[READBACK %1] GDB_SYNC: configuring prompt %2").arg(readbackRunId_, readbackPrompt_));
    sendGdbCommand(QStringLiteral("set prompt %1").arg(readbackPrompt_), false);
    // The sync deadline is deliberately independent of the 60-second evidence
    // deadline and covers prompt setup plus the harmless echo transaction.
    QTimer::singleShot(5000, this,
                       [this]()
                       {
                           if (readbackSyncActive_)
                           {
                               readbackSyncActive_ = false;
                               finishReadback(false, QStringLiteral("GDB_SYNC_TIMEOUT"));
                           }
                       });
}

void DebugColumn3::sendNextReadbackCommand()
{
    if (!readbackEvidenceActive_)
        return;
    if (readbackCommands_.isEmpty())
    {
        finishReadback(true);
        return;
    }
    readbackLastCommand_ = readbackCommands_.dequeue();
    readbackResponseBuffer_.clear();
    ++readbackCommandId_;
    appendLog(QString("[READBACK %1] COMMAND %2: %3").arg(readbackRunId_).arg(readbackCommandId_).arg(readbackLastCommand_));
    if (readbackMiMode_)
    {
        QString expression = readbackLastCommand_;
        if (expression.startsWith(QStringLiteral("print ")))
            expression.remove(0, 6);
        expression.replace('\\', QStringLiteral("\\\\"));
        expression.replace('"', QStringLiteral("\\\""));
        sendReadbackMiCommand(QStringLiteral("-data-evaluate-expression \"%1\"").arg(expression));
        return;
    }
        sendGdbCommand(readbackLastCommand_, false);
}

void DebugColumn3::sendReadbackMiCommand(const QString& command)
{
    if (gdbProcess_->state() == QProcess::NotRunning || readbackMiPendingToken_ != 0)
        return;
    readbackMiPendingToken_ = ++readbackMiToken_;
    if (readbackMiStartupPhase_.isEmpty() && readbackMiFirstEvidenceToken_ == 0)
        readbackMiFirstEvidenceToken_ = readbackMiPendingToken_;
    readbackMiPendingCommand_ = command;
    const QString wire = QString::number(readbackMiPendingToken_) + command;
    appendLog(QString("[READBACK %1] TX token=%2 %3").arg(readbackRunId_).arg(readbackMiPendingToken_).arg(command));
    gdbProcess_->write(wire.toUtf8());
    gdbProcess_->write("\n");
}

void DebugColumn3::advanceReadbackMiStartup(const QString& resultClass, const QString& record)
{
    if (resultClass == QStringLiteral("error"))
    {
        finishReadback(false, QStringLiteral("GDB_MI_STARTUP_ERROR: ") + record);
        return;
    }
    if (readbackMiStartupPhase_ == QStringLiteral("PAGINATION"))
    {
        readbackMiStartupPhase_ = QStringLiteral("CONFIRM");
        sendReadbackMiCommand(QStringLiteral("-gdb-set confirm off"));
    }
    else if (readbackMiStartupPhase_ == QStringLiteral("CONFIRM"))
    {
        readbackMiStartupPhase_ = QStringLiteral("CONNECT");
        sendReadbackMiCommand(QStringLiteral("-target-select extended-remote localhost:3333"));
    }
    else if (readbackMiStartupPhase_ == QStringLiteral("CONNECT"))
    {
        readbackGdbConnected_ = true;
        appendLog(QString("[READBACK %1] GDB_CONNECTED: MI target-select completed").arg(readbackRunId_));
        readbackMiStartupPhase_ = QStringLiteral("INTERRUPT");
        sendReadbackMiCommand(QStringLiteral("-exec-interrupt --all"));
    }
    else if (readbackMiStartupPhase_ == QStringLiteral("INTERRUPT"))
    {
        readbackTargetHalted_ = true;
        readbackSyncTokenSeen_ = true;
        readbackMiStartupPhase_.clear();
        appendLog(QString("[READBACK %1] GDB_SYNC PASS: MI startup records classified; halt acknowledged").arg(readbackRunId_));
        startReadbackEvidence();
    }
}

void DebugColumn3::consumeReadbackMiOutput(const QString& output)
{
    if (!readbackOnly_ || !readbackMiMode_)
        return;
    readbackMiBuffer_ += output;
    while (true)
    {
        const int newline = readbackMiBuffer_.indexOf('\n');
        if (newline < 0)
            return;
        QString record = readbackMiBuffer_.left(newline);
        readbackMiBuffer_.remove(0, newline + 1);
        if (record.endsWith('\r'))
            record.chop(1);
        if (record.isEmpty())
            continue;
        const MiRecord parsed = pvd::mi::parseRecord(record);
        if (parsed.kind == MiRecord::Kind::Empty || parsed.kind == MiRecord::Kind::Prompt)
            continue;
        if (parsed.kind == MiRecord::Kind::ExecAsync || parsed.kind == MiRecord::Kind::StatusAsync ||
            parsed.kind == MiRecord::Kind::NotifyAsync || parsed.kind == MiRecord::Kind::Console ||
            parsed.kind == MiRecord::Kind::Target || parsed.kind == MiRecord::Kind::Log)
        {
            if (parsed.kind == MiRecord::Kind::ExecAsync || parsed.kind == MiRecord::Kind::StatusAsync ||
                parsed.kind == MiRecord::Kind::NotifyAsync)
                ++readbackMiAsyncRecordCount_;
            appendLog(QString("[READBACK %1] RX out-of-band %2").arg(readbackRunId_, record));
            continue;
        }
        if (parsed.kind == MiRecord::Kind::Result)
        {
            const int token = parsed.token;
            const QString resultClass = parsed.resultClass;
            if (token != readbackMiPendingToken_)
            {
                ++readbackMiForeignTokenCount_;
                appendLog(QString("[READBACK %1] RX foreign token=%2 expected=%3 %4")
                              .arg(readbackRunId_).arg(token).arg(readbackMiPendingToken_).arg(record));
                continue;
            }
            const QString command = readbackMiPendingCommand_;
            readbackMiPendingToken_ = 0;
            readbackMiPendingCommand_.clear();
            appendLog(QString("[READBACK %1] RX token=%2 ^%3 %4")
                          .arg(readbackRunId_).arg(token).arg(resultClass).arg(parsed.text));
            if (readbackMiStartupPhase_.isEmpty())
            {
                readbackLastResponse_ = record;
                readbackTranscript_ += QString("COMMAND %1: %2\nRESPONSE:\n%3\n")
                                           .arg(readbackCommandId_).arg(command, record);
                if (resultClass == QStringLiteral("error"))
                {
                    finishReadback(false, QStringLiteral("GDB_MI_EVIDENCE_ERROR: ") + record);
                    return;
                }
                sendNextReadbackCommand();
            }
            else
                advanceReadbackMiStartup(resultClass, record);
            continue;
        }
        if (record.startsWith('*') || record.startsWith('+') || record.startsWith('=') ||
            record.startsWith('~') || record.startsWith('@') || record.startsWith('&'))
        {
            ++readbackMiAsyncRecordCount_;
            appendLog(QString("[READBACK %1] RX async/stream %2").arg(readbackRunId_, record));
            continue;
        }
        ++readbackMiParseFailureCount_;
        appendLog(QString("[READBACK %1] RX unclassified MI record %2").arg(readbackRunId_, record));
    }
}

void DebugColumn3::consumeReadbackOutput(const QString& output)
{
    if (!readbackEvidenceActive_ && !readbackSyncActive_)
        return;
    readbackResponseBuffer_ += output;

    if (readbackSyncActive_)
    {
        if (!readbackSyncPromptSeen_ && readbackResponseBuffer_.contains(readbackPrompt_))
        {
            readbackSyncPromptSeen_ = true;
            readbackResponseBuffer_.clear();
            appendLog(QString("[READBACK %1] GDB_SYNC prompt observed").arg(readbackRunId_));
            sendGdbCommand(QStringLiteral("echo %1").arg(readbackSyncToken_), false);
        }
        if (readbackSyncPromptSeen_ && readbackResponseBuffer_.contains(readbackSyncToken_) &&
            readbackResponseBuffer_.contains(readbackPrompt_))
        {
            readbackSyncTokenSeen_ = true;
            readbackSyncActive_ = false;
            readbackResponseBuffer_.clear();
            appendLog(QString("[READBACK %1] GDB_SYNC PASS token=%2").arg(readbackRunId_, readbackSyncToken_));
            startReadbackEvidence();
        }
        return;
    }

    if (!readbackResponseBuffer_.contains(readbackPrompt_))
        return;
    readbackLastResponse_ = readbackResponseBuffer_;
    readbackTranscript_ += QString("COMMAND %1: %2\nRESPONSE:\n%3\n")
                               .arg(readbackCommandId_).arg(readbackLastCommand_, readbackLastResponse_);
    appendLog(QString("[READBACK %1] RESPONSE %2").arg(readbackRunId_).arg(readbackCommandId_));
    readbackResponseBuffer_.clear();
    sendNextReadbackCommand();
}

void DebugColumn3::finishReadback(bool success, const QString& reason)
{
    if (!readbackEvidenceActive_ && !readbackSyncActive_)
        return;
    readbackSyncActive_ = false;
    readbackEvidenceActive_ = false;
    if (readbackDeadlineTimer_)
        readbackDeadlineTimer_->stop();
    readbackEvidenceCompleted_ = success;
    writeReadbackResult(success, reason);
    if (readbackStatus_)
        readbackStatus_->setText(success ? "Completed" : "Failed");
    appendLog(QString("[READBACK %1] %2").arg(readbackRunId_, success ? "EVIDENCE_COMPLETE" : "FAILED: " + reason));
}

void DebugColumn3::writeReadbackResult(bool success, const QString& reason)
{
    if (readbackLogPath_.isEmpty() || readbackResultPathValue_.isEmpty())
        return;
    QFile log(readbackLogPath_);
    if (log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
    {
        log.write(readbackTranscript_.toUtf8());
        log.flush();
        log.close();
    }
    QJsonObject result;
    result["status"] = success ? "READBACK_COMPLETED" : "READBACK_FAILED";
    result["readback_run_id"] = readbackRunId_;
    result["runId"] = readbackRunId_;
    result["stage_reached"] = success ? "EVIDENCE_COMPLETE" : (readbackStage_.isEmpty() ? QStringLiteral("REQUEST_SETUP") : readbackStage_);
    result["failure_stage"] = success ? "" : (readbackStage_.isEmpty() ? QStringLiteral("REQUEST_SETUP") : readbackStage_);
    result["failure_reason"] = success ? "" : reason;
    result["last_command"] = readbackLastCommand_;
    result["last_response"] = readbackLastResponse_;
    result["openocd_ready"] = readbackOpenOcdReady_;
    result["gdb_connected"] = readbackGdbConnected_;
    result["target_halted"] = readbackTargetHalted_;
    result["evidence_read_started"] = readbackEvidenceReadStarted_;
    result["evidence_read_completed"] = success;
    result["gdb_sync"] = readbackSyncTokenSeen_;
    result["gdb_protocol"] = readbackMiMode_ ? QStringLiteral("mi2") : QStringLiteral("cli");
    result["gdb_association_valid"] = readbackMiMode_ && readbackMiForeignTokenCount_ == 0 &&
                                       readbackMiParseFailureCount_ == 0 && readbackEvidenceCompleted_;
    result["gdb_async_record_count"] = readbackMiAsyncRecordCount_;
    result["gdb_foreign_token_count"] = readbackMiForeignTokenCount_;
    result["gdb_parse_failure_count"] = readbackMiParseFailureCount_;
    result["first_evidence_token"] = readbackMiFirstEvidenceToken_;
    result["last_evidence_token"] = readbackMiMode_ ? readbackMiToken_ : 0;
    result["evidence_command_count"] = readbackCommandId_;
    result["reset_command_count"] = 0;
    result["program_command_count"] = 0;
    result["continue_command_count"] = 0;
    result["memory_write_count"] = 0;
    result["cleanup_completed"] = false;
    result["raw_log_path"] = readbackLogPath_;
    const QString temporary = readbackResultPathValue_ + ".tmp";
    QFile temp(temporary);
    if (temp.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        temp.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
        temp.flush();
        temp.close();
        QFile::remove(readbackResultPathValue_);
        QFile::rename(temporary, readbackResultPathValue_);
    }
}

void DebugColumn3::stopServer()
{
    /**Stops GDB and OpenOCD and returns a halted RP2350 to a running state when appropriate.

    Shutdown is bounded by short timeouts and force-kill fallbacks so closing Pico
    Visual Designer cannot hang indefinitely on a broken debug connection.
    */
    appendLog("stopSession entered (implementation handler: stopServer)");
    appendLog(QString("OpenOCD QProcess pointer valid: %1").arg(openocdProcess_ ? "YES" : "NO"));
    if (openocdProcess_)
        appendLog(QString("QProcess PID: %1 state before stop: %2")
                      .arg(openocdProcess_->processId())
                      .arg(static_cast<int>(openocdProcess_->state())));

    if (!openocdProcess_ || !gdbProcess_)
        return;
    if (openocdProcess_->state() == QProcess::NotRunning && gdbProcess_->state() == QProcess::NotRunning)
    {
        markReadbackCleanupComplete();
        setSessionButtons(false);
        resetCoreSelection();
        readbackOnly_ = false;
        return;
    }

    stopping_ = true;
    setSessionButtons(false);
    readinessTimer_->stop();
    setStatus("Stopping debug session...");
    stopGdb();
    stopOpenOcd();
    markReadbackCleanupComplete();
    stopping_ = false;
    resetCoreSelection();
    gdbOutputBuffer_.clear();
    readbackOnly_ = false;
    setStatus("Stopped");
    appendLog("Debug session stopped.");
}

void DebugColumn3::markReadbackCleanupComplete()
{
    if (!readbackResultPathValue_.isEmpty())
    {
        QFile resultFile(readbackResultPathValue_);
        if (resultFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QJsonParseError parseError;
            QJsonDocument document = QJsonDocument::fromJson(resultFile.readAll(), &parseError);
            resultFile.close();
            if (parseError.error == QJsonParseError::NoError && document.isObject())
            {
                QJsonObject result = document.object();
                result["cleanup_completed"] = true;
                const QString temporary = readbackResultPathValue_ + ".cleanup.tmp";
                QFile temporaryFile(temporary);
                if (temporaryFile.open(QIODevice::WriteOnly | QIODevice::Text))
                {
                    temporaryFile.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
                    temporaryFile.flush();
                    temporaryFile.close();
                    QFile::remove(readbackResultPathValue_);
                    QFile::rename(temporary, readbackResultPathValue_);
                }
            }
        }
    }
}

void DebugColumn3::appendLog(const QString& text)
{
    /**Appends normalized debugger output without creating excessive blank lines.

    QProcess chunks often include trailing line endings, so they are trimmed only at
    the end while preserving internal formatting such as GDB backtraces.
    */
    if (!log_ || text.isEmpty())
        return;
    QString normalized = text;
    while (normalized.endsWith('\n') || normalized.endsWith('\r'))
        normalized.chop(1);
    if (!normalized.isEmpty())
        log_->appendPlainText(normalized);
}

void DebugColumn3::setStatus(const QString& text)
{
    /**Updates the machine-readable and visible debug-session state.

    The dedicated status control gives GUI automation a stable observation point in
    addition to the detailed free-form log.
    */
    if (status_)
        status_->setText(text);
}

void DebugColumn3::interpretOpenOcdOutput(const QString& output)
{
    /**Classifies OpenOCD probe and target evidence into actionable debug states.

    Text matching is intentionally conservative: only well-known CMSIS-DAP/RP2350
    phrases change the stable status while the complete raw transcript remains visible.
    */
    if (output.contains("Listening on port 3333", Qt::CaseInsensitive))
    {
        openocdGdbServerReady_ = true;
        if (readbackOnly_)
            readbackOpenOcdReady_ = true;
        readinessTimer_->stop();
        setStatus("OpenOCD ready â€” GDB server listening");
        startGdb();
        return;
    }
    if (output.contains("unable to find a matching CMSIS-DAP", Qt::CaseInsensitive) ||
        output.contains("no CMSIS-DAP device", Qt::CaseInsensitive))
    {
        setStatus("Probe not found — check USB connection, driver access, and probe selection");
        return;
    }
    if (output.contains("unable to open CMSIS-DAP", Qt::CaseInsensitive) ||
        output.contains("failed to open CMSIS-DAP", Qt::CaseInsensitive))
    {
        setStatus("Probe open failed — check driver access and other OpenOCD instances");
        return;
    }
    if (output.contains("Examination failed", Qt::CaseInsensitive) ||
        output.contains("target examination failed", Qt::CaseInsensitive))
    {
        setStatus("Target not detected — check SWD wiring, target power, ground, and adapter speed");
        return;
    }
    if (output.contains("Examination succeed", Qt::CaseInsensitive))
    {
        setStatus("Target detected — RP2350 examination succeeded");
        return;
    }
    if (output.contains("SWD DPIDR", Qt::CaseInsensitive))
    {
        setStatus("SWD link detected — examining RP2350");
        return;
    }
    if (output.contains("CMSIS-DAP: Interface ready", Qt::CaseInsensitive) ||
        output.contains("CMSIS-DAP: SWD supported", Qt::CaseInsensitive))
    {
        setStatus("Probe found — CMSIS-DAP SWD interface ready");
        return;
    }
    if (output.contains("Listening on port 3333", Qt::CaseInsensitive))
    {
        openocdGdbServerReady_ = true;
        readinessTimer_->stop();
        startGdb();
    }
    setStatus("OpenOCD ready — GDB server listening");
}

void DebugColumn3::interpretGdbOutput(const QString& output)
{
    /**Classifies GDB connection and execution-state evidence for the active target.

    The status summarizes common lifecycle transitions while breakpoint, stack, and
    register details remain available verbatim in the Debug log.
    */
    gdbOutputBuffer_.append(output);
    if (output.contains("connection refused", Qt::CaseInsensitive) ||
        output.contains("connection timed out", Qt::CaseInsensitive))
    {
        setStatus("GDB connection failed — verify OpenOCD port 3333 and target state");
        return;
    }
    if (!gdbStartupCommandsSent_ && output.contains("(gdb)"))
        sendGdbStartupCommands();
    if (output.contains("Remote debugging using", Qt::CaseInsensitive) ||
        output.contains("Remote debugging from host", Qt::CaseInsensitive))
    {
        if (readbackOnly_)
            readbackGdbConnected_ = true;
        setStatus("Debug session ready");
        appendLog("[READBACK] GDB_CONNECTED: remote connection established.");
        resolveCoreThreadMapping();
    }

    QRegularExpression threadLine(QStringLiteral("^\\s*\\*?\\s*(\\d+)\\s+Thread\\s+\\d+\\s+\"rp2350\\.cm([01])\""),
                                  QRegularExpression::MultilineOption);
    auto match = threadLine.match(gdbOutputBuffer_);
    while (match.hasMatch())
    {
        const int threadId = match.captured(1).toInt();
        const DebugCore core = match.captured(2) == "0" ? DebugCore::Core0 : DebugCore::Core1;
        if (core == DebugCore::Core0 || core1Enabled_)
            coreThreadMapping_.insert(threadId, core);
        match = threadLine.match(gdbOutputBuffer_, match.capturedEnd());
    }

    const bool core0Mapped = threadForCore(DebugCore::Core0) >= 0;
    const bool core1Mapped = !core1Enabled_ || threadForCore(DebugCore::Core1) >= 0;
    if (core0Mapped && core1Mapped && !coreThreadMappingResolved_)
    {
        coreThreadMappingResolved_ = true;
        setCoreSelectorEnabled(true);
        appendLog(
            QString("RP2350 core mapping resolved: Core 0 -> thread %1 (%2), Core 1 -> %3")
                .arg(threadForCore(DebugCore::Core0))
                .arg(coreTargetName(DebugCore::Core0))
                .arg(core1Enabled_ ? QString::number(threadForCore(DebugCore::Core1)) : QStringLiteral("not enabled")));
        selectDebugCore(DebugCore::Core0);
    }

    const QRegularExpression switchedThread(QStringLiteral(R"(\[Switching to thread (\d+)\])"));
    const auto switched = switchedThread.match(output);
    if (switched.hasMatch())
        updateSelectedCoreFromThread(switched.captured(1).toInt());

    if (core1Enabled_ && core0Mapped && !core1Mapped && output.contains("info threads", Qt::CaseInsensitive))
        appendLog("Core 1 target rp2350.cm1 was not found in the active GDB thread list.");

    const QRegularExpression hitThread(QStringLiteral(R"(Thread\s+(\d+)\s+"rp2350\.cm[01]".*hit Breakpoint)"));
    const auto hit = hitThread.match(output);
    if (hit.hasMatch())
        updateSelectedCoreFromThread(hit.captured(1).toInt());
    if (output.contains("Breakpoint ", Qt::CaseInsensitive) ||
        output.contains("Program received signal", Qt::CaseInsensitive) ||
        output.contains("stopped", Qt::CaseInsensitive))
    {
        setStatus("Target halted");
        return;
    }
    if (output.contains("Continuing.", Qt::CaseInsensitive))
        setStatus("Target running");
}

bool DebugColumn3::validateConfiguration(QString* error) const
{
    /**Checks all local prerequisites required to start an RP2350 debug session.

    Relative OpenOCD script names and bare executable commands remain legal because
    their final resolution belongs to OpenOCD and the process PATH respectively.
    */
    const auto fail = [error](const QString& message)
    {
        /**Stores one validation failure for the caller.

        The helper keeps each validation branch concise while preserving one clear
        user-facing cause.
        */
        if (error)
            *error = message;
        return false;
    };

    if (openocdExe_.isEmpty())
        return fail("OpenOCD executable is empty.");
    if (commandNeedsFileValidation(openocdExe_) && !QFileInfo::exists(openocdExe_))
        return fail("OpenOCD executable does not exist: " + openocdExe_);
    if (gdbExe_.isEmpty())
        return fail("GDB executable is empty.");
    if (commandNeedsFileValidation(gdbExe_) && !QFileInfo::exists(gdbExe_))
        return fail("GDB executable does not exist: " + gdbExe_);
    if (interface_.isEmpty())
        return fail("OpenOCD interface configuration is empty.");
    if (QFileInfo(interface_).isAbsolute() && !QFileInfo::exists(interface_))
        return fail("OpenOCD interface configuration does not exist: " + interface_);
    if (target_.isEmpty())
        return fail("OpenOCD target configuration is empty.");
    if (QFileInfo(target_).isAbsolute() && !QFileInfo::exists(target_))
        return fail("OpenOCD target configuration does not exist: " + target_);
    if (elfPath_.isEmpty() || !QFileInfo::exists(elfPath_))
        return fail("Built ELF not found. Configure and build the project first: " + elfPath_);
    return true;
}

void DebugColumn3::startOpenOcd()
{
    /**Launches OpenOCD with the selected CMSIS-DAP interface and RP2350 target.

    Reset commands are deliberately deferred until GDB is attached, avoiding the
    target-initialization race that affected earlier OpenOCD startup behavior.
    */
    readinessAttempts_ = 0;
    openocdGdbServerReady_ = false;
    const QStringList args = {"-f", interface_, "-f", target_, "-c", QString("adapter speed %1").arg(speed_)};
    appendLog(QString("OpenOCD QProcess program: %1").arg(openocdExe_));
    appendLog(QString("OpenOCD QProcess arguments: %1").arg(args.join(" | ")));
    appendLog(QString("OpenOCD working directory: %1")
                  .arg(openocdProcess_->workingDirectory().isEmpty() ? QDir::currentPath()
                                                                     : openocdProcess_->workingDirectory()));
    appendLog("$ " + openocdExe_ + " " + args.join(' '));
    setStatus("Starting OpenOCD...");
    openocdProcess_->start(openocdExe_, args);
    connect(
        openocdProcess_, &QProcess::started, this,
        [this]()
        {
            /**Records the concrete child PID after QProcess has created it.*/
            appendLog(QString("OpenOCD started: pid=%1 state=%2 executable=%3")
                          .arg(openocdProcess_->processId())
                          .arg(static_cast<int>(openocdProcess_->state()))
                          .arg(openocdExe_));
        },
        Qt::SingleShotConnection);

    if (!readinessTimer_->isActive())
        readinessTimer_->start();
}

void DebugColumn3::probeOpenOcdReadiness()
{
    /**Detects when OpenOCD's default GDB server is accepting connections on port 3333.

    Polling is capped at roughly ten seconds so a disconnected probe or invalid target
    cannot leave the UI permanently in a starting state.
    */
    if (openocdProcess_->state() == QProcess::NotRunning)
    {
        readinessTimer_->stop();
        return;
    }

    QTcpSocket socket;
    // Readiness is driven by OpenOCD's explicit listening message; no dummy GDB connection is opened.
    if (socket.waitForConnected(40))
    {
        socket.disconnectFromHost();
        readinessTimer_->stop();
        appendLog("OpenOCD GDB server is ready on localhost:3333.");
        setStatus("OpenOCD ready — Running");
        // Socket readiness is authoritative even when OpenOCD split its
        // listening line across QProcess output chunks. Previously GDB was
        // launched only by text interpretation, so readback could wait
        // forever after the server was already ready.
        appendLog("[READBACK] OPENOCD_READY: localhost:3333 accepting connections.");
        startGdb();
        return;
    }

    ++readinessAttempts_;
    if (readinessAttempts_ >= 40)
    {
        readinessTimer_->stop();
        setStatus("OpenOCD startup timeout");
        appendLog("OpenOCD did not expose the GDB server on localhost:3333 within 10 seconds.");
        appendLog("[READBACK] FAILED: OPENOCD_READY_TIMEOUT.");
    }
}

void DebugColumn3::startGdb()
{
    /**Starts GDB for the current ELF and connects it to the OpenOCD remote target.

    The command stream disables pagination and confirmation prompts, then applies the
    selected reset policy only after the remote protocol connection exists.
    */
    if (gdbProcess_->state() != QProcess::NotRunning)
        return;

    QStringList args = {"--quiet", "--nx"};
    if (readbackOnly_)
        args << "--interpreter=mi2";
    args << elfPath_;
    appendLog("$ " + gdbExe_ + " " + args.join(' '));
    setStatus("Starting GDB...");
    gdbStartupCommandsSent_ = false;
    gdbOutputBuffer_.clear();
    readbackMiMode_ = readbackOnly_;
    readbackMiBuffer_.clear();
    readbackMiPendingToken_ = 0;
    readbackMiPendingCommand_.clear();
    readbackMiAsyncRecordCount_ = 0;
    readbackMiForeignTokenCount_ = 0;
    readbackMiParseFailureCount_ = 0;
    readbackMiToken_ = 100;
    resetCoreSelection();
    gdbProcess_->start(gdbExe_, args);

    if (!gdbProcess_->waitForStarted(1000))
    {
        setStatus("GDB failed to start");
        appendLog("GDB failed to start: " + gdbProcess_->errorString());
        return;
    }

    appendLog("GDB process started; waiting for the GDB prompt before connecting to OpenOCD.");
    appendLog("[READBACK] GDB_STARTING: process launched; prompt required before attach.");
    if (readbackMiMode_)
    {
        readbackMiStartupPhase_ = QStringLiteral("PAGINATION");
        sendReadbackMiCommand(QStringLiteral("-gdb-set pagination off"));
    }
}

void DebugColumn3::sendGdbStartupCommands()
{
    /**Connects GDB only after its interactive command prompt is ready.

    QProcess::waitForStarted() confirms process creation, not that GDB has
    initialized its command interpreter. Waiting for the prompt avoids racing
    OpenOCD's single GDB connection slot and makes startup deterministic.
    */
    if (gdbStartupCommandsSent_ || gdbProcess_->state() == QProcess::NotRunning)
        return;

    gdbStartupCommandsSent_ = true;
    sendGdbCommand("set pagination off", false);
    sendGdbCommand("set confirm off", false);
    sendGdbCommand("target extended-remote localhost:3333", false);
    if (!readbackOnly_)
    {
        const QString resetCommand = resetMonitorCommand();
        if (!resetCommand.isEmpty())
            sendGdbCommand(resetCommand, false);
        appendLog("Normal Debug startup commands sent, including configured reset policy.");
    }
    else
    {
        appendLog("Certification readback attach commands sent: target extended-remote only; no reset/load/program/continue.");
        QTimer::singleShot(100, this, [this]() { haltTarget(); });
    }
    setStatus("GDB connecting to OpenOCD...");
}

void DebugColumn3::sendGdbCommand(const QString& command, bool echo)
{
    /**Writes one command to the interactive GDB process.

    Commands are newline-terminated exactly once and optionally echoed with a stable
    prefix so user actions are distinguishable from debugger output.
    */
    if (gdbProcess_->state() == QProcess::NotRunning)
    {
        if (echo)
            appendLog("GDB is not running; command ignored: " + command);
        return;
    }

    if (echo)
        appendLog("gdb> " + command);
    gdbProcess_->write(command.toUtf8());
    gdbProcess_->write("\n");
}

void DebugColumn3::haltTarget()
{
    /**Halts the RP2350 through OpenOCD's control channel even while GDB is continuing.

    Using the server control port avoids relying on terminal Ctrl-C semantics through
    a QProcess pipe and gives the debugger a deterministic halt operation.
    */
    if (openocdProcess_->state() == QProcess::NotRunning)
    {
        appendLog("OpenOCD is not running; halt request ignored.");
        return;
    }
    if (!sendOpenOcdCommands({QStringLiteral("halt")}))
    {
        setStatus("Target halt failed");
        appendLog("OpenOCD control connection failed while requesting target halt.");
        return;
    }
    setStatus("Target halt requested");
    appendLog("OpenOCD halt command sent.");
    appendLog("[READBACK] TARGET_HALT_REQUESTED: OpenOCD control command sent.");
    if (readbackOnly_)
    {
        readbackTargetHalted_ = true;
        setStatus("Target halted");
        if (readbackStatus_)
            readbackStatus_->setText("Target halted");
        QTimer::singleShot(0, this, [this]() { startReadbackSynchronization(); });
    }
}

QString DebugColumn3::resetMonitorCommand() const
{
    /**Translates the persisted reset policy into an OpenOCD GDB monitor command.

    Project storage uses user-facing labels, while GDB requires the concrete
    "monitor reset ..." form after the remote target is connected.
    */
    if (resetMethod_ == "halt reset")
        return QStringLiteral("monitor reset halt");
    if (resetMethod_ == "run reset")
        return QStringLiteral("monitor reset run");
    return {};
}

void DebugColumn3::stopGdb()
{
    /**Detaches and terminates the GDB client with bounded fallbacks.

    Graceful detach is attempted first; terminate and kill ensure a wedged debugger
    cannot retain the target or prevent application shutdown.
    */
    if (gdbProcess_->state() == QProcess::NotRunning)
        return;

    gdbProcess_->write("detach\nquit\n");
    gdbProcess_->waitForBytesWritten(200);
    if (!gdbProcess_->waitForFinished(800))
    {
        gdbProcess_->terminate();
        if (!gdbProcess_->waitForFinished(500))
        {
            gdbProcess_->kill();
            gdbProcess_->waitForFinished(500);
        }
    }
}

void DebugColumn3::stopOpenOcd()
{
    /**Requests an orderly OpenOCD shutdown and force-stops only when necessary.

    Reset policies that can leave the target halted are normalized to reset-run on
    exit; the explicit "none" policy preserves target state and sends only shutdown.
    */
    if (openocdProcess_->state() == QProcess::NotRunning)
        return;

    appendLog(QString("OpenOCD cleanup: owned PID=%1 state=%2")
                  .arg(openocdProcess_->processId())
                  .arg(static_cast<int>(openocdProcess_->state())));
    QStringList commands;
    if (!readbackOnly_ && resetMethod_ != "none")
        commands << "reset run";
    commands << "shutdown";
    const bool gracefulRequestSent = sendOpenOcdCommands(commands);
    appendLog(QString("OpenOCD graceful stop requested: %1").arg(gracefulRequestSent ? "YES" : "NO"));

    const bool finishedGracefully = openocdProcess_->waitForFinished(1200);
    appendLog(QString("OpenOCD graceful wait finished: %1 state=%2")
                  .arg(finishedGracefully ? "YES" : "NO")
                  .arg(static_cast<int>(openocdProcess_->state())));
    if (!finishedGracefully)
    {
        appendLog("OpenOCD did not terminate after shutdown; forcing process termination.");
        openocdProcess_->terminate();
        const bool terminated = openocdProcess_->waitForFinished(500);
        appendLog(QString("OpenOCD terminate wait finished: %1 state=%2")
                      .arg(terminated ? "YES" : "NO")
                      .arg(static_cast<int>(openocdProcess_->state())));
        if (!terminated)
        {
            appendLog("OpenOCD kill fallback called: YES");
            openocdProcess_->kill();
            openocdProcess_->waitForFinished(500);
        }
        else
        {
            appendLog("OpenOCD kill fallback called: NO");
        }
    }
    appendLog(QString("OpenOCD state after cleanup: %1 PID=%2")
                  .arg(static_cast<int>(openocdProcess_->state()))
                  .arg(openocdProcess_->processId()));
}

bool DebugColumn3::sendOpenOcdCommands(const QStringList& commands, int connectTimeoutMs)
{
    /**Sends one or more commands to OpenOCD's telnet control port.

    The control channel is used only for orderly shutdown; normal reset behavior is
    issued through GDB after attachment so startup sequencing stays deterministic.
    */
    QTcpSocket control;
    control.connectToHost(QHostAddress::LocalHost, 4444);
    if (!control.waitForConnected(connectTimeoutMs))
        return false;

    for (const QString& command : commands)
    {
        control.write(command.toUtf8());
        control.write("\n");
    }
    control.waitForBytesWritten(500);
    control.waitForDisconnected(1500);
    return true;
}
} // namespace pvd
