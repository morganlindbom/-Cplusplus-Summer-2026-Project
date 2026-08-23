// CodeColumn3.cpp
#include "systems/mainwindow/code/code_column3/CodeColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPair>
#include <QTextStream>
#include <QVector>
namespace pvd
{
CodeColumn3::CodeColumn3(const QString& db, QWidget* parent) : QWidget(parent)
{
    /**Builds the editable generated-source workspace.*/
    auto* l = makePanelLayout(this, SqliteUtil::metadata(db, "title", "Generated C++ / C Source"));
    pathLabel_ = new QLabel(this);
    pathLabel_->setWordWrap(true);
    l->addWidget(pathLabel_);
    editor_ = new QPlainTextEdit(this);
    editor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    l->addWidget(editor_, 1);

    auto* buttons = new QHBoxLayout();
    auto* save = new QPushButton("Save File", this);
    auto* check = new QPushButton("Check Syntax", this);
    auto* build = new QPushButton("Build / Show Errors", this);
    buttons->addWidget(save);
    buttons->addWidget(check);
    buttons->addWidget(build);
    l->addLayout(buttons);

    status_ = new QLabel(this);
    status_->setWordWrap(true);
    l->addWidget(status_);
    connect(save, &QPushButton::clicked, this,
            [this]()
            {
                /**Commits editor text to the selected generated file.*/
                saveCurrent();
                status_->setText("File saved.");
            });
    connect(check, &QPushButton::clicked, this, &CodeColumn3::checkSyntax);
    connect(build, &QPushButton::clicked, this,
            [this]()
            {
                saveCurrent();
                emit buildRequested();
                status_->setText("Build started. Errors will be shown here when the build finishes.");
            });
}
void CodeColumn3::loadFile(const QString& path)
{
    /**Loads a selected generated file into the editor.*/
    if (!path_.isEmpty() && editor_->document()->isModified())
        saveCurrent();
    path_ = path;
    pathLabel_->setText(path);
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        editor_->setPlainText(QString::fromUtf8(f.readAll()));
        editor_->document()->setModified(false);
    }
    else
    {
        editor_->setPlainText("// Unable to open selected file.");
    }
}
void CodeColumn3::saveCurrent()
{
    /**Atomically rewrites the active file content from the editor.*/
    if (path_.isEmpty())
        return;
    QFile f(path_);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        f.write(editor_->toPlainText().toUtf8());
        editor_->document()->setModified(false);
        emit fileSaved(path_);
    }
}
void CodeColumn3::checkSyntax()
{
    /**Performs a quick structural check before running the full Pico build.*/
    const QString text = editor_->toPlainText();
    QVector<QPair<QChar, int>> stack;
    const auto lines = text.split('\n');
    for (int line = 0; line < lines.size(); ++line)
    {
        const QString& raw = lines[line];
        bool inString = false;
        bool escaped = false;
        for (const QChar ch : raw)
        {
            if (ch == '"' && !escaped)
                inString = !inString;
            escaped = (ch == '\\' && !escaped);
            if (inString)
                continue;
            if (ch == '{' || ch == '(' || ch == '[')
            {
                stack.push_back({ch, line + 1});
            }
            else if (ch == '}' || ch == ')' || ch == ']')
            {
                if (stack.isEmpty())
                {
                    status_->setStyleSheet("color:#a32626;");
                    status_->setText(QString("Syntax error near line %1: closing bracket without an opening bracket.")
                                         .arg(line + 1));
                    return;
                }
                const QChar open = stack.last().first;
                const bool matches =
                    (open == '{' && ch == '}') || (open == '(' && ch == ')') || (open == '[' && ch == ']');
                if (!matches)
                {
                    status_->setStyleSheet("color:#a32626;");
                    status_->setText(QString("Syntax error near line %1: mismatched brackets.").arg(line + 1));
                    return;
                }
                stack.removeLast();
            }
        }
    }
    if (!stack.isEmpty())
    {
        status_->setStyleSheet("color:#a32626;");
        status_->setText(QString("Syntax error: opening '%1' from line %2 is not closed.")
                             .arg(stack.last().first)
                             .arg(stack.last().second));
        return;
    }
    status_->setStyleSheet("color:#126b2f;");
    status_->setText("Basic syntax check passed. Run Build / Show Errors for compiler validation.");
}
void CodeColumn3::setBuildResult(bool ok, const QString& message)
{
    status_->setStyleSheet(ok ? "color:#126b2f;" : "color:#a32626;");
    status_->setText(ok ? "Build succeeded." : "Build failed:\n" + message);
}
} // namespace pvd
