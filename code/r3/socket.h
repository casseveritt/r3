/*
*  socket
*
*/

/* 
Copyright (c) 2008-2010 Cass Everitt
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

#ifndef __R3_SOCKET_H__
#define __R3_SOCKET_H__

#include "r3/common.h"
#include <string>

#if __APPLE__ || ANDROID
#include <arpa/inet.h>
#endif


namespace r3 {
	uint GetIpAddress( const std::string & hostname );

	enum SocketType {
		ST_Invalid,
		ST_Stream,
		ST_Datagram,
		ST_MAX		
	};
	
	struct Socket {
		Socket( uint sock = -1, SocketType socketType = ST_Invalid ) : s( sock ), type( socketType ) {}
		~Socket() {}
		bool Invalid() const { return s == -1 || type == ST_Invalid; }
		bool Connect( uint host, int port );
		void Close();
		void Disconnect() { Close(); }
                void SetNoDelay( bool noDelay = true );
		bool SetNonblocking();

		// UDP methods
		bool SendTo( uint host, int port, char *src, int bytes );
		uint Recv( char *dst, uint dst_bytes );

		
		bool Read( char * dst, uint dst_bytes );
		int ReadPartial( char * dst, uint dst_bytes ); // only make one read attempt
		bool Write( const char * src, uint src_bytes );        

		bool CanRead();

		template <typename T>
		bool Read( T & t ) {
			return Read( (char *) & t, sizeof(T) );
		}

		template <typename T>
		bool Write( const T & t ) {
			return Write( (char *) & t, sizeof(T) );
		}

		SocketType type;
		int s;
	};

	// Listener creates a socket and listens on a port.
	// If can_accept() is true, then accept() returns a
	// Socket for a new connection.
	struct Listener {
		Listener() : s(-1) {}
		~Listener() { StopListening(); }
		bool Listen( int port );
		void StopListening();
		bool SetNonblocking();
		Socket Accept();
		int s;
           private:
		void Close();
	};

}

#endif // __R3_SOCK_H__

