/*
 *  model
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

#include "model.h"

#include "r3/command.h"
#include "r3/output.h"
#include "r3/thread.h"

#include <map>

using namespace std;
using namespace r3;

namespace {
    
    bool initialized = false;
    
    struct ModelDatabase {
        map< string, Model *> models;
        ~ModelDatabase() {
            while( models.size() > 0 ) {
                string n = models.begin()->first;
                Model * m = models.begin()->second;
                delete m;
            }
        }
    };
    ModelDatabase *modelDatabase;
    
    
    // listmodels command
    void ListModels( const vector< Token > & tokens ) {
        if ( modelDatabase == NULL ) {
            return;
        }
        map< string, Model * > &models = modelDatabase->models;
        for( map< string, Model * >::iterator it = models.begin(); it != models.end(); ++it ) {
            const string & n = it->first;
            //Model * m = it->second;
            Output( "%s", n.c_str() );					
        }
    }
    CommandFunc ListModelsCmd( "listmodels", "lists defined models", ListModels );
    
    
#define MAX_VERTS 32768
	IndexBuffer *quadIndexBuffer = NULL;
    
	//int attrib_sizes[] = { 12, 4, 12, 8, 8 };
	
	Mutex initQuadMutex;
	void InitQuadIndexes() {
		if ( quadIndexBuffer ) { // early out without acquiring the mutex
			return;
		}
		ScopedMutex scmutex( initQuadMutex, R3_LOC );
		if ( quadIndexBuffer ) { // early out again if we weren't the first one in...
			return;
		}
		GLushort quadIndexes[ MAX_VERTS * 3 / 2 ];
		for ( int i = 0; i < MAX_VERTS / 4; i++ )  {
			quadIndexes[ i * 6 + 0 ] = i * 4 + 0;  // first triangle
			quadIndexes[ i * 6 + 1 ] = i * 4 + 1;
			quadIndexes[ i * 6 + 2 ] = i * 4 + 2;			
			quadIndexes[ i * 6 + 3 ] = i * 4 + 0;  // second triangle
			quadIndexes[ i * 6 + 4 ] = i * 4 + 2;
			quadIndexes[ i * 6 + 5 ] = i * 4 + 3;
		}
		quadIndexBuffer = new IndexBuffer( "quadIndexBuffer_ib" );
		quadIndexBuffer->SetData( sizeof( quadIndexes ), quadIndexes );
	}

    void EnableAttributeArray( const AttributeArray & a ) {
        if( a.index < 16 ) {
            glEnableVertexAttribArray( a.index );
            glVertexAttribPointer( a.index, a.size, a.type, a.normalized, a.stride, a.pointer );
            return;
        }
        switch ( a.index ) {
            case GL_VERTEX_ARRAY:
                glEnableClientState( a.index );
                glVertexPointer( a.size, a.type, a.stride, a.pointer);
                break;
            case GL_NORMAL_ARRAY:
                glEnableClientState( a.index );
                glNormalPointer( a.type, a.stride, a.pointer );
                break;
            case GL_COLOR_ARRAY:
                glEnableClientState( a.index );
                glColorPointer( a.size, a.type, a.stride, a.pointer );
                break;
            case GL_TEXTURE0:
            case GL_TEXTURE1:
            case GL_TEXTURE2:
            case GL_TEXTURE3:
                glEnableClientStateIndexedEXT( GL_TEXTURE_COORD_ARRAY, a.index - GL_TEXTURE0 );
                glMultiTexCoordPointerEXT( a.index, a.size, a.type, a.stride, a.pointer );
                break;
            default:
                break;
        }
    }

    void DisableAttributeArray( const AttributeArray & a ) {
        if( a.index < 16 ) {
            glDisableVertexAttribArray( a.index );
            return;
        }
        switch ( a.index ) {
            case GL_VERTEX_ARRAY:
                glDisableClientState( a.index );
                break;
            case GL_NORMAL_ARRAY:
                glDisableClientState( a.index );
                break;
            case GL_COLOR_ARRAY:
                glDisableClientState( a.index );
                break;
            case GL_TEXTURE0:
            case GL_TEXTURE1:
            case GL_TEXTURE2:
            case GL_TEXTURE3:
                glDisableClientStateIndexedEXT( GL_TEXTURE_COORD_ARRAY, a.index - GL_TEXTURE0 );
                break;
            default:
                break;
        }
    }
    
}

namespace r3 {
    
    void InitModel() {
        if ( initialized ) {
            return;
        }
        Output( "Initializing r3::Model." );
        modelDatabase = new ModelDatabase;
        
        InitQuadIndexes();
        
        initialized = true;
    }
    
    void ShutdownModel() {
        if( ! initialized ) {
            return;
        }
        Output( "Shutting down r3::Model." );
        delete modelDatabase;
        modelDatabase = NULL;
        initialized = false;
    }
    
    Model::Model( const string & mName ) 
    : name( mName )
    , vertexBuffer( NULL )
    , indexBuffer( NULL ) {
        assert( modelDatabase->models.count( name ) == 0 );
        modelDatabase->models[ name ] = this;
    }
    
    Model::~Model() {
        modelDatabase->models.erase( name );
        delete vertexBuffer;
        delete indexBuffer;
    }
    
    VertexBuffer & Model::GetVertexBuffer() {
        if ( vertexBuffer == NULL ) {
            vertexBuffer = new VertexBuffer( name + "_vb" );
        }
        return *vertexBuffer;
    }
    IndexBuffer & Model::GetIndexBuffer() {
        if ( indexBuffer == NULL ) {
            indexBuffer = new IndexBuffer( name + "_ib" );
        }
        return *indexBuffer;
    }
    
    void Model::Draw() {
        assert( attr.size() );
        assert( vertexBuffer );
        
		vertexBuffer->Bind();
        for( int i = 0; i < (int)attr.size(); i++ ) {
            EnableAttributeArray( attr[i] );
        }
        
		if ( indexBuffer ) {
			indexBuffer->Bind();
			glDrawElements( prim, indexBuffer->GetSize() / 2, GL_UNSIGNED_SHORT, (void *)0 );
			indexBuffer->Unbind();
		} else {
            // FIXME: This needs to be passed some other way.
			assert( numVerts < MAX_VERTS ); 
			if ( prim == GL_QUADS ) { // support non-indexed quads
				quadIndexBuffer->Bind();
				glDrawElements( GL_TRIANGLES, numVerts * 3 / 2, GL_UNSIGNED_SHORT, 0 );
				quadIndexBuffer->Unbind();
			} else {	
				glDrawArrays( prim, 0, numVerts );
			}
		}
        for( int i = 0; i < (int)attr.size(); i++ ) {
            DisableAttributeArray( attr[i] );
        }
        vertexBuffer->Unbind();
    }
    
    
}


