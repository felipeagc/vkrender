#pragma once

#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace sdf {

using String = char[128];

enum struct ValueType {
  INTEGER,
  FLOAT,
  STRING,
};

struct Value {
  ValueType type = ValueType::INTEGER;
  union InternalValue {
    String sval;
    int ival;
    float fval;
  } value;

  Value() {}

  Value(int ival) {
    type = ValueType::INTEGER;
    value.ival = ival;
  }

  Value(float fval) {
    type = ValueType::FLOAT;
    value.fval = fval;
  }

  Value(const char *sval) {
    type = ValueType::STRING;
    strcpy(value.sval, sval);
  }

  inline bool get_string(char *s) const {
    if (type == ValueType::STRING) {
      strcpy(s, value.sval);
      return true;
    } else {
      return false;
    }
  }

  inline bool get_string(std::string &s) const {
    if (type == ValueType::STRING) {
      s = value.sval;
      return true;
    } else {
      return false;
    }
  }

  inline bool get_int(int *i) const {
    if (type == ValueType::INTEGER) {
      *i = value.ival;
      return true;
    } else if (type == ValueType::FLOAT) {
      *i = (int)value.fval;
      return true;
    } else {
      return false;
    }
  }

  inline bool get_uint32(uint32_t *u) const {
    int i;
    if (!get_int(&i)) {
      return false;
    }
    *u = static_cast<uint32_t>(i);
    return true;
  }

  inline bool get_float(float *f) const {
    if (type == ValueType::INTEGER) {
      *f = (float)value.ival;
      return true;
    } else if (type == ValueType::FLOAT) {
      *f = value.fval;
      return true;
    } else {
      return false;
    }
  }
};

struct Property {
  String name;
  std::vector<Value> values;

  inline bool get_string(std::string &s) const {
    if (values.size() == 1) {
      return values[0].get_string(s);
    } else {
      return false;
    }
  }

  inline bool get_string(char *s) const {
    if (values.size() == 1) {
      return values[0].get_string(s);
    } else {
      return false;
    }
  }

  inline bool get_int(int *i) const {
    if (values.size() == 1) {
      return values[0].get_int(i);
    } else {
      return false;
    }
  }

  inline bool get_uint32(uint32_t *u) const {
    if (values.size() == 1) {
      return values[0].get_uint32(u);
    } else {
      return false;
    }
  }

  inline bool get_float(float *f) const {
    if (values.size() == 1) {
      return values[0].get_float(f);
    } else {
      return false;
    }
  }

  inline bool get_vec3(glm::vec3 *v) const {
    if (values.size() == 1) {
      return values[0].get_float(&v->x) && values[0].get_float(&v->y) &&
             values[0].get_float(&v->z);
    } else if (values.size() == 3) {
      return values[0].get_float(&v->x) && values[1].get_float(&v->y) &&
             values[2].get_float(&v->z);
    } else {
      return false;
    }
  }

  inline bool get_vec4(glm::vec4 *v) const {
    if (values.size() < 4)
      return false;
    return values[0].get_float(&v->x) && values[1].get_float(&v->y) &&
           values[2].get_float(&v->z) && values[3].get_float(&v->w);
  }

  inline bool get_quat(glm::quat *q) const {
    if (values.size() == 4) {
      glm::vec4 v;
      if (!get_vec4(&v)) {
        return false;
      }

      *q = glm::angleAxis(
          glm::radians(v.x), glm::normalize(glm::vec3(v.y, v.z, v.w)));
      return true;
    } else if (values.size() == 3) {
      glm::vec3 v;
      if (!get_vec3(&v)) {
        return false;
      }

      *q = glm::quat(glm::radians(v));
      return true;
    } else {
      return false;
    }
  }
};

struct AssetBlock {
  size_t index;
  String name;
  String type;
  std::vector<Property> properties;
};

struct Component {
  String name;
  std::vector<Property> properties;
};

struct EntityBlock {
  String name;
  std::vector<Component> components;
};

struct SceneBlock {
  std::vector<Property> properties;
};

struct SceneFile {
  SceneBlock scene;
  std::vector<AssetBlock> assets;
  std::vector<EntityBlock> entities;
};

} // namespace sdf
