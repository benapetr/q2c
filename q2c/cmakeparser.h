//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef CMAKEPARSER_H
#define CMAKEPARSER_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include "buildmodel.h"

class CMakeCommand
{
    public:
        QString Name;
        QStringList Arguments;
        int Line;
};

class CMakeParser
{
    public:
        CMakeParser();
        bool Parse(QString text, BuildProject *model, QString source_file);

    private:
        QList<CMakeCommand> ParseCommands(QString text);
        QString StripComment(QString line);
        QStringList TokenizeArguments(QString text);
        QString ExpandVariables(QString text);
        QString NormalizeCondition(QStringList args) const;
        BuildTarget *FindOrCreateTarget(QString name, BuildTargetType type);
        BuildTarget *FindTarget(QString name);
        BuildTarget *PrimaryTarget();
        void ProcessCommand(const CMakeCommand &command);
        void ProcessTargetFiles(BuildTarget *target, QStringList args);
        void ProcessTargetList(BuildTarget *target, QList<QString> *list, QStringList args);
        void AddUnique(QList<QString> *list, QString value);
        void AddWarning(QString warning);
        bool IsVisibilityKeyword(QString value) const;
        bool IsQtImportedTarget(QString value) const;
        QString QtModuleFromImportedTarget(QString value) const;

        BuildProject *Model;
        QString SourceFile;
        QHash<QString, QStringList> Variables;
        QList<QString> ConditionStack;
};

#endif // CMAKEPARSER_H
