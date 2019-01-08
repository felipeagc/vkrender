#include <ftl/string_builder.hpp>
#include <gtest/gtest.h>

TEST(string_builder, empty) {
  ftl::string_builder builder;
  EXPECT_TRUE(builder.is_empty());
  EXPECT_EQ(builder.length(), 0);
}

TEST(string_builder, build_empty_string) {
  ftl::string_builder builder;
  EXPECT_TRUE(builder.is_empty());
  EXPECT_EQ(builder.length(), 0);

  ftl::string s = builder.build_string();
  EXPECT_EQ(s.length(), 0);
  EXPECT_STREQ(s.c_str(), "");
}

TEST(string_builder, build_appended_string) {
  ftl::string_builder builder;
  builder.append("Hello");
  EXPECT_TRUE(!builder.is_empty());
  EXPECT_EQ(builder.length(), 5);

  ftl::string s = builder.build_string();
  EXPECT_EQ(s.length(), 5);
  EXPECT_STREQ(s.c_str(), "Hello");
}
