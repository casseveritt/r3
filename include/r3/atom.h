/*
 *  atom
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


#ifndef __R3_ATOM_H__
#define __R3_ATOM_H__

#include <string>
#include <vector>

namespace r3 {
    
	struct Atom {
	private:
		int v;
		char *s;

	public:
		
		Atom() : v( -1 ), s( 0 ) {
		}
		Atom( const Atom & a ) : v( a.v ), s( a.s ) {
		}
		
		Atom( const char *str );
		
		unsigned int Val() const {
			return v;
		}

		std::string Str() const {
			return s;
		}
		
		bool Valid() const {
			return v >= 0;
		}
				
		bool operator == ( const Atom & rhs ) const {
			return v == rhs.v;
		}
		
		bool operator != ( const Atom & rhs ) const {
			return v != rhs.v;
		}
		
		bool operator < ( const Atom & rhs ) const {
			return v < rhs.v;
		}
		
	};
	
	
	// Will return an invalid atom if the string is not already in the table.
	Atom FindAtom( const char * str );
	
	// Get info about the atom table
	int GetAtomTableSize();
	Atom GetAtom( int i );

}

#endif // __ATOM_H__