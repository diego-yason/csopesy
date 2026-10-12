
# CSOPESY S01

MO3 - Marquee Console


Programmed by:

YASON, DIEGO DAVID PEREZ - 12308978

Entry Class File: src/main.cpp

## Compile/Run Instructions

CMake is required.
Create build config
Compile file `main.cpp` and run the resulting .exe file.

```
@echo off
@REM the -pedantic-errors flag is here to ensure consistency regardless of C++ compiler
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\CSOPESY_MO4.exe
```