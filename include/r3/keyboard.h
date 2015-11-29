/*
 * r3 virtual keyboard
 *
 * Copyright (c) 2010 Cass Everitt
 * All rights reserved.
 *
 */

#ifndef __R3_KEYBOARD_H__
#define __R3_KEYBOARD_H__

#include "r3/bounds.h"
#include "r3/linear.h"

namespace r3 {
	
	struct Keyboard {
		
		virtual ~Keyboard() {}

		virtual void SetAspect( float aspect ) = 0;
		virtual void Draw() = 0;
		virtual void BeginFocus() = 0;
		virtual void FocusPosition( Vec2f pos ) = 0;
		virtual void EndFocus() = 0;
		
		virtual void Activate() = 0; // at the current position while focused...
		virtual void SetRect( const Rect & r ) = 0;
	};
	
	Keyboard * CreateSimpleKeyboard();
	
}

#endif // __R3_KEYBOARD_H__