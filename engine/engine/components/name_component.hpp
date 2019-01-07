#pragma once

#include <cstring>

namespace engine {
struct NameComponent {
  char* name = nullptr;

  NameComponent(const char* name) {
    this->name =new char[strlen(name)+1];
    strcpy(this->name, name);
  }

  ~NameComponent() {
    delete name;
  }
};
} // namespace engine
