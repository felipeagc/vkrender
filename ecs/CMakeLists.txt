set(
  ECS_SOURCES
  ecs/world.cpp
  )

add_library(
  ecs
  SHARED
  ${ECS_SOURCES}
  )

target_include_directories(
  ecs
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_link_libraries(
  ecs
  PUBLIC
  ftl
  )

cotire(ecs)
