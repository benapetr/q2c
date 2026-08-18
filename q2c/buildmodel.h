//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef BUILDMODEL_H
#define BUILDMODEL_H

#include <QList>
#include <QString>

enum BuildTargetType
{
    BuildTarget_Unknown,
    BuildTarget_Application,
    BuildTarget_Library,
    BuildTarget_Plugin,
    BuildTarget_Test,
    BuildTarget_Subdirs
};

class BuildSourceLocation
{
    public:
        BuildSourceLocation();
        BuildSourceLocation(QString file_name, int line_number);
        bool IsValid() const;

        QString FileName;
        int LineNumber;
};

class BuildConditionalScope
{
    public:
        BuildConditionalScope();

        QString Condition;
        BuildSourceLocation Location;
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> Defines;
        QList<QString> IncludePaths;
        QList<QString> Libraries;
        QList<QString> TranslationFiles;
        QList<QString> CompileOptions;
        QList<QString> LinkOptions;
        QList<QString> InstallRules;
        QList<QString> Config;
};

class BuildTarget
{
    public:
        BuildTarget();
        QString TypeName() const;

        QString Name;
        BuildTargetType Type;
        BuildSourceLocation Location;
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> UiFiles;
        QList<QString> ResourceFiles;
        QList<QString> TranslationFiles;
        QList<QString> QtModules;
        QList<QString> Config;
        QList<QString> Defines;
        QList<QString> IncludePaths;
        QList<QString> Libraries;
        QList<QString> CompileOptions;
        QList<QString> LinkOptions;
        QList<QString> InstallRules;
        QList<QString> Subdirectories;
        QList<BuildConditionalScope> ConditionalScopes;
};

class BuildProject
{
    public:
        BuildProject();
        void Clear();
        BuildTarget *EnsurePrimaryTarget();
        BuildTarget *PrimaryTarget();
        const BuildTarget *PrimaryTarget() const;
        void AddWarning(QString warning);

        QString Name;
        QString CMakeMinimumVersion;
        QList<QString> GlobalConfig;
        QList<QString> GlobalQtModules;
        QList<QString> Warnings;
        QList<BuildTarget> Targets;
};

#endif // BUILDMODEL_H
