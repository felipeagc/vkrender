#include <ftl/string.hpp>
#include <gtest/gtest.h>

TEST(string, string_blank) {
  ftl::string s;
  EXPECT_EQ(s.length(), 0);
  EXPECT_TRUE(s == "");
  EXPECT_STREQ(s.c_str(), "");
  EXPECT_TRUE(s.is_small());
}

TEST(string, string_simple) {
  ftl::string s = "hello";
  EXPECT_EQ(s.length(), 5);
  EXPECT_TRUE(s == "hello");
  EXPECT_STREQ(s.c_str(), "hello");
  EXPECT_TRUE(!s.is_small());
}

TEST(string, string_copy) {
  ftl::string s = "hello";
  ftl::string copy = s;

  EXPECT_EQ(s.length(), 5);
  EXPECT_EQ(copy.length(), 5);

  EXPECT_TRUE(s == "hello");
  EXPECT_TRUE(copy == "hello");
  EXPECT_TRUE(s == copy);
  EXPECT_TRUE(s.c_str() != copy.c_str());
  EXPECT_STREQ(s.c_str(), "hello");
  EXPECT_STREQ(s.c_str(), copy.c_str());

  EXPECT_TRUE(!s.is_small());
  EXPECT_TRUE(!copy.is_small());
}

TEST(string, string_move_constructor) {
  ftl::string s = "hello";
  char *before = s.c_str();
  ftl::string moved(std::move(s));

  EXPECT_EQ(s.length(), 0);
  EXPECT_EQ(moved.length(), 5);

  EXPECT_EQ(before, moved.c_str());
  EXPECT_STREQ(moved.c_str(), "hello");

  EXPECT_TRUE(s.is_small());
  EXPECT_TRUE(!moved.is_small());
}

TEST(string, string_move_assignment) {
  ftl::string s = "hello";
  char *before = s.c_str();
  ftl::string moved = std::move(s);

  EXPECT_EQ(s.length(), 0);
  EXPECT_EQ(moved.length(), 5);

  EXPECT_EQ(before, moved.c_str());
  EXPECT_STREQ(moved.c_str(), "hello");

  EXPECT_TRUE(s.is_small());
  EXPECT_TRUE(!moved.is_small());
}

TEST(string, string_iterate) {
  ftl::string s = "hello";

  char *cs = new char[s.length() + 1];
  int i = 0;
  for (char c : s) {
    cs[i++] = c;
  }
  cs[i] = '\0';

  EXPECT_STREQ(cs, s.c_str());

  delete[] cs;
}

TEST(string, string_capacity_stack) {
  ftl::basic_string<5> s = "hello";

  EXPECT_TRUE(s.is_small());
  EXPECT_EQ(s.capacity(), 5);
}

TEST(string, string_capacity_heap) {
  ftl::basic_string<3> s = "hello";

  EXPECT_TRUE(!s.is_small());
  EXPECT_EQ(s.capacity(), 5);
}

TEST(string, string_capacity_stack_to_heap) {
  ftl::basic_string<5> s = "hello";
  EXPECT_TRUE(s.is_small());
  EXPECT_EQ(s.capacity(), 5);

  s = "hello world";
  EXPECT_TRUE(!s.is_small());
  EXPECT_EQ(s.capacity(), 11);
}

TEST(string, small_string_blank) {
  ftl::small_string s = "";
  EXPECT_EQ(s.length(), 0);
  EXPECT_TRUE(s == "");
  EXPECT_STREQ(s.c_str(), "");
  EXPECT_TRUE(s.is_small());
}

TEST(string, small_string_simple) {
  ftl::small_string s = "hello";
  EXPECT_EQ(s.length(), 5);
  EXPECT_TRUE(s == "hello");
  EXPECT_STREQ(s.c_str(), "hello");
  EXPECT_TRUE(s.is_small());
}

TEST(string, small_string_copy) {
  ftl::small_string s = "hello";
  ftl::small_string copy = s;

  EXPECT_EQ(s.length(), 5);
  EXPECT_EQ(copy.length(), 5);

  EXPECT_TRUE(s == "hello");
  EXPECT_TRUE(copy == "hello");
  EXPECT_TRUE(s == copy);
  EXPECT_TRUE(s.c_str() != copy.c_str());
  EXPECT_STREQ(s.c_str(), "hello");
  EXPECT_STREQ(s.c_str(), copy.c_str());

  EXPECT_TRUE(s.is_small());
  EXPECT_TRUE(copy.is_small());
}

TEST(string, small_string_move_constructor) {
  ftl::small_string s = "hello";
  char *before = s.c_str();
  ftl::small_string moved(std::move(s));

  EXPECT_EQ(s.length(), 0);
  EXPECT_EQ(moved.length(), 5);

  EXPECT_NE(before, moved.c_str());
  EXPECT_STREQ(moved.c_str(), "hello");

  EXPECT_TRUE(s.is_small());
  EXPECT_TRUE(moved.is_small());
}

TEST(string, small_string_move_assignment) {
  ftl::small_string s = "hello";
  char *before = s.c_str();
  ftl::small_string moved = std::move(s);

  EXPECT_EQ(s.length(), 0);
  EXPECT_EQ(moved.length(), 5);

  EXPECT_NE(before, moved.c_str());
  EXPECT_STREQ(moved.c_str(), "hello");

  EXPECT_TRUE(s.is_small());
  EXPECT_TRUE(moved.is_small());
}
