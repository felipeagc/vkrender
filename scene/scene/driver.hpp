#pragma once

#include "parser.hpp"
#include "scene.hpp"
#include <cstdio>
#include <string>
#include <unordered_set>

#define YY_DECL yy::parser::symbol_type yylex(scene::Driver &drv)
YY_DECL;

namespace scene {
class Driver {
public:
  enum struct Section { eNone, eScene, eAssets, eEntities };

  Driver();

  int parseFile(const std::string &filename, FILE* file);
  int parseText(const std::string &text);

  void postProcessIds();

  void scanBegin();
  void scanEnd();

  void setSection(Section section);

  void addAsset(int id, std::string assetType);
  void addEntity(int id);

  void addValue(int val);
  void addValue(float val);
  void addValue(std::string val);

  void addComponent(std::string name);
  void addProperty(std::string name);

  bool m_traceParsing = false;
  bool m_traceScanning = false;

  FILE* m_file = nullptr;
  std::string m_filename;
  std::string m_text;

  yy::location m_location;

  Entity *m_currentEntity = nullptr;
  Asset *m_currentAsset = nullptr;
  std::string m_currentComponent = "";
  std::string m_currentProp = "";

  Section m_currentSection = Section::eNone;

  std::unordered_set<uint32_t> m_assetIds;
  std::unordered_set<uint32_t> m_entityIds;
  uint32_t m_autoAssetId = 0;
  uint32_t m_autoEntityId = 0;

  Scene m_scene;

private:
  void addValueGeneric(Value value);
};
} // namespace scene
