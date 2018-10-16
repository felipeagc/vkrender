#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <vkr/smallvec.hpp>

TEST(smallvec, test_create_stack) {
  vkr::SmallVec<int, 4> sv{1, 2, 3, 4};
  std::vector<int> v{1, 2, 3, 4};

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_create_heap) {
  vkr::SmallVec<int, 4> sv{1, 2, 3, 4, 5, 6};
  std::vector<int> v{1, 2, 3, 4, 5, 6};

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_push_back_stack) {
  vkr::SmallVec<int, 4> sv{1, 2};
  sv.push_back(3);

  std::vector<int> v{1, 2};
  v.push_back(3);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_push_back_heap) {
  vkr::SmallVec<int, 4> sv{1, 2, 3, 4};
  sv.push_back(5);

  std::vector<int> v{1, 2, 3, 4};
  v.push_back(5);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_push_back_heap2) {
  vkr::SmallVec<int, 4> sv{1, 2, 3, 4, 5};
  sv.push_back(6);

  std::vector<int> v{1, 2, 3, 4, 5};
  v.push_back(6);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_resize_stack) {
  vkr::SmallVec<int, 4> sv{1, 2};
  sv.resize(4);

  std::vector<int> v{1, 2};
  v.resize(4);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_resize_heap) {
  vkr::SmallVec<int, 4> sv{1, 2};
  sv.resize(6);

  std::vector<int> v{1, 2};
  v.resize(6);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_count_constructor_stack) {
  vkr::SmallVec<int, 4> sv(4);
  std::vector<int> v(4);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_count_constructor_heap) {
  vkr::SmallVec<int, 4> sv(10);
  std::vector<int> v(10);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_count_value_constructor_stack) {
  vkr::SmallVec<int, 4> sv(4, 2);
  std::vector<int> v(4, 2);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_count_value_constructor_heap) {
  vkr::SmallVec<int, 4> sv(10, 2);
  std::vector<int> v(10, 2);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(smallvec, test_copy_constructor_stack) {
  vkr::SmallVec<int, 4> sv{1, 2, 3};
  std::vector<int> v{1, 2, 3};

  vkr::SmallVec<int, 4> sv1(sv);
  std::vector<int> v1(v);

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(smallvec, test_copy_constructor_heap) {
  vkr::SmallVec<int, 4> sv{1, 2, 3, 4, 5};
  std::vector<int> v{1, 2, 3, 4, 5};

  vkr::SmallVec<int, 4> sv1(sv);
  std::vector<int> v1(v);

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(smallvec, test_copy_assign_op_stack) {
  vkr::SmallVec<int, 4> sv{1, 2, 3};
  std::vector<int> v{1, 2, 3};

  vkr::SmallVec<int, 4> sv1;
  sv1 = sv;
  std::vector<int> v1;
  v1 = v;

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(smallvec, test_copy_assign_op_heap) {
  vkr::SmallVec<int, 4> sv{1, 2, 3, 4, 5};
  std::vector<int> v{1, 2, 3, 4, 5};

  vkr::SmallVec<int, 4> sv1;
  sv1 = sv;
  std::vector<int> v1;
  v1 = v;

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}
