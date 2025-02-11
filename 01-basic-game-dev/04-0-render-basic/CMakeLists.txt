cmake_minimum_required(VERSION 3.16..3.22)
project(04-0-render-basic CXX)

add_executable(
  04-0-render-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  00_canvas_basic_main.cxx
)
target_compile_features(04-0-render-basic PUBLIC cxx_std_17)

add_executable(
  04-0-render-basic-line
  00_canvas_basic.cxx
  01_line_render.cxx
  01_line_render.hxx
  01_line_render_main.cxx
)
target_compile_features(04-0-render-basic-line PUBLIC cxx_std_17)

add_executable(
  04-0-render-basic-triangle
  00_canvas_basic.cxx
  01_line_render.cxx
  02_triangle_render.cxx
  02_triangle_render.hxx
  02_triangle_render_main.cxx
)
target_compile_features(04-0-render-basic-triangle PUBLIC cxx_std_17)

add_executable(
  04-0-render-basic-triangle-indexed
  00_canvas_basic.cxx
  01_line_render.cxx
  02_triangle_render.cxx
  03_triangle_indexed_render.cxx
  03_triangle_indexed_render.hxx
  03_triangle_indexed_render_main.cxx
)
target_compile_features(04-0-render-basic-triangle-indexed PUBLIC cxx_std_17)

add_executable(
  04-0-render-basic-triangle-interpolated
  00_canvas_basic.cxx
  01_line_render.cxx
  02_triangle_render.cxx
  03_triangle_indexed_render.cxx
  04_triangle_interpolated_render.cxx
  04_triangle_interpolated_render.hxx
  04_triangle_interpolated_render_main.cxx
)
target_compile_features(
  04-0-render-basic-triangle-interpolated
  PUBLIC cxx_std_17
)

find_package(SDL2 REQUIRED)
message(
  STATUS "SDL2_LIBRARIES[${SDL2_LIBRARIES}] "
  "SDL2_INCLUDE_DIRS[${SDL2_INCLUDE_DIRS}]"
)

add_executable(
  04-0-render-basic-windowed
  00_canvas_basic.cxx
  01_line_render.cxx
  02_triangle_render.cxx
  03_triangle_indexed_render.cxx
  04_triangle_interpolated_render.cxx
  04_triangle_interpolated_render.hxx
  05-0-render-basic-windowed.cxx
)
target_link_libraries(04-0-render-basic-windowed PRIVATE ${SDL2_LIBRARIES})
target_include_directories(
  04-0-render-basic-windowed
  PRIVATE ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(
  04-0-render-basic-windowed PRIVATE SDL2::SDL2
  SDL2::SDL2main
)
target_compile_features(04-0-render-basic-windowed PUBLIC cxx_std_17)

add_executable(
  04-0-render-full-windowed
  00_canvas_basic.cxx
  01_line_render.cxx
  02_triangle_render.cxx
  03_triangle_indexed_render.cxx
  04-0-render-full-windowed.cxx
  04_triangle_interpolated_render.cxx
  04_triangle_interpolated_render.hxx
)
target_link_libraries(04-0-render-full-windowed PRIVATE ${SDL2_LIBRARIES})
target_include_directories(
  04-0-render-full-windowed
  PRIVATE ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(
  04-0-render-full-windowed PRIVATE SDL2::SDL2
  SDL2::SDL2main
)
target_compile_features(04-0-render-full-windowed PUBLIC cxx_std_17)

add_executable(
  04-0-render-image-windowed
  00_canvas_basic.cxx
  01_line_render.cxx
  02_triangle_render.cxx
  03_triangle_indexed_render.cxx
  04-0-image-windowed.cxx
  04_triangle_interpolated_render.cxx
  04_triangle_interpolated_render.hxx
)
target_link_libraries(04-0-render-image-windowed PRIVATE ${SDL2_LIBRARIES})
target_include_directories(
  04-0-render-image-windowed
  PRIVATE ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(
  04-0-render-image-windowed PRIVATE SDL2::SDL2
  SDL2::SDL2main
)
target_compile_features(04-0-render-image-windowed PUBLIC cxx_std_17)

add_executable(
  04-06-ray-tracing-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  06_ray_tracing_basic.cxx
)
target_compile_features(04-06-ray-tracing-basic PRIVATE cxx_std_17)

add_executable(
  04-07-ray-tracing-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  07_ray_tracing_basic.cxx
)
target_compile_features(04-07-ray-tracing-basic PRIVATE cxx_std_17)

add_executable(
  04-08-ray-tracing-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  08_ray_tracing_basic.cxx
)
target_compile_features(04-08-ray-tracing-basic PRIVATE cxx_std_17)

add_executable(
  04-09-ray-tracing-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  09_ray_tracing_basic.cxx
)
target_compile_features(04-09-ray-tracing-basic PRIVATE cxx_std_17)

add_executable(
  04-10-ray-tracing-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  10_ray_tracing_basic.cxx
)
target_compile_features(04-10-ray-tracing-basic PRIVATE cxx_std_17)

add_executable(
  04-11-ray-tracing-basic 00_canvas_basic.cxx 00_canvas_basic.hxx
  11_ray_tracing_basic.cxx
)
target_compile_features(04-11-ray-tracing-basic PRIVATE cxx_std_17)

add_executable(
  04-12-rasterize_cube
  00_canvas_basic.cxx
  00_canvas_basic.hxx
  01_line_render.cxx
  01_line_render.hxx
  02_triangle_render.cxx
  02_triangle_render.hxx
  03_triangle_indexed_render.cxx
  03_triangle_indexed_render.hxx
  04_triangle_interpolated_render.cxx
  04_triangle_interpolated_render.hxx
  12_rasterize_cube.cxx
)
target_compile_features(04-12-rasterize_cube PRIVATE cxx_std_17)
