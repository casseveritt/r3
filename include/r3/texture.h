/*
 *  texture
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

#ifndef __R3_TEXTURE_H__
#define __R3_TEXTURE_H__

#include <string>
#include "r3/gl.h"

namespace r3 {
	
	void InitTexture();
  void ShutdownTexture();
  void AllocTexture();
  void DeallocTexture();
	
	enum TextureFormatEnum {
		TextureFormat_INVALID,
		TextureFormat_A___do_not_use_yet,
		TextureFormat_LA___do_not_use_yet,
		TextureFormat_RGB,
		TextureFormat_RGBA,
		TextureFormat_DepthComponent,
		TextureFormat_MAX
	};
  
	enum TextureTargetEnum {
		TextureTarget_1D,
		TextureTarget_2D,
		TextureTarget_3D,
		TextureTarget_Cube
	};
	
	enum TextureFilterEnum {
		TextureFilter_None,
		TextureFilter_Nearest,
		TextureFilter_Linear
	};
  
	struct SamplerParams {
		SamplerParams()
		: magFilter( TextureFilter_Linear )
		, minFilter( TextureFilter_Linear )
		, mipFilter( TextureFilter_Linear )
		, levelBias( 0.0f )
		, levelMin( 0.0f )
		, levelMax( 128.0f )
		, anisoMax( 8.0f )
		{}
		
		SamplerParams( TextureFilterEnum tfMagFilter, TextureFilterEnum tfMinFilter, TextureFilterEnum tfMipFilter, float tfLevelBias, float tfLevelMin, float tfLevelMax, float tfAnisoMax )
		: magFilter( tfMagFilter )
		, minFilter( tfMinFilter )
		, mipFilter( tfMipFilter )
		, levelBias( tfLevelBias )
		, levelMin( tfLevelMin )
		, levelMax( tfLevelMax )
		, anisoMax( tfAnisoMax )
		{}
		
		TextureFilterEnum magFilter;
		TextureFilterEnum minFilter;
		TextureFilterEnum mipFilter;
		float levelBias;
		float levelMin;
		float levelMax;
		float anisoMax;
	};
  
  class Texture;
  
  class TextureAllocListener {
  public:
    virtual void OnTextureAlloc( Texture * tex ) = 0;
  };
  
	
	class Texture {
		std::string name;
		TextureFormatEnum format;
		TextureTargetEnum target;
		SamplerParams sampler;
    bool generated;
    TextureAllocListener *allocListener;
	protected:
    int levelMax; // this describes the highest mip level defined - so 0 if no mipmaps
    unsigned int object;
		Texture( const std::string & texName, TextureFormatEnum texFormat, TextureTargetEnum texTarget );
	public:
    
		virtual ~Texture();
    virtual void Alloc() = 0;
    virtual void Dealloc() = 0;
    
		void Bind( int imageUnit );
		//void Enable( int imageUnit );
		//void Disable( int imageUnit );
		
		const std::string & Name() const {
			return name;
		}
    
		TextureFormatEnum Format() const {
			return format;
		}
		TextureTargetEnum Target() const {
			return target;
		}
    
    GLuint Object() const {
      return object;
    }
    
    bool Generated() const {
      return generated;
    }
    
    void SetGenerated( bool gen ) {
      generated = gen;
    }
		
    int LevelMax() const { return levelMax; }
    
		// egregious hack
		void ClampToEdge();
    
		const SamplerParams & Sampler() const {
			return sampler;
		}
		void SetSampler( const SamplerParams & s );
    
		void SetAllocListener( TextureAllocListener * listener ) {
      allocListener = listener;
    }
    
    void CallAllocListener() {
      if( allocListener ) {
        allocListener->OnTextureAlloc( this );
      }
    }
    
    
	};
	
	class Texture2D : public Texture {
		int width;
		int height;
		int paddedWidth; // to pow2 on OpenGL ES 1
		int paddedHeight;
		Texture2D( const std::string & n, TextureFormatEnum f, int w, int h );
	public:
    virtual void Alloc();
    virtual void Dealloc();
		static Texture2D *Create( const std::string &n, TextureFormatEnum f, int w, int h );
		int Width() const {
			return width;
		}
		
		int Height() const {
			return height;
		}
		
		int PaddedWidth() const {
			return paddedWidth;
		}
		
		int PaddedHeight() const {
			return paddedHeight;
		}
		
		void SetImage( int level, void *data );
		void SetSubImage( int level, int xoff, int yoff, int width, int height, void *data );
		void GetImage( int level, void *pixels );
    
	};
	
	
	Texture2D * CreateTexture2DFromFile( const std::string & filename, TextureFormatEnum f = TextureFormat_INVALID );
	Texture2D * CreateTexture2DTemp();
  
  
  
	class TextureCube : public Texture {
		int width;
		int paddedWidth; // to pow2 on OpenGL ES 1
    int facesSet;
		TextureCube( const std::string & n, TextureFormatEnum f, int w );
	public:
    virtual void Alloc();
    virtual void Dealloc();
		static TextureCube *Create( const std::string &n, TextureFormatEnum f, int w );
		int Width() const {
			return width;
		}
		int PaddedWidth() const {
			return paddedWidth;
		}
		void SetImage( GLenum face, int level, void *data );
		void SetSubImage( GLenum face, int level, int xoff, int yoff, int width, int height, void *data );
		void GetImage( GLenum face, int level, void *pixels );
    
	};
	
	
	TextureCube * CreateTextureCubeFromFile( const std::string & filename, TextureFormatEnum f = TextureFormat_INVALID );
	TextureCube * CreateTextureCubeTemp();
  
	
}

#endif // __R3_TEXTURE_H__
