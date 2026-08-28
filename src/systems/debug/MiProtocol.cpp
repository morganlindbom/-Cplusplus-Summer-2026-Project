#include "systems/debug/MiProtocol.hpp"
#include <QStringList>

namespace pvd::mi
{
namespace
{
bool consumeCString(const QString& text, int* position)
{
    if (!position || *position >= text.size() || text.at(*position) != '"')
        return false;
    ++*position;
    while (*position < text.size())
    {
        const QChar c = text.at(*position);
        ++*position;
        if (c == '"')
            return true;
        if (c == '\\' && *position < text.size())
            ++*position;
    }
    return false;
}

bool consumeValue(const QString& text, int* position)
{
    if (!position || *position >= text.size())
        return false;
    if (text.at(*position) == '"')
        return consumeCString(text, position);
    const QChar open = text.at(*position);
    if (open != '{' && open != '[')
        return false;
    const QChar close = open == '{' ? '}' : ']';
    int depth = 0;
    while (*position < text.size())
    {
        const QChar c = text.at(*position);
        if (c == '"')
        {
            if (!consumeCString(text, position))
                return false;
            continue;
        }
        if (c == open)
            ++depth;
        else if (c == close && --depth == 0)
        {
            ++*position;
            return true;
        }
        ++*position;
    }
    return false;
}

bool validResultList(const QString& payload)
{
    if (payload.isEmpty())
        return true;
    int position = 0;
    while (position < payload.size())
    {
        const int equals = payload.indexOf('=', position);
        if (equals <= position)
            return false;
        for (int i = position; i < equals; ++i)
            if (!(payload.at(i).isLetterOrNumber() || payload.at(i) == '_' || payload.at(i) == '-'))
                return false;
        position = equals + 1;
        if (!consumeValue(payload, &position))
            return false;
        if (position == payload.size())
            return true;
        if (payload.at(position) != ',')
            return false;
        ++position;
        if (position == payload.size())
            return false;
    }
    return false;
}
} // namespace

MiRecord parseRecord(QString line)
{
    line.remove(QChar('\0'));
    while (line.endsWith('\r') || line.endsWith('\n'))
        line.chop(1);
    if (line.trimmed().isEmpty())
        return {MiRecord::Kind::Empty};
    line = line.trimmed();
    if (line == QStringLiteral("(gdb)"))
        return {MiRecord::Kind::Prompt};

    int offset = 0;
    while (offset < line.size() && line.at(offset) >= '0' && line.at(offset) <= '9')
        ++offset;
    const int token = offset == 0 ? -1 : line.left(offset).toInt();
    if (offset >= line.size())
        return {MiRecord::Kind::Malformed, token};
    const QChar marker = line.at(offset++);
    const QString body = line.mid(offset);
    if (marker == '~' || marker == '@' || marker == '&')
    {
        int position = 0;
        if (!consumeCString(body, &position) || position != body.size())
            return {MiRecord::Kind::Malformed, token};
        return {marker == '~' ? MiRecord::Kind::Console : marker == '@' ? MiRecord::Kind::Target : MiRecord::Kind::Log,
                token, {}, line};
    }
    if (marker == '*' || marker == '+' || marker == '=')
    {
        const int comma = body.indexOf(',');
        const QString asyncClass = comma < 0 ? body : body.left(comma);
        if (asyncClass.isEmpty() || (comma >= 0 && !validResultList(body.mid(comma + 1))))
            return {MiRecord::Kind::Malformed, token};
        return {marker == '*' ? MiRecord::Kind::ExecAsync : marker == '+' ? MiRecord::Kind::StatusAsync : MiRecord::Kind::NotifyAsync,
                token, {}, line};
    }
    if (marker == '^')
    {
        const int comma = body.indexOf(',');
        const QString resultClass = comma < 0 ? body : body.left(comma);
        const QStringList resultClasses = {QStringLiteral("done"), QStringLiteral("running"),
                                           QStringLiteral("connected"), QStringLiteral("error"), QStringLiteral("exit")};
        if (!resultClasses.contains(resultClass) || (comma >= 0 && !validResultList(body.mid(comma + 1))))
            return {MiRecord::Kind::Malformed, token};
        return {MiRecord::Kind::Result, token, resultClass, line};
    }
    return {MiRecord::Kind::Malformed, token};
}
} // namespace pvd::mi
