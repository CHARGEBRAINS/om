#include "engine.hxx"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include "picopng.hxx"

// we have to load all extension GL function pointers
// dynamically from OpenGL library
// so first declare function pointers for all we need
static PFNGLCREATESHADERPROC             glCreateShader             = nullptr;
static PFNGLSHADERSOURCEARBPROC          glShaderSource             = nullptr;
static PFNGLCOMPILESHADERARBPROC         glCompileShader            = nullptr;
static PFNGLGETSHADERIVPROC              glGetShaderiv              = nullptr;
static PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog         = nullptr;
static PFNGLDELETESHADERPROC             glDeleteShader             = nullptr;
static PFNGLCREATEPROGRAMPROC            glCreateProgram            = nullptr;
static PFNGLATTACHSHADERPROC             glAttachShader             = nullptr;
static PFNGLBINDATTRIBLOCATIONPROC       glBindAttribLocation       = nullptr;
static PFNGLLINKPROGRAMPROC              glLinkProgram              = nullptr;
static PFNGLGETPROGRAMIVPROC             glGetProgramiv             = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog        = nullptr;
static PFNGLDELETEPROGRAMPROC            glDeleteProgram            = nullptr;
static PFNGLUSEPROGRAMPROC               glUseProgram               = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer      = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray  = nullptr;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation       = nullptr;
static PFNGLUNIFORM1IPROC                glUniform1i                = nullptr;
static PFNGLACTIVETEXTUREPROC            glActiveTexture_           = nullptr;
static PFNGLUNIFORM4FVPROC               glUniform4fv               = nullptr;
static PFNGLUNIFORMMATRIX3FVPROC         glUniformMatrix3fv         = nullptr;

template <typename T>
static void load_gl_func(const char* func_name, T& result)
{
    void* gl_pointer = SDL_GL_GetProcAddress(func_name);
    if (nullptr == gl_pointer)
    {
        throw std::runtime_error(std::string("can't load GL function") +
                                 func_name);
    }
    result = reinterpret_cast<T>(gl_pointer);
}

