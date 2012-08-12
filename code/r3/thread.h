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

#ifndef __R3_THREAD_H__
#define __R3_THREAD_H__

#if _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN 1
# endif
# include <windows.h>
#else
# include <pthread.h>
#endif

#include <string>

#define R3_STRINGIZE(x) R3_STRINGIZE2(x)
#define R3_STRINGIZE2(x) #x
#define R3_LOC __FILE__ ":" R3_STRINGIZE(__LINE__)
//#define R3_MUTEX_DEBUG(op) fprintf( stderr, "Mutex %s by %s: %s\n", op, GetThreadName().c_str(), loc )
#define R3_MUTEX_DEBUG(op) 

namespace r3 {
	
	void InitThread();
    void SetThreadName( const char *name );
    const std::string & GetThreadName();
	
#if _WIN32
    typdef HANDLE NativeThread;
#else
    typedef pthread_t NativeThread;
#endif

    inline NativeThread ThreadSelf() {
#if _WIN32
        return GetCurrentThreadId();
#else
        return pthread_self();
#endif
    }
    
    
#if _WIN32
	class Mutex {
		HANDLE mutex;
	public:
		Mutex() {
			mutex = CreateMutex( 0, FALSE, 0 ); 
		}
		~Mutex() {
			CloseHandle( mutex );
		}
		void Acquire( const char * locstr ) {
            loc = locstr;
            R3_MUTEX_DEBUG( "acquire" );
			WaitForSingleObject( mutex, INFINITE );
		}
		void Release() {
            R3_MUTEX_DEBUG( "release" );
			ReleaseMutex( mutex );
		}
        const char * loc;
	};
    
	class Thread {
		HANDLE threadId;
	public:
		Thread(const char *threadName) : name( threadName ), running( false ) {
		}
        std::string name;
		bool running;
		void Start();
		virtual void Run() = 0;
        HANDLE GetThreadID() const {
            return threadId;
        }
	};
#else
	class Mutex {
		pthread_mutex_t mutex;
	public:
		Mutex() {
			pthread_mutex_init( &mutex, NULL );
		}
		void Acquire( const char * locstr ) {
            loc = locstr;
            R3_MUTEX_DEBUG( "acquire" );
			pthread_mutex_lock( &mutex );
		}
		void Release() {
           R3_MUTEX_DEBUG( "release" );
			pthread_mutex_unlock( &mutex );
		}
        const char * loc;
	};
#endif
    
#if _WIN32
    class Condition {
        
    };
#else
    class Condition {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
    public:
        Condition() {
            pthread_mutex_init( &mutex, NULL );
            pthread_cond_init( &cond, NULL );
        }
        void Wait() {
            pthread_mutex_lock( &mutex );
            pthread_cond_wait( &cond, &mutex );
            pthread_mutex_unlock( &mutex );
        }
        void Broadcast() {
            pthread_cond_broadcast( &cond );
        }
    };
#endif
	
	class Thread {
		pthread_t threadId;
	public:
		Thread(const char *threadName) : name( threadName ), running( false ) {
		}
        std::string name;
		bool running;
		void Start();
		virtual void Run() = 0;
        pthread_t GetThreadID() const {
            return threadId;
        }
	};
	
	class ScopedMutex {
		Mutex * m;
	public:
		ScopedMutex( Mutex & inMutex, const char * loc ) : m( & inMutex ) {
			if( m ) m->Acquire( loc );
		}
		~ScopedMutex() {
			if( m ) m->Release();
		}
	};
    
	class ScopedReverseMutex {
		Mutex * m;
	public:
		ScopedReverseMutex( Mutex & inMutex, const char * loc) : m( & inMutex ) {
            if( m ) { m->loc = loc; m->Release(); }
		}
		~ScopedReverseMutex() {
			if( m ) m->Acquire( m->loc );
		}
	};
    
}

#endif // __R3_THREAD_H__
