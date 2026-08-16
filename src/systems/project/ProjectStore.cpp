// ProjectStore.cpp
#include "systems/project/ProjectStore.hpp"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace pvd {
bool ProjectStore::save(const ApplicationState& state, QString* error)
{
    /**Persists project metadata, function selections and settings to project.sqlite.*/
    if(state.projectPath.isEmpty()){ if(error)*error="Project path is empty."; return false; }
    QDir().mkpath(state.projectPath); const QString path=QDir(state.projectPath).filePath("project.sqlite"); const QString conn="project_save_"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool ok=true;
    {
        QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE",conn); db.setDatabaseName(path); if(!db.open()){if(error)*error=db.lastError().text();ok=false;} else {
            auto fail=[&](const QSqlQuery& query){if(error)*error=query.lastError().text();ok=false;};
            QSqlQuery q(db); db.transaction();
            for(const auto& sql:{QStringLiteral("CREATE TABLE IF NOT EXISTS project(key TEXT PRIMARY KEY,value TEXT)"),QStringLiteral("CREATE TABLE IF NOT EXISTS selections(component_id TEXT PRIMARY KEY,display_name TEXT,physical_pin INTEGER,gpio INTEGER,function_id TEXT,function_name TEXT)"),QStringLiteral("CREATE TABLE IF NOT EXISTS settings(component_id TEXT,setting_key TEXT,value TEXT,PRIMARY KEY(component_id,setting_key))")})if(!q.exec(sql)){fail(q);break;}
            auto put=[&](const QString& k,const QString& v){QSqlQuery x(db);x.prepare("INSERT OR REPLACE INTO project VALUES(?,?)");x.addBindValue(k);x.addBindValue(v);if(!x.exec()){fail(x);return false;}return true;};
            if(ok&&(!put("project_name",state.projectName)||!put("language",state.language)||!put("state",state.state)))ok=false;
            if(ok&&!q.exec("DELETE FROM selections")){fail(q);} if(ok&&!q.exec("DELETE FROM settings")){fail(q);}
            for(const auto& s:state.selections){if(!ok)break;QSqlQuery x(db);x.prepare("INSERT INTO selections VALUES(?,?,?,?,?,?)");x.addBindValue(s.componentId);x.addBindValue(s.displayName);x.addBindValue(s.physicalPin);x.addBindValue(s.gpio);x.addBindValue(s.functionId);x.addBindValue(s.functionName);if(!x.exec()){fail(x);break;}for(auto it=s.settings.cbegin();it!=s.settings.cend();++it){QSqlQuery y(db);y.prepare("INSERT INTO settings VALUES(?,?,?)");y.addBindValue(s.componentId);y.addBindValue(it.key());y.addBindValue(it.value());if(!y.exec()){fail(y);break;}}}
            if(ok){if(!db.commit()){if(error)*error=db.lastError().text();ok=false;}}else db.rollback();db.close();
        }
    }
    QSqlDatabase::removeDatabase(conn); return ok;
}

bool ProjectStore::load(const QString& directory, ApplicationState* state, QString* error)
{
    /**Loads a previously saved project and reconstructs current selections.*/
    if(!state) return false; const QString path=QDir(directory).filePath("project.sqlite"); const QString conn="project_load_"+QUuid::createUuid().toString(QUuid::WithoutBraces); bool ok=true;
    {
        QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE",conn); db.setDatabaseName(path); if(!db.open()){if(error)*error=db.lastError().text();ok=false;} else {
            state->projectPath=directory; QSqlQuery q(db); q.exec("SELECT key,value FROM project"); while(q.next()){const auto k=q.value(0).toString(),v=q.value(1).toString(); if(k=="project_name")state->projectName=v; else if(k=="language")state->language=v; else if(k=="state")state->state=v;}
            state->selections.clear(); q.exec("SELECT component_id,display_name,physical_pin,gpio,function_id,function_name FROM selections"); while(q.next()){FunctionSelection s; s.componentId=q.value(0).toString();s.displayName=q.value(1).toString();s.physicalPin=q.value(2).toInt();s.gpio=q.value(3).toInt();s.functionId=q.value(4).toString();s.functionName=q.value(5).toString();state->selections.insert(s.componentId,s);} q.exec("SELECT component_id,setting_key,value FROM settings"); while(q.next()) if(state->selections.contains(q.value(0).toString())) state->selections[q.value(0).toString()].settings[q.value(1).toString()]=q.value(2).toString(); db.close();
        }
    }
    QSqlDatabase::removeDatabase(conn); return ok;
}
}
