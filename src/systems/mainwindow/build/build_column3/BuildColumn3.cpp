// BuildColumn3.cpp
#include "systems/mainwindow/build/build_column3/BuildColumn3.hpp"

#include "systems/database/SqliteUtil.hpp"
#include "systems/mainwindow/PanelUtil.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QStandardPaths>

namespace
{
QString findExisting(const QStringList& candidates)
{
    /**Returns the first existing path from a prioritized list.

    Empty environment values are ignored so bundled and Pico SDK installation
    locations can be checked without special-case branches.
    */
    for (const QString& path : candidates)
    {
        if (!path.trimmed().isEmpty() && QFileInfo::exists(path))
            return QFileInfo(path).absoluteFilePath();
    }
    return {};
}

QString findExecutable(const QStringList& candidates)
{
    /**Resolves the first executable from explicit paths or PATH-visible names.

    Build tools are permitted to live in the private PVD runtime, the Pico SDK
    installer, or the user's PATH while preserving deterministic precedence.
    */
    for (const QString& candidate : candidates)
    {
        const QString trimmed = candidate.trimmed();
        if (trimmed.isEmpty())
            continue;
        const QFileInfo info(trimmed);
        if (info.isFile())
            return info.absoluteFilePath();
        const QString resolved = QStandardPaths::findExecutable(trimmed);
        if (!resolved.isEmpty())
            return resolved;
    }
    return {};
}

QString normalizeToolchainRoot(const QString& configuredPath)
{
    /**Normalizes a Pico GCC toolchain location to the SDK's required install root.

    Older project scripts stored the bin directory in PICO_TOOLCHAIN_PATH, while
    Pico SDK 2.3 documents this variable as the toolchain installation root.
    */
    const QString trimmed = configuredPath.trimmed();
    if (trimmed.isEmpty())
        return {};

    QDir directory(trimmed);
    if (!directory.exists())
        return {};
    if (directory.dirName().compare(QStringLiteral("bin"), Qt::CaseInsensitive) == 0)
    {
        directory.cdUp();
        return directory.absolutePath();
    }
    return directory.absolutePath();
}

void prependExecutableDirectory(QProcessEnvironment* environment, const QString& executable)
{
    /**Prepends one executable directory to PATH without discarding the host environment.

    CMake may launch Ninja, the compiler, pioasm, and picotool as child processes,
    so private runtime executables must remain discoverable beyond the first process.
    */
    if (!environment || executable.isEmpty())
        return;
    const QString directory = QFileInfo(executable).absolutePath();
    if (directory.isEmpty())
        return;
    const QChar separator = QDir::listSeparator();
    const QString existing = environment->value(QStringLiteral("PATH"));
    environment->insert(QStringLiteral("PATH"), directory + separator + existing);
}
} // namespace

