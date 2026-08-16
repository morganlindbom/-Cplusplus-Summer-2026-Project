// SystemDatabase.cpp
#include "systems/SystemDatabase.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QDir>
#include <QFileInfo>
#include <utility>

namespace pvd {
SystemDatabase::SystemDatabase(QString path) : path_(std::move(path))
{
    /**Stores the governing System database path.*/
}

QString SystemDatabase::componentDatabase(const QString& componentId) const
{
    /**Resolves one registered component database through the System registry.*/
    const auto rows = SqliteUtil::rows(path_, "SELECT database_path FROM components WHERE component_id=?", {componentId});
    if (rows.isEmpty()) return {};
    return QDir(QFileInfo(path_).absolutePath()).filePath(rows.first().value("database_path").toString());
}

QStringList SystemDatabase::workflowOrder() const
{
    /**Returns workflow identifiers in the governing display order.*/
    QStringList result;
    for (const auto& row : SqliteUtil::rows(path_, "SELECT workflow_id FROM workflows ORDER BY sort_order"))
        result << row.value("workflow_id").toString();
    return result;
}
}
