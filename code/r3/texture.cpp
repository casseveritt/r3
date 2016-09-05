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

#include "r3/texture.h"

#include "r3/command.h"
#include "r3/common.h"
#include "r3/image.h"
#include "r3/output.h"
#include "r3/gl.h"

#include <map>

#include <math.h>
#include <assert.h>


using namespace std;
using namespace r3;

#define GET_ERROR( ev )

namespace {
  
  const GLenum GlTarget[] = {
    GL_TEXTURE_1D,       // TextureTarget_1D,
    GL_TEXTURE_2D,       // TextureTarget_2D,
    GL_TEXTURE_3D,       // TextureTarget_3D,
    GL_TEXTURE_CUBE_MAP, // TextureTarget_Cube
  };
  
#if 0 // Need a feature level check
  const GLenum GlInternalFormat[] = {
    0,                          // TextureFormat_INVALID,
    GL_ALPHA,                   // TextureFormat_A,
    GL_LUMINANCE_ALPHA,         // TextureFormat_LA,
    GL_RGB8,                    // TextureFormat_RGB,
    GL_RGBA8,                   // TextureFormat_RGBA,
    GL_DEPTH_COMPONENT16,		// TextureFormat_DepthComponent,
    0                           // TextureFormat_MAX
  };
#else
# define GlInternalFormat GlFormat
#endif
  
  const GLenum GlFormat[] = {
    0,                          // TextureFormat_INVALID,
    GL_ALPHA,                   // TextureFormat_A,
    GL_RGB,                     // TextureFormat_RGB,
    GL_RGBA,                    // TextureFormat_RGBA,
    GL_DEPTH_COMPONENT,			// TextureFormat_DepthComponent,
    0                           // TextureFormat_MAX
  };
  
  const char *FormatString[] = {
    "INVALID",                  // TextureFormat_INVALID,
    "L",                        // TextureFormat_L,
    "RGB",                      // TextureFormat_RGB,
    "RGBA",                     // TextureFormat_RGBA,
    "WTF?"                      // TextureFormat_MAX
  };
  
  
  const GLenum GlMagFilt[3] = {
    GL_NEAREST, // TextureFilter_None
    GL_NEAREST, // TextureFilter_Nearest
    GL_LINEAR   // TextureFilter_linear
  };
  
  const GLenum GlMipMinFilt[3][3] = { // indexed by [MipFilt][MinFilt]
    { // MipFilt = None
      GL_NEAREST,  // None
      GL_NEAREST,  // Nearest
      GL_LINEAR    // Linear
    },
    { // MipFilt = Nearest
      GL_NEAREST_MIPMAP_NEAREST,  // None
      GL_NEAREST_MIPMAP_NEAREST,  // Nearest
      GL_NEAREST_MIPMAP_LINEAR    // Linear
    },
    { // MipFilt = Linear
      GL_LINEAR_MIPMAP_NEAREST,  // None
      GL_LINEAR_MIPMAP_NEAREST,  // Nearest
      GL_LINEAR_MIPMAP_LINEAR    // Linear
    }
  };
  
  
  
  struct TextureDatabase {
    map< string, Texture * > textures;
    ~TextureDatabase() {
      while( textures.size() > 0 ) {
        delete textures.begin()->second;
      }
    }
    Texture * GetTexture( const string & name ) {
      if ( textures.count( name ) ) {
        return textures[ name ];
      }
      return NULL;
    }
    void AddTexture( const string & name, Texture * tex ) {
      assert( tex != NULL );
      assert( textures.count( name ) == 0 );
      textures[ name ] = tex;
    }
    void DeleteTexture( const string & name ) {
      assert( textures.count( name ) != 0 );
      textures.erase( name );
    }
  };
  TextureDatabase *textureDatabase;
  
  int modBindUnit = 0; // what texture unit is used for "bind for modification"?
  
  bool initialized = false;
  
  // listTextures command
  void ListTextures( const vector< Token > & tokens ) {
    if ( textureDatabase == NULL ) {
      return;
    }
    map< string, Texture * > &textures = textureDatabase->textures;
    for( map< string, Texture * >::iterator it = textures.begin(); it != textures.end(); ++it ) {
      const string & n = it->first;
      Texture *tex = it->second;
      const char *fmt = FormatString[ (int)tex->Format() ];
      switch ( tex->Target() ) {
        case TextureTarget_2D:
        {
          Texture2D *tex2d = static_cast< Texture2D * >( tex );
          Output( "2D - w=%d h=%d fmt=%s - %s", tex2d->Width(), tex2d->Height(), fmt, n.c_str() );
        }
          break;
        default:
          break;
      }
    }
  }
  CommandFunc ListTexturesCmd( "listtextures", "lists defined textures", ListTextures );
  