namespace pvd
{
BuildColumn3::BuildColumn3(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds the Pico SDK configure/build panel and captures complete process evidence.

    Toolchain selection is delegated to Pico SDK 2.3 through PICO_TOOLCHAIN_PATH
    instead of pinning compiler executables into the CMake cache.
    */
    auto* layout = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Build"));
    auto* row = new QHBoxLayout();
    auto* configure = new QPushButton("Configure", this);
    auto* buildButton = new QPushButton("Build", this);
    configure->setObjectName("configure_project");
    buildButton->setObjectName("build_project");
    row->addWidget(configure);
    row->addWidget(buildButton);
    layout->addLayout(row);

    status_ = new QLabel("Idle", this);
    status_->setObjectName("build_status");
    // QLabel exposes its visible text as the UIA Name.  Keep the stable
    // objectName for discovery and expose the semantic control description
    // separately, so automation reads the current lifecycle state rather than
    // a constant accessible name.
    status_->setAccessibleDescription("Build status");
    status_->setWordWrap(true);
    status_->setToolTip("Build status");
    layout->addWidget(status_);

    log_ = new QPlainTextEdit(this);
    log_->setObjectName("build_log");
    log_->setReadOnly(true);
    layout->addWidget(log_, 1);

    process_ = new QProcess(this);
    connect(process_, &QProcess::readyReadStandardOutput, this,
            [this]()
            {
                /**Appends normal CMake and Ninja output to the build evidence log.

                Output is intentionally preserved verbatim because generated-command
                evidence is useful when diagnosing Pico SDK configuration failures.
                */
                log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardOutput()));
            });
    connect(process_, &QProcess::readyReadStandardError, this,
            [this]()
            {
                /**Appends compiler and build-system diagnostics to the same evidence log.

                A unified transcript keeps configure, compile, link, and toolchain
                failures available to GUI automation and the user.
                */
                log_->appendPlainText(QString::fromLocal8Bit(process_->readAllStandardError()));
            });
    connect(process_, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error)
            {
                /**Reports process-launch failures with the operating-system error text.

                A failed build launch also completes the build contract immediately so
                GUI automation and Transfer never wait for a result that cannot arrive.
                */
                const QString message = "Build process error: " + process_->errorString();
                log_->appendPlainText(message);
                if (currentProcessIsBuild_)
                    setWorkflowState(BuildWorkflowState::BuildFailed);
                else
                    setWorkflowState(BuildWorkflowState::ConfigureFailed);
                if (currentProcessIsBuild_ && error == QProcess::FailedToStart && !buildCompletionReported_)
                {
                    buildCompletionReported_ = true;
                    emit buildCompleted(false, log_->toPlainText());
                }
            });
    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status)
            {
                /**Records build success and creates the freshness marker used by Transfer.

                Configure completion is logged but only an actual build updates the
                transfer-safety marker and emits buildCompleted.
                */
                if (!currentProcessIsBuild_)
                {
                    const bool ok = code == 0 && status == QProcess::NormalExit;
                    log_->appendPlainText(ok ? "Configure completed." : "Configure failed.");
                    setWorkflowState(ok ? BuildWorkflowState::ConfigureCompleted
                                        : BuildWorkflowState::ConfigureFailed);
                    return;
                }

                const bool ok = status == QProcess::NormalExit && code == 0;
                if (ok)
                {
                    QFile marker(QDir(build_).filePath(".pvd_build_success"));
                    if (marker.open(QIODevice::WriteOnly | QIODevice::Text))
                    {
                        marker.write(QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toUtf8());
                        marker.close();
                    }
                }
                setWorkflowState(ok ? BuildWorkflowState::BuildCompleted : BuildWorkflowState::BuildFailed);
                if (!buildCompletionReported_)
                {
                    buildCompletionReported_ = true;
                    emit buildCompleted(ok, log_->toPlainText());
                }
            });

    connect(configure, &QPushButton::clicked, this, &BuildColumn3::configureProject);
    connect(buildButton, &QPushButton::clicked, this, &BuildColumn3::build);
}

void BuildColumn3::setPaths(const QString& source, const QString& build)
{
    /**Sets the generated source tree and its isolated firmware build directory.

    Separating these paths from the desktop application's own build directory avoids
    accidental reuse of host-tool CMake cache state.
    */
    source_ = source;
    build_ = build;
}

void BuildColumn3::setBuildOptions(bool debugBuild, bool deoptimizedDebug, bool verboseEvidence)
{
    /**Applies project-state build policy to subsequent configure and build commands.

    Debug builds retain symbols, PICO_DEOPTIMIZED_DEBUG can disable optimization for
    reliable source stepping, and verbose evidence exposes exact Ninja commands.
    */
    debugBuild_ = debugBuild;
    deoptimizedDebug_ = debugBuild && deoptimizedDebug;
    verboseEvidence_ = verboseEvidence;
}

void BuildColumn3::setWorkflowState(BuildWorkflowState state)
{
    if (!status_)
        return;

    switch (state)
    {
    case BuildWorkflowState::Idle:
        status_->setText("Idle");
        break;
    case BuildWorkflowState::ConfigureRunning:
        status_->setText("Configure running…");
        break;
    case BuildWorkflowState::ConfigureCompleted:
        status_->setText("Configure completed");
        break;
    case BuildWorkflowState::ConfigureFailed:
        status_->setText("Configure failed");
        break;
    case BuildWorkflowState::BuildRunning:
        status_->setText("Build running…");
        break;
    case BuildWorkflowState::BuildCompleted:
        status_->setText("Build completed");
        break;
    case BuildWorkflowState::BuildFailed:
        status_->setText("Build failed");
        break;
    }
}

void BuildColumn3::build()
{
    /**Builds the already-configured Pico firmware with optional verbose command evidence.

    The existing freshness marker is removed before launch so a failed or interrupted
    build can never be mistaken for the latest successful artifact.
    */
    QStringList args = {"--build", build_, "--parallel", "4"};
    if (verboseEvidence_)
        args << "--verbose";
    run("cmake", args, true);
}

void BuildColumn3::configureProject()
{
    /**Creates a clean Pico SDK CMake configuration for the selected project state.

    Compiler auto-detection is left to Pico SDK 2.3 using PICO_TOOLCHAIN_PATH; only
    board, build type, and explicit debug-optimization policy are persisted in cache.
    */
    if (!build_.isEmpty())
    {
        QFile::remove(QDir(build_).filePath("CMakeCache.txt"));
        QDir(QDir(build_).filePath("CMakeFiles")).removeRecursively();
    }

    const QString buildType = debugBuild_ ? QStringLiteral("Debug") : QStringLiteral("Release");
    run("cmake",
        {"-S", source_, "-B", build_, "-G", "Ninja", "-DPICO_BOARD=pico2_w", "-DCMAKE_BUILD_TYPE=" + buildType,
         QString("-DPICO_DEOPTIMIZED_DEBUG=%1").arg(deoptimizedDebug_ ? 1 : 0)},
        false);
}

