//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "logs.h"
#include "configuration.h"
#include <iostream>

void Logs::DebugLog(QString text, int verbosity)
{
    if (verbosity <= Configuration::verbosity_level)
    {
        std::cout << "[DEBUG] " << text.toStdString() << std::endl;
    }
}

void Logs::ErrorLog(QString text)
{
    std::cerr << "[ERROR] " << text.toStdString() << std::endl;
}

void Logs::Log(QString text)
{
    std::cout << "[INFO] " << text.toStdString() << std::endl;
}
