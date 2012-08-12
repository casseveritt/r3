/*
 *  thread
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

#include "r3/thread.h"

#include "r3/output.h"

#if ANDROID
# include <unistd.h> 
#endif

#include <map>

using namespace r3;
using namespace std;

namespace  {
	Mutex mainThreadMutex;
	int numThreads;
    string invalidThread;
    map<NativeThread,string> threadToName;    
}


#if __APPLE__ || ANDROID || __linux__
void *r3ThreadStart( void * );
void *r3ThreadStart( void *data ) 
#elif _WIN32
DWORD WINAPI r3ThreadStart( LPVOID data )
#endif
{
	Thread * thread = static_cast< Thread * > ( data );

	mainThreadMutex.Acquire( R3_LOC );
	numThreads++;
    threadToName[ thread->GetThreadID() ] = thread->name;
	mainThreadMutex.Release();
#if ANDROID
        Output( "r3ThreadStart starting: pthread_t: %d, pid_t: %d", thread->GetThreadID(), getpid() );
#endif

	thread->Run();
	
	mainThreadMutex.Acquire( R3_LOC );
    threadToName.erase( thread->GetThreadID() );
	numThreads--;
	thread->running = false;
	mainThreadMutex.Release();
#if ANDROID
	extern void platformDetachThread();
	platformDetachThread();
        Output( "r3ThreadStart finishing: pthread_t: %d, pid_t: %d", thread->GetThreadID(), getpid() );
#endif
	return NULL;
}

namespace r3 {
	
	void InitThread() {
        invalidThread = "INVALID THREAD";
        SetThreadName( "main" );
	}
    
    const string & GetThreadName() {
        NativeThread thr = ThreadSelf();
        ScopedMutex m( mainThreadMutex, R3_LOC );
        if( threadToName.count( thr ) ) {
            return threadToName[ thr ];
        }
        return invalidThread;
    }
    
    void SetThreadName( const char *name ) {
        NativeThread thr = ThreadSelf();
        ScopedMutex m( mainThreadMutex, R3_LOC );
        if( name ) {
            threadToName[ thr ] = name;
        } else {
            threadToName.erase( thr );
        }
    }

	
	void Thread::Start() {
		ScopedMutex m( mainThreadMutex, R3_LOC );
		if ( running ) {
			return;
		}
		running = true;
#if __APPLE__ || ANDROID || __linux__
		pthread_create( &threadId, NULL, r3ThreadStart, this );
#elif _WIN32
		threadId = CreateThread( NULL, 0, r3ThreadStart, this, 0, NULL );
#endif
	}
	
	
}

