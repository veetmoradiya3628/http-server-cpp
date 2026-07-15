#!/bin/sh
rm -rf build
cmake -S . -B build -G "Unix Makefiles"
cmake --build build
./build/http-server