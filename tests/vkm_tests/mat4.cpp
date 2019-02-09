#include <gtest/gtest.h>
#include <vkm/vkm.h>

TEST(mat4, zero) {
  mat4_t mat = mat4_zero();

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mat.columns[i][j], 0.0f);
    }
  }
}

TEST(mat4, diagonal) {
  mat4_t mat = mat4_diagonal(2.0f);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        EXPECT_EQ(mat.columns[i][j], 2.0f);
      } else {
        EXPECT_EQ(mat.columns[i][j], 0.0f);
      }
    }
  }
}

TEST(mat4, identity) {
  mat4_t mat = mat4_identity();

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (i == j) {
        EXPECT_EQ(mat.columns[i][j], 1.0f);
      } else {
        EXPECT_EQ(mat.columns[i][j], 0.0f);
      }
    }
  }
}

TEST(mat4, transpose_identity) {
  mat4_t mata = mat4_identity();
  mat4_t matb = mat4_transpose(mata);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mata.columns[i][j], matb.columns[i][j]);
    }
  }
}

TEST(mat4, transpose) {
  mat4_t mata = mat4_identity();
  mata.columns[0][1] = 5.0f;
  mata.columns[0][2] = 6.0f;
  mata.columns[0][3] = 7.0f;
  mata.columns[1][3] = 8.0f;
  mata.columns[2][3] = 9.0f;
  mata.columns[3][2] = 10.0f;
  mat4_t matb = mat4_transpose(mata);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mata.columns[i][j], matb.columns[j][i]);
    }
  }
}

TEST(mat4, addition) {
  mat4_t mata = mat4_identity();
  mata.columns[0][1] = 5.0f;
  mat4_t matb = mat4_identity();
  matb.columns[0][1] = 1.0f;
  matb.columns[0][3] = 5.0f;
  matb.columns[1][2] = 3.0f;

  mat4_t sum = mat4_add(mata, matb);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mata.columns[i][j] + matb.columns[i][j], sum.columns[i][j]);
    }
  }
}

TEST(mat4, subtraction) {
  mat4_t mata = mat4_identity();
  mata.columns[0][1] = 5.0f;
  mat4_t matb = mat4_identity();
  matb.columns[0][1] = 1.0f;
  matb.columns[0][3] = 5.0f;
  matb.columns[1][2] = 3.0f;

  mat4_t sub = mat4_sub(mata, matb);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mata.columns[i][j] - matb.columns[i][j], sub.columns[i][j]);
    }
  }
}

TEST(mat4, multiplication_scalar) {
  mat4_t mat = mat4_identity();
  mat4_t mul = mat4_muls(mat, 5.0f);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mat.columns[i][j] * 5.0f, mul.columns[i][j]);
    }
  }
}

TEST(mat4, division_scalar) {
  mat4_t mat = mat4_identity();
  mat4_t div = mat4_divs(mat, 5.0f);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mat.columns[i][j] / 5.0f, div.columns[i][j]);
    }
  }
}

TEST(mat4, multiplication) {
  mat4_t mata = {{
      {1, 2, 1, 1},
      {0, 1, 0, 1},
      {1, 1, 1, 1},
      {1, 1, 1, 1},
  }};

  mat4_t matb = {{
      {2, 5, 1, 1},
      {6, 7, 1, 1},
      {1, 8, 1, 1},
      {1, 1, 1, 1},
  }};

  mat4_t mul = mat4_mul(mata, matb);

  mat4_t expected = {{
      {16, 28, 5, 5},
      {7, 8, 2, 2},
      {10, 21, 4, 4},
      {10, 21, 4, 4},
  }};

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      EXPECT_EQ(mul.columns[i][j], expected.columns[i][j]);
    }
  }
}
