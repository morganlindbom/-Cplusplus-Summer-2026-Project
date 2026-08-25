// SqliteUtil.hpp
#pragma once
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVector>

namespace pvd
{
    class SqliteUtil final
    {
    public:
        static QSqlDatabase open(const QString &path, const QString &connectionName);
        static QString metadata(const QString &path, const QString &key, const QString &fallback = {});
        static bool hasColumn(const QString &path, const QString &table, const QString &column);
        static QVector<QVariantMap> rows(const QString &path, const QString &sql, const QVariantList &bindings = {});
    };
}
