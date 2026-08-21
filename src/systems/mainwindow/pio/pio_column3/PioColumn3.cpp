// PioColumn3.cpp
#include "systems/mainwindow/pio/pio_column3/PioColumn3.hpp"
#include "systems/mainwindow/PanelUtil.hpp"
#include "systems/database/SqliteUtil.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>
namespace pvd {
PioColumn3::PioColumn3(const QString& db,QWidget* parent):QWidget(parent)
{
    /**Builds the PIO source editor and validation controls.*/
    auto* l=makePanelLayout(this,SqliteUtil::metadata(db,"title","PIO Source"));editor_=new QPlainTextEdit(this);editor_->setLineWrapMode(QPlainTextEdit::NoWrap);l->addWidget(editor_,1);auto* row=new QHBoxLayout();auto* generate=new QPushButton("Generate PIO Code",this);auto* validateButton=new QPushButton("Validate PIO Code",this);auto* restore=new QPushButton("Restore Generated PIO Code",this);row->addWidget(generate);row->addWidget(validateButton);row->addWidget(restore);l->addLayout(row);status_=new QLabel(this);l->addWidget(status_);connect(editor_,&QPlainTextEdit::textChanged,this,[this](){/**Stores current PIO source in memory.*/if(!current_.isEmpty()){programs_[current_]=editor_->toPlainText();emit programsChanged();}});connect(generate,&QPushButton::clicked,this,[this](){/**Replaces the current buffer with a generated skeleton.*/if(!current_.isEmpty())editor_->setPlainText(generatedTemplate(current_));});connect(restore,&QPushButton::clicked,this,[this](){/**Restores the deterministic generated PIO skeleton.*/if(!current_.isEmpty())editor_->setPlainText(generatedTemplate(current_));});connect(validateButton,&QPushButton::clicked,this,[this](){/**Runs lightweight structural PIO validation.*/QString e;status_->setText(validate(&e)?"PIO validation: PASS":"PIO validation: "+e);});
}
void PioColumn3::selectProgram(const QString& name)
{
    /**Switches the editor to one named PIO program.*/
    current_=name;if(!programs_.contains(name))programs_[name]=generatedTemplate(name);editor_->setPlainText(programs_[name]);
}
void PioColumn3::reloadGeneratedFiles(const QStringList& files)
{
    /**Reloads generated ASM/PIO files after Generate so the editor matches disk.*/
    QHash<QString,QString> generated;
    for(const auto& path:files){if(QFileInfo(path).suffix().toLower()!="pio")continue;QFile file(path);if(file.open(QIODevice::ReadOnly|QIODevice::Text))generated.insert(QFileInfo(path).completeBaseName(),QString::fromUtf8(file.readAll()));}
    if(generated.isEmpty())return;
    programs_=generated;
    if(!programs_.contains(current_))current_=programs_.constBegin().key();
    if(!current_.isEmpty()){const QSignalBlocker blocker(editor_);editor_->setPlainText(programs_.value(current_));editor_->document()->setModified(false);}
    emit programsChanged();
}
QHash<QString,QString> PioColumn3::programs()const
{
    /**Returns the current project PIO source map for generation.*/
    return programs_;
}
bool PioColumn3::validate(QString* error)const
{
    /**Checks core PIO source invariants without invoking pioasm.*/
    const QString t=editor_->toPlainText();if(!t.contains(".program")){if(error)*error="missing .program";return false;}if(!t.contains(".wrap_target")){if(error)*error="missing .wrap_target";return false;}if(!t.contains(".wrap")){if(error)*error="missing .wrap";return false;}return true;
}
QString PioColumn3::generatedTemplate(const QString& name)const
{
    /**Creates a minimal valid PIO program template.*/
    return QString(
        "; %1.pio\n"
        "; Beginner PIO template. Replace the nop instruction with your own\n"
        "; deterministic, cycle-by-cycle hardware behaviour.\n"
        "; .program gives the program its name for the C/C++ generated code.\n"
        ".program %1\n"
        "; .wrap_target marks the first instruction in the repeating loop.\n"
        ".wrap_target\n"
        "; nop means 'no operation'. It consumes one PIO clock cycle.\n"
        "    nop\n"
        "; .wrap sends execution back to .wrap_target.\n"
        ".wrap\n").arg(name);
}
}