  // alloctextures command
  void AllocTextures( const vector< Token > & tokens ) {
    if ( textureDatabase == NULL ) {
      return;
    }
    AllocTexture();
  }
  CommandFunc AllocTexturesCmd( "alloctextures", "allocs/loads GL texture object for defined textures", AllocTextures );
  
  // dealloctextures command
  void DeallocTextures( const vector< Token > & tokens ) {
    if ( textureDatabase == NULL ) {
      return;
    }
    DeallocTexture();
  }
  CommandFunc DeallocTexturesCmd( "dealloctextures", "deallocs GL texture object for defined textures", DeallocTextures );
  
  bool IsPowerOfTwo( int val ) {
    return ( val > 0 ) && ( val & ( val - 1 ) ) == 0;
  }
  
  int NextPowerOfTwo( int val ) {
    int lnext = (int)ceil( log( float( val ) ) / log( 2.0f ) );
    return 1 << lnext;
  }
  
  bool hasAniso = false;
}


namespace r3 {
  
  void InitTexture() {
    if ( initialized ) {
      return;
    }
    Output( "Initializing r3::Texture." );
    textureDatabase = new TextureDatabase;
    const char * ext = (const char *)glGetString( GL_EXTENSIONS );
    string s;
    if( ext ) {
      s = ext;
      s += " ";
    } else {
      int num;
      glGetIntegerv( GL_NUM_EXTENSIONS, & num );
      for( int i = 0; i < num; i++ ) {
        ext = (const char *)glGetStringi( GL_EXTENSIONS, i );
        s += string( ext ) + " ";
      }
    }
    
    if( s.find( "_texture_filter_anisotropic " ) != string::npos ) {
      hasAniso = true;
    }
    
    initialized = true;
  }
  
  void ShutdownTexture() {
    if( ! initialized ) {
      return;
    }
    Output( "Shutting down r3::Texture." );
    delete textureDatabase;
    textureDatabase = NULL;
    initialized = false;
  }
  
  void AllocTexture() {
    for( map< string, Texture * >::iterator it = textureDatabase->textures.begin(); it != textureDatabase->textures.end(); ++it ) {
      it->second->Alloc();
    }
  }
  
  void DeallocTexture() {
    for( map< string, Texture * >::iterator it = textureDatabase->textures.begin(); it != textureDatabase->textures.end(); ++it ) {
      it->second->Dealloc();
    }
  }
  
  Texture::Texture( const std::string & texName, TextureFormatEnum texFormat, TextureTargetEnum texTarget )
  : name( texName ), format( texFormat ), target( texTarget ), generated( true ), object( 0 ), allocListener( NULL ) {
    textureDatabase->AddTexture( name, this );
    glGenTextures( 1, (GLuint *) &object );
    SetSampler( sampler );
    levelMax = 0;
  }
  
  Texture::~Texture() {
    textureDatabase->DeleteTexture( name );
    glDeleteTextures( 1, &object );
  }
  
  
  void Texture::Bind( int imageUnit ) {
    Bind( modBindUnit );
    glBindTexture( GlTarget[ target ], object );
  }
  
