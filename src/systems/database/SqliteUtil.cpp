// SqliteUtil.cpp
#include "systems/database/SqliteUtil.hpp"
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <stdexcept>

namespace pvd {
QSqlDatabase SqliteUtil::open(const QString& path, const QString& connectionName)
{
    /**Opens one SQLite database using an explicit Qt connection name.*/
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        throw std::runtime_error(QString("Cannot open SQLite database %1: %2").arg(path, db.lastError().text()).toStdString());
    }
    return db;
}

QString SqliteUtil::metadata(const QString& path, const QString& key, const QString& fallback)
{
    /**Reads one metadata value from a component database.*/
    const QString connection = "meta_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString result = fallback;
    {
        QSqlDatabase db = open(path, connection);
        QSqlQuery q(db);
        q.prepare("SELECT value FROM metadata WHERE key=?");
        q.addBindValue(key);
        if (q.exec() && q.next()) result = q.value(0).toString();
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
    return result;
}

QVector<QVariantMap> SqliteUtil::rows(const QString& path, const QString& sql, const QVariantList& bindings)
{
    /**Executes a read query and returns rows as key/value maps.*/
    const QString connection = "rows_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVector<QVariantMap> result;
    {
        QSqlDatabase db = open(path, connection);
        QSqlQuery q(db);
        q.prepare(sql);
        for (const auto& value : bindings) q.addBindValue(value);
        if (!q.exec()) throw std::runtime_error(q.lastError().text().toStdString());
        const auto record = q.record();
        while (q.next()) {
            QVariantMap row;
            for (int i=0;i<record.count();++i) row.insert(record.fieldName(i), q.value(i));
            result.push_back(row);
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connection);
    return result;
}
}
