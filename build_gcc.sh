mkdir -p build_gcc
cd build_gcc
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
cd ..
