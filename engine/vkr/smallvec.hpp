#pragma once

#include <array>
#include <cstring>
#include <exception>

namespace vkr {
template <typename T, size_t N = 8> class SmallVec {
public:
  SmallVec() {}

  SmallVec(std::initializer_list<T> const &data) {
    size_t len = data.end() - data.begin();

    this->resize(len);
    this->count = len;

    size_t i = 0;
    for (auto p = data.begin(); p < data.end(); p++) {
      this->heapVector[i++] = *p;
    }
  }

  ~SmallVec() {
    if (heapVector != array.data()) {
      delete[] heapVector;
    }
  }

  SmallVec(const SmallVec &other) {
    this->resize(other.count);
    this->count = other.count;

    for (size_t i = 0; i < other.count; i++) {
      this->heapVector[i] = other[i];
    }
  }

  SmallVec &operator=(const SmallVec &other) {
    this->resize(other.count);
    this->count = other.count;

    for (size_t i = 0; i < other.count; i++) {
      this->heapVector[i] = other[i];
    }

    return *this;
  }

  void push_back(const T &value) noexcept {
    if (this->count >= this->capacity) {
      this->capacity *= 2;
      T *newStorage = new T[this->capacity];
      memcpy(newStorage, this->heapVector, this->count * sizeof(T));

      // Delete heap vector if it's not the stack allocated array
      if (this->heapVector != this->array.data()) {
        delete[] this->heapVector;
      }

      this->heapVector = newStorage;
    }
    this->heapVector[this->count++] = value;
  }

  T &operator[](size_t index) const noexcept { return heapVector[index]; }

  T &at(size_t index) const {
    if (index >= count) {
      throw std::runtime_error("Bad index");
    }
    return heapVector[index];
  }

  void resize(size_t newCapacity) noexcept {
    if (newCapacity <= N) {
      return;
    }

    T *newStorage = new T[newCapacity];
    size_t copySize =
        newCapacity <= this->capacity ? newCapacity : this->capacity;
    memcpy(newStorage, heapVector, copySize * sizeof(T));

    if (heapVector != array.data()) {
      delete[] heapVector;
    }

    heapVector = newStorage;
    this->capacity = newCapacity;
    this->count = this->count <= newCapacity ? this->count : newCapacity;
  }

  size_t size() const noexcept { return this->count; }

protected:
  std::array<T, N> array;
  size_t count{0};
  size_t capacity{N};
  T *heapVector{array.data()};
};
} // namespace vkr
