// DebugColumn2.cpp
#include "systems/mainwindow/debug/debug_column2/DebugColumn2.hpp"

#include "systems/database/SqliteUtil.hpp"
#include "systems/mainwindow/PanelUtil.hpp"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace
{
QString executableFromLocation(const QString& location, const QString& executableName)
{
    /**Resolves an executable from either a file path, directory, or PATH-style name.

    Environment variables used by Pico tooling are not guaranteed to point to the
    executable itself, so both installation-directory and binary-directory layouts
    are accepted before falling back to QStandardPaths.
    */
    const QString trimmed = location.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QFileInfo locationInfo(trimmed);
    if (locationInfo.isFile())
        return locationInfo.absoluteFilePath();

    if (locationInfo.isDir())
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

QString firstExistingExecutable(const QStringList& locations, const QString& executableName)
{
    /**Returns the first executable that can be resolved from the supplied locations.

    This centralizes discovery so OpenOCD and GDB follow the same deterministic
    precedence: explicit environment configuration, PATH, modern Pico SDK layout,
    then legacy compatibility locations.
    */
    for (const QString& location : locations)
    {
        const QString resolved = executableFromLocation(location, executableName);
        if (!resolved.isEmpty())
            return resolved;
    }
    return {};
}

QString openOcdScriptsRoot(const QString& executable)
{
    /**Finds the OpenOCD scripts directory associated with the selected executable.

    Raspberry Pi and upstream OpenOCD distributions use slightly different layouts,
    therefore common sibling and share-directory locations are checked without
    hard-coding one packaging convention.
    */
    if (executable.isEmpty())
        return {};

    const QDir executableDir = QFileInfo(executable).absoluteDir();
    const QStringList candidates = {
        executableDir.filePath("scripts"),
        executableDir.filePath("../scripts"),
        executableDir.filePath("../share/openocd/scripts"),
        executableDir.filePath("../../share/openocd/scripts"),
    };

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
DebugColumn2::DebugColumn2(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds the debug-tool configuration panel and discovers Pico SDK tools.

    The UI remains editable so a user can override automatic discovery, while the
    defaults prefer the current Pico SDK environment and installation conventions.
    */
    auto* layout = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Debug Configuration"));
    auto* form = new QFormLayout();
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString runtimeRoot = QStringLiteral(PVD_RUNTIME_ROOT);
    const QString openocd = firstExistingExecutable(
        {environment.value("PICO_OPENOCD_PATH"),
         QDir(runtimeRoot).filePath("openocd"),
         QStandardPaths::findExecutable("openocd"),
         QDir::home().filePath(".pico-sdk/openocd/0.12.0+dev")},
        QStringLiteral("openocd.exe"));

    const QString gdb = firstExistingExecutable(
        {environment.value("PICO_TOOLCHAIN_PATH"),
         QDir(runtimeRoot).filePath("arm-toolchain"),
         QStandardPaths::findExecutable("arm-none-eabi-gdb"),
         QStandardPaths::findExecutable("arm-none-eabi-gdb.exe"),
         QDir::home().filePath(".pico-sdk/toolchain/15_2_Rel1")},
        QStringLiteral("arm-none-eabi-gdb.exe"));

    const QString scripts = openOcdScriptsRoot(openocd);
    const QString interfaceCfg = QDir(scripts).filePath("interface/cmsis-dap.cfg");
    const QString targetCfg = QDir(scripts).filePath("target/rp2350.cfg");

    openocd_ = new QLineEdit(openocd.isEmpty() ? QStringLiteral("openocd") : openocd, this);
    openocd_->setObjectName("debug_openocd");
    gdb_ = new QLineEdit(gdb.isEmpty() ? QStringLiteral("arm-none-eabi-gdb") : gdb, this);
    gdb_->setObjectName("debug_gdb");
    interface_ = new QLineEdit(QFileInfo::exists(interfaceCfg) ? interfaceCfg
                                                               : QStringLiteral("interface/cmsis-dap.cfg"),
                               this);
    interface_->setObjectName("debug_interface");
    target_ = new QLineEdit(QFileInfo::exists(targetCfg) ? targetCfg : QStringLiteral("target/rp2350.cfg"), this);
    target_->setObjectName("debug_target");

    speed_ = new QComboBox(this);
    speed_->setObjectName("debug_speed");
    speed_->setEditable(true);
    speed_->addItems({"100", "500", "1000", "2000", "5000", "10000", "25000", "50000", "Custom..."});
    speed_->setCurrentText("5000");
    connect(speed_, qOverload<int>(&QComboBox::activated), this,
            [this](int index)
            {
                /**Prepares the editable speed field when the Custom entry is selected.

                Clearing the text avoids accidentally treating the literal word
                "Custom..." as a numeric adapter speed.
                */
                if (index == speed_->count() - 1)
                    speed_->setEditText(QString{});
            });

    form->addRow("OpenOCD", openocd_);
    form->addRow("GDB", gdb_);
    form->addRow("Interface", interface_);
    form->addRow("Target", target_);
    form->addRow("Adapter kHz", speed_);
    layout->addLayout(form);
    layout->addStretch();
}

QString DebugColumn2::openocd() const
{
    /**Returns the configured OpenOCD executable.

    The value may be an absolute path resolved by the UI or a PATH-resolved command
    deliberately entered by the user.
    */
    return openocd_->text().trimmed();
}

QString DebugColumn2::gdb() const
{
    /**Returns the configured Arm GDB executable.

    GDB is kept independent from OpenOCD so the debugger client and server can be
    diagnosed separately when toolchain discovery fails.
    */
    return gdb_->text().trimmed();
}

QString DebugColumn2::interfaceCfg() const
{
    /**Returns the OpenOCD debug-probe configuration.

    Relative values remain valid because OpenOCD can resolve files from its scripts
    search path; automatically discovered installations normally provide an absolute path.
    */
    return interface_->text().trimmed();
}

QString DebugColumn2::targetCfg() const
{
    /**Returns the OpenOCD RP2350 target configuration.

    Keeping the target file user-editable supports alternate RP2350 configurations
    without coupling the UI to one OpenOCD package layout.
    */
    return target_->text().trimmed();
}

int DebugColumn2::speed() const
{
    /**Returns a validated SWD adapter speed in kHz.

    Invalid, empty, or non-positive custom values fall back to the established
    5000 kHz project default instead of launching OpenOCD with an invalid command.
    */
    bool ok = false;
    const int value = speed_->currentText().trimmed().toInt(&ok);
    return ok && value > 0 ? value : 5000;
}

QString DebugColumn2::resetMethod() const
{
    /**Returns the persisted reset policy used when a debugger attaches.

    The stored values are project-level semantic labels and are translated to
    concrete OpenOCD monitor commands by DebugColumn3.
    */
    return resetMethod_;
}

void DebugColumn2::applySetting(const QString& key, const QString& value)
{
    /**Applies persisted debug-probe settings to the live configuration panel.

    Existing adapter-speed and reset-method keys remain backwards compatible while
    optional tool-path keys are accepted for future project databases.
    */
    if (key == "adapter_speed")
        speed_->setCurrentText(value);
    else if (key == "reset_method" && (value == "run reset" || value == "halt reset" || value == "none"))
        resetMethod_ = value;
    else if (key == "openocd_path")
        openocd_->setText(value);
    else if (key == "gdb_path")
        gdb_->setText(value);
    else if (key == "interface_cfg")
        interface_->setText(value);
    else if (key == "target_cfg")
        target_->setText(value);
}
} // namespace pvd
