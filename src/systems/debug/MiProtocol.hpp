#pragma once

#include <QString>

namespace pvd::mi
{
struct MiRecord
{
    enum class Kind { Result, ExecAsync, StatusAsync, NotifyAsync, Console, Target, Log, Prompt, Empty, Malformed };
    Kind kind = Kind::Malformed;
    int token = -1;
    QString resultClass;
    QString text;
};

MiRecord parseRecord(QString line);
}
