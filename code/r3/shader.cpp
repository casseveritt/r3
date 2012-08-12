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

#include "r3/shader.h"

#include "r3/common.h"
#include "r3/command.h"
#include "r3/filesystem.h"
#include "r3/output.h"

#include <GL/Regal.h>

#include <map>

using namespace std;
using namespace r3;

namespace {
    
    struct ShaderDatabase {
        map< string, Shader * > shaders;
        ~ShaderDatabase() {
            while( shaders.size() > 0 ) {
                delete shaders.begin()->second;
            }
        }
        Shader * GetShader( const string & name ) {
            if ( shaders.count( name ) ) {
                return shaders[ name ];
            }
            return NULL;
        }
        void AddShader( const string & name, Shader * shd ) {
            r3Assert( shd != NULL );
            r3Assert( shaders.count( name ) == 0 );
            shaders[ name ] = shd;
        }
        void DeleteShader( const string & name ) {
            r3Assert( shaders.count( name ) != 0 );
            shaders.erase( name );
        }
    };
    ShaderDatabase *shaderDatabase;
    
    bool initialized = false;
    
    // listShaders command
    void ListShaders( const vector< Token > & tokens ) {
        if ( shaderDatabase == NULL ) {
            return;
        }
        map< string, Shader * > &shaders = shaderDatabase->shaders;
        for( map< string, Shader * >::iterator it = shaders.begin(); it != shaders.end(); ++it ) {
            const string & n = it->first;
            //Shader *shd = it->second;
            Output( "shader: %s", n.c_str() );					
        }
    }
    CommandFunc ListShadersCmd( "listshaders", "lists defined shaders", ListShaders );
    
    // allocshaders command
    void AllocShaders( const vector< Token > & tokens ) {
        if ( shaderDatabase == NULL ) {
            return;
        }
        AllocShader();
    }
    CommandFunc AllocShadersCmd( "allocshaders", "allocs/loads GL shader object for defined shaders", AllocShaders );
    
    // deallocshaders command
    void DeallocShaders( const vector< Token > & tokens ) {
        if ( shaderDatabase == NULL ) {
            return;
        }
        DeallocShader();
    }
    CommandFunc DeallocShadersCmd( "deallocshaders", "deallocs GL shader object for defined shaders", DeallocShaders );
    
    void LoadShaderFromFile( GLuint sobj, const string & filename ) {
        vector<unsigned char> data;
        FileReadToMemory( filename, data );
        data.push_back( 0 );
        Output( "%s", &data[0] );
        const GLchar *srcs[] = { (const GLchar *)&data[0] };
        GLint len[] = { 0 };
        len[0] = (GLint)data.size() - 1;
        glShaderSource( sobj, 1, srcs, len );
        glCompileShader( sobj );
        char dbgLog[1<<15];
        int dbgLogLen = 0;
        glGetShaderInfoLog( sobj, (1<<15) - 2, &dbgLogLen, dbgLog );
        dbgLog[ dbgLogLen ] = 0;
        Output( "%s\n", dbgLog );
    }
    
    GLint GetSlot( GLuint pgObject, map< string, GLint > & uniform, const std::string & name ) {
        map<string, GLint>::iterator it = uniform.find( name );
        if( it == uniform.end() ) {
            uniform[ name ] = glGetUniformLocation( pgObject, name.c_str() );
            it = uniform.find( name );
        }
        return (*it).second;
    }
    
}


namespace r3 {
    
    void InitShader() {
        if ( initialized ) {
            return;
        }
        Output( "Initializing r3::Shader." );
        shaderDatabase = new ShaderDatabase;
        initialized = true;
    }
    
    void ShutdownShader() {
        if( ! initialized ) {
            return;
        }
        Output( "Shutting down r3::Shader." );
        delete shaderDatabase;
        shaderDatabase = NULL;
        initialized = false;
    }
    
    void AllocShader() {
        for( map< string, Shader * >::iterator it = shaderDatabase->shaders.begin(); it != shaderDatabase->shaders.end(); ++it ) {
            it->second->Alloc();
        }
    }
    
    void DeallocShader() {
        for( map< string, Shader * >::iterator it = shaderDatabase->shaders.begin(); it != shaderDatabase->shaders.end(); ++it ) {
            it->second->Dealloc();
        }
    }
    
    Shader::Shader( const std::string & shdName )
    : name( shdName ), generated( true ), allocListener( NULL ) {
        shaderDatabase->AddShader( name, this );
        pgObject = glCreateProgram();
        vsObject = glCreateShader( GL_VERTEX_SHADER );
        fsObject = glCreateShader( GL_FRAGMENT_SHADER );
    }
    
    Shader::~Shader() {
        shaderDatabase->DeleteShader( name );
        glDeleteProgram( pgObject );
        glDeleteShader( vsObject );
        glDeleteShader( fsObject );
    }
    
    void Shader::Alloc() {
    }
    
    void Shader::Dealloc() {
    }
    
    void Shader::SetVertexSource( const char *src  ) {
    }

    void Shader::SetFragmentSource( const char *src  ) {
    }
    
    void Shader::SetUniform( const string & name, const Matrix4f & m ) {
        GLint slot = GetSlot( pgObject, uniform, name );
        glProgramUniformMatrix4fvEXT( pgObject, slot, 1, GL_FALSE, m.Ptr() );
    }    
    void Shader::SetUniform( const string & name, const Vec3f & v ) {
        GLint slot = GetSlot( pgObject, uniform, name );
        glProgramUniform3fvEXT( pgObject, slot, 1, v.Ptr() );
    }
    void Shader::SetUniform( const string & name, GLint i ) {
        GLint slot = GetSlot( pgObject, uniform, name );
        glProgramUniform1iEXT( pgObject, slot, i );
    }    
    
    Shader * CreateShaderFromFile( const std::string & filename ) {
        Shader * shd = shaderDatabase->GetShader( filename );
        if ( shd ) {
            Output( "Returned already loaded shader %s", filename.c_str() );
            return shd;
        }
        shd = new Shader( filename );
        LoadShaderFromFile( shd->vsObject, filename + ".vp" );
        glAttachShader( shd->pgObject, shd->vsObject );
        LoadShaderFromFile( shd->fsObject, filename + ".fp" );
        glAttachShader( shd->pgObject, shd->fsObject );

        //
        glLinkProgram( shd->pgObject );
        char dbgLog[1<<15];
        int dbgLogLen = 0;
        glGetProgramInfoLog( shd->pgObject, (1<<15) - 2, &dbgLogLen, dbgLog );
        dbgLog[ dbgLogLen ] = 0;
        Output( "%s\n", dbgLog );
        
        glUseProgram( shd->pgObject );
        // set up samplers and uniforms
        glUseProgram( 0 );
        
        // load the shaders!
        return shd;
    }
    
    
}
