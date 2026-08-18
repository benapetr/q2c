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
#include "buildmodel.h"
#include "cmakegenerator.h"

class Project
{
    public:
        Project();
        bool Load(QString text);
        bool ParseQmake(QString text);
        bool ParseCmake(QString text);
        QString ToQmake();
        QString ToCmake();
        QList<CMakeOption> CMakeOptions;
        CMakeQtVersion Version;
        QString ProjectName;
        QString CMakeMinumumVersion;
        const BuildProject &GetModel() const;
    private:
        BuildProject Model;
};

#endif // PROJECT_H
