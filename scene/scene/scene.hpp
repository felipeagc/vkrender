#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>
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

    InternalValue(){};
    ~InternalValue(){};
  } value;

  inline float getFloat() {
    if (this->type == ValueType::eFloat) {
      return this->value.fval;
    } else if (this->type == ValueType::eInt) {
      return static_cast<float>(this->value.ival);
    }
    throw std::runtime_error("Tried to get float from incompatible value type");
  }

  inline int getInt() {
    if (this->type == ValueType::eFloat) {
      return static_cast<int>(this->value.fval);
    } else if (this->type == ValueType::eInt) {
      return this->value.ival;
    }
    throw std::runtime_error("Tried to get int from incompatible value type");
  }

  inline std::string getString() {
    if (this->type == ValueType::eString) {
      return this->value.sval;
    } else if (this->type == ValueType::eInt) {
      return std::to_string(this->value.ival);
    } else if (this->type == ValueType::eFloat) {
      return std::to_string(this->value.fval);
    }
    throw std::runtime_error(
        "Tried to get string from incompatible value type");
  }

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
    new (&value.sval) std::string(sval);
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

  inline std::string getString() {
    if (values.size() >= 1) {
      return values[0].getString();
    }

    throw std::runtime_error("Could not get string from property");
  }

  inline float getFloat() {
    if (values.size() >= 1) {
      return values[0].getFloat();
    }

    throw std::runtime_error("Could not get float from property");
  }

  inline int getInt() {
    if (values.size() >= 1) {
      return values[0].getInt();
    }

    throw std::runtime_error("Could not get int from property");
  }

  inline uint32_t getUint32() {
    if (values.size() >= 1) {
      return static_cast<uint32_t>(values[0].getInt());
    }

    throw std::runtime_error("Could not get uint32_t from property");
  }

  inline glm::vec3 getVec3() {
    if (values.size() == 1) {
      return glm::vec3(values[0].getFloat());
    }
    if (values.size() == 3) {
      return glm::vec3(
          values[0].getFloat(), values[1].getFloat(), values[2].getFloat());
    }

    throw std::runtime_error("Could not get vec3 from property");
  }

  inline glm::vec4 getVec4() {
    if (values.size() == 1) {
      return glm::vec4(values[0].getFloat());
    }
    if (values.size() == 4) {
      return glm::vec4(
          values[0].getFloat(),
          values[1].getFloat(),
          values[2].getFloat(),
          values[3].getFloat());
    }

    throw std::runtime_error("Could not get vec4 from property");
  }

  inline glm::quat getQuat() {
    if (values.size() == 4) {
      return glm::quat(
          values[0].getFloat(),
          values[1].getFloat(),
          values[2].getFloat(),
          values[3].getFloat());
    }

    throw std::runtime_error("Could not get quat from property");
  }
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
