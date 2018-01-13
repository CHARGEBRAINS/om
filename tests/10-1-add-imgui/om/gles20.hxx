#pragma once

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

// we have to load all extension GL function pointers
// dynamically from OpenGL library
// so first declare function pointers for all we need
extern PFNGLCREATESHADERPROC             glCreateShader;
extern PFNGLSHADERSOURCEARBPROC          glShaderSource;
extern PFNGLCOMPILESHADERARBPROC         glCompileShader;
extern PFNGLGETSHADERIVPROC              glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC             glDeleteShader;
extern PFNGLCREATEPROGRAMPROC            glCreateProgram;
extern PFNGLATTACHSHADERPROC             glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC       glBindAttribLocation;
extern PFNGLLINKPROGRAMPROC              glLinkProgram;
extern PFNGLGETPROGRAMIVPROC             glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC            glDeleteProgram;
extern PFNGLUSEPROGRAMPROC               glUseProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation;
extern PFNGLUNIFORM1IPROC                glUniform1i;
extern PFNGLACTIVETEXTUREPROC            glActiveTexture_;
extern PFNGLUNIFORM4FVPROC               glUniform4fv;
extern PFNGLUNIFORMMATRIX3FVPROC         glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv;
extern PFNGLBINDBUFFERPROC               glBindBuffer;
extern PFNGLBUFFERDATAPROC               glBufferData;
extern PFNGLGENBUFFERSPROC               glGenBuffers;
extern PFNGLGETATTRIBLOCATIONPROC        glGetAttribLocation;
extern PFNGLBLENDFUNCSEPARATEPROC        glBlendFuncSeparate;
extern PFNGLBLENDEQUATIONSEPARATEPROC    glBlendEquationSeparate;
extern PFNGLDETACHSHADERPROC             glDetachShader;
extern PFNGLDELETEBUFFERSPROC            glDeleteBuffers;
