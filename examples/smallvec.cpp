#include <iostream>
#include <vkr/smallvec.hpp>

int main() {
  vkr::SmallVec<int, 4> vec{1, 2, 3, 4};

  vec = {3};
  vec = {5, 2, 3, 4};

  vec.resize(32);

  vkr::SmallVec<int, 4> vec2{vec};

  for (size_t i = 0; i < vec.size(); i++) {
    std::cout << vec[i] << std::endl;
  }

  std::cout << "---" << std::endl;

  for (size_t i = 0; i < vec2.size(); i++) {
    std::cout << vec2[i] << std::endl;
  }
}
