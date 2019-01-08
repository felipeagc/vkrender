#include <ftl/stack_allocator.hpp>
#include <gtest/gtest.h>
#include <string>

struct Type {
  int *t = nullptr;
  Type() { t = new int(3); }
  ~Type() { delete t; }
};

TEST(stack_allocator, basic) {
  ftl::stack_allocator allocator;

  void *ptr = allocator.alloc(sizeof(int), alignof(int));
  Type *test = new (ptr) Type();
  EXPECT_EQ(*test->t, 3);
  test->~Type();
}

TEST(stack_allocator, allocate_string) {
  ftl::stack_allocator allocator;

  using std::string;

  void *ptr = allocator.alloc(sizeof(string), alignof(string));

  std::string *s = new (ptr) std::string("Hello, world!");
  EXPECT_STREQ(s->c_str(), "Hello, world!");
  s->~string();
}

TEST(stack_allocator, multiple_blocks) {
  ftl::stack_allocator allocator(sizeof(int) * 3);

  for (int i = 0; i < 10; i++) {
    allocator.alloc(sizeof(int), alignof(int));
  }
}
