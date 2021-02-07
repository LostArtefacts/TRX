#!/bin/sh
i686-w64-mingw32-gcc -shared src/*.c -ldbghelp -o build/TR1Main.dll
