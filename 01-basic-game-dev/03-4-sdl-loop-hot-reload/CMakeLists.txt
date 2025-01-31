cmake_minimum_required(VERSION 3.16...3.23)
project(03-4-sdl-loop-hot-reload)

add_executable(engine-03-4 engine.cxx engine.hxx engine.cxx)
target_compile_features(engine-03-4 PUBLIC cxx_std_17)

target_compile_definitions(engine-03-4 PRIVATE $<$<PLATFORM_ID:Windows>: OM_DECLSPEC=__declspec(dllexport) >)
target_compile_definitions(engine-03-4 PRIVATE $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS=1>)

set_target_properties(
  engine-03-4 PROPERTIES
  ENABLE_EXPORTS 1 # we need to be able to link to exe functions from game
)

find_package(SDL2 REQUIRED)
if(MINGW)
  target_link_libraries(engine-03-4 PRIVATE -lmingw32)
  add_custom_command(
    TARGET engine-03-4 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:engine-03-4>
  )
endif()
target_link_libraries(engine-03-4 PRIVATE SDL2::SDL2main SDL2::SDL2)

add_library(game-03-4 SHARED game.cxx)

target_compile_definitions(game-03-4 PRIVATE $<$<PLATFORM_ID:Windows>:OM_DECLSPEC=__declspec(dllexport) >)

target_compile_features(game-03-4 PUBLIC cxx_std_17)
target_link_libraries(game-03-4 PRIVATE engine-03-4)
