/*
 *  image
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



#ifndef __R3_IMAGE_H__
#define __R3_IMAGE_H__

#include <string.h>
#include <string>

namespace r3 {
	
	template< typename T >
	class Image {
	public:
		typedef T ComponentType;
		int width;
		int height;
		int components;
    
		ComponentType *data;
    Image() : width( 0 ), height( 0 ), components( 0 ), data( NULL ) {}
    
    ~Image() { delete [] data; }
    
    void SetSize( int w, int h, int c ) {
      delete [] data;
      data = new ComponentType[ w * h * c ];
      width = w;
      height = h;
      components = c;
    }
    
		int Width() const {
			return width;
		}
		
		int Height() const {
			return height;
		}
		
		int Components() const {
			return components;
		}
    
    int Index( int x, int y, int c ) const {
      return ( y * width + x ) * components + c;
    }
    
    T & operator()( int x, int y, int c ) {
      return data[ Index( x, y, c ) ];
    }
    
    const T & operator()( int x, int y, int c ) const {
      return data[ Index( x, y, c ) ];
    }
    
    void CopySubImage( Image<T> & subimg, int x, int y ) const {
      int w = subimg.Width();
      int bytes = w * components * sizeof( T );
      for( int j = 0; j < subimg.Height(); j++ ) {
        memcpy( &subimg( 0, j, 0 ), &((*this)( x, y + j, 0 ) ), bytes );
      }
      
    }
    
    void Rotate( int quarterTurns ) {
      quarterTurns = quarterTurns & 3;
      if( quarterTurns == 0 || ( width != height ) ) {
        return;
      }
      ComponentType * buf = new ComponentType[ width * height * components ];
      
      for( int t = 0; t < quarterTurns; t++ ) {
        for( int j = 0; j < height; j++ ) {
          for( int i = 0; i < width; i++ ) {
            for( int c = 0; c < components; c++ ) {
              buf[ Index( i, j, c ) ] = data[ Index( j, width - 1 - i, c ) ];
            }
          }
        }
        memcpy( data, buf, width * height * components * sizeof( T ) );
      }
      delete [] buf;
    }
		
	};
	
	Image<unsigned char> * ReadImageFile( const std::string & filename, int desiredComponents = 0 );
	void WriteImageFile( const std::string & filename, const Image<unsigned char > *img );
	
}

#endif // __R3_IMAGE_H__