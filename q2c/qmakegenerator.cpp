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

QString QMakeGenerator::Generate(const BuildProject &project)
{
    const BuildTarget *target = project.PrimaryTarget();
    QString target_name = target != nullptr ? target->Name : project.Name;

    QString source = "#-----------------------------------------------------------------\n";
    source += "# Project converted from cmake file using q2c\n";
    source += "# https://github.com/benapetr/q2c at " + QDateTime::currentDateTime().toString() + "\n";
    source += "#-----------------------------------------------------------------\n";
    source += "TARGET = " + target_name;
    return source;
}
