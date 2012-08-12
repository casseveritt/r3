/*
 *  buffer
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

#include "r3/buffer.h"

#include "r3/common.h"
#include "r3/command.h"
#include "r3/draw.h"
#include "r3/output.h"

#include <assert.h>
#include <string.h>
#include <map>

using namespace std;
using namespace r3;

namespace {

  bool initialized = false;

  struct BufferDatabase {
    map< string, Buffer *> buffers;
    ~BufferDatabase() {
      while( buffers.size() > 0 ) {
        delete buffers.begin()->second;
      }            
    }
    Buffer * GetBuffer( const string & name ) {
      if ( buffers.count( name ) ) {
        return buffers[ name ];
      }
      return NULL;
    }
    void AddBuffer( const string & name, Buffer * buf ) {
      assert( buffers.count( name ) == 0 );
      buffers[ name ] = buf;
    }
    void DeleteBuffer( const string & name ) {
      assert( buffers.count( name ) != 0 );
      buffers.erase( name );
    }		
  };
  BufferDatabase * bufferDatabase;

  // listbuffers command
  void ListBuffers( const vector< Token > & tokens ) {
    if ( bufferDatabase == NULL ) {
      return;
    }
    map< string, Buffer * > &buffers = bufferDatabase->buffers;
    for( map< string, Buffer * >::iterator it = buffers.begin(); it != buffers.end(); ++it ) {
      const string & n = it->first;
      Buffer * b = it->second;
      Output( "%s - sz=%d", n.c_str(), b->GetSize() );					
    }
  }
  CommandFunc ListBuffersCmd( "listbuffers", "lists defined buffers", ListBuffers );

  // allocbuffers command
  void AllocBuffers( const vector< Token > & tokens ) {
    if ( bufferDatabase == NULL ) {
      return;
    }
    AllocBuffer();
  }
  CommandFunc AllocBuffersCmd( "allocbuffers", "alloc/initialize GL buffer objects for defined buffers", AllocBuffers );

  // deallocbuffers command
  void DeallocBuffers( const vector< Token > & tokens ) {
    if ( bufferDatabase == NULL ) {
      return;
    }
    DeallocBuffer();
  }
  CommandFunc DeallocBuffersCmd( "deallocbuffers", "destroy GL buffer objects for defined buffers", DeallocBuffers );
    /*
    GLuint  	index,
 	GLint  	size,
 	GLenum  	type,
 	GLboolean  	normalized,
 	GLsizei  	stride,
 	const GLvoid *  	pointer
     */
    
/*
 GLuint attrib_index[] = { 0, 3, 2, 8, 9 };
 GLint attrib_size[] = { 3, 4, 3, 2, 2 };
 GLenum attrib_type[] = { GL_FLOAT, GL_UNSIGNED_BYTE, GL_FLOAT, GL_FLOAT, GL_FLOAT };
 GLboolean attrib_norm[] = { GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE };
 GLint attrib_bytes[] = { 12, 4, 12, 8, 8 };

 void ComputeArrays( int varying, vector<VertexBuffer::Array> & array ) {
        int offsets[ ARRAY_ELEMENTS( attrib_size ) + 1 ];
        offsets[0] = 0;
		for ( int i = 0; i < ARRAY_ELEMENTS( attrib_size ); i++ ) {
			offsets[i + 1] = offsets[i];
			offsets[i + 1] += ( varying & ( 1 << i ) ) ? attrib_bytes[ i ] : 0; 
		}
        int stride = offsets[ ARRAY_ELEMENTS( attrib_size ) ];
        array.clear();
		for ( int i = 0; i < ARRAY_ELEMENTS( attrib_size ); i++ ) {
            if( varying & ( 1 << i ) ) {
                array.push_back( VertexBuffer::Array( 
                                                     attrib_index[ i ],
                                                     attrib_size[ i ],
                                                     attrib_type[ i ],
                                                     attrib_norm[ i ],
                                                     stride, 
                                                     (const GLvoid *)offsets[ i ]
                                ) );
            }
		}
        
        
	}
  
 */
}


