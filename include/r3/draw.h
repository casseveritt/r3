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

#ifndef __R3_DRAW_H__
#define __R3_DRAW_H__

#include "r3/buffer.h"
#include "r3/linear.h"
#include "r3/gl.h"

#include <vector>

namespace r3 {
	
	void InitDraw();
    void ShutdownDraw();
	
#define R3_NUM_VARYINGS 5
	
	enum VaryingEnum {
		Varying_Nothing      = 0x00,
		Varying_PositionBit  = 0x01,
		Varying_ColorBit     = 0x02,
		Varying_NormalBit    = 0x04,
		Varying_TexCoord0Bit = 0x08,
		Varying_TexCoord1Bit = 0x10
	};
	
    int GetDepthBits();
	int GetVertexSize( int varying );
	
	// set the texture env
	//void TexEnvCombineAlpha( int index );
    //void TexEnvCombineAlphaModulate( int index );

    void DrawQuad( float x0, float y0, float x1, float y1 );
	void ImTexturedQuad( float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1 );
	void DrawTexturedQuad( float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1 );
	void DrawSprite( float x0, float y0, float x1, float y1 );
	
}

#endif // __R3_DRAW_H__
