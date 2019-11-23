# SpaghettiDungeonGenerator
A brute force dungeon generator kept in line by a series of constraints.
It's not quite ready for release, but here it is regardless.

## Requirements
C++17 compiler
Boost
OpenCV (Unless GAME_COMPILE is defined)
rapidjson


## Compile modes
The intention of GAME_COMPILE is that the dungeon generator will be compiled as a shared library with reduced dependencies (So far, that means no OpenCV).
