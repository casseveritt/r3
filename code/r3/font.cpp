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

#include "r3/bounds.h"
#include "r3/common.h"
#include "r3/draw.h"
#include "r3/filesystem.h"
#include "r3/font.h"
#include "r3/output.h"
#include "r3/texture.h"

#include "r3/gl.h"

#include "r3/stb_image_write.h"

#include <map>
#include <string>

using namespace std;
using namespace r3;

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#define STBTT_malloc(x,u)  malloc(x)
#define STBTT_free(x,u)    free(x)
#include "r3/stb_truetype.h"

#include <math.h>
#include <assert.h>

#include <stdio.h>

namespace {
	
	struct FontDatabase {
		map< string, r3::Font * > fontMap;
		~FontDatabase() {
			for( map< string, r3::Font *>::iterator it = fontMap.begin(); it != fontMap.end(); ++it ) {
				Output( "Deleting font %s", it->first.c_str() );
				delete it->second;
			}
		}
		string GetName( const string & name, int imgSize ) {
			char szstr[32];
			r3Sprintf( szstr, "_%d", imgSize );
			return name + szstr;
		}
		r3::Font * GetFont( const string & fullname ) {
			if ( fontMap.count( fullname ) ) {
				return fontMap[ fullname ];
			}
			return NULL;
		}
		void AddFont( const string & fullname, r3::Font * f ) {
			assert( fontMap.count( fullname ) == 0 );
			assert( f != NULL );
			fontMap[ fullname ] = f;
		}
	};
	FontDatabase *fontDatabase;
  
	bool initialized = false;
	
	
	
	void UnescapeUnicode( const string & s, vector<int> & c ) {
		c.clear();
		int state = 0;
		int unicode;
		for( int i = 0; i < s.size(); i++ ) {
			switch( state ) {
				case 0:
					if( s[ i ] == '\\' ) {
						state++;
						unicode = 0;
					} else if ( s[ i ] == '&' ) {
            state = 10;
						unicode = 0;
					} else {
						c.push_back( s[ i ] );
					}
					break;
				case 1:
					if( s[ i ] == 'u' ) {
						state++;
					} else {
						while( state >= 0 ) {
							c.push_back( s[ i - state ] );
							state--;
						}
						state = 0;
					}
					break;
				case 2:
				case 3:
				case 4:
				case 5:
				{
					int hexval = -1;
					if ( s[ i ] >= '0' && s[ i ] <= '9' ) {
						hexval = s[ i ] - '0';
					} else if ( s[ i ] >= 'A' && s[ i ] <= 'F' ) {
						hexval = ( s[ i ] - 'A' ) + 10;
					} else if ( s[ i ] >= 'a' && s[ i ] <= 'f' ) {
						hexval = ( s[ i ] - 'a' ) + 10;
					}
					if( hexval >= 0 ) {
						unicode *= 16;
						unicode += hexval;
						state++;
						if ( state == 6 ) {
							c.push_back( unicode );
							state = 0;
						}
					} else {
						while( state >= 0 ) {
							c.push_back( s[ i - state ] );
							state--;
						}
						state = 0;
					}
					
				}
					break;
				case 10:
					if( s[ i ] == '#' ) {
						state++;
					} else {
						state -= 9;
						while( state >= 0 ) {
							c.push_back( s[ i - state ] );
							state--;
						}
						state = 0;
					}
					break;
				case 11:
				case 12:
				case 13:
				case 14:
				{
					if( s[ i ] == ';' ) {
						c.push_back( unicode );
						state = 0;
					} else if ( state == 14 ) {
						state -= 9;
						while( state >= 0 ) {
							c.push_back( s[ i - state ] );
							state--;
						}
						state = 0;
					} else {
						int ival = -1;
						if ( s[ i ] >= '0' && s[ i ] <= '9' ) {
							ival = s[ i ] - '0';
							unicode *= 10;
							unicode += ival;
							state++;
						} else {
							state -= 9;
							while( state >= 0 ) {
								c.push_back( s[ i - state ] );
								state--;
							}
							state = 0;
						}
					}
				}
					
				default:
					break;
			}
		}
	}
  
	struct CachedGlyph {
		unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
		float xoff, yoff, xadvance;
		bool reverse;
		uint64 lastUsed;
	};
	
	struct GlyphQuad {
		float x0, y0, s0, t0; // top-left
		float x1, y1, s1, t1; // bottom-right
	};
	
