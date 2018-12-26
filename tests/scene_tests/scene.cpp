#include <scene/driver.hpp>
#include <gtest/gtest.h>

TEST(scene, empty_scene) {
  std::string text = R"()";

  scene::Driver drv;
  EXPECT_EQ(drv.parseText(text), 0);
  EXPECT_EQ(drv.m_scene.properties.size(), 0);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 0);
}

TEST(scene, scene_properties) {
  std::string text = R"(
Scene:
a 1 2 3

b [1 2]

c [
1 2
]

d [
1 2]
)";

  scene::Driver drv;
  EXPECT_EQ(drv.parseText(text), 0);
  EXPECT_EQ(drv.m_scene.properties.size(), 4);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 0);
}

TEST(scene, scene_blank_property) {
  std::string text = R"(
Scene:

blank
)";

  scene::Driver drv;
  EXPECT_EQ(drv.parseText(text), 0);
  EXPECT_EQ(drv.m_scene.properties.size(), 1);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 0);
}

TEST(scene, asset_properties) {
  std::string text = R"(
Assets:

# SomeAsset
a 1 2 3

b [1 2]

c [
1 2
]

d [
1 2]
)";

  scene::Driver drv;
  EXPECT_EQ(drv.parseText(text), 0);
  EXPECT_EQ(drv.m_scene.properties.size(), 0);
  EXPECT_EQ(drv.m_scene.assets.size(), 1);
  EXPECT_EQ(drv.m_scene.assets[0].properties.size(), 4);
  EXPECT_EQ(drv.m_scene.entities.size(), 0);
}

TEST(scene, entity_components) {
  std::string text = R"(
Entities:
#12
Transform {
  position 1 2 3
  rotation 1 2 3 4
  scale 1 2 3
}

Billboard { asset 1 }
)";

  scene::Driver drv;
  EXPECT_EQ(drv.parseText(text), 0);
  EXPECT_EQ(drv.m_scene.properties.size(), 0);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 1);
  auto &transform = drv.m_scene.entities[0].components["Transform"];
  EXPECT_EQ(transform.properties.size(), 3);
  EXPECT_EQ(transform.properties["position"].values.size(), 3);
  EXPECT_EQ(transform.properties["rotation"].values.size(), 4);
  EXPECT_EQ(transform.properties["scale"].values.size(), 3);
  auto &billboard = drv.m_scene.entities[0].components["Billboard"];
  EXPECT_EQ(billboard.properties.size(), 1);
  EXPECT_EQ(billboard.properties["asset"].values[0].value.ival, 1);
}

TEST(scene, duplicate_asset_id) {
  std::string text = R"(
Assets:
#1
#1
)";

  scene::Driver drv;
  EXPECT_NE(drv.parseText(text), 0);
}

TEST(scene, duplicate_entity_id) {
  std::string text = R"(
Entities:
#1
#1
)";

  scene::Driver drv;
  EXPECT_NE(drv.parseText(text), 0);
}

TEST(scene, duplicate_entity_component) {
  std::string text = R"(
Entities:
# 
Transform {}
Transform {}
)";

  scene::Driver drv;
  EXPECT_NE(drv.parseText(text), 0);
}
