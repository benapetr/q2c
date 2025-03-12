//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QList>
#include <QHash>
#include <QDateTime>
#include "logs.h"

class CMakeOption
{
    public:
        CMakeOption(QString name, QString description, QString __default);
        QString Name;
        QString Description;
        QString Default;
};

class Project
{
    enum QtVersion
    {
        QtVersion_Qt4,
        QtVersion_Qt5,
        QtVersion_Qt6,
        QtVersion_All
    };

    enum ParserState
    {
        ParserState_LookingForKeyword,
        ParserState_FetchingData
    };

    public:
        Project();
        bool Load(QString text);
        bool ParseQmake(QString text);
        QString ToQmake();
        QString ToCmake();
        QList<CMakeOption> CMakeOptions;
        QtVersion Version;
        QString ProjectName;
        QString CMakeMinumumVersion;
    private:
        static QString FinishCut(QString text);
        bool ParseStandardQMakeList(QList<QString> *list, QString line, QString text);
        bool ProcessSimpleKeyword(QString word, QString line);
        bool ProcessComplexKeyword(QString word, QString line, QString data_buffer);
        QString GetCMakeDefaultQtLibs();
        QString GetCMakeQt4Libs();
        QString GetCMakeQt5Libs();
        QString GetCMakeQt6Libs();
        QString GetCMakeQtModules();
        QString ProcessConfigOptions();
        QString ProcessDefines();
        QString ProcessIncludePaths();
        QString ProcessLibs();
        QString ProcessUIFiles();
        QString ProcessResources();
        QString ProcessSubdirsInCMake();
        
        QList<QString> KnownSimpleKeywords;
        QList<QString> KnownComplexKeywords;
        QList<QString> RequiredKeywords;
        QList<QString> RemainingRequiredKeywords;
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> Modules;
        QList<QString> UIList;
        QList<QString> Config;
        QList<QString> Defines;
        QList<QString> IncludePaths;
        QList<QString> Libraries;
        QHash<QString, QString> Variables;
        QList<QString> UIFiles;
        QList<QString> ResourceFiles;
        QList<QString> Subdirectories;
        bool IsSubdirsProject;

        bool ParseCondition(QString condition);
        bool ProcessScope(QString line, QStringList &lines, int &currentLine);
        bool EvaluateCondition(QString condition);
        QString ProcessPlatformSpecific();
        
        // Nested scopes for conditions
        struct ConditionalBlock {
            QString condition;
            bool active;
            QList<QString> Sources;
            QList<QString> Headers;
            QList<QString> Defines;
            QList<QString> IncludePaths;
            QList<QString> Libraries;
            QList<QString> Config;
        };
        QList<ConditionalBlock> ConditionalBlocks;
};

#endif // PROJECT_H