  void Texture::ClampToEdge() {
    Bind( modBindUnit );
    GLenum t = GlTarget[ target ];
    glTexParameteri( t, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( t, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE	);
  }
  
  void Texture::SetSampler( const SamplerParams & s ) {
    GET_ERROR( e );
    Bind( modBindUnit );
    sampler = s;
    GLenum t = GlTarget[ target ];
    glTexParameteri( t, GL_TEXTURE_MAG_FILTER, GlMagFilt[ s.magFilter ] );
    GLenum f = GlMipMinFilt[ sampler.mipFilter ][ sampler.minFilter ];
    glTexParameteri( t, GL_TEXTURE_MIN_FILTER,  f );
#if 0 // Need a feature level check here
    glTexParameterf( t, GL_TEXTURE_LOD_BIAS, s.levelBias );
    glTexParameterf( t, GL_TEXTURE_MIN_LOD, s.levelMin );
    glTexParameterf( t, GL_TEXTURE_MAX_LOD, s.levelMax );
#endif
    if( hasAniso ) {
      glTexParameterf( t, GL_TEXTURE_MAX_ANISOTROPY_EXT, s.anisoMax );
    }
    GET_ERROR( e2 );
    //Output( "SetSampler on %s - e = %x, e2 = %x", name.c_str(), e, e2 );
  }
  
  Texture2D::Texture2D( const std::string & texName, TextureFormatEnum texFormat, int texWidth, int texHeight  )
  : Texture( texName, texFormat, TextureTarget_2D ), width( texWidth ), height( texHeight ) {
  }
  
  Texture2D * Texture2D::Create( const std::string & texName, TextureFormatEnum texFormat, int texWidth, int texHeight ) {
    if ( textureDatabase->GetTexture( texName ) ) {
      Output( "texName \"%s\" already in use.  Returning NULL.", texName.c_str() );
      return NULL;
    }
    return new Texture2D( texName, texFormat, texWidth, texHeight );
  }
  
  void Texture2D::Alloc() {
    if( object == 0 ) {
      glGenTextures( 1, &object );
    }
    
    SetSampler( Sampler() );
    
    if( Generated() ) {
      SetImage( 0, NULL );
    } else {
      Image<byte> * img = ReadImageFile( Name(), Format() );
      if ( img == NULL ) {
        Output( "Failed creating texture for %s", Name().c_str() );
        return;
      }
      SetImage( 0, &img->data[0] );
      Output( "Reloaded image %s (w=%d, h=%d).", Name().c_str(), Width(), Height() );
      delete img;
    }
    
    CallAllocListener();
  }
  
  void Texture2D::Dealloc() {
    if( object == 0 ) {
      return;
    }
    glDeleteTextures( 1, &object );
    object = 0;
  }
  
  
  void Texture2D::SetImage( int level, void *data ) {
    GET_ERROR( e );
    Bind( modBindUnit );
    
    // FIXME:  Need a feature level query
    paddedWidth = NextPowerOfTwo( width );
    paddedHeight = NextPowerOfTwo( height );
    
    
    glTexImage2D( GlTarget[ Target() ], level, GlInternalFormat[ Format() ], paddedWidth, paddedHeight, 0, GlFormat[ Format() ], GL_UNSIGNED_BYTE, NULL );
    GET_ERROR( e2 );
    //Output( "setting image data - e = %x, e2 = %x, format = %d, width = %d, height = %d, paddedWidth = %d, paddedHeight = %d", e, e2, Format(), width, height, paddedWidth, paddedHeight );
    if( data ) {
      glTexSubImage2D( GlTarget[ Target() ], level, 0, 0, width, height, GlFormat[ Format() ], GL_UNSIGNED_BYTE, data );
    }
    if ( level == 0 ) {
        glGenerateMipmap( GlTarget[ Target() ] );
        levelMax = int( log( double( max( width, height ) ) ) / log( 2.0 ) + 0.5 );
    }
  }
  
  void Texture2D::SetSubImage( int level, int xoff, int yoff, int w, int h, void *data ) {
    GET_ERROR( e );
    Bind( modBindUnit );
    glTexSubImage2D( GlTarget[ Target() ], level, xoff, yoff, w, h, GlFormat[ Format() ], GL_UNSIGNED_BYTE, data );
    GET_ERROR( e2 );
    //Output( "setting sub image data - e=%x, e2=%x, xoff=%d, yoff=%d, w=%d, h=%d", e, e2, xoff, yoff, w, h );
  }
  
  void Texture2D::GetImage( int level, void *pixels ) {
    Bind( modBindUnit );
    
    // FIXME: Need a feature level query, or Regal needs to construct an FBO with this texture and read it back via ReadPixels
    glGetTexImage( GlTarget[ Target() ], level, GlFormat[ Format() ], GL_UNSIGNED_BYTE, pixels );
  }
  
  Texture2D * CreateTexture2DFromFile( const std::string & filename, TextureFormatEnum f ) {
    Texture2D * tex = (Texture2D *)textureDatabase->GetTexture( filename );
    if ( tex ) {
      Output( "Returned already loaded texture %s", filename.c_str() );
      return tex;
    }
    Image<byte> * img = ReadImageFile( filename, f );
    if ( img == NULL ) {
      Output( "Failed creating texture for %s", filename.c_str() );
      return NULL;
    }
    Output( "Creating texture for %s", filename.c_str() );
    
    tex = Texture2D::Create( filename, (TextureFormatEnum)img->Components(), img->Width(), img->Height() );
    tex->SetGenerated( false );
    tex->SetImage( 0, &img->data[0] );
    Output( "Loaded image %s (w=%d, h=%d)", filename.c_str(), img->Width(), img->Height() );
    delete img;
    return tex;
  }
  
  Texture2D * CreateTexture2DTemp() {
    string name = "temp2d";
    Texture2D * tex = (Texture2D *)textureDatabase->GetTexture( name );
    if ( tex ) {
      Output( "Returned already loaded texture %s", name.c_str() );
      return tex;
    }
    Image<byte> * img = new Image<byte>();
    int w = 128;
    img->SetSize( w, w, 4 );
    for( int i = 0; i < w; i++ ) {
      for( int j = 0; j < w; j++ ) {
        byte * c = &(*img)( i, j, 0 );
        byte x = (byte)(i ^ j);
        c[0] = c[1] = c[2] = c[3] = x;
      }
    }
    Output( "Creating texture for %s", name.c_str() );
    
    tex = Texture2D::Create( name, (TextureFormatEnum)img->Components(), img->Width(), img->Height() );
    tex->SetGenerated( true );
    tex->SetImage( 0, &img->data[0] );
    Output( "Loaded image %s (w=%d, h=%d)", name.c_str(), img->Width(), img->Height() );
    delete img;
    return tex;
  }
  
  TextureCube::TextureCube( const std::string & texName, TextureFormatEnum texFormat, int texWidth )
  : Texture( texName, texFormat, TextureTarget_Cube ), width( texWidth ), facesSet( 0 ) {
  }
  
  TextureCube * TextureCube::Create( const std::string & texName, TextureFormatEnum texFormat, int texWidth ) {
    if ( textureDatabase->GetTexture( texName ) ) {
      Output( "texName \"%s\" already in use.  Returning NULL.", texName.c_str() );
      return NULL;
    }
    return new TextureCube( texName, texFormat, texWidth );
  }
  
  void TextureCube::Alloc() {
    if( object == 0 ) {
      glGenTextures( 1, &object );
    }
    
    SetSampler( Sampler() );
    
    GLenum face[] = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    
    if( Generated() ) {
      for( int i = 0; i < 6; i++ ) {
        SetImage( face[i], 0, NULL );
      }
    } else {
      Image<byte> * img = ReadImageFile( Name(), Format() );
      if ( img == NULL ) {
        Output( "Failed creating texture for %s", Name().c_str() );
        return;
      }
      if ( img->Width() * 2 != img->Height() * 3 ) {
        Output( "Failed creating texture for %s, image dimensions must be 3:2 - w=%d, h=%d", Name().c_str(), img->Width(), img->Height() );
        return;
      }
      Output( "Creating texture for %s", Name().c_str() );
      
      int w = img->Height() / 2;
      
      GLenum face[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
      };
      int xoff[] = { 0, 1, 2, 0, 1, 2 };
      int yoff[] = { 1, 1, 1, 0, 0, 0 };
      int rot[] = { 0, 1, 2, 2, 2, 2 };
      Image<byte> subimg;
      subimg.SetSize( w, w, img->Components());
      for( int i = 0; i < 6; i++ ) {
        img->CopySubImage( subimg, w * xoff[i], w * yoff[i] );
        subimg.Rotate( rot[i] );
        SetImage( face[i], 0, &subimg.data[0] );
      }
      Output( "Reloaded image %s (w=%d, h=%d)", Name().c_str(), img->Width(), img->Height() );
      delete img;
    }
    
    CallAllocListener();
  }
  
  void TextureCube::Dealloc() {
    if( object == 0 ) {
      return;
    }
    glDeleteTextures( 1, &object );
    object = 0;
  }
  
  
  void TextureCube::SetImage( GLenum face, int level, void *data ) {
    GET_ERROR( e );
    Bind( modBindUnit );
    
    // FIXME:  Need a feature level query
    paddedWidth = NextPowerOfTwo( width );
    
    switch( face ) {
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X: facesSet |= 0x01; break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: facesSet |= 0x02; break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: facesSet |= 0x04; break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: facesSet |= 0x08; break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: facesSet |= 0x10; break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: facesSet |= 0x20; break;
      default: break;
    }
    
    //Output( "Loading face %d", face );
    glTexImage2D( face, level, GlInternalFormat[ Format() ], paddedWidth, paddedWidth, 0, GlFormat[ Format() ], GL_UNSIGNED_BYTE, NULL );
    GET_ERROR( e2 );
    //Output( "setting image data - e = %x, e2 = %x, format = %d, width = %d, height = %d, paddedWidth = %d, paddedHeight = %d", e, e2, Format(), width, height, paddedWidth, paddedHeight );
    if( data ) {
      glTexSubImage2D( face, level, 0, 0, width, width, GlFormat[ Format() ], GL_UNSIGNED_BYTE, data );
    }
    if ( level == 0 && facesSet == 0x3f ) {
        glGenerateMipmap( GlTarget[ Target() ] );
        levelMax = int( log( double( width ) ) / log( 2.0 ) + 0.5 );
    }
  }
  
  void TextureCube::SetSubImage( GLenum face, int level, int xoff, int yoff, int w, int h, void *data ) {
    GET_ERROR( e );
    Bind( modBindUnit );
    glTexSubImage2D( face, level, xoff, yoff, w, h, GlFormat[ Format() ], GL_UNSIGNED_BYTE, data );
    GET_ERROR( e2 );
    //Output( "setting sub image data - e=%x, e2=%x, xoff=%d, yoff=%d, w=%d, h=%d", e, e2, xoff, yoff, w, h );
  }
  
  void TextureCube::GetImage( GLenum face, int level, void *pixels ) {
    Bind( modBindUnit );
    
    // FIXME: Need a feature level query, or Regal needs to construct an FBO with this texture and read it back via ReadPixels
    glGetTexImage( face, level, GlFormat[ Format() ], GL_UNSIGNED_BYTE, pixels );
  }
  
  
  
  TextureCube * CreateTextureCubeFromFile( const std::string & filename, TextureFormatEnum f ) {
    TextureCube * tex = (TextureCube *)textureDatabase->GetTexture( filename );
    if ( tex ) {
      Output( "Returned already loaded texture %s", filename.c_str() );
      return tex;
    }
    Image<byte> * img = ReadImageFile( filename, f );
    if ( img == NULL ) {
      Output( "Failed creating texture for %s", filename.c_str() );
      return NULL;
    }
    if ( img->Width() * 2 != img->Height() * 3 ) {
      Output( "Failed creating texture for %s, image dimensions must be 3:2 - w=%d, h=%d", filename.c_str(), img->Width(), img->Height() );
      return NULL;
    }
    Output( "Creating texture for %s", filename.c_str() );
    
    int w = img->Height() / 2;
    tex = TextureCube::Create( filename, (TextureFormatEnum)img->Components(), w );
    tex->SetGenerated( false );
    
    GLenum face[] = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };
    int xoff[] = { 0, 1, 2, 0, 1, 2 };
    int yoff[] = { 1, 1, 1, 0, 0, 0 };
    int rot[] = { 0, 1, 2, 2, 2, 2 };
    Image<byte> subimg;
    subimg.SetSize( w, w, img->Components());
    for( int i = 0; i < 6; i++ ) {
      img->CopySubImage( subimg, w * xoff[i], w * yoff[i] );
      subimg.Rotate( rot[i] );
      tex->SetImage( face[i], 0, &subimg.data[0] );
    }
    Output( "Loaded image %s (w=%d, h=%d)", filename.c_str(), img->Width(), img->Height() );
    delete img;
    return tex;
  }
  
  
  TextureCube * CreateTextureCubeTemp() {
    string name = "tempcube";
    TextureCube * tex = (TextureCube *)textureDatabase->GetTexture( name );
    if ( tex ) {
      Output( "Returned already loaded texture %s", name.c_str() );
      return tex;
    }
    Image<byte> * img = new Image<byte>();
    int w = 128;
    img->SetSize( w, w, 4 );
    for( int i = 0; i < w; i++ ) {
      for( int j = 0; j < w; j++ ) {
        byte * c = &(*img)( i, j, 0 );
        byte x = (byte)(i ^ j);
        c[0] = c[1] = c[2] = c[3] = x;
      }
    }
    Output( "Creating texture for %s", name.c_str() );
    
    tex = TextureCube::Create( name, (TextureFormatEnum)img->Components(), w );
    tex->SetGenerated( false );
    
    GLenum face[] = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };
    for( int i = 0; i < 6; i++ ) {
      tex->SetImage( face[i], 0, &img->data[0] );
    }
    Output( "Loaded image %s (w=%d, h=%d)", name.c_str(), img->Width(), img->Height() );
    delete img;
    return tex;
  }
  
  
}