#define OM_GL_CHECK()                                                          \
    {                                                                          \
        const unsigned int err = glGetError();                                 \
        if (err != GL_NO_ERROR)                                                \
        {                                                                      \
            std::cerr << __FILE__ << '(' << __LINE__ - 1 << ") error: ";       \
            switch (err)                                                       \
            {                                                                  \
                case GL_INVALID_ENUM:                                          \
                    std::cerr << "GL_INVALID_ENUM" << std::endl;               \
                    break;                                                     \
                case GL_INVALID_VALUE:                                         \
                    std::cerr << "GL_INVALID_VALUE" << std::endl;              \
                    break;                                                     \
                case GL_INVALID_OPERATION:                                     \
                    std::cerr << "GL_INVALID_OPERATION" << std::endl;          \
                    break;                                                     \
                case GL_INVALID_FRAMEBUFFER_OPERATION:                         \
                    std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"            \
                              << std::endl;                                    \
                    break;                                                     \
                case GL_OUT_OF_MEMORY:                                         \
                    std::cerr << "GL_OUT_OF_MEMORY" << std::endl;              \
                    break;                                                     \
            }                                                                  \
            assert(false);                                                     \
        }                                                                      \
    }

namespace om
{

vec2::vec2()
    : x(0.f)
    , y(0.f)
{
}
vec2::vec2(float x_, float y_)
    : x(x_)
    , y(y_)
{
}

vec2 operator+(const vec2& l, const vec2& r)
{
    vec2 result;
    result.x = l.x + r.x;
    result.y = l.y + r.y;
    return result;
}

mat2x3::mat2x3()
    : col0(1.0f, 0.f)
    , col1(0.f, 1.f)
    , delta(0.f, 0.f)
{
}

mat2x3 mat2x3::identiry()
{
    return mat2x3::scale(1.f);
}

mat2x3 mat2x3::scale(float scale)
{
    mat2x3 result;
    result.col0.x = scale;
    result.col1.y = scale;
    return result;
}

mat2x3 mat2x3::scale(float sx, float sy)
{
    mat2x3 r;
    r.col0.x = sx;
    r.col1.y = sy;
    return r;
}

mat2x3 mat2x3::rotation(float thetha)
{
    mat2x3 result;

    result.col0.x = std::cos(thetha);
    result.col0.y = std::sin(thetha);

    result.col1.x = -std::sin(thetha);
    result.col1.y = std::cos(thetha);

    return result;
}

mat2x3 mat2x3::move(const vec2& delta)
{
    mat2x3 r = mat2x3::identiry();
    r.delta  = delta;
    return r;
}

vec2 operator*(const vec2& v, const mat2x3& m)
{
    vec2 result;
    result.x = v.x * m.col0.x + v.y * m.col0.y + m.delta.x;
    result.y = v.x * m.col1.x + v.y * m.col1.y + m.delta.y;
    return result;
}

mat2x3 operator*(const mat2x3& m1, const mat2x3& m2)
{
    mat2x3 r;

    r.col0.x = m1.col0.x * m2.col0.x + m1.col1.x * m2.col0.y;
    r.col1.x = m1.col0.x * m2.col1.x + m1.col1.x * m2.col1.y;
    r.col0.y = m1.col0.y * m2.col0.x + m1.col1.y * m2.col0.y;
    r.col1.y = m1.col0.y * m2.col1.x + m1.col1.y * m2.col1.y;

    r.delta.x = m1.delta.x * m2.col0.x + m1.delta.y * m2.col0.y + m2.delta.x;
    r.delta.y = m1.delta.x * m2.col1.x + m1.delta.y * m2.col1.y + m2.delta.y;

    return r;
}

texture::~texture()
{
}

vertex_buffer::~vertex_buffer()
{
}

class vertex_buffer_impl final : public vertex_buffer
{
public:
    vertex_buffer_impl(const tri2* tri, std::size_t n)
        : triangles(n)
    {
        assert(tri != nullptr);
        for (size_t i = 0; i < n; ++i)
        {
            triangles[i] = tri[i];
        }
    }
    ~vertex_buffer_impl() final;

    const v2*      data() const final { return &triangles.data()->v[0]; }
    size_t size() const final { return triangles.size() * 3; }

private:
    std::vector<tri2> triangles;
};

static std::string_view get_sound_format_name(uint16_t format_value)
{
    static const std::map<int, std::string_view> format = {
        { AUDIO_U8, "AUDIO_U8" },         { AUDIO_S8, "AUDIO_S8" },
        { AUDIO_U16LSB, "AUDIO_U16LSB" }, { AUDIO_S16LSB, "AUDIO_S16LSB" },
        { AUDIO_U16MSB, "AUDIO_U16MSB" }, { AUDIO_S16MSB, "AUDIO_S16MSB" },
        { AUDIO_S32LSB, "AUDIO_S32LSB" }, { AUDIO_S32MSB, "AUDIO_S32MSB" },
        { AUDIO_F32LSB, "AUDIO_F32LSB" }, { AUDIO_F32MSB, "AUDIO_F32MSB" },
    };

    auto it = format.find(format_value);
    return it->second;
}

static std::size_t get_sound_format_size(uint16_t format_value)
{
    static const std::map<int, std::size_t> format = {
        { AUDIO_U8, 1 },     { AUDIO_S8, 1 },     { AUDIO_U16LSB, 2 },
        { AUDIO_S16LSB, 2 }, { AUDIO_U16MSB, 2 }, { AUDIO_S16MSB, 2 },
        { AUDIO_S32LSB, 4 }, { AUDIO_S32MSB, 4 }, { AUDIO_F32LSB, 4 },
        { AUDIO_F32MSB, 4 },
    };

    auto it = format.find(format_value);
    return it->second;
}

class sound_buffer_impl final : public sound_buffer
{
public:
    sound_buffer_impl(std::string_view path, SDL_AudioDeviceID device,
                      SDL_AudioSpec audio_spec);
    ~sound_buffer_impl() final;

    void play(const properties prop) final
    {
        // Lock callback function
        SDL_LockAudioDevice(device);

        // here we can change properties
        // of sound and dont collade with multithreaded playing
        current_index = 0;
        is_playing    = true;
        is_looped     = (prop == properties::looped);

        // unlock callback for continue mixing of audio
        SDL_UnlockAudioDevice(device);
    }

    std::unique_ptr<uint8_t[]> tmp_buf;
    uint8_t*                   buffer;
    uint32_t                   length;
    uint32_t                   current_index = 0;
    SDL_AudioDeviceID          device;
    bool                       is_playing = false;
    bool                       is_looped  = false;
};

sound_buffer_impl::sound_buffer_impl(std::string_view  path,
                                     SDL_AudioDeviceID device_,
                                     SDL_AudioSpec     device_audio_spec)
    : buffer(nullptr)
    , length(0)
    , device(device_)
{
    SDL_RWops* file = SDL_RWFromFile(path.data(), "rb");
    if (file == nullptr)
    {
        throw std::runtime_error(std::string("can't open audio file: ") +
                                 path.data());
    }

    // freq, format, channels, and samples - used by SDL_LoadWAV_RW
    SDL_AudioSpec file_audio_spec;

    if (nullptr == SDL_LoadWAV_RW(file, 1, &file_audio_spec, &buffer, &length))
    {
        throw std::runtime_error(std::string("can't load wav: ") + path.data());
    }

    std::cout << "audio format for: " << path << '\n'
              << "format: " << get_sound_format_name(file_audio_spec.format)
              << '\n'
              << "sample_size: "
              << get_sound_format_size(file_audio_spec.format) << '\n'
              << "channels: " << static_cast<uint32_t>(file_audio_spec.channels)
              << '\n'
              << "frequency: " << file_audio_spec.freq << '\n'
              << "length: " << length << '\n'
              << "time: "
              << static_cast<double>(length) /
                     (file_audio_spec.channels * file_audio_spec.freq *
                      get_sound_format_size(file_audio_spec.format))
              << "sec" << std::endl;

    if (file_audio_spec.channels != device_audio_spec.channels ||
        file_audio_spec.format != device_audio_spec.format ||
        file_audio_spec.freq != device_audio_spec.freq)
    {
        SDL_AudioCVT cvt;
        SDL_BuildAudioCVT(&cvt, file_audio_spec.format,
                          file_audio_spec.channels, file_audio_spec.freq,
                          device_audio_spec.format, device_audio_spec.channels,
                          device_audio_spec.freq);
        SDL_assert(cvt.needed); // obviously, this one is always needed.
        // read your data into cvt.buf here.
        cvt.len = length;
        // we have to make buffer for inplace conversion
        tmp_buf.reset(new uint8_t[cvt.len * cvt.len_mult]);
        uint8_t* buf = tmp_buf.get();
        std::copy_n(buffer, length, buf);
        cvt.buf = buf;
        if (0 != SDL_ConvertAudio(&cvt))
        {
            std::cout << "failed to convert audio from file: " << path
                      << " to audio device format" << std::endl;
        }
        // cvt.buf has cvt.len_cvt bytes of converted data now.
        SDL_FreeWAV(buffer);
        buffer = tmp_buf.get();
        length = cvt.len_cvt;
    }
}

sound_buffer::~sound_buffer()
{
}

sound_buffer_impl::~sound_buffer_impl()
{
    if (!tmp_buf)
    {
        SDL_FreeWAV(buffer);
    }
    buffer = nullptr;
    length = 0;
}

vertex_buffer_impl::~vertex_buffer_impl()
{
}

class texture_gl_es20 final : public texture
{
public:
    explicit texture_gl_es20(std::string_view path);
    ~texture_gl_es20() override;

    void bind() const
    {
        GLboolean is_texture = glIsTexture(tex_handl);
        assert(is_texture);
        OM_GL_CHECK();
        glBindTexture(GL_TEXTURE_2D, tex_handl);
        OM_GL_CHECK();
    }

    std::uint32_t get_width() const final { return width; }
    std::uint32_t get_height() const final { return height; }

private:
    std::string   file_path;
    GLuint        tex_handl = 0;
    std::uint32_t width     = 0;
    std::uint32_t height    = 0;
};

class shader_gl_es20
{
public:
    shader_gl_es20(
        std::string_view vertex_src, std::string_view         fragment_src,
        const std::vector<std::tuple<GLuint, const GLchar*>>& attributes)
    {
        vert_shader = compile_shader(GL_VERTEX_SHADER, vertex_src);
        frag_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_src);
        if (vert_shader == 0 || frag_shader == 0)
        {
            throw std::runtime_error("can't compile shader");
        }
        program_id = link_shader_program(attributes);
        if (program_id == 0)
        {
            throw std::runtime_error("can't link shader");
        }
    }

    void use() const
    {
        glUseProgram(program_id);
        OM_GL_CHECK();
    }

    void set_uniform(std::string_view uniform_name, texture_gl_es20* texture)
    {
        assert(texture != nullptr);
        const int location =
            glGetUniformLocation(program_id, uniform_name.data());
        OM_GL_CHECK();
        if (location == -1)
        {
            std::cerr << "can't get uniform location from shader\n";
            throw std::runtime_error("can't get uniform location");
        }
        unsigned int texture_unit = 0;
        glActiveTexture_(GL_TEXTURE0 + texture_unit);
        OM_GL_CHECK();

        texture->bind();

        // http://www.khronos.org/opengles/sdk/docs/man/xhtml/glUniform.xml
        glUniform1i(location, static_cast<int>(0 + texture_unit));
        OM_GL_CHECK();
    }

    void set_uniform(std::string_view uniform_name, const color& c)
    {
        const int location =
            glGetUniformLocation(program_id, uniform_name.data());
        OM_GL_CHECK();
        if (location == -1)
        {
            std::cerr << "can't get uniform location from shader\n";
            throw std::runtime_error("can't get uniform location");
        }
        float values[4] = { c.get_r(), c.get_g(), c.get_b(), c.get_a() };
        glUniform4fv(location, 1, &values[0]);
        OM_GL_CHECK();
    }

    void set_uniform(std::string_view uniform_name, const mat2x3& m)
    {
        const int location =
            glGetUniformLocation(program_id, uniform_name.data());
        OM_GL_CHECK();
        if (location == -1)
        {
            std::cerr << "can't get uniform location from shader\n";
            throw std::runtime_error("can't get uniform location");
        }
        // OpenGL wants matrix in column major order
        // clang-format off
        float values[9] = { m.col0.x,  m.col0.y, m.delta.x,
                            m.col1.x, m.col1.y, m.delta.y,
                            0.f,      0.f,       1.f };
        // clang-format on
        glUniformMatrix3fv(location, 1, GL_FALSE, &values[0]);
        OM_GL_CHECK();
    }

private:
    GLuint compile_shader(GLenum shader_type, std::string_view src)
    {
        GLuint shader_id = glCreateShader(shader_type);
        OM_GL_CHECK();
        std::string_view vertex_shader_src = src;
        const char*      source            = vertex_shader_src.data();
        glShaderSource(shader_id, 1, &source, nullptr);
        OM_GL_CHECK();

        glCompileShader(shader_id);
        OM_GL_CHECK();

        GLint compiled_status = 0;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_status);
        OM_GL_CHECK();
        if (compiled_status == 0)
        {
            GLint info_len = 0;
            glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_len);
            OM_GL_CHECK();
            std::vector<char> info_chars(static_cast<size_t>(info_len));
            glGetShaderInfoLog(shader_id, info_len, NULL, info_chars.data());
            OM_GL_CHECK();
            glDeleteShader(shader_id);
            OM_GL_CHECK();

            std::string shader_type_name =
                shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment";
            std::cerr << "Error compiling shader(vertex)\n"
                      << vertex_shader_src << "\n"
                      << info_chars.data();
            return 0;
        }
        return shader_id;
    }
    GLuint link_shader_program(
        const std::vector<std::tuple<GLuint, const GLchar*>>& attributes)
    {
        GLuint program_id_ = glCreateProgram();
        OM_GL_CHECK();
        if (0 == program_id_)
        {
            std::cerr << "failed to create gl program";
            throw std::runtime_error("can't link shader");
        }

        glAttachShader(program_id_, vert_shader);
        OM_GL_CHECK();
        glAttachShader(program_id_, frag_shader);
        OM_GL_CHECK();

        // bind attribute location
        for (const auto& attr : attributes)
        {
            GLuint        loc  = std::get<0>(attr);
            const GLchar* name = std::get<1>(attr);
            glBindAttribLocation(program_id_, loc, name);
            OM_GL_CHECK();
        }

        // link program after binding attribute locations
        glLinkProgram(program_id_);
        OM_GL_CHECK();
        // Check the link status
        GLint linked_status = 0;
        glGetProgramiv(program_id_, GL_LINK_STATUS, &linked_status);
        OM_GL_CHECK();
        if (linked_status == 0)
        {
            GLint infoLen = 0;
            glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &infoLen);
            OM_GL_CHECK();
            std::vector<char> infoLog(static_cast<size_t>(infoLen));
            glGetProgramInfoLog(program_id_, infoLen, NULL, infoLog.data());
            OM_GL_CHECK();
            std::cerr << "Error linking program:\n" << infoLog.data();
            glDeleteProgram(program_id_);
            OM_GL_CHECK();
            return 0;
        }
        return program_id_;
    }

    GLuint vert_shader = 0;
    GLuint frag_shader = 0;
    GLuint program_id  = 0;
};

