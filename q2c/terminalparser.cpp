//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "terminalparser.h"
using namespace std;

TerminalParser::TerminalParser(int argc_, QStringList argv)
{
    this->argc = argc_;
    this->Silent = false;
    this->args = argv;
}

bool TerminalParser::Parse()
{
    int x = 1;
    while (x < this->args.count())
    {
        bool valid = false;
        QString text = this->args.at(x);
        if (text == "-h" || text == "--help")
        {
            DisplayHelp();
            return true;
        }
        if (!text.startsWith("--") && text.startsWith("-"))
        {
            text = text.mid(1);
            while (text.length())
            {
                if (this->ParseChar(text.at(0)))
                {
                    return true;
                }
                text = text.mid(1);
            }
            valid = true;
        }
        if (!valid)
        {
            if (!this->Silent)
            {
                cout << (QString("This parameter isn't valid: ") + text).toStdString() << endl;
            }
            return true;
        }
        x++;
    }
    return false;
}

bool TerminalParser::ParseChar(QChar x)
{
    switch (x.toLatin1())
    {
        case 'v':
            Configuration::Verbosity++;
            return false;
        case 'h':
            this->DisplayHelp();
            return true;
    }
    return false;
}

void TerminalParser::DisplayHelp()
{
    if (this->Silent)
    {
        return;
    }
    cout << "q2c" << endl << endl;
    cout << "By default q2c checks the current folder for any qmake (.pro) file," << endl;
    cout << "it fails if it find 0 or more files" << endl << endl;
    cout << "Parameters:" << endl;
    cout << "  -v: Increases verbosity" << endl;
    cout << "  -o|--out <file>: define a name of output file" << endl;
    cout << "  -i|--in <file>: define a name of input file" << endl;
    cout << "  -h | --help: Display this help" << endl<< endl;
    cout << "q2c is an open source, contribute at https://github.com/benapetr/q2c" << endl;
}
