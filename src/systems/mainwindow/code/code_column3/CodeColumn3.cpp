// CodeColumn3.cpp
#include "systems/mainwindow/code/code_column3/CodeColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextStream>
namespace pvd {
CodeColumn3::CodeColumn3(const QString& db,QWidget* parent):QWidget(parent)
{
    /**Builds the editable generated-source workspace.*/
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","Generated C++ / C Source"));pathLabel_=new QLabel(this);pathLabel_->setWordWrap(true);l->addWidget(pathLabel_);editor_=new QPlainTextEdit(this);editor_->setLineWrapMode(QPlainTextEdit::NoWrap);l->addWidget(editor_,1);auto* save=new QPushButton("Save File",this);l->addWidget(save);connect(save,&QPushButton::clicked,this,[this](){/**Commits editor text to the selected generated file.*/saveCurrent();});
}
void CodeColumn3::loadFile(const QString& path)
{
    /**Loads a selected generated file into the editor.*/
    if(!path_.isEmpty()&&editor_->document()->isModified())saveCurrent();path_=path;pathLabel_->setText(path);QFile f(path);if(f.open(QIODevice::ReadOnly|QIODevice::Text)){editor_->setPlainText(QString::fromUtf8(f.readAll()));editor_->document()->setModified(false);}else editor_->setPlainText("// Unable to open selected file.");
}
void CodeColumn3::saveCurrent()
{
    /**Atomically rewrites the active file content from the editor.*/
    if(path_.isEmpty())return;QFile f(path_);if(f.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate)){f.write(editor_->toPlainText().toUtf8());editor_->document()->setModified(false);emit fileSaved(path_);}
}
}