std::ostream& operator<<(std::ostream& stream, const input_data& i)
{
    static const std::array<std::string_view, 8> key_names = {
        { "left", "right", "up", "down", "select", "start", "button1",
          "button2" }
    };

    const std::string_view& key_name = key_names[static_cast<size_t>(i.key)];

    stream << "key: " << key_name << " is_down: " << i.is_down;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const hardware_data& h)
{
    stream << "reset console: " << h.is_reset;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const event e)
{
    switch (e.type)
    {
        case om::event_type::input_key:
            stream << std::get<om::input_data>(e.info);
            break;
        case om::event_type::hardware:
            stream << std::get<om::hardware_data>(e.info);
            break;
    };
    return stream;
}

tri0::tri0()
    : v{ v0(), v0(), v0() }
{
}

tri1::tri1()
    : v{ v1(), v1(), v1() }
{
}

tri2::tri2()
    : v{ v2(), v2(), v2() }
{
}

std::ostream& operator<<(std::ostream& out, const SDL_version& v)
{
    out << static_cast<int>(v.major) << '.';
    out << static_cast<int>(v.minor) << '.';
    out << static_cast<int>(v.patch);
    return out;
}

std::istream& operator>>(std::istream& is, mat2x3& m)
{
    is >> m.col0.x;
    is >> m.col1.x;
    is >> m.col0.y;
    is >> m.col1.y;
    return is;
}

