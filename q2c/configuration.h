//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QString>

class Configuration
{
    public:
        static bool debug;
        static bool only_qt4;
        static bool only_qt5;
        static bool only_qt6;
        static int verbosity_level;
        static QString InputFile;
        static QString OutputFile;
        static bool force;      // Single flag for force overwrite
        static bool q2c;       // Direction of conversion (true = qmake to cmake, false = cmake to qmake)
};

#endif // CONFIGURATION_H
