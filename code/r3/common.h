/*
 *  common
 */

/* 
 Copyright (c) 2010 Cass Everitt
 All rights reserved.
 
 Redistribution and use in source and Binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:
 
 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the following
 disclaimer.
 
 * Redistributions in Binary form must reproduce the above
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

#ifndef __R3_COMMON_H__
#define __R3_COMMON_H__

#include <string>
#include <stdio.h>
#include <assert.h>

#define ARRAY_ELEMENTS( a ) ( sizeof( a ) / sizeof ( a[0] ) )

#ifndef R3_HAS_GL
#define R3_HAS_GL 1
#endif

#ifndef R3_HAS_CONSOLE
#define R3_HAS_CONSOLE R3_HAS_GL
#endif

#define r3Assert( a ) assert( a )

namespace r3 {
	
	typedef long long int64;
	typedef unsigned long long uint64;
	typedef unsigned int uint;
	typedef unsigned short ushort;
	typedef	unsigned char uchar;
	typedef unsigned char byte;
	
}

#if _MSC_VER
# pragma warning(disable: 4018) // signed/unsigned mismatch
# pragma warning(disable: 4244) // conversion possible loss of data
# pragma warning(disable: 4305) // truncation of double to float

# define strdup _strdup
# define r3Sscanf( src, fmt, ... ) sscanf_s( src, fmt, ## __VA_ARGS__ )
# define r3Sprintf( dst, fmt, ... ) sprintf_s( dst, ARRAY_ELEMENTS( dst ), fmt, ## __VA_ARGS__ )
# define r3Vsprintf( dst, fmt, args ) vsprintf_s( dst, ARRAY_ELEMENTS( dst ), fmt, args )
#else
# define r3Sscanf  sscanf
# define r3Sprintf sprintf
# define r3Vsprintf vsprintf
#endif

#define r3ToLower tolower

inline std::string LowerCase( const std::string & str ) {
	std::string ret = str;
	for ( int i = 0; i < (int)ret.size(); i++ ) {
		ret[ i ] = r3ToLower( ret[ i ] );
	}
	return ret;
}

#endif // __R3_COMMON_H__
