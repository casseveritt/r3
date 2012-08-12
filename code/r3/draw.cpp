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
#include <GL/Regal.h>

#include <assert.h>

#define IM_QUADS 999
#define PRIM_INVALID 1000
#include <string.h>

using namespace std;
using namespace r3;

namespace {
		
}

namespace r3 {
	
	void InitDraw() {
	}
    
    void ShutdownDraw() {
    }

	int GetDepthBits() {
		int r;
		glGetIntegerv( GL_DEPTH_BITS, &r );
		return r;
	}
			
	void TexEnvCombineAlpha( int index ) {
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );			
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_TEXTURE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );			
	}

	void TexEnvCombineAlphaModulate( int index ) {
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_ALPHA );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );			
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_TEXTURE );
		glMultiTexEnviEXT( GL_TEXTURE0 + index, GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );			
	}
		void DrawQuad( float x0, float y0, float x1, float y1 ) {
		glBegin( GL_QUADS );
		glVertex2f( x0, y0 );
		glVertex2f( x1, y0 );
		glVertex2f( x1, y1 );
		glVertex2f( x0, y1 );		
		glEnd();
	}
	
	void ImTexturedQuad( float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1 ) {
		glMultiTexCoord2f( GL_TEXTURE0, s0, t0 );
		glVertex2f( x0, y0 );
		glMultiTexCoord2f( GL_TEXTURE0, s1, t0 );
		glVertex2f( x1, y0 );
		glMultiTexCoord2f( GL_TEXTURE0, s1, t1 );
		glVertex2f( x1, y1 );
		glMultiTexCoord2f( GL_TEXTURE0, s0, t1 );
		glVertex2f( x0, y1 );		
	}
	
	void DrawTexturedQuad( float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1 ) {
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
	}
	
	void DrawSprite( float x0, float y0, float x1, float y1 ) {
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
	}
	
	
}
