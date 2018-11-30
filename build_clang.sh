mkdir -p build
cd build
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
cd ..
