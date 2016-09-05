/*
 *  rendertarget
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

#include "r3/rendertarget.h"

#include "r3/gl.h"

#include <assert.h>

namespace {

	GLenum GlRenderBufferFormat[] = {
		0,                          // TextureFormat_INVALID,
		0,                          // TextureFormat_L,
		0,                          // TextureFormat_LA,
		GL_RGB8,                    // TextureFormat_RGB,
		GL_RGBA8,                   // TextureFormat_RGBA,
		GL_DEPTH_COMPONENT16,		// TextureFormat_DepthComponent,
		0                           // TextureFormat_MAX	
	};	
	
}


namespace r3 {
	
	RenderBuffer::RenderBuffer( TextureFormatEnum rbFormat, int rbWidth, int rbHeight ) 
	: format( rbFormat )
	, width( rbWidth )
	, height( rbHeight ) {
		glGenRenderbuffers( 1, & obj );
		glRenderbufferStorage(GL_RENDERBUFFER, GlRenderBufferFormat[ format ], width, height);
	}
	
	RenderBuffer::~RenderBuffer() {
		glDeleteRenderbuffers( 1, & obj );
	}
	
	RenderBuffer * RenderBuffer::Create( TextureFormatEnum format, int width, int height ) {
		return new RenderBuffer( format, width, height );
	}
	
	void RenderBuffer::Bind() {
		glBindRenderbuffer( GL_RENDERBUFFER, obj );
	}
	
	void RenderBuffer::Unbind() {
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );
	}
	
}