std::istream& operator>>(std::istream& is, vec2& v)
{
    is >> v.x;
    is >> v.y;
    return is;
}

std::istream& operator>>(std::istream& is, color& c)
{
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    float a = 0.f;
    is >> r;
    is >> g;
    is >> b;
    is >> a;
    c = color(r, g, b, a);
    return is;
}

std::istream& operator>>(std::istream& is, v0& v)
{
    is >> v.pos.x;
    is >> v.pos.y;

    return is;
}

std::istream& operator>>(std::istream& is, v1& v)
{
    is >> v.pos.x;
    is >> v.pos.y;
    is >> v.c;
    return is;
}

std::istream& operator>>(std::istream& is, v2& v)
{
    is >> v.pos.x;
    is >> v.pos.y;
    is >> v.uv;
    is >> v.c;
    return is;
}

std::istream& operator>>(std::istream& is, tri0& t)
{
    is >> t.v[0];
    is >> t.v[1];
    is >> t.v[2];
    return is;
}

std::istream& operator>>(std::istream& is, tri1& t)
{
    is >> t.v[0];
    is >> t.v[1];
    is >> t.v[2];
    return is;
}

std::istream& operator>>(std::istream& is, tri2& t)
{
    is >> t.v[0];
    is >> t.v[1];
    is >> t.v[2];
    return is;
}

struct bind
{
    bind(std::string_view s, SDL_Keycode k, keys om_k)
        : name(s)
        , key(k)
        , om_key(om_k)
    {
    }

    std::string_view name;
    SDL_Keycode      key;

    om::keys om_key;
};

const std::array<bind, 8> keys{
    { bind{ "up", SDLK_w, keys::up }, bind{ "left", SDLK_a, keys::left },
      bind{ "down", SDLK_s, keys::down }, bind{ "right", SDLK_d, keys::right },
      bind{ "button1", SDLK_LCTRL, keys::button1 },
      bind{ "button2", SDLK_SPACE, keys::button2 },
      bind{ "select", SDLK_ESCAPE, keys::select },
      bind{ "start", SDLK_RETURN, keys::start } }
};

static bool check_input(const SDL_Event& e, const bind*& result)
{
    using namespace std;

    const auto it = find_if(begin(keys), end(keys), [&](const bind& b) {
        return b.key == e.key.keysym.sym;
    });

    if (it != end(keys))
    {
        result = &(*it);
        return true;
    }
    return false;
}