void BuildColumn3::run(const QString& program, const QStringList& args, bool isBuild)
{
    /**Launches one CMake operation with the current Pico SDK/toolchain environment.

    Environment-first discovery allows explicit user configuration while retaining
    the current project fallback paths for SDK 2.3.0 and the GCC 15.2 toolchain.
    */
    if (process_->state() != QProcess::NotRunning)
    {
        log_->appendPlainText("A build-system process is already running; request ignored.");
        setWorkflowState(isBuild ? BuildWorkflowState::BuildFailed : BuildWorkflowState::ConfigureFailed);
        return;
    }

    if (source_.isEmpty() || build_.isEmpty())
    {
        log_->appendPlainText("Build paths are not configured. Open or create a project first.");
        setWorkflowState(isBuild ? BuildWorkflowState::BuildFailed : BuildWorkflowState::ConfigureFailed);
        if (isBuild)
        {
            buildCompletionReported_ = true;
            emit buildCompleted(false, log_->toPlainText());
        }
        return;
    }

    currentProcessIsBuild_ = isBuild;
    buildCompletionReported_ = false;
    setWorkflowState(isBuild ? BuildWorkflowState::BuildRunning : BuildWorkflowState::ConfigureRunning);
    if (isBuild)
    {
        QFile::remove(QDir(build_).filePath(".pvd_build_success"));
        emit buildStarted();
    }

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString runtimeRoot = QStringLiteral(PVD_RUNTIME_ROOT);
    const QString sdk = findExisting({environment.value("PICO_SDK_PATH"), QDir(runtimeRoot).filePath("pico-sdk"),
                                      QDir::home().filePath(".pico-sdk/sdk/2.3.0")});
    const QString configuredToolchain = !environment.value("PICO_TOOLCHAIN_PATH").trimmed().isEmpty()
                                            ? environment.value("PICO_TOOLCHAIN_PATH")
                                            : findExisting({QDir(runtimeRoot).filePath("arm-toolchain"),
                                                            QDir::home().filePath(".pico-sdk/toolchain/15_2_Rel1")});
    const QString toolchain = normalizeToolchainRoot(configuredToolchain);
    const QString cmake =
        findExecutable({environment.value("PVD_CMAKE_PATH"), QDir(runtimeRoot).filePath("cmake/bin/cmake.exe"),
                        QDir(runtimeRoot).filePath("cmake/bin/cmake"), program});
    const QString ninja = findExecutable({environment.value("CMAKE_MAKE_PROGRAM"), environment.value("NINJA_PATH"),
                                          QDir(runtimeRoot).filePath("ninja/ninja.exe"),
                                          QDir(runtimeRoot).filePath("ninja/ninja"), QStringLiteral("ninja")});

    if (!sdk.isEmpty())
        environment.insert("PICO_SDK_PATH", sdk);
    if (!toolchain.isEmpty())
        environment.insert("PICO_TOOLCHAIN_PATH", toolchain);
    prependExecutableDirectory(&environment, cmake);
    prependExecutableDirectory(&environment, ninja);
    if (!toolchain.isEmpty())
        prependExecutableDirectory(&environment, QDir(toolchain).filePath("bin/arm-none-eabi-gcc.exe"));

    QStringList effectiveArgs = args;
    if (!isBuild && !ninja.isEmpty())
        effectiveArgs << "-DCMAKE_MAKE_PROGRAM=" + QDir::toNativeSeparators(ninja);

    process_->setProcessEnvironment(environment);
    const QString effectiveProgram = cmake.isEmpty() ? program : cmake;
    log_->appendPlainText("$ " + effectiveProgram + " " + effectiveArgs.join(' '));
    log_->appendPlainText("PICO_SDK_PATH=" + environment.value("PICO_SDK_PATH"));
    log_->appendPlainText("PICO_TOOLCHAIN_PATH=" + environment.value("PICO_TOOLCHAIN_PATH"));
    log_->appendPlainText("Ninja=" + (ninja.isEmpty() ? QStringLiteral("PATH/default") : ninja));
    log_->appendPlainText(QString("Build mode=%1, deoptimized debug=%2, verbose evidence=%3")
                              .arg(debugBuild_ ? "Debug" : "Release")
                              .arg(deoptimizedDebug_ ? "ON" : "OFF")
                              .arg(verboseEvidence_ ? "ON" : "OFF"));
    process_->start(effectiveProgram, effectiveArgs);
}
} // namespace pvd