	GlyphQuad GetGlyphQuad( const CachedGlyph & g, int pw, int ph, float *xpos, float ypos, float scale ) {
		GlyphQuad q;
		
		q.x0 = (*xpos + g.xoff * scale );
		q.y0 = ( ypos + g.yoff * scale );
		q.x1 = q.x0 + ( g.x1 - g.x0 ) * scale;
		q.y1 = q.y0 + ( g.y1 - g.y0 ) * scale;
		float w = pw;
		float h = ph;
		q.s0 = g.x0 / w;
		q.t0 = g.y0 / h;
		q.s1 = g.x1 / w;
		q.t1 = g.y1 / h;
    
		*xpos += g.xadvance * scale;
		return q;
	}
	
	
	
	struct StbCachedGlyphFont : public r3::Font, public TextureAllocListener {
		StbCachedGlyphFont( const string & texName, const string & ttfFilename, const string &ttfFallback );
		virtual ~StbCachedGlyphFont();
		virtual void Print( const std::string &text, float x, float y, float scale = 1.0f, TextAlignmentEnum hAlign = Align_Min, TextAlignmentEnum vAlign = Align_Min );
		virtual Bounds2f GetStringDimensions( const std::string &text, float scale = 1.0f );
		virtual float GetAscent() { return fv[0].ascent; }
		virtual float GetDescent() { return fv[0].descent; }
		virtual float GetLineGap() { return fv[0].lineGap; }
    virtual void OnTextureAlloc( Texture * t ) { usageCount = 1; memset( used, 0, sizeof( used ) ); gm.clear(); }
		map< int, CachedGlyph > gm;
		static const int imgSize = 512;
		Texture2D *ftex;
		struct FontData {
			FontData() {}
			FontData( const FontData & rhs ) {
				ttf = rhs.ttf;
				if( ttf.size() > 0 ) {
					stbtt_InitFont( &font, &ttf[0], 0 );
					ascent = rhs.ascent;
					descent = rhs.descent;
					lineGap = rhs.lineGap;
					scale = rhs.scale;
					reverse = rhs.reverse;
				}
			}
			void Init( const string & ttfFilename, int imgSize ) {
				File *f = FileOpenForRead( ttfFilename );
#if ANDROID
				if( f == NULL ) {
					f = FileOpenForRead( string( "/system/fonts/" ) + ttfFilename );
				}
        if( f == NULL ) {
          f = FileOpenForRead( "LiberationSans-Regular.ttf" );
        }
#endif
				ttf.resize( f->Size() );
				f->Read( &ttf[0], 1, (int)ttf.size() );
				delete f;
				stbtt_InitFont( &font, &ttf[0], 0 );
				int iAscent, iDescent, iLineGap;
				stbtt_GetFontVMetrics( &font, &iAscent, &iDescent, &iLineGap );
				float pixelSize = imgSize / 9.0f;
				float sc = pixelSize / ( iAscent - iDescent );
				ascent = iAscent * sc;
				descent = iDescent * sc;
				lineGap = iLineGap * sc;
				scale = stbtt_ScaleForPixelHeight( &font, pixelSize );
				if( ttfFilename.find( "Hebrew" ) != string::npos || ttfFilename.find( "Arabic" ) != string::npos ) {
					reverse = true;
				} else {
					reverse = false;
				}
				
			}
			stbtt_fontinfo font;
			float ascent;
			float descent;
			float lineGap;
			float scale;
			bool reverse;
			vector<uchar> ttf;  // contents of the ttf file
		};
    
		vector<FontData> fv;
		uint64 usageCount;
		uint64 used[64];    // 64b x 64b used mask... each bit corresponds to an 8x8 pixel block in the font image
		const CachedGlyph & GetGlyph( int idx );
		bool FindSpace( int glyphBlocksWide, int glyphBlocksHigh, int & x, int & y );
		void MarkGlyph( int idx, bool bit );
	};
	
	StbCachedGlyphFont::StbCachedGlyphFont( const string & texName, const string & ttfFilename, const string &ttfFallback ) {
		name = ttfFilename;
		fv.push_back( FontData() );
		fv.back().Init( ttfFilename, imgSize );
		if( ttfFallback.size() > 0 ) {
			fv.push_back( FontData() );
			fv.back().Init( ttfFallback, imgSize );
		}
    
		usageCount = 1;
		memset( used, 0, sizeof( used ) );
		
		ftex = Texture2D::Create( texName, TextureFormat_RGBA, imgSize, imgSize );
		uchar * img =  new uchar[ imgSize * imgSize * 4 ];
		memset( img, 0, imgSize * imgSize * 4 );
		ftex->SetImage( 0, img );
		delete [] img;
    ftex->SetAllocListener( this );
	}
	
