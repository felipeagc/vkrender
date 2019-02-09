#include <gtest/gtest.h>
#include <vkm/vkm.h>

TEST(vec4, addition) {
  vec4_t v1 = {1, 2, 3, 4};
  vec4_t v2 = {5, 6, 7, 8};
  vec4_t sum = vec4_add(v1, v2);

  EXPECT_EQ(sum.x, 1+5);
  EXPECT_EQ(sum.y, 2+6);
  EXPECT_EQ(sum.z, 3+7);
  EXPECT_EQ(sum.w, 4+8);
}

TEST(vec4, multiplication) {
  vec4_t v1 = {1, 2, 3, 4};
  vec4_t v2 = {5, 6, 7, 8};
  vec4_t mul = vec4_mul(v1, v2);

  EXPECT_EQ(mul.x, 1*5);
  EXPECT_EQ(mul.y, 2*6);
  EXPECT_EQ(mul.z, 3*7);
  EXPECT_EQ(mul.w, 4*8);
}

TEST(vec4, dot_product) {
  vec4_t v1 = {1, 2, 3, 4};
  vec4_t v2 = {5, 6, 7, 8};
  float dot = vec4_dot(v1, v2);

  EXPECT_EQ(dot, 1*5 + 2*6 + 3*7 + 4*8);
}
