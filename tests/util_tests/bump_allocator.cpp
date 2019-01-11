#include <gtest/gtest.h>
#include <util/bump_allocator.hpp>

TEST(bump_allocator, create_destroy) {
  bump_allocator_t allocator;
  bump_allocator_init(&allocator, sizeof(uint32_t));

  bump_allocator_destroy(&allocator);
}

TEST(bump_allocator, alloc_fail_small_size) {
  bump_allocator_t allocator;
  bump_allocator_init(&allocator, sizeof(uint32_t));

  uint32_t *i1 = bump_alloc(&allocator, uint32_t);
  uint64_t *i2 = bump_alloc(&allocator, uint64_t);

  EXPECT_NE((void *)i1, (void *)NULL);
  EXPECT_EQ((void *)i2, (void *)NULL);

  bump_allocator_destroy(&allocator);
}

TEST(bump_allocator, alloc_same_block) {
  bump_allocator_t allocator;
  bump_allocator_init(&allocator, sizeof(uint32_t) * 2);

  uint32_t *i1 = bump_alloc(&allocator, uint32_t);
  uint32_t *i2 = bump_alloc(&allocator, uint32_t);

  EXPECT_NE((void *)i1, (void *)NULL);
  EXPECT_NE((void *)i2, (void *)NULL);
  EXPECT_NE(i1 + sizeof(uint32_t), i2);

  bump_allocator_destroy(&allocator);
}

TEST(bump_allocator, alloc_new_block) {
  bump_allocator_t allocator;
  bump_allocator_init(&allocator, sizeof(uint32_t));

  uint32_t *i1 = bump_alloc(&allocator, uint32_t);
  uint32_t *i2 = bump_alloc(&allocator, uint32_t);

  EXPECT_NE((void *)i1, (void *)NULL);
  EXPECT_NE((void *)i2, (void *)NULL);

  bump_allocator_destroy(&allocator);
}
