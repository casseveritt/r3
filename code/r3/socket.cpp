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


#include "r3/socket.h"

#if ! R3_OUTPUT_ABSENT
# include "r3/output.h"
#else
# define Output printf
#endif

#if ! _WIN32
# define SOCKET int
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
#else 
# include <io.h>
# include <winsock2.h>
# include <windows.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>


using namespace std;
using namespace r3;

#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
namespace {
	void InitWindowsSockets() {
		static bool initialized = false;
		if( ! initialized ) {
			WSADATA wsadata;
			if( int err = WSAStartup( MAKEWORD(2,0), &wsadata ) ) {
				Output( "Could not initialize Windows sockets." );
			}
			initialized = true;
		}
	}
}
#define INIT_SOCKET_LIB() InitWindowsSockets()
#define GET_ERROR() WSAGetLastError()
#else
#define INIT_SOCKET_LIB()
#define GET_ERROR() errno
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif

#define SOCKADDR sockaddr


namespace r3 {

	uint GetIpAddress( const string & hostname ) {
		INIT_SOCKET_LIB();
		hostent *hp;
		uint addr;

		if(hostname.size() == 0) {
			return 0;
		}

		addr = inet_addr( hostname.c_str() );

		if( addr != -1 ) {
			return addr;
		}

		hp = gethostbyname( hostname.c_str() );

		if( hp ) {
			addr = *(uint *) hp->h_addr;
			return addr;
		}

		return -1;
	}


	bool Socket::Connect( uint host, int port ) {
		INIT_SOCKET_LIB();
		sockaddr_in addr;
		int one = 1;

		if ( Invalid() == false ) {
			Output( "r3::Socket::Connect() call to existing socket" );			
		}
		
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = host;

		s = socket(AF_INET, SOCK_STREAM, 0);
		if( s < 0 ) {
			Output( "r3::Socket::Connect() call to ::socket() failed" );
			s = -1;
			return false;
		}
		type = ST_Stream;

		if( connect( s, (sockaddr *) & addr, sizeof(addr) ) < 0 ) {
			perror("connect error");
			Output( "r3::Socket::Connect() call to ::connect() failed" );
			Close();
			return false;
		}

		if( setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (char *) & one, sizeof(one) ) < 0 ) {
			perror("setsockopt error");
			Output( "r3::Socket::Connect() call to ::setsockopt() failed" );
			Close();
			return false;
		}

		return true;
	}

	bool Socket::SendTo( uint host, int port, char *src, int bytes ) {
		INIT_SOCKET_LIB();
		
		if ( Invalid() ) {
			s = socket(AF_INET, SOCK_DGRAM, 0);
			if( s < 0 ) {
				Output( "r3::Socket::SendTo() call to ::socket() failed" );
				s = -1;
				return false;
			}
			
			/*
			sockaddr_in a;
			a.sin_family = AF_INET;
			a.sin_port = 0;
			a.sin_addr.s_addr = INADDR_ANY;
			
			if ( bind( s, (sockaddr *)& a, sizeof( a ) ) == -1 ) {
				Output( "r3::Socket::SendTo() call to ::bind() failed" );				
				Close();
				return false;
			} 
			*/
			type = ST_Datagram;
			
		}

		if ( type != ST_Datagram ) {
			Output( "r3::Socket::SendTo() called on non-datagram socket" );			
			return false;
		}
		
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = host;
		
		if ( sendto( s, src, bytes, 0, (sockaddr*) & addr, sizeof( addr ) ) != bytes ) {
			perror("sendto error");
			Output( "r3::Socket::SendTo() call to sendto() failed" );
			return false;
		}
		
		return true;
	}
	
	uint Socket::Recv( char *dst, uint dst_bytes ) {
		if ( Invalid() || type != ST_Datagram ) {
			Output( "r3::Socket::Recv() invalid socket or socket type" );
			return -1;
		}
		return (uint)recv( s, dst, dst_bytes, 0 );
	}

	void Socket::Close() {
		if( Invalid() ) {
			return;
                }
		
#ifdef _WIN32
		closesocket( s );
#else
		close((int)s);
#endif
		type = ST_Invalid;
		s = -1;
	}

        void Socket::SetNoDelay( bool noDelay ) {
                unsigned int setting = noDelay ? 1 : 0;
#if ! _WIN32
		if( setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (char *) & setting, sizeof( setting ) ) < 0 ) {
			perror("setsockopt error");
                }
