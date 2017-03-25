//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "project.h"

QString Project::QT_Target = "TARGET =";

Project::Project()
{
    ProjectName = "";
}

QString generateCMakeOptions(QList<CMakeOption> *options)
{
    QString result;
    foreach (CMakeOption option, *options)
    {
        result += "option(" + option.Name + " \"" + option.Description + "\" " + option.Default + ")\n";
    }
    return result;
}

bool Project::Load(QString text)
{
    if (!text.contains(QT_Target))
    {
        Logs::ErrorLog("Required TARGET not found");
        return false;
    }
    QString target = text.mid(text.indexOf(QT_Target) + QT_Target.length());
    // remove all leading space
    while (target.startsWith(" "))
    {
        target = target.mid(1);
    }
    target = FinishCut(target);
    ProjectName = target;
    return true;
}

QString Project::ToQmake()
{
    QString source = "#-------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-------------------------------------------------\n";
    source += "TARGET = " + ProjectName;
    return source;
}

QString Project::ToCmake()
{
    QString source = "#-------------------------------------------------\n";
    source += "# Project converted from qmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-------------------------------------------------\n";
    source += "cmake_minimum_required (VERSION 2.6)\n";
    source += "project(" + ProjectName + ")\n";
    //! \todo Somewhere here we should generate options for CMake based on Qt version preference
    source += generateCMakeOptions(&this->CMakeOptions);
    return source;
}

QString Project::FinishCut(QString text)
{
    if (text.contains("\n"))
    {
        text = text.mid(0, text.indexOf("\n"));
    }
    if (text.contains(" "))
    {
        text = text.mid(0, text.indexOf(" "));
    }
    return text;
}
