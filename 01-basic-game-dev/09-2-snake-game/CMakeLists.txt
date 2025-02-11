cmake_minimum_required(VERSION 3.16)
project(09-2-start-game-protorype)

add_executable(engine-09-2 om/engine.cxx
                           om/engine.hxx
                           om/stb_image.h
                           om/math.hxx)
set_target_properties(engine-09-2 PROPERTIES ENABLE_EXPORTS TRUE)
target_compile_features(engine-09-2 PRIVATE cxx_std_17)

if(WIN32)
  target_compile_definitions(engine-09-2 PRIVATE "-DOM_DECLSPEC=__declspec(dllexport)")
endif(WIN32)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

if (MINGW)
    find_package(sdl2 REQUIRED)
    target_include_directories(engine-09-2 PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(engine-09-2
               mingw32
               SDL2main
               SDL2
               -mwindows
               opengl32
               )
elseif(UNIX)
    target_link_libraries(engine-09-2
               SDL2::SDL2
               SDL2::SDL2main
               OpenGL::OpenGL
               )
elseif(MSVC)
    find_package(sdl2 REQUIRED)
    target_link_libraries(engine-09-2 PRIVATE SDL2::SDL2 SDL2::SDL2main
                          opengl32)
endif()

add_library(game-09-2 SHARED game/game.cxx
                             game/game_object.cxx
                             game/snake.cxx
                             game/fruit.cxx
                             game/configuration_loader.hxx
                             game/game_object.hxx
                             game/snake.hxx
                             game/fruit.hxx)
target_include_directories(game-09-2 PRIVATE .)
target_compile_features(game-09-2 PRIVATE cxx_std_17)
target_link_libraries(game-09-2 PRIVATE engine-09-2)

if(MSVC)
  target_compile_definitions(game-09-2 PRIVATE "-DOM_DECLSPEC=__declspec(dllimport)")
  target_compile_definitions(game-09-2 PRIVATE "-DOM_EXPORT=__declspec(dllexport)")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # using regular Clang or AppleClang
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

install(TARGETS engine-09-2 game-09-2
        RUNTIME DESTINATION ${CMAKE_CURRENT_LIST_DIR}/../../bin/tests
        LIBRARY DESTINATION ${CMAKE_CURRENT_LIST_DIR}/../../bin/tests
        ARCHIVE DESTINATION ${CMAKE_CURRENT_LIST_DIR}/../../bin/tests)