	StbCachedGlyphFont::~StbCachedGlyphFont() {
#if 0
		vector<byte> pixels;
		pixels.resize( imgSize * imgSize * 4 );
		ftex->Bind( 0 );
		{
			ftex->GetImage( 0, &pixels[ 0 ] );
			int sz = 0;
			unsigned char * bytes = stbi_write_png_to_mem( &pixels[ 0 ] + ( imgSize * ( imgSize - 1 ) * 4 ), -imgSize * 4, imgSize, imgSize, 4, &sz );
			File *file = FileOpenForWrite( name + ".png" );
			if( file != NULL ) {
				file->Write( bytes, 1, sz );
				delete file;
			}
			free( bytes );
		}
		{
      int level = 1;
			ftex->GetImage( level, &pixels[ 0 ] );
			int isz = imgSize >> level;
			int sz = 0;
			unsigned char * bytes = stbi_write_png_to_mem( &pixels[ 0 ] + ( isz * ( isz - 1 ) * 4 ), -isz * 4, isz, isz, 4, &sz );
			File *file = FileOpenForWrite( name + "_1.png" );
			if( file != NULL ) {
				file->Write( bytes, 1, sz );
				delete file;
			}
			free( bytes );
		}
#endif
	}
	
	bool StbCachedGlyphFont::FindSpace( int glyphBlocksWide, int glyphBlocksHigh, int & x, int & y ) {
		for( int j = 0; j < 64 - glyphBlocksHigh; j++ ) {
			for( int i = 0; i < 64 - glyphBlocksWide; i++ )  {
				bool fits = true;
				uint64 hmask = ( 1ULL << glyphBlocksWide ) - 1;
				hmask <<= i;
				for( int bj = 0; ( bj < glyphBlocksHigh ) && fits; bj++ ) {
					if ( ( hmask & used[ j + bj ] ) != 0 ) {
						fits = false;
					}
				}
				if( fits ) {
					x = i;
					y = j;
					//Output( "Found space for glyph w=%d, h=%d at x=%d, y=%d", glyphBlocksWide, glyphBlocksHigh, x, y );
					return true;
				}
			}
		}
		Output( "No space found for glyph w=%d, h=%d", glyphBlocksWide, glyphBlocksHigh );
		return false;
	}
	
	void StbCachedGlyphFont::MarkGlyph( int idx, bool bit ) {
		assert( idx >= 0 );
		assert( gm.count( idx ) != 0 );
		CachedGlyph g = gm[ idx ];
		int gw = g.x1 - g.x0;
		int gh = g.y1 - g.y0;
		int bx = g.x0 / 8;
		int by = g.y0 / 8;
		int gbw = ( gw + 15 ) / 8;
		int gbh = ( gh + 15 ) / 8;
		uint64 hmask = ( ( ( 1ULL << gbw ) - 1 ) << bx ) ;
		//Output( "%s glyph %d from %s, x=%d, y=%d, w=%d, h=%d, hmask=%llx", bit ? "Marking" : "Erasing", idx, name.c_str(), bx, by, gbw, gbh, hmask  );
		if( bit ) {
			for( int j = by; j < by + gbh; j++ ) {
				used[ j ] = used[ j ] | hmask;
			}
		} else {
			for( int j = by; j < by + gbh; j++ ) {
				used[ j ] = used[ j ] & ~hmask;
			}
		}
	}
	