class engine_impl final : public engine
{
public:
    /// create main window
    /// on success return empty string
    std::string initialize(std::string_view /*config*/) final;
    /// return seconds from initialization
    float get_time_from_init() final
    {
        std::uint32_t ms_from_library_initialization = SDL_GetTicks();
        float         seconds = ms_from_library_initialization * 0.001f;
        return seconds;
    }
    /// pool event from input queue
    /// return true if more events in queue
    bool read_event(event& e) final
    {
        using namespace std;
        // collect all events from SDL
        SDL_Event sdl_event;
        if (SDL_PollEvent(&sdl_event))
        {
            const bind* binding = nullptr;

            if (sdl_event.type == SDL_QUIT)
            {
                e.info      = om::hardware_data{ true };
                e.timestamp = sdl_event.common.timestamp * 0.001;
                e.type      = om::event_type::hardware;
                return true;
            }
            else if (sdl_event.type == SDL_KEYDOWN ||
                     sdl_event.type == SDL_KEYUP)
            {
                if (check_input(sdl_event, binding))
                {
                    bool is_down = sdl_event.type == SDL_KEYDOWN;
                    e.info       = om::input_data{ binding->om_key, is_down };
                    e.timestamp  = sdl_event.common.timestamp * 0.001;
                    e.type       = om::event_type::input_key;
                    return true;
                }
            }
        }
        return false;
    }

    bool is_key_down(const enum keys key) final
    {
        const auto it =
            std::find_if(begin(keys), end(keys),
                         [&](const bind& b) { return b.om_key == key; });

        if (it != end(keys))
        {
            const std::uint8_t* state         = SDL_GetKeyboardState(nullptr);
            int                 sdl_scan_code = SDL_GetScancodeFromKey(it->key);
            return state[sdl_scan_code];
        }
        return false;
    }

    texture* create_texture(std::string_view path) final
    {
        return new texture_gl_es20(path);
    }
    void destroy_texture(texture* t) final { delete t; }

    vertex_buffer* create_vertex_buffer(const tri2* triangles, std::size_t n)
    {
        return new vertex_buffer_impl(triangles, n);
    }
    void destroy_vertex_buffer(vertex_buffer* buffer) { delete buffer; }

    sound_buffer* create_sound_buffer(std::string_view path) final
    {
        SDL_LockAudioDevice(audio_device);
        sound_buffer_impl* s =
            new sound_buffer_impl(path, audio_device, audio_device_spec);
        sounds.push_back(s);
        SDL_UnlockAudioDevice(audio_device);
        return s;
    }
    void destroy_sound_buffer(sound_buffer* sound) final { delete sound; }

    void render(const tri0& t, const color& c) final
    {
        shader00->use();
        shader00->set_uniform("u_color", c);
        // vertex coordinates
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(v0),
                              &t.v[0].pos.x);
        OM_GL_CHECK();
        glEnableVertexAttribArray(0);
        OM_GL_CHECK();

        glDrawArrays(GL_TRIANGLES, 0, 3);
        OM_GL_CHECK();
    }
    void render(const tri1& t) final
    {
        shader01->use();
        // positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(t.v[0]),
                              &t.v[0].pos);
        OM_GL_CHECK();
        glEnableVertexAttribArray(0);
        OM_GL_CHECK();
        // colors
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(t.v[0]),
                              &t.v[0].c);
        OM_GL_CHECK();
        glEnableVertexAttribArray(1);
        OM_GL_CHECK();

        glDrawArrays(GL_TRIANGLES, 0, 3);
        OM_GL_CHECK();

        glDisableVertexAttribArray(1);
        OM_GL_CHECK();
    }
    void render(const tri2& t, texture* tex) final
    {
        shader02->use();
        texture_gl_es20* texture = static_cast<texture_gl_es20*>(tex);
        texture->bind();
        shader02->set_uniform("s_texture", texture);
        // positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(t.v[0]),
                              &t.v[0].pos);
        OM_GL_CHECK();
        glEnableVertexAttribArray(0);
        OM_GL_CHECK();
        // colors
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(t.v[0]),
                              &t.v[0].c);
        OM_GL_CHECK();
        glEnableVertexAttribArray(1);
        OM_GL_CHECK();

        // texture coordinates
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(t.v[0]),
                              &t.v[0].uv);
        OM_GL_CHECK();
        glEnableVertexAttribArray(2);
        OM_GL_CHECK();

        glDrawArrays(GL_TRIANGLES, 0, 3);
        OM_GL_CHECK();

        glDisableVertexAttribArray(1);
        OM_GL_CHECK();
        glDisableVertexAttribArray(2);
        OM_GL_CHECK();
    }
    void render(const tri2& t, texture* tex, const mat2x3& m) final
    {
        shader03->use();
        texture_gl_es20* texture = static_cast<texture_gl_es20*>(tex);
        texture->bind();
        shader03->set_uniform("s_texture", texture);
        shader03->set_uniform("u_matrix", m);
        // positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(t.v[0]),
                              &t.v[0].pos);
        OM_GL_CHECK();
        glEnableVertexAttribArray(0);
        OM_GL_CHECK();
        // colors
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(t.v[0]),
                              &t.v[0].c);
        OM_GL_CHECK();
        glEnableVertexAttribArray(1);
        OM_GL_CHECK();

        // texture coordinates
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(t.v[0]),
                              &t.v[0].uv);
        OM_GL_CHECK();
        glEnableVertexAttribArray(2);
        OM_GL_CHECK();

        glDrawArrays(GL_TRIANGLES, 0, 3);
        OM_GL_CHECK();

        glDisableVertexAttribArray(1);
        OM_GL_CHECK();
        glDisableVertexAttribArray(2);
        OM_GL_CHECK();
    }
    void render(const vertex_buffer& buff, texture* tex, const mat2x3& m) final
    {
        shader03->use();
        texture_gl_es20* texture = static_cast<texture_gl_es20*>(tex);
        texture->bind();
        shader03->set_uniform("s_texture", texture);
        shader03->set_uniform("u_matrix", m);

        const v2* t = buff.data();
        // positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(v2), &t->pos);
        OM_GL_CHECK();
        glEnableVertexAttribArray(0);
        OM_GL_CHECK();
        // colors
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(v2),
                              &t->c);
        OM_GL_CHECK();
        glEnableVertexAttribArray(1);
        OM_GL_CHECK();

        // texture coordinates
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(v2), &t->uv);
        OM_GL_CHECK();
        glEnableVertexAttribArray(2);
        OM_GL_CHECK();

        GLsizei num_of_vertexes = static_cast<GLsizei>(buff.size());
        glDrawArrays(GL_TRIANGLES, 0, num_of_vertexes);
        OM_GL_CHECK();

        glDisableVertexAttribArray(1);
        OM_GL_CHECK();
        glDisableVertexAttribArray(2);
        OM_GL_CHECK();
    }
    void swap_buffers() final
    {
        SDL_GL_SwapWindow(window);

        glClear(GL_COLOR_BUFFER_BIT);
        OM_GL_CHECK();
    }
    void uninitialize() final
    {
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

private:
    static void audio_callback(void*, uint8_t*, int);

    SDL_Window*   window     = nullptr;
    SDL_GLContext gl_context = nullptr;

    shader_gl_es20* shader00 = nullptr;
    shader_gl_es20* shader01 = nullptr;
    shader_gl_es20* shader02 = nullptr;
    shader_gl_es20* shader03 = nullptr;

    SDL_AudioDeviceID               audio_device;
    SDL_AudioSpec                   audio_device_spec;
    std::vector<sound_buffer_impl*> sounds;
};

