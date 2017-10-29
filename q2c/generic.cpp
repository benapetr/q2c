//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena 2017

#include "generic.h"

QString Generic::ExpandedString(QString string, unsigned int minimum_size, unsigned int maximum_size)
{
    if (maximum_size > 0 && static_cast<unsigned int>(string.size()) > maximum_size)
    {
        if (maximum_size < 4)
        {
            string = string.mid(0, maximum_size);
        } else
        {
            string = string.mid(0, maximum_size - 3);
            string += "...";
        }
        return string;
    }

    while (static_cast<unsigned int>(string.size()) < minimum_size)
        string += " ";

    return string;
}
