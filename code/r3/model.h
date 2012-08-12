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

#ifndef __R3_MODEL_H__
#define __R3_MODEL_H__

#include "r3/buffer.h"
#include "r3/draw.h"
#include "r3/linear.h"

#include <string>
#include <vector>

namespace r3 {
	
	void InitModel();
	void ShutdownModel();
    
    struct AttributeArray {
        AttributeArray( GLuint idx, GLint sz, GLenum tp, GLboolean n, GLsizei str, const GLvoid * ptr ) 
        : index( idx ), size( sz ), type( tp ), normalized( n ), stride( str ), pointer( ptr )
        {}
        AttributeArray( GLuint idx, GLint sz, GLenum tp, GLboolean n, GLsizei str, int offset ) 
        : index( idx ), size( sz ), type( tp ), normalized( n ), stride( str ), pointer( (GLvoid *)(((char *)NULL) + offset) )
        {}
        GLuint index;
        GLint size;
        GLenum type;
        GLboolean normalized;
        GLsizei stride;
        const GLvoid * pointer;
    };
    
	class Model {
		std::string name;
		VertexBuffer *vertexBuffer;
		IndexBuffer *indexBuffer;
		GLenum prim;
        int numVerts;
        std::vector<AttributeArray> attr;
		// disallow copying and assignment
		Model( const Model & rhs ) {}
		const Model & operator= ( const Model & rhs ) {
			return *this;
		}
	public:
		Model( const std::string & mName );
		~Model();
		const std::string GetName() const {
			return name;
		}
		VertexBuffer & GetVertexBuffer();
		IndexBuffer & GetIndexBuffer();
		GLenum GetPrimitive() const {
			return prim;
		}
		void SetPrimitive( GLenum mPrim ) {
			prim = mPrim;
		}
        GLint GetNumVertexes() {
            if( indexBuffer ) {
                return indexBuffer->GetSize() / 2;                
            }
            return numVerts;
        }
        void SetNumVertexes( int num ) {
            numVerts = num;
        }
        void ClearAttributeArrays() { attr.clear(); }
        void AddAttributeArray( const AttributeArray & a ) { attr.push_back( a ); }
		void Draw();        
	};

		
}

#endif // __R3_MODEL_H__
















