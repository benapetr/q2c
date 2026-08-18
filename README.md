q2c
===

'''IMPORTANT:''' q2c was never finished nor released. It's a work in progress and most likely not useable in production. Your contributions are welcome.

qmake &lt;-> cmake convertor tool


How does it work
=================

This is lightweight tool which allows to convert Qt projects made using qmake to
cmake and other way (from cmake to qmake). It is cross platform and works only
in terminal.

How to compile
===============

Enter the folder with source code and type `qmake && make`


How to install
===============
    sudo make install

How to use
===========

Enter folder with .pro file and type

    q2c

This will automatically detect input and output file, you can also use

    q2c -i test.pro -o test.cmake

By default q2c detects the conversion direction from the input file name:

* `.pro` and `.pri` files are treated as qmake input and converted to CMake.
* `CMakeLists.txt` and `.cmake` files are treated as CMake input. This direction
  is detected, but CMake to qmake conversion is not implemented yet.

Useful options:

    q2c --check -i test.pro
    q2c --dry-run -i test.pro
    q2c --version
    q2c --qmake-to-cmake -i test.pro -o CMakeLists.txt
    q2c --cmake-to-qmake -i CMakeLists.txt -o test.pro

Existing output files are not overwritten unless `-f` or `--force` is used.
