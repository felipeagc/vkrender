#include <ftl/vector.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

TEST(vector, test_create_stack) {
  ftl::vector<int, 4> sv{1, 2, 3, 4};
  std::vector<int> v{1, 2, 3, 4};

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_create_heap) {
  ftl::vector<int, 4> sv{1, 2, 3, 4, 5, 6};
  std::vector<int> v{1, 2, 3, 4, 5, 6};

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_push_back_stack) {
  ftl::vector<int, 4> sv{1, 2};
  sv.push_back(3);

  std::vector<int> v{1, 2};
  v.push_back(3);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_push_back_heap) {
  ftl::vector<int, 4> sv{1, 2, 3, 4};
  sv.push_back(5);

  std::vector<int> v{1, 2, 3, 4};
  v.push_back(5);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_push_back_heap2) {
  ftl::vector<int, 4> sv{1, 2, 3, 4, 5};
  sv.push_back(6);

  std::vector<int> v{1, 2, 3, 4, 5};
  v.push_back(6);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_resize_stack) {
  ftl::vector<int, 4> sv{1, 2};
  sv.resize(4);

  std::vector<int> v{1, 2};
  v.resize(4);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_resize_heap) {
  ftl::vector<int, 4> sv{1, 2};
  sv.resize(10);

  std::vector<int> v{1, 2};
  v.resize(10);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_count_constructor_stack) {
  ftl::vector<int, 4> sv(4);
  std::vector<int> v(4);

  EXPECT_EQ(sv.size(), v.size());
}

TEST(vector, test_count_constructor_heap) {
  ftl::vector<int, 4> sv(10);
  std::vector<int> v(10);

  EXPECT_EQ(sv.size(), v.size());
}

TEST(vector, test_count_value_constructor_stack) {
  ftl::vector<int, 4> sv(4, 2);
  std::vector<int> v(4, 2);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_count_value_constructor_heap) {
  ftl::vector<int, 4> sv(10, 2);
  std::vector<int> v(10, 2);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_copy_constructor_stack) {
  ftl::vector<int, 4> sv{1, 2, 3};
  std::vector<int> v{1, 2, 3};

  ftl::vector<int, 4> sv1(sv);
  std::vector<int> v1(v);

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(vector, test_copy_constructor_heap) {
  ftl::vector<int, 4> sv{1, 2, 3, 4, 5};
  std::vector<int> v{1, 2, 3, 4, 5};

  ftl::vector<int, 4> sv1(sv);
  std::vector<int> v1(v);

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(vector, test_copy_assign_op_stack) {
  ftl::vector<int, 4> sv{1, 2, 3};
  std::vector<int> v{1, 2, 3};

  ftl::vector<int, 4> sv1;
  sv1 = sv;
  std::vector<int> v1;
  v1 = v;

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(vector, test_copy_assign_op_heap) {
  ftl::vector<int, 4> sv{1, 2, 3, 4, 5};
  std::vector<int> v{1, 2, 3, 4, 5};

  ftl::vector<int, 4> sv1;
  sv1 = sv;
  std::vector<int> v1;
  v1 = v;

  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], sv1[i]);
  }
}

TEST(vector, test_initialization_stack) {
  ftl::vector<int, 10> sv(10);
  std::vector<int> v(10);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_initialization_heap) {
  ftl::vector<int, 4> sv(10);
  std::vector<int> v(10);

  EXPECT_EQ(sv.size(), v.size());

  for (size_t i = 0; i < sv.size(); i++) {
    EXPECT_EQ(v[i], 0);
    EXPECT_EQ(v[i], sv[i]);
  }
}

TEST(vector, test_move_stack) {
  ftl::vector<std::string, 4> sv1(10, "hello");
  std::vector<std::string> v1(10, "hello");

  EXPECT_EQ(sv1.size(), 10);
  EXPECT_EQ(sv1.size(), v1.size());

  for (size_t i = 0; i < sv1.size(); i++) {
    EXPECT_EQ(v1[i], "hello");
    EXPECT_EQ(v1[i], sv1[i]);
  }

  ftl::vector<std::string, 4> sv2 = std::move(sv1);
  std::vector<std::string> v2 = std::move(v1);

  EXPECT_EQ(sv1.size(), 0);
  EXPECT_EQ(sv1.size(), v1.size());

  EXPECT_EQ(sv2.size(), 10);
  EXPECT_EQ(sv2.size(), v2.size());

  for (size_t i = 0; i < sv2.size(); i++) {
    EXPECT_EQ(v2[i], "hello");
    EXPECT_EQ(v2[i], sv2[i]);
  }
}
