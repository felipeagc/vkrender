set(
  GAME_SOURCES
  main.cpp
  )

add_executable(game ${GAME_SOURCES})

target_link_libraries(
  game
  PRIVATE
  vkrender
  )