static bool already_exist = false;

engine* create_engine()
{
    if (already_exist)
    {
        throw std::runtime_error("engine already exist");
    }
    engine* result = new engine_impl();
    already_exist  = true;
    return result;
}

void destroy_engine(engine* e)
{
    if (already_exist == false)
    {
        throw std::runtime_error("engine not created");
    }
    if (nullptr == e)
    {
        throw std::runtime_error("e is nullptr");
    }
    delete e;
}

color::color(std::uint32_t rgba_)
    : rgba(rgba_)
{
}
color::color(float r, float g, float b, float a)
{
    assert(r <= 1 && r >= 0);
    assert(g <= 1 && g >= 0);
    assert(b <= 1 && b >= 0);
    assert(a <= 1 && a >= 0);

    std::uint32_t r_ = static_cast<std::uint32_t>(r * 255);
    std::uint32_t g_ = static_cast<std::uint32_t>(g * 255);
    std::uint32_t b_ = static_cast<std::uint32_t>(b * 255);
    std::uint32_t a_ = static_cast<std::uint32_t>(a * 255);

    rgba = a_ << 24 | b_ << 16 | g_ << 8 | r_;
}

float color::get_r() const
{
    std::uint32_t r_ = (rgba & 0x000000FF) >> 0;
    return r_ / 255.f;
}
float color::get_g() const
{
    std::uint32_t g_ = (rgba & 0x0000FF00) >> 8;
    return g_ / 255.f;
}
float color::get_b() const
{
    std::uint32_t b_ = (rgba & 0x00FF0000) >> 16;
    return b_ / 255.f;
}
float color::get_a() const
{
    std::uint32_t a_ = (rgba & 0xFF000000) >> 24;
    return a_ / 255.f;
}

void color::set_r(const float r)
{
    std::uint32_t r_ = static_cast<std::uint32_t>(r * 255);
    rgba &= 0xFFFFFF00;
    rgba |= (r_ << 0);
}
void color::set_g(const float g)
{
    std::uint32_t g_ = static_cast<std::uint32_t>(g * 255);
    rgba &= 0xFFFF00FF;
    rgba |= (g_ << 8);
}
void color::set_b(const float b)
{
    std::uint32_t b_ = static_cast<std::uint32_t>(b * 255);
    rgba &= 0xFF00FFFF;
    rgba |= (b_ << 16);
}
void color::set_a(const float a)
{
    std::uint32_t a_ = static_cast<std::uint32_t>(a * 255);
    rgba &= 0x00FFFFFF;
    rgba |= a_ << 24;
}

engine::~engine()
{
}

texture_gl_es20::texture_gl_es20(std::string_view path)
    : file_path(path)
{
    std::vector<unsigned char> png_file_in_memory;
    std::ifstream              ifs(path.data(), std::ios_base::binary);
    if (!ifs)
    {
        throw std::runtime_error("can't load texture");
    }
    ifs.seekg(0, std::ios_base::end);
    std::streamoff pos_in_file = ifs.tellg();
    png_file_in_memory.resize(static_cast<size_t>(pos_in_file));
    ifs.seekg(0, std::ios_base::beg);
    if (!ifs)
    {
        throw std::runtime_error("can't load texture");
    }

    ifs.read(reinterpret_cast<char*>(png_file_in_memory.data()), pos_in_file);
    if (!ifs.good())
    {
        throw std::runtime_error("can't load texture");
    }

    const om::png_image img = decode_png_file_from_memory(
        png_file_in_memory, convert_color::to_rgba32,
        origin_point::bottom_left);

    // if there's an error, display it
    if (img.error != 0)
    {
        std::cerr << "error: " << img.error << std::endl;
        throw std::runtime_error("can't load texture");
    }

    glGenTextures(1, &tex_handl);
    OM_GL_CHECK();
    glBindTexture(GL_TEXTURE_2D, tex_handl);
    OM_GL_CHECK();

    GLint   mipmap_level = 0;
    GLint   border       = 0;
    GLsizei width_       = static_cast<GLsizei>(img.width);
    GLsizei height_      = static_cast<GLsizei>(img.height);
    glTexImage2D(GL_TEXTURE_2D, mipmap_level, GL_RGBA, width_, height_, border,
                 GL_RGBA, GL_UNSIGNED_BYTE, &img.raw_image[0]);
    OM_GL_CHECK();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    OM_GL_CHECK();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    OM_GL_CHECK();
}

