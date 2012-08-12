/*
 *  output
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

#include "r3/output.h"
#include "r3/common.h"
#include "r3/console.h"
#include "r3/thread.h"
#include "r3/var.h"

#include <stdio.h>
#include <stdarg.h>

#if _WIN32
# include <Windows.h>
#endif
r3::VarBool win_OutputDebugString( "win_OutputDebugString", "Dump output to debugger in Windows.", r3::Var_Archive, false );

#if ANDROID
extern void platformOutput(const char *msg);
#endif

using namespace r3;
using namespace std;

namespace {
	Mutex outputMutex;

#if _MSC_VER
	void outputFunction( const char *msg ) {
		if ( win_OutputDebugString.GetVal() ) {
			OutputDebugStringA( str );
			OutputDebugStringA( "\n" );
		}
	}
#elif ANDROID
	void outputFunction( const char *msg ) {
		platformOutput( msg );
	}
#else
	void outputFunction( const char *msg ) {
		fprintf( stderr, "%s\n", msg );
	}
#endif

	void (*theOutputFunction)( const char *msg ) = outputFunction;

	
}

namespace r3 {
	
	void InitOutput() {
	}

	void SetOutputFunction( void (*outFunc)( const char *msg ) ) {
		theOutputFunction = outFunc;
	}
	
	void Output( const char *fmt, ... ) {
		char str[16384];
		va_list args;
		va_start( args, fmt );
		r3Vsprintf( str, fmt, args );
		va_end( args );
#if R3_HAS_CONSOLE
		extern Console *console;
        if( console  != NULL ) {
            console->AppendOutput( str );
        }
#endif
		theOutputFunction( str );
	}

	void OutputDebug( const char *fmt, ... ) {
		char str[1024];
		va_list args;
		va_start( args, fmt );
		r3Vsprintf( str, fmt, args );
		va_end( args );
		// should I make a theDebugFunction()?
		theOutputFunction( str );
	}
	
}
