/*
 *  shader
 *
 */

/* 
 Copyright (c) 2012 Cass Everitt
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

#ifndef __R3_SHADER_H__
#define __R3_SHADER_H__

#include <string>
#include <map>
#include "r3/linear.h"
#include "r3/gl.h"

namespace r3 {
    
    void InitShader();
    void ShutdownShader();
    void AllocShader();
    void DeallocShader();
    
    class Shader;
    
    class ShaderAllocListener {
    public:
        virtual void OnShaderAlloc( Shader * shd ) = 0;
    };
    
    
    class Shader {
        std::string name;
        std::map< std::string, GLint > uniform;
        bool generated;
        ShaderAllocListener *allocListener;
    public:
        GLuint pgObject;
        GLuint vsObject;
        GLuint fsObject;

        Shader( const std::string & shdName );
        
        virtual ~Shader();
        virtual void Alloc();
        virtual void Dealloc();
                
        const std::string & Name() const {
            return name;
        }
        
        void SetAllocListener( ShaderAllocListener * listener ) {
            allocListener = listener;
        }
        
        void CallAllocListener() {
            if( allocListener ) {
                allocListener->OnShaderAlloc( this );
            }
        }
        
        void SetVertexSource( const char *src  );
        
        void SetFragmentSource( const char *src  );
        
        void SetUniform( const std::string & name, GLint i );
        void SetUniform( const std::string & name, const Vec3f & v );
        void SetUniform( const std::string & name, const Matrix4f & m );
    };
    
    Shader * CreateShaderFromFile( const std::string & filename );
}

#endif // __R3_SHADER_H__