texture_gl_es20::~texture_gl_es20()
{
    glDeleteTextures(1, &tex_handl);
    OM_GL_CHECK();
}

std::string engine_impl::initialize(std::string_view)
{
    using namespace std;

    stringstream serr;

    SDL_version compiled = { 0, 0, 0 };
    SDL_version linked   = { 0, 0, 0 };

    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);

    if (SDL_COMPILEDVERSION !=
        SDL_VERSIONNUM(linked.major, linked.minor, linked.patch))
    {
        serr << "warning: SDL2 compiled and linked version mismatch: "
             << compiled << " " << linked << endl;
    }

    const int init_result = SDL_Init(
        SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS |
        SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
    if (init_result != 0)
    {
        const char* err_message = SDL_GetError();
        serr << "error: failed call SDL_Init: " << err_message << endl;
        return serr.str();
    }

    cout << "initialize om::engine" << std::endl;
    if (std::string_view("Windows") == SDL_GetPlatform())
    {
        if (!cout)
        {
#ifdef _WIN32
            AllocConsole();
#endif
            std::freopen("CON", "w", stdout);
            cout.clear();
            cerr << "test" << std::endl;
            if (!cout)
            {
                throw std::runtime_error("can't print with std::cout");
            }
        }
    }

    window =
        SDL_CreateWindow("title", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 640, 480, ::SDL_WINDOW_OPENGL);

    if (window == nullptr)
    {
        const char* err_message = SDL_GetError();
        serr << "error: failed call SDL_CreateWindow: " << err_message << endl;
        SDL_Quit();
        return serr.str();
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        std::string msg("can't create opengl context: ");
        msg += SDL_GetError();
        serr << msg << endl;
        return serr.str();
    }

    int gl_major_ver = 0;
    int result =
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_major_ver);
    assert(result == 0);
    int gl_minor_ver = 0;
    result = SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gl_minor_ver);
    assert(result == 0);

    if (gl_major_ver <= 2 && gl_minor_ver < 1)
    {
        serr << "current context opengl version: " << gl_major_ver << '.'
             << gl_minor_ver << '\n'
             << "need opengl version at least: 2.1\n"
             << std::flush;
        return serr.str();
    }
    try
    {
        load_gl_func("glCreateShader", glCreateShader);
        load_gl_func("glShaderSource", glShaderSource);
        load_gl_func("glCompileShader", glCompileShader);
        load_gl_func("glGetShaderiv", glGetShaderiv);
        load_gl_func("glGetShaderInfoLog", glGetShaderInfoLog);
        load_gl_func("glDeleteShader", glDeleteShader);
        load_gl_func("glCreateProgram", glCreateProgram);
        load_gl_func("glAttachShader", glAttachShader);
        load_gl_func("glBindAttribLocation", glBindAttribLocation);
        load_gl_func("glLinkProgram", glLinkProgram);
        load_gl_func("glGetProgramiv", glGetProgramiv);
        load_gl_func("glGetProgramInfoLog", glGetProgramInfoLog);
        load_gl_func("glDeleteProgram", glDeleteProgram);
        load_gl_func("glUseProgram", glUseProgram);
        load_gl_func("glVertexAttribPointer", glVertexAttribPointer);
        load_gl_func("glEnableVertexAttribArray", glEnableVertexAttribArray);
        load_gl_func("glDisableVertexAttribArray", glDisableVertexAttribArray);
        load_gl_func("glGetUniformLocation", glGetUniformLocation);
        load_gl_func("glUniform1i", glUniform1i);
        load_gl_func("glActiveTexture", glActiveTexture_);
        load_gl_func("glUniform4fv", glUniform4fv);
        load_gl_func("glUniformMatrix3fv", glUniformMatrix3fv);
    }
    catch (std::exception& ex)
    {
        return ex.what();
    }

    shader00 = new shader_gl_es20(R"(
                                  attribute vec2 a_position;
                                  void main()
                                  {
                                  gl_Position = vec4(a_position, 0.0, 1.0);
                                  }
                                  )",
                                  R"(
                                  uniform vec4 u_color;
                                  void main()
                                  {
                                  gl_FragColor = u_color;
                                  }
                                  )",
                                  { { 0, "a_position" } });

    shader00->use();
    shader00->set_uniform("u_color", color(1.f, 0.f, 0.f, 1.f));

    shader01 = new shader_gl_es20(
        R"(
                attribute vec2 a_position;
                attribute vec4 a_color;
                varying vec4 v_color;
                void main()
                {
                v_color = a_color;
                gl_Position = vec4(a_position, 0.0, 1.0);
                }
                )",
        R"(
                varying vec4 v_color;
                void main()
                {
                gl_FragColor = v_color;
                }
                )",
        { { 0, "a_position" }, { 1, "a_color" } });

    shader01->use();

    shader02 = new shader_gl_es20(
        R"(
                attribute vec2 a_position;
                attribute vec2 a_tex_coord;
                attribute vec4 a_color;
                varying vec4 v_color;
                varying vec2 v_tex_coord;
                void main()
                {
                v_tex_coord = a_tex_coord;
                v_color = a_color;
                gl_Position = vec4(a_position, 0.0, 1.0);
                }
                )",
        R"(
                varying vec2 v_tex_coord;
                varying vec4 v_color;
                uniform sampler2D s_texture;
                void main()
                {
                gl_FragColor = texture2D(s_texture, v_tex_coord) * v_color;
                }
                )",
        { { 0, "a_position" }, { 1, "a_color" }, { 2, "a_tex_coord" } });

    // turn on rendering with just created shader program
    shader02->use();

    shader03 = new shader_gl_es20(
        R"(
                uniform mat3 u_matrix;
                attribute vec2 a_position;
                attribute vec2 a_tex_coord;
                attribute vec4 a_color;
                varying vec4 v_color;
                varying vec2 v_tex_coord;
                void main()
                {
                v_tex_coord = a_tex_coord;
                v_color = a_color;
                vec3 pos = vec3(a_position, 1.0) * u_matrix;
                gl_Position = vec4(pos, 1.0);
                }
                )",
        R"(
                varying vec2 v_tex_coord;
                varying vec4 v_color;
                uniform sampler2D s_texture;
                void main()
                {
                gl_FragColor = texture2D(s_texture, v_tex_coord) * v_color;
                }
                )",
        { { 0, "a_position" }, { 1, "a_color" }, { 2, "a_tex_coord" } });
    shader03->use();

    glEnable(GL_BLEND);
    OM_GL_CHECK();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    OM_GL_CHECK();

    glClearColor(0.f, 0.0, 0.f, 0.0f);
    OM_GL_CHECK();

    glViewport(0, 0, 640, 480);
    OM_GL_CHECK();

    // initialize audio
    audio_device_spec.freq     = 48000;
    audio_device_spec.format   = AUDIO_S16LSB;
    audio_device_spec.channels = 2;
    audio_device_spec.samples  = 1024; // must be power of 2
    audio_device_spec.callback = engine_impl::audio_callback;
    audio_device_spec.userdata = this;

    const int num_audio_drivers = SDL_GetNumAudioDrivers();
    for (int i = 0; i < num_audio_drivers; ++i)
    {
        std::cout << "audio_driver #:" << i << " " << SDL_GetAudioDriver(i)
                  << '\n';
    }
    std::cout << std::flush;

    // TODO on windows 10 only directsound - works for me
    if (std::string_view("Windows") == SDL_GetPlatform())
    {
        const char* selected_audio_driver = SDL_GetAudioDriver(1);
        std::cout << "selected_audio_driver: " << selected_audio_driver
                  << std::endl;

        if (0 != SDL_AudioInit(selected_audio_driver))
        {
            std::cout << "can't init SDL audio\n" << std::flush;
        }
    }

    const char* default_audio_device_name = nullptr;

    const int num_audio_devices = SDL_GetNumAudioDevices(SDL_FALSE);
    if (num_audio_devices > 0)
    {
        default_audio_device_name = SDL_GetAudioDeviceName(0, SDL_FALSE);
        for (int i = 0; i < num_audio_devices; ++i)
        {
            std::cout << "audio device #" << i << ": "
                      << SDL_GetAudioDeviceName(i, SDL_FALSE) << '\n';
        }
    }
    std::cout << std::flush;

    audio_device =
        SDL_OpenAudioDevice(default_audio_device_name, 0, &audio_device_spec,
                            nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (audio_device == 0)
    {
        std::cerr << "failed open audio device: " << SDL_GetError();
        throw std::runtime_error("audio failed");
    }
    else
    {
        std::cout << "audio device selected: " << default_audio_device_name
                  << '\n'
                  << "freq: " << audio_device_spec.freq << '\n'
                  << "format: "
                  << get_sound_format_name(audio_device_spec.format) << '\n'
                  << "channels: "
                  << static_cast<uint32_t>(audio_device_spec.channels) << '\n'
                  << "samples: " << audio_device_spec.samples << '\n'
                  << std::flush;

        // unpause device
        SDL_PauseAudioDevice(audio_device, SDL_FALSE);
    }

    return "";
}

