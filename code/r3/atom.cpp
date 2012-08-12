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


#include "r3/atom.h"

#include "r3/common.h"

#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <assert.h>

using namespace std;

namespace {
	
	// We have to do this to have static initialization of atoms works.
    void InitAtom();
    
    
	struct Atoms {
		Atoms() {
			// Output("Constructing Atoms singleton.\n" );
		}
		vector<char *> table;
		map<string, int> lookup;
	};
	
	Atoms * atoms = NULL;

	
	int GetAtomIndex( const char * str ) {
        InitAtom();
        string s( str );
		if ( atoms->lookup.count( s ) ) {
			return atoms->lookup[ s ];
		}
		int v = (int)atoms->table.size();
		atoms->table.push_back( strdup( str ) );
		//Output( "Atom: adding %d %s\n", v, str );
		atoms->lookup[ s ] = v;
		return v;
	}
	
	char * GetAtomString( const char * str) {
        InitAtom();
		string s( str );
		if ( atoms->lookup.count( s ) ) {
			int v = atoms->lookup[ s ];
			return atoms->table[ v ];
		}
		return NULL;
	}
	
    void InitAtom() {
		if ( atoms == NULL ) {
			atoms = new Atoms;
		}		
	}
    

}	

namespace r3 {

    
	// Return an invalid atom if it isn't already in the atom table.
	Atom FindAtom( const char *str ) {
        InitAtom();
		string s( str );
		if ( atoms->lookup.count( s ) ) {
			return Atom( str );
		}
		return Atom();
	}
	
	int GetAtomTableSize() {
        InitAtom();
		return (int)atoms->table.size();
	}

	Atom GetAtom( int i ) {
        InitAtom();
		if ( i < (int)atoms->table.size() ) {
			return FindAtom( atoms->table[ i ] );
		}
		return Atom();
	}

	
	Atom::Atom( const char * str ) 
	: v( GetAtomIndex( str ) )
	, s( GetAtomString( str ) )
	{
	}
	

}
