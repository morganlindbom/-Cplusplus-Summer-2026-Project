// TransferColumn3.cpp
#include "systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp"

#include "systems/database/SqliteUtil.hpp"
#include "systems/mainwindow/PanelUtil.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>

namespace
{
QString executableFromLocation(const QString& location, const QString& executableName)
{
    /**Resolves an executable from a file, installation directory, or PATH-visible name.

    Pico tooling is distributed in several layouts, therefore a single resolver is
    used instead of embedding one developer-machine path in production code.
    */
    const QString trimmed = location.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QFileInfo info(trimmed);
    if (info.isFile())
        return info.absoluteFilePath();
    if (info.isDir())
    {
        const QStringList candidates = {QDir(trimmed).filePath(executableName),
                                        QDir(trimmed).filePath("bin/" + executableName)};
        for (const QString& candidate : candidates)
        {
            if (QFileInfo::exists(candidate))
                return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return QStandardPaths::findExecutable(trimmed);
}

QString firstExecutable(const QStringList& locations, const QString& executableName)
{
    /**Returns the first executable found in deterministic discovery order.

    Environment overrides are preferred, followed by the private PVD runtime, the
    Raspberry Pi user installation, and finally PATH.
    */
    for (const QString& location : locations)
    {
        const QString executable = executableFromLocation(location, executableName);
        if (!executable.isEmpty())
            return executable;
    }
    return {};
}

QString findOpenOcd()
{
    /**Finds the OpenOCD executable used for CMSIS-DAP transfer.

    The resolver supports both the reconstructed development environment and a
    self-contained runtime without depending on a legacy Program Files install.
    */
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString runtimeRoot = QStringLiteral(PVD_RUNTIME_ROOT);
    const QString found =
        firstExecutable({environment.value("PICO_OPENOCD_PATH"), QDir(runtimeRoot).filePath("openocd"),
                         QDir::home().filePath(".pico-sdk/openocd/0.12.0+dev"), QStringLiteral("openocd")},
                        QStringLiteral("openocd.exe"));
    return found.isEmpty() ? QStringLiteral("openocd") : found;
}

QString findPicotool()
{
    /**Finds picotool for direct RP2350 firmware loading.

    A private runtime copy is preferred over PATH so installed PVD deployments do
    not accidentally depend on an unrelated host version.
    */
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString runtimeRoot = QStringLiteral(PVD_RUNTIME_ROOT);
    const QString found = firstExecutable(
        {environment.value("PICOTOOL_PATH"), QDir(runtimeRoot).filePath("picotool"), QStringLiteral("picotool")},
        QStringLiteral("picotool.exe"));
    return found.isEmpty() ? QStringLiteral("picotool") : found;
}

QString openOcdScriptsRoot(const QString& executable)
{
    /**Finds the scripts directory belonging to the selected OpenOCD executable.

    Upstream and Raspberry Pi packages place scripts in different relative paths,
    so all common layouts are checked before allowing OpenOCD's own defaults.
    */
    if (executable.isEmpty() || !QFileInfo(executable).isFile())
        return {};

    const QDir executableDir = QFileInfo(executable).absoluteDir();
    const QStringList candidates = {executableDir.filePath("scripts"), executableDir.filePath("../scripts"),
                                    executableDir.filePath("../share/openocd/scripts"),
                                    executableDir.filePath("../../share/openocd/scripts")};
    for (const QString& candidate : candidates)
    {
        if (QFileInfo(candidate).isDir())
            return QDir::cleanPath(candidate);
    }
    return {};
}
} // namespace

namespace pvd
{
TransferColumn3::TransferColumn3(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds a freshness-gated transfer panel with automatic BOOTSEL-drive discovery.

    Transfer remains a separate boundary from Build: only a successful current build
    marker and source/artifact timestamp checks authorize firmware programming.
    */
    auto* layout = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Transfer"));
    auto* form = new QFormLayout();

    drive_ = new QLineEdit(this);
    drive_->setObjectName("transfer_drive");
    drive_->setPlaceholderText("Automatically detects an RPI-RP2 BOOTSEL drive");
    form->addRow("UF2 drive (copy method)", drive_);
    layout->addLayout(form);

    status_ = new QLabel("Ready", this);
    status_->setObjectName("transfer_status");
    status_->setWordWrap(true);
    layout->addWidget(status_);

    auto* transfer = new QPushButton("Transfer Firmware", this);
    transfer->setObjectName("transfer_firmware");
    layout->addWidget(transfer);

    log_ = new QPlainTextEdit(this);
    log_->setObjectName("transfer_log");
    log_->setReadOnly(true);
    layout->addWidget(log_, 1);

    process_ = new QProcess(this);
    connect(process_, &QProcess::readyReadStandardOutput, this,
            [this]()
            {
                /**Appends transfer-tool standard output to the persistent evidence log.

                Programming and verification messages remain visible for certification
                and user diagnosis.
                */
                const QString output = QString::fromLocal8Bit(process_->readAllStandardOutput());
                log_->appendPlainText(output);
                transferOutput_ += output;
            });
    connect(process_, &QProcess::readyReadStandardError, this,
            [this]()
            {
                /**Appends transfer-tool diagnostic output without treating stderr as failure.

                OpenOCD emits normal probe and target evidence on stderr, therefore final
                process status is determined by its exit code.
                */
                const QString output = QString::fromLocal8Bit(process_->readAllStandardError());
                log_->appendPlainText(output);
                transferOutput_ += output;
            });
    connect(process_, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError)
            {
                /**Reports transfer-tool launch/runtime failures with operating-system detail.

                Missing OpenOCD or picotool executables are distinguishable from target
                communication failures in the normal transcript.
                */
                setStatus("Transfer process error");
                log_->appendPlainText("Transfer process error: " + process_->errorString());
            });
    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus)
            {
                /**Publishes a stable terminal result after external transfer tools finish.

                Exit status complements textual OpenOCD/picotool evidence and gives GUI
                automation a deterministic completion point.
                */
                if (openOcdTransferActive_)
                    finishOpenOcdTransfer(exitCode, exitStatus);
                else
                {
                    const bool ok = exitStatus == QProcess::NormalExit && exitCode == 0;
                    setStatus(ok ? "Transfer completed" : "Transfer failed");
                    log_->appendPlainText(ok ? "Transfer process completed successfully."
                                             : QString("Transfer process failed with exit code %1.").arg(exitCode));
                }
            });
    connect(transfer, &QPushButton::clicked, this, &TransferColumn3::startTransfer);

