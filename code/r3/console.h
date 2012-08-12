/*
 *  console
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

#ifndef __R3_CONSOLE_H__
#define __R3_CONSOLE_H__

#include "r3/input.h"

#include <string>
#include <deque>

namespace r3 {

    void InitConsole();
    void ShutdownConsole();

	class Console : public KeyListener, public PointerListener {
		std::deque<std::string> outputBuffer;
		std::deque<std::string> history;
		std::string commandLine;
		int outputBufferPos;
		int cursorPos;
		int historyPos;
		bool active;
	public:
		Console();
        ~Console();
		std::string CommandLine() const {
			return commandLine;
		}
		bool IsActive() const {
			return active;
		}
		void Activate();
		void Deactivate();
		virtual void OnKeyEvent( KeyEvent &keyEvent );
		virtual void OnPointerEvent( PointerEvent &pointerEvent );
		void AppendOutput( const std::string & line );
		void ReadHistory();
		void WriteHistory();
		void Draw();
	};
	
	extern Console *console;

    
}

#endif // __R3_CONSOLE_H__