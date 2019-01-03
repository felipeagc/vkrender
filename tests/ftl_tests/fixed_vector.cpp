#include <ftl/fixed_vector.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

TEST(fixed_vector, test_create_stack) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3, 4};
  std::vector<int> v{1, 2, 3, 4};

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_create_heap) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3, 4, 5, 6};
  std::vector<int> v{1, 2, 3, 4, 5, 6};

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_push_back_stack) {
  ftl::fixed_vector<int, 4> sv{1, 2};
  sv.push_back(3);

  std::vector<int> v{1, 2};
  v.push_back(3);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_push_back_heap) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3, 4};
  sv.push_back(5);

  std::vector<int> v{1, 2, 3, 4};
  v.push_back(5);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_push_back_heap2) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3, 4, 5};
  sv.push_back(6);

  std::vector<int> v{1, 2, 3, 4, 5};
  v.push_back(6);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_resize_stack) {
  ftl::fixed_vector<int, 4> sv{1, 2};
  sv.resize(4);

  std::vector<int> v{1, 2};
  v.resize(4);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_resize_heap) {
  ftl::fixed_vector<int, 4> sv{1, 2};
  sv.resize(6);

  std::vector<int> v{1, 2};
  v.resize(6);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_count_constructor_stack) {
  ftl::fixed_vector<int, 4> sv(4);
  std::vector<int> v(4);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_count_constructor_heap) {
  ftl::fixed_vector<int, 4> sv(10);
  std::vector<int> v(10);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_count_value_constructor_stack) {
  ftl::fixed_vector<int, 4> sv(4, 2);
  std::vector<int> v(4, 2);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_count_value_constructor_heap) {
  ftl::fixed_vector<int, 4> sv(10, 2);
  std::vector<int> v(10, 2);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(fixed_vector, test_copy_constructor_stack) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3};
  std::vector<int> v{1, 2, 3};

  ftl::fixed_vector<int, 4> sv1(sv);
  std::vector<int> v1(v);

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(fixed_vector, test_copy_constructor_heap) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3, 4, 5};
  std::vector<int> v{1, 2, 3, 4, 5};

  ftl::fixed_vector<int, 4> sv1(sv);
  std::vector<int> v1(v);

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(fixed_vector, test_copy_assign_op_stack) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3};
  std::vector<int> v{1, 2, 3};

  ftl::fixed_vector<int, 4> sv1;
  sv1 = sv;
  std::vector<int> v1;
  v1 = v;

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(fixed_vector, test_copy_assign_op_heap) {
  ftl::fixed_vector<int, 4> sv{1, 2, 3, 4, 5};
  std::vector<int> v{1, 2, 3, 4, 5};

  ftl::fixed_vector<int, 4> sv1;
  sv1 = sv;
  std::vector<int> v1;
  v1 = v;

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}
