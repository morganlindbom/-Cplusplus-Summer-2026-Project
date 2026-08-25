// ProjectStore.cpp
#include "systems/project/ProjectStore.hpp"
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QUuid>

namespace pvd
{
namespace
{
QString databaseFileName(const QString& projectName)
{
    QString name = projectName.trimmed();
    name.replace(QRegularExpression("[^A-Za-z0-9_ -]"), "_");
    name = name.trimmed();
    return (name.isEmpty() ? QStringLiteral("project") : name) + QStringLiteral(".sqlite");
}

bool fail(QString* error, const QString& message)
{
    if (error)
        *error = message;
    return false;
}

bool checkQuery(const QSqlQuery& query, QString* error, const QString& operation)
{
    if (query.lastError().isValid())
        return fail(error, operation + QStringLiteral(": ") + query.lastError().text());
    return true;
}

bool validateState(const ApplicationState& state, QString* error)
{
    if (state.projectName.trimmed().isEmpty())
        return fail(error, QStringLiteral("Project name is empty."));
    if (state.product.trimmed().isEmpty() || state.language.trimmed().isEmpty() || state.state.trimmed().isEmpty())
        return fail(error, QStringLiteral("Project metadata is incomplete."));
    for (auto it = state.selections.cbegin(); it != state.selections.cend(); ++it)
    {
        const FunctionSelection& selection = it.value();
        if (selection.componentId.isEmpty() || selection.componentId != it.key() || selection.functionId.isEmpty())
            return fail(error, QStringLiteral("Project contains an invalid function selection."));
        if (selection.gpio < -1 || selection.gpio > 47 || selection.physicalPin < 0)
            return fail(error, QStringLiteral("Project contains an invalid pin mapping."));
    }
    return true;
}

void canonicalizeSelection(FunctionSelection& selection)
{
    /**Migrates legacy GPIO convenience IDs to the canonical SIO representation.*/
    if (selection.functionId == QStringLiteral("gpio.input"))
    {
        selection.functionId = QStringLiteral("sio");
        selection.functionName = QStringLiteral("SIO");
        selection.settings.insert(QStringLiteral("direction"), QStringLiteral("Input"));
    }
    else if (selection.functionId == QStringLiteral("gpio.output"))
    {
        selection.functionId = QStringLiteral("sio");
        selection.functionName = QStringLiteral("SIO");
        selection.settings.insert(QStringLiteral("direction"), QStringLiteral("Output"));
    }
    else if (selection.functionId == QStringLiteral("sio"))
    {
        // SIO Input is the canonical default. Persist it per selected
        // component so an untouched second pin is not lost during Save.
        selection.settings.insert(QStringLiteral("direction"),
                                  selection.settings.value(QStringLiteral("direction"), QStringLiteral("Input")));
        selection.settings.insert(QStringLiteral("pull"),
                                  selection.settings.value(QStringLiteral("pull"), QStringLiteral("None")));
    }
}

QStringList derivedGeneratedFiles(const QString& projectPath)
{
    const QDir generated(QDir(projectPath).filePath(QStringLiteral("generated")));
    if (!generated.exists())
        return {};
    QStringList files;
    for (const QString& file : generated.entryList(QDir::Files, QDir::Name))
        files << generated.absoluteFilePath(file);
    return files;
}
} // namespace

bool ProjectStore::save(const ApplicationState& state, QString* error)
{
    /**Persists authoritative project data in one SQLite transaction.*/
    if (state.projectPath.isEmpty())
        return fail(error, QStringLiteral("Project path is empty."));
    ApplicationState canonicalState = state;
    for (auto it = canonicalState.selections.begin(); it != canonicalState.selections.end(); ++it)
        canonicalizeSelection(it.value());
    if (!validateState(canonicalState, error))
        return false;
    if (!QDir().mkpath(state.projectPath))
        return fail(error, QStringLiteral("Cannot create project directory."));

    const QString path = QDir(state.projectPath).filePath(databaseFileName(state.projectName));
    const QString connection = QStringLiteral("project_save_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    db.setDatabaseName(path);
    bool ok = db.open();
    if (!ok)
        fail(error, db.lastError().text());
    if (ok && !db.transaction())
        ok = fail(error, db.lastError().text());

    auto exec = [&](const QString& sql, const QString& operation)
    {
        if (!ok)
            return;
        QSqlQuery query(db);
        if (!query.exec(sql))
            ok = fail(error, operation + QStringLiteral(": ") + query.lastError().text());
    };
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS project(key TEXT PRIMARY KEY,value TEXT)"),
         QStringLiteral("Create project table"));
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS selections(component_id TEXT PRIMARY KEY,display_name "
                        "TEXT,physical_pin INTEGER,gpio INTEGER,function_id TEXT,function_name TEXT)"),
         QStringLiteral("Create selections table"));
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS settings(component_id TEXT,setting_key TEXT,value TEXT,PRIMARY "
                        "KEY(component_id,setting_key))"),
         QStringLiteral("Create settings table"));

    auto putMetadata = [&](const QString& key, const QString& value)
    {
        if (!ok)
            return;
        QSqlQuery query(db);
        if (!query.prepare(QStringLiteral("INSERT OR REPLACE INTO project VALUES(?,?)")))
            ok = fail(error, QStringLiteral("Prepare project metadata: ") + query.lastError().text());
        else
        {
            query.addBindValue(key);
            query.addBindValue(value);
            if (!query.exec())
                ok = fail(error, QStringLiteral("Save project metadata: ") + query.lastError().text());
        }
    };
    putMetadata(QStringLiteral("project_name"), state.projectName);
    putMetadata(QStringLiteral("product"), state.product);
    putMetadata(QStringLiteral("language"), state.language);
    putMetadata(QStringLiteral("state"), state.state);
    putMetadata(QStringLiteral("debug_session_tools"),
                state.debugSessionTools ? QStringLiteral("true") : QStringLiteral("false"));
    putMetadata(QStringLiteral("runtime_diagnostics"),
                state.runtimeDiagnostics ? QStringLiteral("true") : QStringLiteral("false"));
    putMetadata(QStringLiteral("verbose_build_evidence"),
                state.verboseBuildEvidence ? QStringLiteral("true") : QStringLiteral("false"));
    exec(QStringLiteral("DELETE FROM selections"), QStringLiteral("Clear selections"));
    exec(QStringLiteral("DELETE FROM settings"), QStringLiteral("Clear settings"));

    for (auto it = canonicalState.selections.cbegin(); ok && it != canonicalState.selections.cend(); ++it)
    {
        const FunctionSelection& selection = it.value();
        QSqlQuery query(db);
        if (!query.prepare(QStringLiteral("INSERT INTO selections VALUES(?,?,?,?,?,?)")))
            ok = fail(error, QStringLiteral("Prepare selection: ") + query.lastError().text());
        else
        {
            query.addBindValue(selection.componentId);
            query.addBindValue(selection.displayName);
            query.addBindValue(selection.physicalPin);
            query.addBindValue(selection.gpio);
            query.addBindValue(selection.functionId);
            query.addBindValue(selection.functionName);
            if (!query.exec())
                ok = fail(error, QStringLiteral("Save selection: ") + query.lastError().text());
        }
        for (auto setting = selection.settings.cbegin(); ok && setting != selection.settings.cend(); ++setting)
        {
            QSqlQuery settingQuery(db);
            if (!settingQuery.prepare(QStringLiteral("INSERT INTO settings VALUES(?,?,?)")))
                ok = fail(error, QStringLiteral("Prepare setting: ") + settingQuery.lastError().text());
            else
            {
                settingQuery.addBindValue(selection.componentId);
                settingQuery.addBindValue(setting.key());
                settingQuery.addBindValue(setting.value());
                if (!settingQuery.exec())
                    ok = fail(error, QStringLiteral("Save setting: ") + settingQuery.lastError().text());
            }
        }
    }
    if (ok && !db.commit())
        ok = fail(error, QStringLiteral("Commit project: ") + db.lastError().text());
    if (!ok && db.isOpen())
        db.rollback();
    db.close();
    QSqlDatabase::removeDatabase(connection);
    return ok;
}

