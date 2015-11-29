/*
 *  time
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

#ifndef __R3_TIME_H__
#define __R3_TIME_H__

#if _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN 1
# endif
# include <windows.h>
# include <sys/timeb.h>
# include <time.h>
#else
# include <sys/time.h>
# include <unistd.h>
#endif 

#if ANDROID
# include <time.h>
# include <time64.h>
#endif

namespace r3 {

	void SetTimeOffset( double offset );
	double GetTimeOffset();
	
#if _WIN32
	inline double GetTime() {
		struct _timeb timebuffer;
		_ftime64_s( &timebuffer );
		return timebuffer.time + timebuffer.millitm / 1000.0 + GetTimeOffset();
	}
	inline struct tm *Gmtime( const time_t *gmt ) {
		static tm val;
		gmtime_s( &val, gmt );
		return &val;
	}
#else
	inline double GetTime() {
		timeval tv;
		gettimeofday( &tv, NULL );
		return tv.tv_sec + tv.tv_usec / 1000000.0 + GetTimeOffset();
	}
# define Gmtime gmtime

#endif



	inline float GetSeconds() {
		static double tstartup;
		double t = GetTime();
		if ( tstartup == 0.0 ) {
			tstartup = t;
		}
		return float( t - tstartup );
	}
	
	inline void SleepMilliseconds( int i ) {
#if __APPLE__ || ANDROID
		usleep( i * 1000 );
#elif _WIN32
		Sleep( i );
#endif
	}
	
}

#endif // __R3_TIME_H__
