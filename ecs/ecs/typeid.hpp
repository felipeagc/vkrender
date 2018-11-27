#pragma once

#include <atomic>
#include <vector>

namespace ecs {

template <typename...> class TypeId {
  inline static std::atomic<std::size_t> identifier;

  template <typename...>
  inline static const auto inner = identifier.fetch_add(1);

public:
  template <typename... Type>
  inline static const std::size_t type = inner<std::decay_t<Type>...>;
};

} // namespace ecs
