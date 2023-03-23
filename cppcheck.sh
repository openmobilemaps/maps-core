#!/bin/bash
if which cppcheck >/dev/null; then
	echo "Running cppcheck"
	cppcheck -j 4 --enable=all --std=c++17 --template=gcc  shared/* -ishared/src/external
else
    echo "warning: cppcheck not installed, install here: http://brewformulas.org/Cppcheck"
fi