	const CachedGlyph & StbCachedGlyphFont::GetGlyph( int idx ) {
		if ( gm.count( idx ) > 0 ) {
			CachedGlyph & g = gm[ idx ];
			g.lastUsed = usageCount++;
			return g; // return the hit
		}
		
		for( int i = 0; i < fv.size(); i++ ) {
			stbtt_fontinfo & font = fv[i].font;
			float scale = fv[i].scale;
			bool reverse = fv[i].reverse;
			// 1) get glyph size in pixels
			// 2) find a space that can contain it, else
			//    a) delete the oldest glyph from gm
			//    b) clear it's used bits
			//    c) repeat until success
			// 3) rasterize glyph into temporary buffer
			// 4) return glyph
			
			// 1)
			int gi = stbtt_FindGlyphIndex( &font, idx );
			if ( gi == 0 ) {
				continue;
			}
			int advanceWidth, leftSideBearing;
			stbtt_GetGlyphHMetrics( &font, gi, &advanceWidth, &leftSideBearing );
			int x0, x1, y0, y1;
			stbtt_GetGlyphBitmapBox( &font, gi, scale, scale, &x0, &y0, &x1, &y1 );
			int gw = x1 - x0;
			int gh = y1 - y0;
			
			// 2)
			int gbw = ( gw + 15 ) / 8; // half-block border for mips
			int gbh = ( gh + 15 ) / 8;
			int x, y;
			while( FindSpace( gbw, gbh, x, y ) == false  ) {
				uint64 oldest = ~0;
				int victim = -1;
				for( map< int, CachedGlyph >::iterator it = gm.begin(); it != gm.end(); ++it ) {
					CachedGlyph & g = it->second;
					if( g.lastUsed < oldest ) {
						oldest = g.lastUsed;
						victim = it->first;
					}
				}
				MarkGlyph( victim, false ); // clear used bits
				gm.erase( victim );
			}
			CachedGlyph & g = gm[ idx ];
			g.reverse = reverse;
			g.lastUsed = usageCount++;
			int xoff = ( gbw * 8 - gw ) / 2;
			g.x0 = x * 8 + xoff;  // center glyph within its allocated area
			g.x1 = g.x0 + gw;
			int yoff = ( gbh * 8 - gh ) / 2;
			g.y0 = y * 8 + yoff;
			g.y1 = g.y0 + gh;
			g.xoff = x0;
			g.yoff = -y1;
			g.xadvance = scale * advanceWidth;
			MarkGlyph( idx, true );
			
			// 3)
			vector<byte> pixels;
			{
				int ww = gbw * 8;
				int hh = gbh * 8;
				vector<byte> lum;
				lum.resize( ww * hh, 0 );
				pixels.resize( ww * hh * 4 );
				stbtt_MakeGlyphBitmap( &font, &lum[0] + yoff * ww + xoff, gw, gh, ww, scale, scale, gi );
				
				// flip vertically
				for( int j = 0; j < hh / 2; j++ ) {
					byte * a = & lum[ j * ww ];
					byte * b = & lum[ ( hh - 1 - j ) * ww ];
					for( int i = 0; i < ww; i++ ) {
						byte tmp = a[ i ];
						a[ i ] = b[ i ];
						b[ i ] = tmp;
					}
				}
				
				// little gaussian smoothing - temporarily disabled
				int w[ 5 ][ 5 ] = {
					{  1,  4,  7,  4,  1 },
					{  4, 16, 26, 16,  4 },
					{  7, 26, 41, 26,  7 },
					{  4, 16, 26, 16,  4 },
					{  1,  4,  7,  4,  1 }
				};
				float sum = 273.0f;
				for( int j = 0; j < hh; j++ ) {
					for( int i = 0; i < ww; i++ ) {
						int idx = ( j * ww + i ) * 4;
						pixels[ idx + 0 ] = pixels[ idx + 1 ] = pixels[ idx + 2 ] = 255;
						pixels[ idx + 3 ] = lum[ idx >> 2 ];
						if ( false && ( i >= 2 || i < ( ww - 2 ) ) && ( j >= 2 || j < ( hh - 2 ) ) ) {
							int wsum = 0;
							for( int jj = -2; jj < 3; jj++ ) {
								for( int ii = -2; ii < 3; ii++ ) {
									wsum += w[ ii + 2 ][ jj + 2 ] * lum[ ( j + jj ) * ww + i + ii ];
								}
							}
							pixels[ idx + 3 ] = byte( wsum / sum );
						}
					}
				}
			}
			
			ftex->Bind( 0 );
			// block size is 8, so we do mip levels 4 levels
			for( int level = 0; level < min( ftex->LevelMax()+1, 4 ); level++ ) {
				int xx = ( x * 8 ) >> level;
				int yy = ( y * 8 ) >> level;
				int ww = ( gbw * 8 ) >> level;
				int hh = ( gbh * 8 ) >> level;
				ftex->SetSubImage( level, xx , yy, ww, hh, &pixels[ 0 ] );
				int w2 = ww >> 1;
				int h2 = hh >> 1;
				for( int j = 0; j < h2; j++ ) {
					for( int i = 0; i < w2; i++ ) {
						int alpha = 0;
						alpha += pixels[ ( ( 2 * j + 0 ) * ww + ( 2 * i + 0 ) ) * 4 + 3 ];
						alpha += pixels[ ( ( 2 * j + 0 ) * ww + ( 2 * i + 1 ) ) * 4 + 3 ];
						alpha += pixels[ ( ( 2 * j + 1 ) * ww + ( 2 * i + 0 ) ) * 4 + 3 ];
						alpha += pixels[ ( ( 2 * j + 1 ) * ww + ( 2 * i + 1 ) ) * 4 + 3 ];
						alpha += 2;
						alpha >>= 2;
						pixels[ ( j * w2 + i ) * 4 + 3 ] = byte( alpha );
					}
				}
			}
      
			if( gi != 0 ) {
				break;
			}
		}
		
		return gm[ idx ];
	}
	
