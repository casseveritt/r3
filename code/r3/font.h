/*
 *  font
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

#ifndef __R3_FONT_H__
#define __R3_FONT_H__

#include "r3/texture.h"
#include "r3/bounds.h"

namespace r3 {
	void InitFont();
	void ShutdownFont();

	enum TextAlignmentEnum {
		Align_Min,
		Align_Mid,
		Align_Max
	};
    
    inline float AlignAdjust( float len, TextAlignmentEnum align ) {
		switch( align ) {
			case Align_Max:
				return -len;
			case Align_Mid:
				return -len/2;
			case Align_Min:
			default:
				return 0;
		}
		return 0;
	}
    
	
	struct Font {
		virtual ~Font() {}
		std::string GetName() {
			return name;
		}
		virtual float GetAscent() = 0;
		virtual float GetDescent() = 0;
		virtual float GetLineGap() = 0;
		virtual void Print( const std::string &text, float x, float y, float scale = 1.0f, TextAlignmentEnum hAlign = Align_Min, TextAlignmentEnum vAlign = Align_Min ) = 0;	
		virtual Bounds2f GetStringDimensions( const std::string &text, float scale = 1.0f ) = 0;
		std::string name;
	};

	Font * CreateStbFont( const std::string & fontName, const std::string & fallbackFontName, float pointSize );
	
	
}

#endif // __R3_FONT_H__