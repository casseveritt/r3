/* 
 *  gfxcontext
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

#include <GL/Regal.h>
#if TARGET_OS_MAC
#include <GL/RegalCGL.h>
#endif
#include "r3/output.h"
#include "r3/thread.h"

#include "r3/gfxcontext.h"

#include <vector>
#include <map>

using namespace std;
using namespace r3;

#if __APPLE__
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE
struct EAGLContext;
typedef EAGLContext * GlContext;
# define NULL_GL_CONTEXT NULL
# endif

#elif ANDROID
typedef int GlContext;
# define NULL_GL_CONTEXT -1

#elif __linux__
typedef GLXContext GlContext;
#include <stdio.h>

#endif

#if TARGET_OS_IPHONE || ANDROID || __linux__
GlContext GfxGetCurrentContext();
void GfxSetCurrentContext( GlContext context );
GlContext GfxCreateContext( GlContext context );
void GfxDestroyContext( GlContext context );
#endif

namespace {


    map< NativeThread, r3::GfxContext * > threadToContext;
    
#if TARGET_OS_IPHONE || ANDROID
    
    struct PlatformGfxContext : public r3::GfxContext {
    private:
        PlatformGfxContext() {}
        PlatformGfxContext( const PlatformGfxContext & src ) {}
    public:
        PlatformGfxContext( r3::GfxContext * gctx ) {
            if ( gctx ) {
                PlatformGfxContext * ctx = static_cast< PlatformGfxContext * >( gctx );
                owner = true;
                gc = GfxCreateContext( ctx->gc );
            } else {
                owner = false;
                gc = GfxGetCurrentContext();
            }
        }
        
        virtual ~PlatformGfxContext() {
            if( owner ) {
                GfxDestroyContext( gc );
            }
        }
        
        virtual void Acquire() {
            NativeThread thr = ThreadSelf();
            if( threadToContext.count( thr ) ) {
                return;
            }
            //Output( "PlatformGfxContext::Acquire() called." );
            mutex.Acquire( R3_LOC );
            threadToContext[ thr ] = this;
            GfxSetCurrentContext( gc );
        }
        
        virtual void Release() {
            NativeThread thr = ThreadSelf();
            if( threadToContext.count( thr ) != 1 || threadToContext[ thr ] != this ){
                return;
            }
            GfxSetCurrentContext( NULL_GL_CONTEXT );
            threadToContext.erase( thr );
            mutex.Release();
            //Output( "PlatformGfxContext::Release() called." );
        }
        
        virtual void Finish() {
            glFinish();
        }
        
        r3::Mutex mutex;
        bool owner;
        GlContext gc;
    };
#elif TARGET_OS_MAC
//# include <OpenGL/OpenGL.h>
//# include <OpenGL/CGLCurrent.h>
    
    struct PlatformGfxContext : public r3::GfxContext {
        PlatformGfxContext( r3::GfxContext * gctx ) {
            if ( gctx ) {
                PlatformGfxContext * ctx = static_cast< PlatformGfxContext * >( gctx );
                owner = true;
                vector<CGLPixelFormatAttribute> attribs;
                attribs.push_back( kCGLPFAColorSize );
                attribs.push_back( (CGLPixelFormatAttribute)24 );
                attribs.push_back( kCGLPFAAlphaSize );
                attribs.push_back( (CGLPixelFormatAttribute)8 );
                attribs.push_back( (CGLPixelFormatAttribute)0 ); // end of attribs
                CGLPixelFormatObj pix;
                GLint npix;
                CGLError err = CGLChoosePixelFormat( &attribs[0], &pix, &npix );
                if ( err != kCGLNoError ) {
                    Output( "GfxContext constructor failed to choose a pixel format. (%d: %s)", err, CGLErrorString( err ) );
                    return;
                }
                err = CGLCreateContext( pix, ctx->gc, &gc );
                if ( err != kCGLNoError ) {
                    Output( "GfxContext constructor failed to create a context. (%d: %s)", err, CGLErrorString( err ) );					
                }
            } else {
                owner = false;
                gc = CGLGetCurrentContext();
                CGLSetCurrentContext( gc );
            }
        }
        
        virtual ~PlatformGfxContext() {
            if( owner ) {
                CGLDestroyContext( gc );
            }
        }
        
        virtual void Acquire() {
            NativeThread thr = ThreadSelf();
            if( threadToContext.count( thr ) ) {
                //Output( "Thread <%s - %p> already acquired context %p!\n", GetThreadName().c_str(), thr, threadToContext[thr] );
                return;
            }
            mutex.Acquire( R3_LOC );
            threadToContext[ thr ] = this;
            if( thr != lastThread ) {
                //Output( "Thread <%s - %p> acquired context %p\n", GetThreadName().c_str(), thr, this );
            }
            CGLError e = CGLSetCurrentContext( gc );
            if( e != kCGLNoError ) {
                Output( "CGLSetCurrentContext( %p ) error: %d", gc, e );
            }
        }
        
        virtual void Release() {
            NativeThread thr = ThreadSelf();
            if( threadToContext.count( thr ) != 1 || threadToContext[ thr ] != this ){
                if( threadToContext.count( thr ) == 0 ) {
                    //Output( "Thread <%s - %p> does not have a context!\n", GetThreadName().c_str(), thr );
                } else {
                    //Output( "Thread <%s - %p> already acquired this context %p, trying to release %p!\n", GetThreadName().c_str(), thr, threadToContext[thr], this );
                }
                return;
            }
            CGLError e = CGLSetCurrentContext( NULL );
            threadToContext.erase( thr );
            if( e != kCGLNoError ) {
                Output( "CGLSetCurrentContext( NULL ) error: %d", e );            
            }
            if( thr != lastThread ) {
                //Output( "Thread <%s> released context %p\n", GetThreadName().c_str(), this );
            }
            lastThread = thr;
            mutex.Release();
        }
        
        virtual void Finish() {
            glFinish();
        }
        
        r3::Mutex mutex;
        bool owner;
        pthread_t lastThread;
        CGLContextObj gc;
    };
    
#elif _MSC_VER // Windows
    
    struct PlatformGfxContext : public r3::GfxContext {
        PlatformGfxContext( r3::GfxContext * gctx ) {
            if ( gctx ) {
                PlatformGfxContext * ctx = static_cast< PlatformGfxContext * >( gctx );
                owner = true;
                hDC = ctx->hDC;
                hGLRC = wglCreateContext( ctx->hDC );
                wglShareLists( ctx->hGLRC, hGLRC );
            } else {
                owner = false;
                hDC = wglGetCurrentDC();
                hGLRC = wglGetCurrentContext();
            }
        }
        
        virtual ~PlatformGfxContext() {
            if( owner ) {
                wglDeleteContext( hGLRC );
            }
        }
        
        virtual void Acquire() {
            mutex.Acquire( R3_LOC );
            wglMakeCurrent( hDC, hGLRC );
        }
        
        virtual void Release() {
            wglMakeCurrent( NULL, NULL );
            mutex.Release();
        }
        
        virtual void Finish() {
            glFinish();
        }
        
        r3::Mutex mutex;
        bool owner;
        HDC hDC;
        HGLRC hGLRC;
    };
#elif __linux__
    
    struct PlatformGfxContext : public r3::GfxContext {
        PlatformGfxContext( r3::GfxContext * gctx ) {
            if ( gctx ) {
                PlatformGfxContext * ctx = static_cast< PlatformGfxContext * >( gctx );
                owner = true;
                dpy = ctx->dpy;
                int attributes[] = {
                    GLX_RENDER_TYPE, GLX_RGBA_BIT,
                    GLX_RED_SIZE, 8,
                    GLX_GREEN_SIZE, 8,
                    GLX_BLUE_SIZE, 8,
                    GLX_ALPHA_SIZE, 8,
                    GLX_DEPTH_SIZE, 24,
                    None
                };
                
                int nconfigs;
                GLXFBConfig *configs = glXChooseFBConfig( dpy, DefaultScreen( dpy ), attributes, &nconfigs );
                
                if( nconfigs < 1 ) {
                    printf( "No OpenGL framebuffer configurations available!, nconfigs=%d\n", nconfigs );
                }
                
                context = glXCreateNewContext( dpy, *configs, GLX_RGBA_TYPE, ctx->context, True );
            } else {
                owner = false;
                dpy = glXGetCurrentDisplay(); 
                drawable = glXGetCurrentDrawable();
                context = glXGetCurrentContext();
            }
        }
        
        virtual ~PlatformGfxContext() {
            if( owner ) {
                glXDestroyContext( dpy, context );
            }
        }
        
        virtual void Acquire() {
            mutex.Acquire( R3_LOC );
            glXMakeCurrent( dpy, drawable, context );
        }
        
        virtual void Release() {
            glXMakeCurrent( dpy, None, NULL );
            mutex.Release();
        }
        
        virtual void Finish() {
            glFinish();
        }
        
        r3::Mutex mutex;
        bool owner;
        Display *dpy;
        GLXDrawable drawable;
        GLXContext context;
    };
    
#endif	
    
}

namespace r3 {
    
    GfxContext * CreateGfxContext( GfxContext * ctx ) {
        return new PlatformGfxContext( ctx );
    }
    
}

