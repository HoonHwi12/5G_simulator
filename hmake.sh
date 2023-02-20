#!/bin/bash

make -j
cd dqn/build
cmake --build .
cd ../../
echo "make finish"
