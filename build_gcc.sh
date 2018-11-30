mkdir -p build
cd build
CC=gcc CXX=g++ cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
cd ..
