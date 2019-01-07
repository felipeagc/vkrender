#include <gtest/gtest.h>
#include <sdf/parser.hpp>

using namespace sdf;

void printValue(const Value &val) {
  switch (val.type) {
  case ValueType::INTEGER:
    printf("Value: %d\n", val.value.ival);
    break;
  case ValueType::FLOAT:
    printf("Value: %f\n", val.value.fval);
    break;
  case ValueType::STRING:
    printf("Value: \"%s\"\n", val.value.sval);
    break;
  }
}

void printAsset(const AssetBlock &asset) {
  printf("Asset ID: %lu\n", asset.index);
  printf("Asset type: %s\n", asset.type);
  printf("Asset name: %s\n", asset.name);
  for (auto &prop : asset.properties) {
    printf("  Property: %s\n", prop.name);
    for (auto &val : prop.values) {
      printf("    ");
      printValue(val);
    }
  }
}

void printEntity(const EntityBlock &entity) {
  printf("Entity: %s\n", entity.name);

  for (auto &comp : entity.components) {
    printf("  Component: %s\n", comp.name);
    for (auto &prop : comp.properties) {
      printf("    Property: %s\n", prop.name);
      for (auto &val : prop.values) {
        printf("      ");
        printValue(val);
      }
    }
  }
}

void printScene(const SceneFile &scene) {
  printf("Scene:\n\n");
  for (auto &prop : scene.scene.properties) {
    printf("  Property: %s\n", prop.name);
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

  auto result = parse_string(text.c_str());
  EXPECT_TRUE((bool)result);
  auto scene = result.value;
  printScene(scene);

  EXPECT_EQ(scene.scene.properties.size(), 0);
  EXPECT_EQ(scene.assets.size(), 0);
  EXPECT_EQ(scene.entities.size(), 0);
}

TEST(scene, scene_properties) {
  std::string text = R"(




# This is a comment


scene {
a 1 2 3
b 1 2
c
d 
1 2
}

# Hey hey hey


)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE((bool)result);
  auto scene = result.value;
  printScene(scene);

  EXPECT_EQ(scene.scene.properties.size(), 4);
  EXPECT_EQ(scene.assets.size(), 0);
  EXPECT_EQ(scene.entities.size(), 0);
}

TEST(scene, no_whitespace) {
  std::string text = R"(scene{})";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE((bool)result);
  auto scene = result.value;
  printScene(scene);

  EXPECT_EQ(scene.scene.properties.size(), 0);
  EXPECT_EQ(scene.assets.size(), 0);
  EXPECT_EQ(scene.entities.size(), 0);
}

TEST(scene, asset_properties) {
  std::string text = R"(

asset SomeAsset "some_asset" {
  a 1 2 3
  b 1 2
  c 1 2
  d 1 2
}

)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE((bool)result);
  auto scene = result.value;
  printScene(scene);

  EXPECT_EQ(scene.scene.properties.size(), 0);
  EXPECT_EQ(scene.assets.size(), 1);
  EXPECT_EQ(scene.assets[0].properties.size(), 4);
  EXPECT_EQ(scene.entities.size(), 0);
}

TEST(scene, entity_components) {
  std::string text = R"(

entity {
  Transform {
    position 1 2 3
    rotation 1 2 3 4
    scale 1 2 3
  }

  Billboard { asset "billboard" }
}
)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE((bool)result);
  auto scene = result.value;
  printScene(scene);

  EXPECT_EQ(scene.scene.properties.size(), 0);
  EXPECT_EQ(scene.assets.size(), 0);
  EXPECT_EQ(scene.entities.size(), 1);
  auto &transform = scene.entities[0].components[0];
  EXPECT_EQ(transform.properties.size(), 3);
  EXPECT_EQ(transform.properties[0].values.size(), 3);
  EXPECT_EQ(transform.properties[1].values.size(), 4);
  EXPECT_EQ(transform.properties[1].values[0].type, ValueType::INTEGER);
  EXPECT_EQ(transform.properties[1].values[0].value.ival, 1);
  EXPECT_EQ(transform.properties[1].values[1].value.ival, 2);
  EXPECT_EQ(transform.properties[1].values[2].value.ival, 3);
  EXPECT_EQ(transform.properties[1].values[3].value.ival, 4);
  EXPECT_EQ(transform.properties[2].values.size(), 3);
  auto &billboard = scene.entities[0].components[1];
  EXPECT_EQ(billboard.properties.size(), 1);
  EXPECT_EQ(billboard.properties[0].values[0].type, ValueType::INTEGER);
}

TEST(scene, duplicate_asset_name) {
  std::string text = R"(
asset GltfModel "name" {}
asset GltfModel "name" {}
)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE(!result);
}

TEST(scene, duplicate_entity_name) {
  std::string text = R"(
entity "name" {}
entity "name" {}
)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE(!result);
}

TEST(scene, duplicate_entity_component) {
  std::string text = R"(
entity {
  Transform {}
  Transform {}
}
)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE(!result);
}

TEST(scene, value_without_property) {
  std::string text = R"(
scene {
  "asdd" 123
}
)";

  auto result = parse_string(text.c_str());
  EXPECT_TRUE(!result);
}
