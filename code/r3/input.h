/*
 *  input
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

#ifndef __R3_INPUT_H__
#define __R3_INPUT_H__

// use X keysym defs
#define XK_MISCELLANY 1
#define XK_LATIN1 1
#include "r3/keysymdef.h"
#include "r3/linear.h"
#include <algorithm>

namespace r3 {
	
	void InitInput();

	enum KeyStateEnum {
		KeyState_Down,
		KeyState_Up
	};
	
	struct KeyEvent {
		KeyEvent( int inKey, KeyStateEnum inState ) : key( inKey ), state( inState ), handled( false ) {}
		int key;
		KeyStateEnum state;
		bool handled;
	};

	struct KeyListener {
		virtual void OnKeyEvent( KeyEvent &keyEvent ) = 0;
	};

	void PushKeyListener( KeyListener *listener );
	KeyListener *PopKeyListener();
	void CreateKeyEvent( int key, KeyStateEnum keyState );


	struct PointerEvent {
		PointerEvent( bool inActive, int inX, int inY ) : active( inActive ), x( inX ), y( inY ), handled( false ) {}
		bool active;
		int x;
		int y;
		bool handled;
	};

	struct PointerListener {
		virtual void OnPointerEvent( PointerEvent &pointerEvent ) = 0;
	};

	void PushPointerListener( PointerListener *listener );
	PointerListener *PopPointerListener();
	void CreatePointerEvent( bool active, int x, int y );

	
	struct ReshapeEvent {
		ReshapeEvent( int w, int h ) : width( w ), height( h ), handled( false ) {}
		int width;
		int height;
		bool handled;
	};

	struct ReshapeListener {
		virtual void OnReshape( ReshapeEvent &reshapeEvent ) = 0;
	};
	
	void PushReshapeListener( ReshapeListener *reshapeListener );
	ReshapeListener *PopReshapeListener();
	void CreateReshapeEvent( int w, int h );


	int AsciiToKey( unsigned char key );
	
	struct Trackball {
		Trackball() {}
		Trackball( int w, int h ) : width( w ), height( h ) {}

		int width;
		int height;
		
		Rotationf GetRotation( Vec2f p0, Vec2f p1 ) {
			Vec2f dp = p1 - p0;
			if( dp.x == 0 && dp.y == 0) {
				return Rotationf();
			}
			
			float minDim = std::min<float>( (float)width, (float)height );
			minDim /= 2.f;
			Vec3f offset( width / 2.f, height / 2.f, 0);
			Vec3f a = ( Vec3f( p0.x, p0.y, 0) - offset ) / minDim;
			Vec3f b = ( Vec3f( p1.x, p1.y, 0) - offset ) / minDim;
			
			a.z = -(float)pow(2, -0.5 * a.Length() );
			a.Normalize();
			b.z = -(float)pow(2, -0.5 * b.Length() );
			b.Normalize();
			
			Vec3f axis = a.Cross( b );
			axis.Normalize();
			
			float angle = acos( a.Dot( b ) );
			return Rotationf( axis, angle );			
		}
		
		
	};
	
}

#endif // __R3_INPUT_H__