#include <gtest/gtest.h>
#include <util/bump_allocator.h>

TEST(bump_allocator, create_destroy) {
  ut_bump_allocator_t allocator;
  ut_bump_allocator_init(&allocator, sizeof(uint32_t));

  ut_bump_allocator_destroy(&allocator);
}

TEST(bump_allocator, alloc_fail_small_size) {
  ut_bump_allocator_t allocator;
  ut_bump_allocator_init(&allocator, sizeof(uint32_t));

  uint32_t *i1 = ut_bump_alloc(&allocator, uint32_t);
  uint64_t *i2 = ut_bump_alloc(&allocator, uint64_t);

  EXPECT_NE((void *)i1, (void *)NULL);
  EXPECT_EQ((void *)i2, (void *)NULL);

  ut_bump_allocator_destroy(&allocator);
}

TEST(bump_allocator, alloc_same_block) {
  ut_bump_allocator_t allocator;
  ut_bump_allocator_init(&allocator, sizeof(uint32_t) * 2);

  uint32_t *i1 = ut_bump_alloc(&allocator, uint32_t);
  uint32_t *i2 = ut_bump_alloc(&allocator, uint32_t);

  EXPECT_NE((void *)i1, (void *)NULL);
  EXPECT_NE((void *)i2, (void *)NULL);
  EXPECT_NE(i1 + sizeof(uint32_t), i2);

  ut_bump_allocator_destroy(&allocator);
}

TEST(bump_allocator, alloc_new_block) {
  ut_bump_allocator_t allocator;
  ut_bump_allocator_init(&allocator, sizeof(uint32_t));

  uint32_t *i1 = ut_bump_alloc(&allocator, uint32_t);
  uint32_t *i2 = ut_bump_alloc(&allocator, uint32_t);

  EXPECT_NE((void *)i1, (void *)NULL);
  EXPECT_NE((void *)i2, (void *)NULL);

  ut_bump_allocator_destroy(&allocator);
}
