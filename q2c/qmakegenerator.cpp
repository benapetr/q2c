//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "qmakegenerator.h"
#include <QDateTime>
#include <QStringList>

static QString JoinList(const QList<QString> &items)
{
    QStringList quoted;
    foreach (QString item, items)
    {
        if (item.contains(" "))
            quoted << "\"" + item + "\"";
        else
            quoted << item;
    }
    return quoted.join(" ");
}

QString QMakeGenerator::Generate(const BuildProject &project)
{
    const BuildTarget *target = project.PrimaryTarget();
    QString target_name = target != nullptr ? target->Name : project.Name;

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    foreach (QString warning, project.Warnings)
        source += "# q2c warning: " + warning + "\n";
    source += "TARGET = " + target_name + "\n";

    if (target == nullptr)
        return source;

    if (target->Type == BuildTarget_Library || target->Type == BuildTarget_Plugin)
        source += "TEMPLATE = lib\n";
    else if (target->Type == BuildTarget_Subdirs)
        source += "TEMPLATE = subdirs\n";
    else
        source += "TEMPLATE = app\n";

    if (!target->QtModules.isEmpty())
        source += "QT += " + JoinList(target->QtModules) + "\n";
    if (!target->Config.isEmpty())
        source += "CONFIG += " + JoinList(target->Config) + "\n";
    if (!target->Defines.isEmpty())
        source += "DEFINES += " + JoinList(target->Defines) + "\n";
    if (!target->IncludePaths.isEmpty())
        source += "INCLUDEPATH += " + JoinList(target->IncludePaths) + "\n";
    if (!target->Sources.isEmpty())
        source += "SOURCES += " + JoinList(target->Sources) + "\n";
    if (!target->Headers.isEmpty())
        source += "HEADERS += " + JoinList(target->Headers) + "\n";
    if (!target->UiFiles.isEmpty())
        source += "FORMS += " + JoinList(target->UiFiles) + "\n";
    if (!target->ResourceFiles.isEmpty())
        source += "RESOURCES += " + JoinList(target->ResourceFiles) + "\n";
    if (!target->TranslationFiles.isEmpty())
        source += "TRANSLATIONS += " + JoinList(target->TranslationFiles) + "\n";
    if (!target->Libraries.isEmpty())
        source += "LIBS += " + JoinList(target->Libraries) + "\n";
    if (!target->CompileOptions.isEmpty())
        source += "QMAKE_CXXFLAGS += " + JoinList(target->CompileOptions) + "\n";
    if (!target->LinkOptions.isEmpty())
        source += "QMAKE_LFLAGS += " + JoinList(target->LinkOptions) + "\n";
    if (!target->Subdirectories.isEmpty())
        source += "SUBDIRS += " + JoinList(target->Subdirectories) + "\n";
    return source;
}