namespace r3 {
  void ComputeOffsets( int varyings, int & stride, int * offsets );

  void InitBuffer() {
    if ( initialized ) {
      return;
    }
    Output( "Initializing r3::Buffer." );
    bufferDatabase = new BufferDatabase();
    initialized = true;
  }

  void ShutdownBuffer() {
    if( ! initialized ) {
      return;
    }
    Output( "Shutting down r3::Buffer." );
    delete bufferDatabase;
    initialized = false;
  }

  void AllocBuffer() {
    for( map< string, Buffer * >::iterator it = bufferDatabase->buffers.begin(); it != bufferDatabase->buffers.end(); ++it ) {
      it->second->Alloc();
    }
  }

  void DeallocBuffer() {
    for( map< string, Buffer * >::iterator it = bufferDatabase->buffers.begin(); it != bufferDatabase->buffers.end(); ++it ) {
      it->second->Dealloc();
    }
  }

  Buffer::Buffer( const std::string & bufName, int bufTarget ) : name( bufName ), target( bufTarget ), allocListener( NULL ) {
    glGenBuffers( 1, & obj );
    bufferDatabase->AddBuffer( name, this );
  }
  Buffer::~Buffer() {
    glDeleteBuffers( 1, & obj );
    bufferDatabase->DeleteBuffer( name );
  }

  void Buffer::Bind() const {
    glBindBuffer( target, obj );
  }

  void Buffer::Unbind() const {
    glBindBuffer( target, 0 );		
  }


  void Buffer::SetData( int sz, const void * data ) {
    size = sz;
    cache.resize( size );
    memcpy( &cache[0], data, size );
    glBindBuffer( target, obj );
    glBufferData( target, size, data, GL_DYNAMIC_DRAW );
    glBindBuffer( target, 0 );
  }

  void Buffer::SetSubdata( int offset, int sz, const void * data ) {
    assert( ( offset + sz ) <= size );
    memcpy( &cache[offset], data, sz );
    glBindBuffer( target, obj );
    glBufferSubData( target, offset, sz, data );
    glBindBuffer( target, 0 );
  }

  void Buffer::GetData( void * data ) {
    memcpy( data, &cache[0], cache.size() );
  }

  void Buffer::GetSubData( int offset, int sz, void * data ) {
    assert( ( offset + sz ) <= size );
    memcpy( data, &cache[ offset ], sz );
  }

  void Buffer::Alloc() {
    if( obj == 0 ) {
      glGenBuffers( 1, & obj );
    }
    glBindBuffer( target, obj );
    glBufferData( target, size, &cache[0], GL_DYNAMIC_DRAW );        
    CallAllocListener();
    glBindBuffer( target, 0 );
  }

  void Buffer::Dealloc() {
    if( obj == 0 ) {
      return;
    }
    glDeleteBuffers( 1, & obj );
    obj = 0;
  }

  VertexBuffer::VertexBuffer( const std::string & vbName ) : Buffer( vbName, GL_ARRAY_BUFFER ) {}
    /*
  void VertexBuffer::SetVarying( int vbVarying ) { 
      ComputeArrays( vbVarying, array );
  }
     */

    /*
  void VertexBuffer::Enable() const {
      for( int i = 0; i < array.size(); i++ ) {
          const VertexBuffer::Array & a = array[ i ];
          glEnableVertexAttribArray( a.index );
          glVertexAttribPointer( a.index, a.size, a.type, a.normalized, a.stride, a.pointer );
      }
  }

  void VertexBuffer::Disable() const {
      for( int i = 0; i < array.size(); i++ ) {
          const VertexBuffer::Array & a = array[ i ];
          glDisableVertexAttribArray( a.index );
      }
  }
 */

  IndexBuffer::IndexBuffer( const std::string & ibName ) : Buffer( ibName, GL_ELEMENT_ARRAY_BUFFER ) {}


}



