//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

#include "configuration.h"

bool Configuration::debug = false;
bool Configuration::only_qt4 = false;
bool Configuration::only_qt5 = false;
bool Configuration::only_qt6 = false;
bool Configuration::force = false;
int Configuration::verbosity_level = 0;
QString Configuration::InputFile = "";
QString Configuration::OutputFile = "";
bool Configuration::q2c = true;