#endif
        }

	bool Socket::SetNonblocking() {
#ifdef _WIN32
		u_long one = 1;
		if( ioctlsocket(s, FIONBIO, & one) ) {
			Output( "r3::Socket::SetNonblocking: ioctlsocket" );
			return false;
		}
#else 
		if( fcntl( s, F_SETFL, O_NONBLOCK ) < 0 ) {
			Output( "r3::Socket::set_nonblocking: fcntl" );
			return false;
		}
#endif
		return true;

	}

	bool Socket::Read( char * dst, uint dst_bytes ) {
		//Output( "Socket::read() %d bytes to 0x%x", dst_bytes, dst);
		int n = dst_bytes;
		while( n > 0 ) {
			int i = (int)recv( s, dst, n, 0 );
			if( i < 0 ) {
#ifdef _WIN32
				int wsaerr = GET_ERROR();
				if( wsaerr == 10035 ) {
					Sleep(1); // no data, recv would block
					continue;	  // wait a msec and try again
				}
				return false;
#else
				if( /*errno == EWOULDBLOCK ||*/ errno == EAGAIN ) {
					i=0;
				} else {
					Output( "r3::Socket::read: recv" );
					return false;
				}
#endif
			} else if ( i == 0 ) {
				Output( "Socket::read(): connection closed on remote end\n");
				return false;
			}
			//Output( "i=%d, n=%d",i,n );
			dst += i;
			n -= i;
		}
		//fprintf(stderr,"Done\n");
		return true;
	}

	int Socket::ReadPartial( char * dst, uint dst_bytes ) {
		//fprintf(stderr,"r3::Socket::read() %d bytes to 0x%x\n", dst_bytes, dst);
    
		int i = (int)recv( s, dst, dst_bytes, 0 );
		if( i < 0 ) {
#ifdef _WIN32
			int wsaerr = GET_ERROR();
			if( wsaerr == 10035 ) {
				i=0;
			}
#else
			if( errno == EWOULDBLOCK || errno == EAGAIN ) {
				i=0;
			}
			perror("r3::Socket::read: recv");
#endif
		} else if ( i == 0 ) {
			Output( "r3::Socket::Read(): connection closed on remote end" );
			Disconnect();
			i=-1; // we'll use negative return to mean something bad happened...
		}
		return i;
	}

	bool Socket::CanRead() {
    if( s < 0 ) {
      return false;
    }
		fd_set fds;
		FD_ZERO( & fds );
		FD_SET( s, & fds );
		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		int r = 0;
		if( (r = select( (int)s+1, & fds, NULL, NULL, &tv )) <= 0 ) {
			if( r < 0 ) {
				Output( "r3::Socket::CanRead: select error" );
				Disconnect();
			}
			return false;
		}
		return true;
	}

	bool Socket::Write( const char * src, uint src_bytes ) {
		//Output( "r3::Socket::Write() %d bytes from 0x%x\n", src_bytes, src );
		int i=0;

		while( i < (int)src_bytes ) {
			int j =(int)send( s, src+i, src_bytes-i, 0);
			if( j < 0 ) {
				if( /* errno == EWOULDBLOCK || */ errno == EAGAIN ) {
					fd_set fds;
					FD_ZERO( & fds );
					FD_SET( s, & fds );

					if( select( (int)s+1, NULL, & fds, NULL, NULL ) <= 0 ) {
						Output( "r3::Socket::Write: select error" );
						return false;
					}
					j=0;
				} else {
					Output( "r3::Socket::Write: write failed" );
					return false;
				}
			} else if( j == 0 ) {
				Output( "r3::Socket::Write() failed" );
				return false;
			}
			//Output( "r3::Socket::Write send j=%d, i=%d\n", j, i );
			i += j;
		}
		return true;
	}


	void Listener::Close() {
		if( s < 0 ) 
			return;
#ifdef _WIN32
		closesocket( s );
#else
		close((int)s);
#endif
		s = INVALID_SOCKET;
	}


	bool Listener::SetNonblocking() {
#ifdef _WIN32
		u_long one = 1;
		if( ioctlsocket(s, FIONBIO, & one) ) {
			Output( "r3::Listener::SetNonblocking: ioctlsocket failed" );
			return false;
		}
#else 
		if( fcntl( s, F_SETFL, O_NONBLOCK ) < 0 ) {
			Output( "r3::Listener::SetNonblocking: fcntl failed");
			return false;
		}
#endif
		return true;

	}


	bool Listener::Listen( int port ) {
		INIT_SOCKET_LIB();

                if ( s == INVALID_SOCKET ) {
		    s = socket(AF_INET, SOCK_STREAM, 0);
#if ! _WIN32
                    unsigned int opt = 1;
                    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==-1) {
                        perror("setsockopt(s,SOL_SOCKET, SO_REUSEADDR,1)");
                    }
#endif 
		    if (s == INVALID_SOCKET) {
			Output( "Error at socket(): %ld\n", (long)GET_ERROR() );
			Close();
			return false;
		    }

		    sockaddr_in service;
		    memset( &service, 0, sizeof( service ) );
	 	    service.sin_family = AF_INET;
		    service.sin_addr.s_addr = INADDR_ANY;
		    service.sin_port = htons(port);

		    if ( bind( s, (SOCKADDR*) &service, sizeof(service)) == SOCKET_ERROR ) {
		    	Output( "bind() failed." );
#if ! _WIN32
			perror( "bind error" );
#endif
			Close();
			return false;
		    }

                }
		if ( listen( s, 1 ) == SOCKET_ERROR ) {
			Output( "Error listening on socket." );
#if ! _WIN32
			perror( "bind error" );
#endif
			Close();
			return false;
		}

		//SetNonblocking();
		return true;
	}

	Socket Listener::Accept() {
		if( s == -1 ) {
			return -1;
		}

		SOCKET client = accept( s, NULL, NULL );
		if ( client == SOCKET_ERROR ) {
#ifdef _WIN32
			int wsaerr = GET_ERROR();
			if( wsaerr != WSAEWOULDBLOCK ) {
				Close();
			} 
#else
                        perror( "Accept failed." );
#endif
			return Socket(-1);
		}
                //Output( "Accept() returning client %d\n", client );
		return Socket( client, ST_Stream );
	}

	void Listener::StopListening() {
		Close();
	}
}
