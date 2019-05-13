#include <glm/vec3.hpp>
#include <string>

// you can use this header file directly in your build
// also you can update during runtime this values with
// properties_reader.hxx

bool        enable_depth  = true;
std::string title         = "1-triangles, 2-lines, 3-line-strip, 4-line-loop";
float       z_near        = 0.1f;
float       z_far         = 100.f;
float       fovy          = 45.f;
float       screen_width  = 640.f;
float       screen_height = 480.f;
float       aspect        = screen_width / screen_height;
