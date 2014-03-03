#include "logs.h"

void Logs::Log(QString text)
{
    std::cout << text.toStdString() << std::endl;
}

void Logs::ErrorLog(QString text)
{
    std::cerr << text.toStdString() << std::endl;
}

void Logs::DebugLog(QString text, int verbosity)
{
    if (verbosity <= Configuration::Verbosity)
    {
        Log(text);
    }
}
