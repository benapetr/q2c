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

Enter the folder with source code
```
qmake
make
```

How to use
===========

Enter folder with .pro file and type
    q2c

This will automatically detect input and output file, you can also use
    q2c -i test.pro -o test.cmake

