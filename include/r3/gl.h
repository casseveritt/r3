/* 
 *  gl
 *
 */

/* 
 Copyright (c) 2010 Cass Everitt
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:
 
 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the following
 disclaimer.
 
 * Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials
 provided with the distribution.
 
 * The names of contributors to this software may not be used
 to endorse or promote products derived from this software
 without specific prior written permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 POSSIBILITY OF SUCH DAMAGE. 
 
 
 Cass Everitt
 */


#ifndef __R3_GL_H__
#define __R3_GL_H__

#ifdef __APPLE__
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE
#  include <OpenGLES/ES1/gl.h>
#  include <OpenGLES/ES1/glext.h>
#  define R3_GLES1 1
# else
#  include <OpenGL/gl.h>
#  include "r3/GL/glext.h"
# endif

#elif _MSC_VER
# include <windows.h>
# include <GL/gl.h>
# include "r3/GL/glext.h"
# include "r3/GL/entry.h"

#elif ANDROID
# include <GLES/gl.h>
# include <GLES/glext.h>
# define R3_GLES1 1 
#endif

#if R3_GLES1
#  define GL_TEXTURE_1D 0
#  define GL_TEXTURE_3D 0
#  define GL_TEXTURE_CUBE_MAP 0
#  define GL_TEXTURE_LOD_BIAS GL_TEXTURE_LOD_BIAS_EXT
#  define GL_TEXTURE_MIN_LOD 0
#  define GL_TEXTURE_MAX_LOD 0
#  define GL_DEPTH_COMPONENT GL_DEPTH_COMPONENT16_OES
#  define glGenerateMipmapEXT glGenerateMipmapOES
#  define GlInternalFormat GlFormat

#  define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
#  define GL_RENDERBUFFER_EXT GL_RENDERBUFFER_OES
#  define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0_OES
#  define glGenRenderbuffersEXT glGenRenderbuffersOES
#  define glDeleteRenderbuffersEXT glDeleteRenderbuffersOES
#  define glBindRenderbufferEXT glBindRenderbufferOES
#  define glRenderbufferStorageEXT glRenderbufferStorageOES
#  define glGenerateMipmapEXT glGenerateMipmapOES

#  define glOrtho glOrthof

#elif __linux__

#define GL_GLEXT_LEGACY 1
#error "Why am I here?"
#include <GL/gl.h>
#include "r3/GL/entry.h"

#endif


#endif // __R3_GL_H__
