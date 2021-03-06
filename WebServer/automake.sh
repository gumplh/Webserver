#!bin/bash

if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake ..
make -j2
cp ../server.yaml .
cp ../static . -r

if [ ! -d "logs" ]; then
    mkdir logs
fi