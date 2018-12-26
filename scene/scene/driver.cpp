#include "driver.hpp"

using namespace scene;

Driver::Driver() {}

int Driver::parseFile(const std::string &filename, FILE *file) {
  m_file = file;
  m_filename = filename;
  m_location.initialize(&m_filename);
  scanBegin();
  yy::parser parse(*this);
  parse.set_debug_level(m_traceParsing);
  int res = parse();
  scanEnd();

  postProcessIds();

  return res;
}

int Driver::parseText(const std::string &text) {
  m_text = text;
  m_location.initialize(&m_filename);
  m_file = fmemopen((void *)text.c_str(), text.length(), "r");
  scanBegin();
  yy::parser parse(*this);
  parse.set_debug_level(m_traceParsing);
  int res = parse();
  scanEnd();

  postProcessIds();

  return res;
}

void Driver::postProcessIds() {
  // Assets
  for (auto &asset : m_scene.assets) {
    if (asset.id == static_cast<uint32_t>(-1)) {
      do {
        asset.id = m_autoAssetId++;
      } while (m_assetIds.count(asset.id) != 0);
    }
  }

  // Entities
  for (auto &entity : m_scene.entities) {
    if (entity.id == static_cast<uint32_t>(-1)) {
      do {
        entity.id = m_autoEntityId++;
      } while (m_entityIds.count(entity.id) != 0);
    }
  }
}

void Driver::setSection(Section section) {
  m_currentSection = section;
  m_currentAsset = nullptr;
  m_currentEntity = nullptr;
  m_currentProp = "";
  m_currentComponent = "";
}

void Driver::addAsset(int id, std::string assetType) {
  if (id != -1 && m_assetIds.count(static_cast<uint32_t>(id)) != 0) {
    throw yy::parser::syntax_error(m_location, "duplicate asset ID");
  }
  scene::Asset asset;
  asset.id = static_cast<uint32_t>(id);
  asset.type = assetType;
  m_assetIds.insert(asset.id);
  m_scene.assets.push_back(asset);
  m_currentAsset = &m_scene.assets[m_scene.assets.size() - 1];
}

void Driver::addEntity(int id) {
  if (id != -1 && m_entityIds.count(static_cast<uint32_t>(id)) != 0) {
    throw yy::parser::syntax_error(m_location, "duplicate entity ID");
  }
  scene::Entity entity;
  entity.id = static_cast<uint32_t>(id);
  m_entityIds.insert(entity.id);
  m_scene.entities.push_back(entity);
  m_currentEntity = &m_scene.entities[m_scene.entities.size() - 1];
}

void Driver::addValue(int val) {
  scene::Value value(val);
  addValueGeneric(value);
}

void Driver::addValue(float val) {
  scene::Value value(val);
  addValueGeneric(value);
}

void Driver::addValue(std::string val) {
  scene::Value value(val);
  addValueGeneric(value);
}

void Driver::addValueGeneric(Value value) {
  if (m_currentProp.length() == 0) {
    throw yy::parser::syntax_error(m_location, "value without property");
  }

  if (m_currentSection == scene::Driver::Section::eScene) {
    m_scene.properties[m_currentProp].values.push_back(value);
  } else if (m_currentSection == scene::Driver::Section::eAssets) {
    m_currentAsset->properties[m_currentProp].values.push_back(value);
  } else if (m_currentSection == scene::Driver::Section::eEntities) {
    m_currentEntity->components[m_currentComponent]
        .properties[m_currentProp]
        .values.push_back(value);
  } else {
    throw yy::parser::syntax_error(m_location, "value outside a section");
  }
}

void Driver::addComponent(std::string name) {
  if (m_currentEntity == nullptr) {
    throw yy::parser::syntax_error(m_location, "component without entity");
  }
  if (m_currentEntity->components.find(name) !=
      m_currentEntity->components.end()) {
    throw yy::parser::syntax_error(
        m_location, "duplicate component declaration");
  }

  m_currentComponent = name;
  m_currentEntity->components[m_currentComponent] = {};
}

void Driver::addProperty(std::string name) {
  if (m_currentSection == scene::Driver::Section::eScene) {
    if (m_scene.properties.find(name) != m_scene.properties.end()) {
      throw yy::parser::syntax_error(
          m_location, "duplicate property declaration");
    }

    m_currentProp = name;
    m_scene.properties[m_currentProp] = {};
  } else if (m_currentSection == scene::Driver::Section::eAssets) {
    if (m_currentAsset != nullptr) {
      if (m_currentAsset->properties.find(name) !=
          m_currentAsset->properties.end()) {
        throw yy::parser::syntax_error(
            m_location, "duplicate property declaration");
      }

      m_currentProp = name;
      m_currentAsset->properties[m_currentProp] = {};
    } else {
      throw yy::parser::syntax_error(m_location, "property outside an asset");
    }
  } else if (m_currentSection == scene::Driver::Section::eEntities) {
    if (m_currentEntity != nullptr && m_currentComponent.length() != 0) {
      if (m_currentEntity->components[m_currentComponent].properties.find(
              name) !=
          m_currentEntity->components[m_currentComponent].properties.end()) {
        throw yy::parser::syntax_error(
            m_location, "duplicate property declaration");
      }

      m_currentProp = name;
      m_currentEntity->components[m_currentComponent]
          .properties[m_currentProp] = {};
    } else {
      throw yy::parser::syntax_error(
          m_location, "property outside a component");
    }
  } else {
    throw yy::parser::syntax_error(m_location, "property outside a section");
  }
}
