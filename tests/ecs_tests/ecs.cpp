#include <ecs/world.hpp>
#include <gtest/gtest.h>

TEST(ecs, test_create_remove_entity) {
  struct Position {
    int x, y;
  };

  ecs::World world;
  ecs::Entity e = world.createEntity();
  world.assign<Position>(e, 1, 2);

  EXPECT_EQ(world.hasComponents(e), true);
  world.removeEntity(e);

  EXPECT_EQ(world.hasComponents(e), false);
}

TEST(ecs, test_component_filter) {
  struct Position {
    int x, y;
  };

  struct Velocity {
    int x, y;
  };

  ecs::World world;
  ecs::Entity e1 = world.createEntity();
  world.assign<Position>(e1, 1, 2);
  world.assign<Velocity>(e1, 1, 2);

  ecs::Entity e2 = world.createEntity();
  world.assign<Position>(e2, 1, 2);

  std::vector<ecs::Entity> entities;

  world.each<Position, Velocity>(
      [&](ecs::Entity e, Position &, Velocity &) { entities.push_back(e); });

  EXPECT_EQ(entities, std::vector{e1});
}

TEST(ecs, test_component_remove_1) {
  static int count = 0;
  struct Test {
    Test() { count++; }

    ~Test() { count--; }
  };

  ecs::World world;
  ecs::Entity e1 = world.createEntity();
  world.assign<Test>(e1);

  EXPECT_EQ(count, 1);
  EXPECT_EQ((world.getComponent<Test>(e1) != nullptr), true);

  world.removeComponent<Test>(e1);

  EXPECT_EQ((world.getComponent<Test>(e1) == nullptr), true);
  EXPECT_EQ(count, 0);
}

TEST(ecs, test_component_free_1) {
  static int count = 0;
  struct Test {
    Test() { count++; }

    ~Test() { count--; }
  };

  ecs::World world;
  ecs::Entity e1 = world.createEntity();
  world.assign<Test>(e1);

  EXPECT_EQ(count, 1);

  world.removeEntity(e1);

  EXPECT_EQ(count, 0);
}

TEST(ecs, test_component_free_2) {
  static int count = 0;
  struct Test {
    Test() { count++; }

    ~Test() { count--; }
  };

  {
    ecs::World world;
    ecs::Entity e1 = world.createEntity();
    world.assign<Test>(e1);

    EXPECT_EQ(count, 1);
  }

  EXPECT_EQ(count, 0);
}

TEST(ecs, test_component_free_3) {
  struct Test {
    int *p = nullptr;
    Test(int n) : p(new int(n)) {}

    ~Test() { delete p; }
  };

  ecs::World world;
  ecs::Entity e1 = world.createEntity();
  world.assign<Test>(e1, 3);
  ecs::Entity e2 = world.createEntity();
  world.assign<Test>(e2, 3);

  world.removeEntity(e1);
  world.removeEntity(e2);
}

TEST(ecs, test_component_free_4) {
  struct Test {
    int *p = nullptr;
    Test(int n) : p(new int(n)) {}

    ~Test() { delete p; }
  };

  {
    ecs::World world;
    ecs::Entity e1 = world.createEntity();
    world.assign<Test>(e1, 3);
  }
}
