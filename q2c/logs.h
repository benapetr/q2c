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
