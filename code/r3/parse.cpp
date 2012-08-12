/*
 *  parse
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

#include "r3/common.h"
#include "r3/parse.h"

#include <map>
#include <stdlib.h>

using namespace std;

namespace {
	
	bool initialized = false;
	struct OnHeap {
		map< char, char > parenMap;		
	};
	OnHeap *oh = NULL;
		
	void initialize() {
		if ( initialized ) {
			return;
		}
		oh = new OnHeap;
		
		oh->parenMap.clear();
		oh->parenMap['"'] = '"';
		oh->parenMap['('] = ')';
		oh->parenMap['['] = ']';
		oh->parenMap['{'] = '}';
		
		initialized = true;
	}

}

namespace r3 {
	
	bool StringToFloat( const string & s, float & val ){
		if ( s.size() == 0 ) {
			return false;
		}
		char * end;
		const char * bgn = s.c_str();
		val = strtod( bgn, &end );
		return ( end - bgn ) == s.size();
	}

	bool IsDelimiter( const char *delimiters, char c );
    bool IsDelimiter( const char *delimiters, char c ) {
		while( *delimiters != 0 && *delimiters != c ) {
			delimiters++;
		}
		return *delimiters == c;
	}
	
	vector< Token > TokenizeString( const char *str, const char *delimiters ) {
		string s( str );
		vector< Token > toks;
		//Output("Called TokenizeString( %s )\n", str );
		initialize();
		
		while( s.size() > 0 ) {
			while( s.size() > 0 && IsDelimiter( delimiters, s[0] ) ) {
				//Output( "Erasing char: %s\n", s.c_str() );
				s.erase( s.begin() );
			}
			string t;
			vector<char> parenStack;
			bool prevWasBackslash = false;
			bool hadParens = false;
			while( s.size() > 0 && ( prevWasBackslash || ( ! IsDelimiter( delimiters, s[0] ) ) || ( parenStack.size() > 0 ) ) ) {
				bool append = true;
				if ( prevWasBackslash ) {
					prevWasBackslash = false;
				} else if ( s[0] == '\\' ) {
					prevWasBackslash = true;
					append = false;
				} else if ( parenStack.size() > 0 && s[0] == parenStack.back() ) {
					parenStack.pop_back();
					append = s[0] != '"';
				} else if ( oh->parenMap.count( s[0] ) ) {
					parenStack.push_back( oh->parenMap[ s[0] ] );
					append = s[0] != '"';
					hadParens = true;
				}
				if ( append ) {
					t += s[0];
				}
				s.erase( s.begin() );
				//Output( "Moving char: t=%s s=%s\n", t.c_str(), s.c_str()  );
			}
			if ( t.size() > 0 || hadParens ) {
				Token tok;
				tok.valString = t;
				if ( StringToFloat( t, tok.valNumber ) ) {
					tok.type = TokenType_Number;
					//Output( "Adding Number token %d : %f\n", int( toks.size() ), tok.valNumber );
				} else {
					tok.type = TokenType_String;
					//Output( "Adding String token %d : %s\n", int( toks.size() ), tok.valString.c_str() );
				}
				toks.push_back( tok );
			}
		}
		return toks;
	}
}