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
#include <QDateTime>
#include "logs.h"

class CMakeOption
{
    public:
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
        QtVersion_All
    };

    public:
        Project();
        bool Load(QString text);
        QString ToQmake();
        QString ToCmake();
        QList<CMakeOption> CMakeOptions;
        //! This is used to determine which target libraries are needed, if you specify All it means 
        //! that there will be switch in CMake that lets user decide in build time
        QtVersion Version;
        QString ProjectName;
    private:
        static QString QT_Target;
        static QString FinishCut(QString text);
};

#endif // PROJECT_H
