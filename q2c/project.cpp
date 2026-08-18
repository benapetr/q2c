//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "project.h"
#include "configuration.h"
#include "qmakegenerator.h"
#include "qmakeparser.h"

Project::Project()
{
    this->ProjectName = "";
    this->CMakeMinumumVersion = "VERSION 3.1.0";
    this->Version = CMakeQtVersion_All;
    if (Configuration::only_qt4)
    {
        this->Version = CMakeQtVersion_Qt4;
    } else if (Configuration::only_qt5)
    {
        this->Version = CMakeQtVersion_Qt5;
    } else if (Configuration::only_qt6)
    {
        this->Version = CMakeQtVersion_Qt6;
        this->CMakeMinumumVersion = "VERSION 3.16.0";
    }
    this->Model.CMakeMinimumVersion = this->CMakeMinumumVersion;
}

bool Project::Load(QString text)
{
    return this->ParseQmake(text);
}

bool Project::ParseQmake(QString text)
{
    QMakeParser parser;
    if (!parser.Parse(text, &this->Model, Configuration::InputFile, this->CMakeMinumumVersion))
        return false;

    this->ProjectName = this->Model.Name;
    return true;
}

QString Project::ToQmake()
{
    QMakeGenerator generator;
    return generator.Generate(this->Model);
}

QString Project::ToCmake()
{
    CMakeGenerator generator(this->Version);
    return generator.Generate(this->Model, this->CMakeOptions);
}

const BuildProject &Project::GetModel() const
{
    return this->Model;
}
