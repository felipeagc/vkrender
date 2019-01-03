add_library(ftl INTERFACE)

target_include_directories(
  ftl INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

cotire(ftl)
