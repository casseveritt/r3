/*
 *  draw
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

#include "r3/draw.h"

#include "r3/buffer.h"
#include "r3/common.h"
#include "r3/output.h"
#include "r3/thread.h"
#include "r3/gl.h"

#include <assert.h>

#define IM_QUADS 999
#define PRIM_INVALID 1000
#include <string.h>

using namespace std;
using namespace r3;

namespace {
		
  struct Immediate {
    GLuint vao;
    GLuint dummyVao;
    GLuint pos;
    GLuint tex;
  };
  Immediate imm;
}

namespace r3 {
  
  void InitDraw() {
    glGenVertexArrays( 1, &imm.vao );
    glGenVertexArrays( 1, &imm.dummyVao );
    glGenBuffers( 1, &imm.pos );
    glGenBuffers( 1, &imm.tex );
    glBindVertexArray( imm.vao );
    glBindBuffer( GL_ARRAY_BUFFER, imm.pos );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( GLfloat ), 0 );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, imm.tex );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( GLfloat ), 0 );
    glEnableVertexAttribArray( 1 );
    glBindVertexArray( imm.dummyVao );
  }
  
  void ShutdownDraw() {
    glDeleteBuffers( 1, &imm.tex );
    glDeleteBuffers( 1, &imm.pos );
    glDeleteVertexArrays( 1, &imm.vao );
  }
  
  int GetDepthBits() {
    int r = 0;
    //glGetIntegerv( GL_DEPTH_BITS, &r );
    return r;
  }
  
  void DrawQuad( float x0, float y0, float x1, float y1 ) {
      /*
       glBegin( GL_QUADS );
       glVertex2f( x0, y0 );
       glVertex2f( x1, y0 );
       glVertex2f( x1, y1 );
       glVertex2f( x0, y1 );
       glEnd();
       */
  }
  
  void ImTexturedQuad( float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1 ) {
    /*
     glMultiTexCoord2f( GL_TEXTURE0, s0, t0 );
     glVertex2f( x0, y0 );
     glMultiTexCoord2f( GL_TEXTURE0, s1, t0 );
     glVertex2f( x1, y0 );
     glMultiTexCoord2f( GL_TEXTURE0, s1, t1 );
     glVertex2f( x1, y1 );
     glMultiTexCoord2f( GL_TEXTURE0, s0, t1 );
     glVertex2f( x0, y1 );
     */
  }
  
  void DrawTexturedQuad( float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1 ) {
    float v[2 * 4] = { x0, y0, x0, y1, x1, y0, x1, y1 };
    float t[2 * 4] = { s0, t0, s0, t1, s1, t0, s1, t1 };
    glBindVertexArray( imm.vao );
    glBindBuffer( GL_ARRAY_BUFFER, imm.pos );
    glBufferData( GL_ARRAY_BUFFER, sizeof( v ), v, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, imm.tex );
    glBufferData( GL_ARRAY_BUFFER, sizeof( t ), t, GL_DYNAMIC_DRAW );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    glBindVertexArray( imm.dummyVao );
    
    /*
     glBegin( GL_QUADS );
     glMultiTexCoord2f( GL_TEXTURE0, s0, t0 );
     glVertex2f( x0, y0 );
     glMultiTexCoord2f( GL_TEXTURE0, s1, t0 );
     glVertex2f( x1, y0 );
     glMultiTexCoord2f( GL_TEXTURE0, s1, t1 );
     glVertex2f( x1, y1 );
     glMultiTexCoord2f( GL_TEXTURE0, s0, t1 );
     glVertex2f( x0, y1 );
     glEnd();
     */
  }
  
  void DrawSprite( float x0, float y0, float x1, float y1 ) {
    /*
     glBegin( GL_QUADS );
     glMultiTexCoord2f( GL_TEXTURE0, 0, 0 );
     glVertex2f( x0, y0 );
     glMultiTexCoord2f( GL_TEXTURE0, 1, 0 );
     glVertex2f( x1, y0 );
     glMultiTexCoord2f( GL_TEXTURE0, 1, 1 );
     glVertex2f( x1, y1 );
     glMultiTexCoord2f( GL_TEXTURE0, 0, 1 );
     glVertex2f( x0, y1 );		
     glEnd();
     */
  }
  
  
}