    const QString detectedDrive = currentBootselDrive();
    if (!detectedDrive.isEmpty())
        drive_->setText(detectedDrive);
}

bool TransferColumn3::postProgramStartStatesValid(const QString& core0State, const QString& core1State)
{
    // Fail closed: only explicit running states for both configured cores pass.
    return core0State.compare(QStringLiteral("running"), Qt::CaseInsensitive) == 0 &&
           core1State.compare(QStringLiteral("running"), Qt::CaseInsensitive) == 0;
}

void TransferColumn3::finishOpenOcdTransfer(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool processOk = exitStatus == QProcess::NormalExit && exitCode == 0;
    // The target configuration names are emitted by the installed RP2350 cfg and
    // are deliberately checked from OpenOCD's `targets` output, not inferred from
    // a UI selection or a blind resume operation.
    const QRegularExpression core0(QStringLiteral("PVD_CORE0_STATE=([^\\s\\r\\n]+)"));
    const QRegularExpression core1(QStringLiteral("PVD_CORE1_STATE=([^\\s\\r\\n]+)"));
    const auto core0Match = core0.match(transferOutput_);
    const auto core1Match = core1.match(transferOutput_);
    const bool core0Known = core0Match.hasMatch();
    const bool core1Known = core1Match.hasMatch();
    const bool postStart = processOk && core0Known && core1Known &&
                           postProgramStartStatesValid(core0Match.captured(1), core1Match.captured(1));
    if (postStart)
    {
        setStatus("Transfer completed");
        log_->appendPlainText("PROGRAM PASS; VERIFY PASS; RESET RUN; POST_PROGRAM_START PASS; SHUTDOWN PASS");
    }
    else
    {
        setStatus("Transfer failed");
        log_->appendPlainText(QString("POST_PROGRAM_START FAIL: process=%1 core0=%2 core1=%3")
                                  .arg(processOk ? "PASS" : "FAIL")
                                  .arg(core0Known ? core0Match.captured(1) : "UNKNOWN")
                                  .arg(core1Known ? core1Match.captured(1) : "UNKNOWN"));
    }
    transferOutput_.clear();
    openOcdTransferActive_ = false;
}

