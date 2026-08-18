//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef QMAKEPARSER_H
#define QMAKEPARSER_H

#include <QList>
#include <QString>
#include <QStringList>
#include "buildmodel.h"

class QMakeParser
{
    enum ParserState
    {
        ParserState_LookingForKeyword,
        ParserState_FetchingData
    };

    struct ConditionalBlock
    {
        QString condition;
        bool active;
        int line;
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> Defines;
        QList<QString> IncludePaths;
        QList<QString> Libraries;
        QList<QString> Config;
    };

    public:
        QMakeParser();
        bool Parse(QString text, BuildProject *model, QString source_file, QString cmake_minimum_version);

    private:
        bool ParseStandardQMakeList(QList<QString> *list, QString line, QString text);
        bool ProcessSimpleKeyword(QString word, QString line);
        bool ProcessComplexKeyword(QString word, QString line, QString data_buffer);
        bool ProcessScope(QString line, QStringList &lines, int &current_line);
        QString ParseCondition(QString condition);
        bool EvaluateCondition(QString condition);
        void RefreshModel();
        BuildTargetType TargetTypeFromTemplate(QString value);
        BuildTargetType TargetTypeFromConfig(BuildTargetType current_type) const;

        BuildProject *Model;
        QString SourceFile;
        QString CMakeMinimumVersion;
        QString ProjectName;
        QString TemplateName;
        QList<QString> KnownSimpleKeywords;
        QList<QString> KnownComplexKeywords;
        QList<QString> RequiredKeywords;
        QList<QString> RemainingRequiredKeywords;
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> Modules;
        QList<QString> Config;
        QList<QString> Defines;
        QList<QString> IncludePaths;
        QList<QString> Libraries;
        QList<QString> UIFiles;
        QList<QString> ResourceFiles;
        QList<QString> Subdirectories;
        QList<ConditionalBlock> ConditionalBlocks;
        BuildTargetType TargetType;
        int CurrentLineNumber;
        int TargetLine;
        bool IsSubdirsProject;
};

#endif // QMAKEPARSER_H
