#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace scene {
enum struct ValueType {
  eInt,
  eFloat,
  eString,
};

struct Value {
  ValueType type = ValueType::eInt;
  union InternalValue {
    std::string sval;
    int ival;
    float fval;

    InternalValue() {};
    ~InternalValue() {};
  } value;

  Value(int ival) {
    type = ValueType::eInt;
    value.ival = ival;
  }

  Value(float fval) {
    type = ValueType::eFloat;
    value.fval = fval;
  }

  Value(std::string sval) {
    type = ValueType::eString;
    new(&value.sval) std::string(sval);
  }

  Value(const Value &other) : type(other.type) {
    switch (other.type) {
    case ValueType::eInt:
      this->value.ival = other.value.ival;
      break;
    case ValueType::eFloat:
      this->value.fval = other.value.fval;
      break;
    case ValueType::eString:
      new (&value.sval) std::string(other.value.sval);
      break;
    }
  }

  ~Value() {
    if (type == ValueType::eString) {
      using std::string;
      value.sval.~string();
    }
  }
};

struct Property {
  std::vector<Value> values;
};

struct Asset {
  uint32_t id;
  std::string type;
  std::unordered_map<std::string, Property> properties;
};

struct Component {
  std::unordered_map<std::string, Property> properties;
};

struct Entity {
  uint32_t id;
  std::unordered_map<std::string, Component> components;
};

struct Scene {
  std::unordered_map<std::string, Property> properties;
  std::vector<Asset> assets;
  std::vector<Entity> entities;
};
} // namespace scene
