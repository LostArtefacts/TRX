#!/bin/bash
shopt -s globstar
i686-w64-mingw32-gcc -W -static -shared src/**/*.c -ldbghelp -o build/TR1Main.dll
