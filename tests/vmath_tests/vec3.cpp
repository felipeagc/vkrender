#include <gtest/gtest.h>
#include <vmath/vmath.h>

TEST(vec3, addition) {
  vec3_t v1 = {1, 2, 3};
  vec3_t v2 = {4, 5, 6};
  vec3_t sum = vec3_add(v1, v2);

  EXPECT_EQ(sum.x, 1 + 4);
  EXPECT_EQ(sum.y, 2 + 5);
  EXPECT_EQ(sum.z, 3 + 6);
}

TEST(vec3, multiplication) {
  vec3_t v1 = {1, 2, 3};
  vec3_t v2 = {4, 5, 6};
  vec3_t mul = vec3_mul(v1, v2);

  EXPECT_EQ(mul.x, 1 * 4);
  EXPECT_EQ(mul.y, 2 * 5);
  EXPECT_EQ(mul.z, 3 * 6);
}

TEST(vec3, dot_product) {
  vec3_t v1 = {1, 2, 3};
  vec3_t v2 = {4, 5, 6};
  float dot = vec3_dot(v1, v2);

  EXPECT_EQ(dot, 1 * 4 + 2 * 5 + 3 * 6);
}