void TransferColumn3::configure(const QString& artifact, const QString& method, const QString& generatedDirectory,
                                const QString& buildDirectory, const QString& expectedTarget,
                                bool latestBuildSuccessful)
{
    /**Applies the active project's transfer contract without starting a programming operation.

    The in-memory build-success flag is retained as UI context; durable authorization is
    still established by artifactIsCurrent through the build marker and timestamps.
    */
    artifact_ = artifact;
    method_ = method;
    generatedDirectory_ = generatedDirectory;
    buildDirectory_ = buildDirectory;
    expectedTarget_ = expectedTarget;
    Q_UNUSED(latestBuildSuccessful);

    if (method_ == "Copy UF2 to drive" && drive_->text().trimmed().isEmpty())
    {
        const QString detectedDrive = currentBootselDrive();
        if (!detectedDrive.isEmpty())
            drive_->setText(detectedDrive);
    }
    setStatus(artifact_.isEmpty() ? "Current firmware artifact not found" : "Ready");
}

bool TransferColumn3::artifactIsCurrent(QString* reason) const
{
    /**Validates that the selected firmware belongs to the active project and current sources.

    Build marker, target identity, directory ownership, and source timestamps are all
    checked before any transfer tool is allowed to touch hardware.
    */
    const QFileInfo artifactInfo(artifact_);
    if (artifact_.isEmpty() || !artifactInfo.exists())
    {
        if (reason)
            *reason = "the expected firmware artifact does not exist.";
        return false;
    }

    const QFileInfo buildMarker(QDir(buildDirectory_).filePath(".pvd_build_success"));
    if (!buildMarker.exists())
    {
        if (reason)
            *reason = "the active build directory has no successful build marker.";
        return false;
    }
    if (buildMarker.lastModified() < artifactInfo.lastModified())
    {
        if (reason)
            *reason = "the active build directory has no successful build marker newer than the firmware.";
        return false;
    }
    if (artifactInfo.completeBaseName() != expectedTarget_)
    {
        if (reason)
            *reason = QString("artifact '%1' does not match active target '%2'.")
                          .arg(artifactInfo.completeBaseName(), expectedTarget_);
        return false;
    }

    const QString artifactDir = QDir(artifactInfo.absolutePath()).canonicalPath();
    const QString expectedBuildDir = QDir(buildDirectory_).canonicalPath();
    if (artifactDir.isEmpty() || expectedBuildDir.isEmpty() || artifactDir != expectedBuildDir)
    {
        if (reason)
            *reason = "artifact is outside the active project's configured build directory.";
        return false;
    }

    QDirIterator generated(generatedDirectory_, QDir::Files, QDirIterator::Subdirectories);
    while (generated.hasNext())
    {
        const QFileInfo sourceInfo(generated.next());
        if (sourceInfo.lastModified() > artifactInfo.lastModified())
        {
            if (reason)
                *reason = QString("generated source '%1' is newer than the firmware; rebuild before transfer.")
                              .arg(sourceInfo.fileName());
            return false;
        }
    }

    if (method_ == "OpenOCD probe")
    {
        QString elf = artifact_;
        elf.replace(QRegularExpression("\\.uf2$", QRegularExpression::CaseInsensitiveOption), ".elf");
        const QFileInfo elfInfo(elf);
        if (!elfInfo.exists())
        {
            if (reason)
                *reason = "matching ELF is missing.";
            return false;
        }

        QDirIterator generatedForElf(generatedDirectory_, QDir::Files, QDirIterator::Subdirectories);
        while (generatedForElf.hasNext())
        {
            const QFileInfo sourceInfo(generatedForElf.next());
            if (sourceInfo.lastModified() > elfInfo.lastModified())
            {
                if (reason)
                    *reason = QString("generated source '%1' is newer than the ELF; rebuild before transfer.")
                                  .arg(sourceInfo.fileName());
                return false;
            }
        }
    }
    return true;
}

