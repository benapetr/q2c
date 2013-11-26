//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include <QCoreApplication>
#include "terminalparser.h"

int main(int argc, char *argv[])
{
    int c = 0;
    QStringList args;
    while (c < argc)
    {
        args.append(QString(argv[c]));
        c++;
    }
    TerminalParser *tp = new TerminalParser(argc, args);
    if (tp->Parse())
    {
        QCoreApplication a(argc, argv);

        return a.exec();
    }
    return 0;
}
