//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2017

#ifndef GENERIC_H
#define GENERIC_H

#include <QString>

class Generic
{
    public:
        static QString ExpandedString(QString string, unsigned int minimum_size, unsigned int maximum_size = 0);
        static QString Indent(QString input, unsigned int indentation = 4);
        static QString CapitalFirst(QString text);
};

#endif // GENERIC_H