QString TransferColumn3::currentBootselDrive() const
{
    /**Finds a mounted Raspberry Pi BOOTSEL volume labelled RPI-RP2.

    The returned root path is platform-native and empty when no matching writable
    volume is currently mounted.
    */
    for (const QStorageInfo& volume : QStorageInfo::mountedVolumes())
    {
        if (!volume.isValid() || !volume.isReady() || volume.isReadOnly())
            continue;
        const QString name = volume.name().trimmed();
        const QString displayName = volume.displayName().trimmed();
        if (name.compare(QStringLiteral("RPI-RP2"), Qt::CaseInsensitive) == 0 ||
            displayName.compare(QStringLiteral("RPI-RP2"), Qt::CaseInsensitive) == 0)
            return QDir::toNativeSeparators(volume.rootPath());
    }
    return {};
}

void TransferColumn3::setStatus(const QString& text)
{
    /**Updates the stable transfer state exposed to users and GUI automation.

    Detailed process evidence remains in transfer_log while this control provides the
    lifecycle boundary needed for deterministic certification.
    */
    if (status_)
        status_->setText(text);
}

void TransferColumn3::startTransfer()
{
    /**Executes the selected transfer method only after all freshness checks pass.

    OpenOCD uses CMSIS-DAP and the RP2350 target, picotool uses forced loading, and
    BOOTSEL copying resolves the RPI-RP2 drive without a hard-coded drive letter.
    */
    if (process_->state() != QProcess::NotRunning)
    {
        setStatus("Transfer already running");
        log_->appendPlainText("Transfer request ignored because another transfer process is running.");
        return;
    }

    QString safetyReason;
    if (!artifactIsCurrent(&safetyReason))
    {
        setStatus("Transfer blocked");
        log_->appendPlainText("Transfer blocked: " + safetyReason);
        return;
    }

    if (method_ == "OpenOCD probe")
    {
        const QString openocd = findOpenOcd();
        const QString scripts = openOcdScriptsRoot(openocd);
        QString elf = artifact_;
        elf.replace(QRegularExpression("\\.uf2$", QRegularExpression::CaseInsensitiveOption), ".elf");
        elf.replace('\\', '/');
        if (!QFileInfo::exists(elf))
        {
            setStatus("Matching ELF not found");
            log_->appendPlainText("Matching ELF not found: " + elf);
            return;
        }

        // Keep OpenOCD alive after programming.  The explicit reset/run and
        // state queries are the post-program start contract; the old combined
        // combined programming/reset/exit command provided no such proof.
        const QString programCommand = QString("program \"%1\" verify").arg(elf);
        QStringList arguments;
        if (!scripts.isEmpty())
            arguments << "-s" << scripts;
        arguments << "-f" << "interface/cmsis-dap.cfg" << "-f" << "target/rp2350.cfg" << "-c"
                  << "adapter speed 5000" << "-c" << programCommand
                  << "-c" << "reset run"
                  << "-c" << "targets"
                  << "-c" << "echo PVD_CORE0_STATE=[rp2350.cm0 curstate]"
                  << "-c" << "echo PVD_CORE1_STATE=[rp2350.cm1 curstate]"
                  << "-c" << "shutdown";
        setStatus("Programming through OpenOCD");
        transferOutput_.clear();
        openOcdTransferActive_ = true;
        log_->appendPlainText("$ " + openocd + " " + arguments.join(' '));
        process_->start(openocd, arguments);
        return;
    }

    if (method_.startsWith("picotool"))
    {
        const QString picotool = findPicotool();
        const QStringList arguments = {"load", "-f", artifact_};
        setStatus("Programming through picotool");
        log_->appendPlainText("$ " + picotool + " " + arguments.join(' '));
        process_->start(picotool, arguments);
        return;
    }

    QString drive = drive_->text().trimmed();
    if (drive.isEmpty())
        drive = currentBootselDrive();
    if (drive.isEmpty())
    {
        setStatus("RPI-RP2 BOOTSEL drive not found");
        log_->appendPlainText("Copy failed: no RPI-RP2 BOOTSEL drive was detected.");
        return;
    }

    drive_->setText(drive);
    const QString target = QDir(drive).filePath(QFileInfo(artifact_).fileName());
    QFile::remove(target);
    if (QFile::copy(artifact_, target))
    {
        setStatus("Transfer completed");
        log_->appendPlainText("Copied to " + QDir::toNativeSeparators(target));
    }
    else
    {
        setStatus("UF2 copy failed");
        log_->appendPlainText("Copy failed. Check BOOTSEL mode, drive access, and free space.");
    }
}
} // namespace pvd
