#include <scene/driver.hpp>
#include <gtest/gtest.h>

void printValue(const scene::Value &val) {
  switch (val.type) {
  case scene::ValueType::eInt:
    printf("Value: %d\n", val.value.ival);
    break;
  case scene::ValueType::eFloat:
    printf("Value: %f\n", val.value.fval);
    break;
  case scene::ValueType::eString:
    printf("Value: \"%s\"\n", val.value.sval.c_str());
    break;
  }
}

void printAsset(const scene::Asset &asset) {
  printf("Asset ID: %d\n", asset.id);
  printf("Asset type: %s\n", asset.type.c_str());
  for (auto &[prop_name, prop] : asset.properties) {
    printf("  Property: %s\n", prop_name.c_str());
    for (auto &val : prop.values) {
      printf("    ");
      printValue(val);
    }
  }
}

void printEntity(const scene::Entity &entity) {
  printf("Entity ID: %d\n", entity.id);

  for (auto &[comp_name, comp] : entity.components) {
    printf("  Component: %s\n", comp_name.c_str());
    for (auto &[prop_name, prop] : comp.properties) {
      printf("    Property: %s\n", prop_name.c_str());
      for (auto &val : prop.values) {
        printf("      ");
        printValue(val);
      }
    }
  }
}

void printScene(const scene::Scene &scene) {
  printf("Scene:\n\n");
  for (auto &[prop_name, prop] : scene.properties) {
    printf("  Property: %s\n", prop_name.c_str());
    for (auto &val : prop.values) {
      printf("    ");
      printValue(val);
    }
  }

  printf("\nAssets:\n\n");

  for (auto &asset : scene.assets) {
    printAsset(asset);
  }

  printf("\nEntities:\n\n");

  for (auto &entity : scene.entities) {
    printEntity(entity);
  }
}


TEST(scene, empty_scene) {
  std::string text = R"()";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_EQ(drv.parseText(text), 0);
  printScene(drv.m_scene);

  EXPECT_EQ(drv.m_scene.properties.size(), 0);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 0);
}

TEST(scene, scene_properties) {
  std::string text = R"(




// This is a comment


Scene:
a 1 2 3

b 1 2

c
1 2


// Hey hey hey

/*
Multiline comment 
babyy

Woohoo
*/

d 
1 2
)";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_EQ(drv.parseText(text), 0);
  printScene(drv.m_scene);

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
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_EQ(drv.parseText(text), 0);
  printScene(drv.m_scene);

  EXPECT_EQ(drv.m_scene.properties.size(), 1);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 0);
}

TEST(scene, asset_properties) {
  std::string text = R"(
Assets:

# SomeAsset
a 1 2 3

b 1 2

c 
1 2

d 
1 
2
)";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_EQ(drv.parseText(text), 0);
  printScene(drv.m_scene);

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
  rotation 
  1
  2
  3
  4
  scale 1 2 3
}

Billboard { asset 1 }
)";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_EQ(drv.parseText(text), 0);
  printScene(drv.m_scene);

  EXPECT_EQ(drv.m_scene.properties.size(), 0);
  EXPECT_EQ(drv.m_scene.assets.size(), 0);
  EXPECT_EQ(drv.m_scene.entities.size(), 1);
  auto &transform = drv.m_scene.entities[0].components["Transform"];
  EXPECT_EQ(transform.properties.size(), 3);
  EXPECT_EQ(transform.properties["position"].values.size(), 3);
  EXPECT_EQ(transform.properties["rotation"].values.size(), 4);
  EXPECT_EQ(transform.properties["rotation"].values[0].type, scene::ValueType::eInt);
  EXPECT_EQ(transform.properties["rotation"].values[0].value.ival, 1);
  EXPECT_EQ(transform.properties["rotation"].values[1].value.ival, 2);
  EXPECT_EQ(transform.properties["rotation"].values[2].value.ival, 3);
  EXPECT_EQ(transform.properties["rotation"].values[3].value.ival, 4);
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
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_NE(drv.parseText(text), 0);
  printScene(drv.m_scene);
}

TEST(scene, duplicate_entity_id) {
  std::string text = R"(
Entities:
#1
#1
)";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_NE(drv.parseText(text), 0);
  printScene(drv.m_scene);
}

TEST(scene, duplicate_entity_component) {
  std::string text = R"(
Entities:
# 
Transform {}
Transform {}
)";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_NE(drv.parseText(text), 0);
  printScene(drv.m_scene);
}

TEST(scene, value_without_property) {
  std::string text = R"(
Scene:
"asda" 123
)";

  scene::Driver drv;
  drv.m_traceScanning = true;
  drv.m_traceParsing = true;

  EXPECT_NE(drv.parseText(text), 0);
  printScene(drv.m_scene);
}
