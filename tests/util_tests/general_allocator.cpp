#include <gtest/gtest.h>
#include <util/general_allocator.h>

uint32_t header_count(ut_general_block_t *block) {
  uint32_t headers = 0;

  ut_general_alloc_header_t *header = block->first_header;
  while (header != NULL) {
    headers++;
    header = header->next;
  }

  return headers;
}

uint32_t block_count(ut_general_allocator_t *allocator) {
  uint32_t blocks = 0;

  ut_general_block_t *block = allocator->last_block;
  while (block != NULL) {
    blocks++;
    block = block->prev;
  }

  return blocks;
}

TEST(general_allocator, create_destroy) {
  ut_general_allocator_t allocator;
  ut_general_allocator_init(&allocator, 120);

  EXPECT_EQ(header_count(allocator.last_block), 1);
  EXPECT_EQ(block_count(&allocator), 1);

  uint32_t *alloc1 = UT_GENERAL_ALLOC(&allocator, uint32_t);
  *alloc1 = 32;

  EXPECT_EQ(header_count(allocator.last_block), 2);
  EXPECT_EQ(block_count(&allocator), 1);

  EXPECT_EQ(
      (uintptr_t)allocator.last_block->first_header +
          sizeof(ut_general_alloc_header_t),
      (uintptr_t)allocator.last_block->first_header->addr);

  ut_general_allocator_free(&allocator, alloc1);
  EXPECT_EQ(header_count(allocator.last_block), 1);
  EXPECT_EQ(block_count(&allocator), 1);

  uint32_t *alloc2 = UT_GENERAL_ALLOC(&allocator, uint32_t);
  EXPECT_EQ(header_count(allocator.last_block), 2);
  EXPECT_EQ(block_count(&allocator), 1);

  uint32_t *alloc3 = UT_GENERAL_ALLOC(&allocator, uint32_t);
  *alloc3 = 64;
  EXPECT_EQ(*alloc3, 64);

  EXPECT_EQ(header_count(allocator.last_block), 3);
  EXPECT_EQ(block_count(&allocator), 1);

  ut_general_allocator_free(&allocator, alloc3);

  EXPECT_EQ(header_count(allocator.last_block), 2);
  EXPECT_EQ(block_count(&allocator), 1);

  EXPECT_EQ((uintptr_t)alloc1, (uintptr_t)alloc2);
  EXPECT_EQ(*alloc2, 32);

  alloc3 = UT_GENERAL_ALLOC(&allocator, uint32_t);

  EXPECT_EQ(header_count(allocator.last_block), 3);
  EXPECT_EQ(block_count(&allocator), 1);

  uint32_t *alloc4 = UT_GENERAL_ALLOC(&allocator, uint32_t);
  *alloc4 = 36;
  EXPECT_EQ(header_count(allocator.last_block), 3);
  EXPECT_EQ(block_count(&allocator), 1);
  EXPECT_EQ(*alloc4, 36);

  uint32_t *alloc5 = UT_GENERAL_ALLOC(&allocator, uint32_t);
  *alloc5 = 45;
  EXPECT_EQ(header_count(allocator.last_block), 2);
  EXPECT_EQ(block_count(&allocator), 2);
  EXPECT_EQ(*alloc5, 45);

  typedef uint32_t bigarray[3000];
  bigarray *alloc6 = UT_GENERAL_ALLOC(&allocator, bigarray);
  EXPECT_EQ((uintptr_t)alloc6, NULL);

  ut_general_allocator_destroy(&allocator);
}
