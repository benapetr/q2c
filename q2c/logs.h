//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#ifndef LOGS_H
#define LOGS_H

#include <iostream>
#include "configuration.h"

namespace Logs
{
    void Log(QString text);
    void ErrorLog(QString text);
    void DebugLog(QString text, int verbosity = 1);
}

#endif // LOGS_H