	void StbCachedGlyphFont::Print( const string &text, float x, float y, float scale, TextAlignmentEnum hAlign, TextAlignmentEnum vAlign ) {
		if ( ftex == 0 ) {
			return;
		}
		
		Bounds2f b = GetStringDimensions( text, scale );
		
		x += AlignAdjust( b.Width(), hAlign );
		y += AlignAdjust( b.Height(), vAlign );
		
		// assume orthographic projection with units = screen pixels, origin at top left
		ftex->Bind( 0 );
		//ftex->Enable( 0 );
		//TexEnvCombineAlpha( 0 );
		
        #if 0
    glBegin( GL_QUADS );
		vector<int> uc;
		UnescapeUnicode( text, uc );
		bool reverse = false;
		for ( int i = 0; i < (int)uc.size() ; i++ ) {
			int c = uc[i];
			reverse = reverse || GetGlyph( c ).reverse;
      if( reverse ) {
        break;
      }
		}
		if ( reverse ) {
			std::reverse( uc.begin(), uc.end() );
		}
		for ( int i = 0; i < (int)uc.size(); i++ ) {
			int c = uc[i];
			GlyphQuad q = GetGlyphQuad( GetGlyph( c ), imgSize, imgSize, &x, y, scale );
			ImTexturedQuad( q.x0, q.y0, q.x1, q.y1, q.s0, q.t0, q.s1, q.t1 );
		}
    glEnd();
    #endif
    
    //glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		//ftex->Disable( 0 );
	}
	
	
	Bounds2f StbCachedGlyphFont::GetStringDimensions( const std::string & text, float scale ) {
		float x = 0;
		float y = 0;
		Bounds2f b;
		vector<int> uc;
		UnescapeUnicode( text, uc );
		for ( int i = 0; i < (int)uc.size(); i++ ) {
			int c = uc[i];
			GlyphQuad q = GetGlyphQuad( GetGlyph( c ), imgSize, imgSize, &x, 0, scale );
			b.Add( Vec2f( q.x0, q.y0 ) );
			b.Add( Vec2f( q.x1, q.y1 ) );
		}
		b.Add( Vec2f( x, y ) );
		return b;
	}
	
}


namespace r3 {
	
	void InitFont() {
		if ( initialized ) {
			return;
		}
		fontDatabase = new FontDatabase();
		initialized = true;
	}
	
	void ShutdownFont() {
		if ( ! initialized ) {
			return;
		}
		delete fontDatabase;
		initialized = false;
	}
  
	// FIXME: make this honor point size
	Font * CreateStbFont( const std::string &ttfname, const std::string &ttfFallback, float pointSize ) {
		assert( initialized );
		int imgSize = (int)pow( 2.0, ceil( log( 32 * pointSize ) / log( 2.0 ) ) ) + 0.5;
		Output( "Generating %d x %d font image for %s (%f).", imgSize, imgSize, ttfname.c_str(), pointSize );
		string fn = fontDatabase->GetName( ttfname, imgSize );
		Font * f = fontDatabase->GetFont( fn );
		if ( f == NULL ) {
			f = new StbCachedGlyphFont( fn, ttfname, ttfFallback );
			fontDatabase->AddFont( fn, f );
			Output( "Returning existing font for \"%s\"", fn.c_str() );
		}
		return f;
	}
  
	
}
