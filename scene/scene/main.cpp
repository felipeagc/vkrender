#include "driver.hpp"
#include "parser.hpp"
#include "scene.hpp"
#include <cstdio>

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

int main(int argc, char **argv) {
  scene::Driver drv;

//   std::string text = R"(
// Assets:
// # Hello
// path "../derp"
// )";

//   if (drv.parseText(text) == 0) {
//     printScene(drv.m_scene);
//   }

  if (argc >= 2) {
    FILE* file = fopen(argv[1], "r");
    if (drv.parseFile(argv[1], file) == 0) {
      std::cout << "Parsing succeded!" << std::endl;
      printScene(drv.m_scene);
    }
    fclose(file);
  } else {
    if (drv.parseFile("stdin", stdin) == 0) {
      std::cout << "Parsing succeded!" << std::endl;
      printScene(drv.m_scene);
    }
  }

  return 0;
}