void engine_impl::audio_callback(void* engine_ptr, uint8_t* stream,
                                 int stream_size)
{
    // no sound default
    std::fill_n(stream, stream_size, '\0');

    engine_impl* e = static_cast<engine_impl*>(engine_ptr);

    for (sound_buffer_impl* snd : e->sounds)
    {
        if (snd->is_playing)
        {
            uint32_t rest         = snd->length - snd->current_index;
            uint8_t* current_buff = &snd->buffer[snd->current_index];

            if (rest <= static_cast<uint32_t>(stream_size))
            {
                // copy rest to buffer
                SDL_MixAudioFormat(stream, current_buff,
                                   e->audio_device_spec.format, rest,
                                   SDL_MIX_MAXVOLUME);
                snd->current_index += rest;
            }
            else
            {
                SDL_MixAudioFormat(
                    stream, current_buff, e->audio_device_spec.format,
                    static_cast<uint32_t>(stream_size), SDL_MIX_MAXVOLUME);
                snd->current_index += static_cast<uint32_t>(stream_size);
            }

            if (snd->current_index == snd->length)
            {
                if (snd->is_looped)
                {
                    // start from begining
                    snd->current_index = 0;
                }
                else
                {
                    snd->is_playing = false;
                }
            }
        }
    }
}

} // end namespace om
