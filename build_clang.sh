mkdir -p build_clang
cd build_clang
CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
cd ..