bool ProjectStore::load(const QString& directory, ApplicationState* state, QString* error)
{
    /**Loads into a validated candidate and replaces live state only after success.*/
    if (!state)
        return fail(error, QStringLiteral("Destination project state is null."));
    QString path = directory;
    if (QFileInfo(path).isDir())
    {
        const QStringList files = QDir(path).entryList({QStringLiteral("*.sqlite")}, QDir::Files, QDir::Name);
        if (files.isEmpty())
            return fail(error, QStringLiteral("No project database found in the selected folder."));
        path = QDir(path).filePath(files.first());
    }
    if (!QFileInfo::exists(path))
        return fail(error, QStringLiteral("Project database does not exist."));

    const QString connection = QStringLiteral("project_load_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
    db.setDatabaseName(path);
    if (!db.open())
    {
        const QString message = db.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connection);
        return fail(error, message);
    }
    ApplicationState candidate;
    candidate.projectPath = QFileInfo(path).absolutePath();
    bool ok = true;
    QSqlQuery metadata(db);
    if (!metadata.exec(QStringLiteral("SELECT key,value FROM project")))
        ok = fail(error, QStringLiteral("Load project metadata: ") + metadata.lastError().text());
    QSet<QString> metadataKeys;
    while (ok && metadata.next())
    {
        const QString key = metadata.value(0).toString();
        const QString value = metadata.value(1).toString();
        metadataKeys.insert(key);
        if (key == QStringLiteral("project_name"))
            candidate.projectName = value;
        else if (key == QStringLiteral("product"))
            candidate.product = value;
        else if (key == QStringLiteral("language"))
            candidate.language = value;
        else if (key == QStringLiteral("state"))
            candidate.state = value;
        else if (key == QStringLiteral("debug_session_tools"))
            candidate.debugSessionTools = value == QStringLiteral("true");
        else if (key == QStringLiteral("runtime_diagnostics"))
            candidate.runtimeDiagnostics = value == QStringLiteral("true");
        else if (key == QStringLiteral("verbose_build_evidence"))
            candidate.verboseBuildEvidence = value == QStringLiteral("true");
    }
    if (ok && !checkQuery(metadata, error, QStringLiteral("Read project metadata")))
        ok = false;
    for (const QString& required : {QStringLiteral("project_name"), QStringLiteral("product"),
                                    QStringLiteral("language"), QStringLiteral("state")})
        if (ok && !metadataKeys.contains(required))
            ok = fail(error, QStringLiteral("Project metadata is missing: ") + required);

    QSqlQuery selections(db);
    if (ok && !selections.exec(QStringLiteral(
                  "SELECT component_id,display_name,physical_pin,gpio,function_id,function_name FROM selections")))
        ok = fail(error, QStringLiteral("Load selections: ") + selections.lastError().text());
    while (ok && selections.next())
    {
        FunctionSelection selection;
        selection.componentId = selections.value(0).toString();
        selection.displayName = selections.value(1).toString();
        selection.physicalPin = selections.value(2).toInt();
        selection.gpio = selections.value(3).toInt();
        selection.functionId = selections.value(4).toString();
        selection.functionName = selections.value(5).toString();
        candidate.selections.insert(selection.componentId, selection);
    }
    if (ok && !checkQuery(selections, error, QStringLiteral("Read selections")))
        ok = false;

    QSqlQuery settings(db);
    if (ok && !settings.exec(QStringLiteral("SELECT component_id,setting_key,value FROM settings")))
        ok = fail(error, QStringLiteral("Load settings: ") + settings.lastError().text());
    while (ok && settings.next())
    {
        const QString componentId = settings.value(0).toString();
        const QString settingKey = settings.value(1).toString();
        if (componentId.isEmpty() || settingKey.isEmpty())
            ok = fail(error, QStringLiteral("Project contains an invalid setting record."));
        else if (!candidate.selections.contains(componentId))
            ok = fail(error, QStringLiteral("Setting belongs to an unknown selection: ") + componentId);
        else
            candidate.selections[componentId].settings.insert(settingKey, settings.value(2).toString());
    }
    if (ok && !checkQuery(settings, error, QStringLiteral("Read settings")))
        ok = false;
    for (auto it = candidate.selections.begin(); ok && it != candidate.selections.end(); ++it)
        canonicalizeSelection(it.value());
    candidate.generatedFiles = derivedGeneratedFiles(candidate.projectPath);
    if (ok)
        ok = validateState(candidate, error);
    db.close();
    QSqlDatabase::removeDatabase(connection);
    if (!ok)
        return false;
    *state = std::move(candidate);
    return true;
}
} // namespace pvd
