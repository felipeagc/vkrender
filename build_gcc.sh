mkdir -p build-gcc
cd build-gcc
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
cd ..
