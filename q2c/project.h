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
        //! This is used to determine which target libraries are needed, if you specify All it means 
        //! that there will be switch in CMake that lets user decide in build time
        QtVersion Version;
        QString ProjectName;
    private:
        static QString FinishCut(QString text);
        bool ParseStandardQMakeList(QList<QString> *list, QString line, QString text);
        bool ProcessSimpleKeyword(QString word, QString line);
        bool ProcessComplexKeyword(QString word, QString line, QString data_buffer);
        QList<QString> KnownSimpleKeywords;
        QList<QString> KnownComplexKeywords;
        //! Keywords that must be in source document
        QList<QString> RequiredKeywords;
        QList<QString> RemainingRequiredKeywords;
        QList<QString> Sources;
        QList<QString> Headers;
        QList<QString> UIList;
};

#endif // PROJECT_H
