/*
 *  var
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


#include "r3/var.h"
#include "r3/command.h"
#include "r3/common.h"
#include "r3/output.h"
#include "r3/filesystem.h"

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <stdio.h>

using namespace std;
using namespace r3;

namespace {

	// We have to do this to have static initialization of vars work.
	struct Vars {
		Vars() {
			// Output( "Constructing Vars singleton.\n" );
		}
		map< Atom, Var * > lookup;
	};
	Vars *vars = NULL;

	bool reading;
	
	void Set( const vector< Token > & tokens ) {
		if ( tokens.size() < 3 ) {
			Output( "usage: set <varname> <value>" );
			return;
		}
		const string & name = tokens[1].valString;
		Atom aName( name.c_str() );
		if ( vars->lookup.count( aName ) ) {
			Var *v = vars->lookup[ aName ];
			const string & val = tokens[2].valString;
			if ( reading && ( v->Flags() & Var_Archive ) == 0 ) {
				return;
			}
			v->Set( val.c_str() );				
			Output( "%s = %s", name.c_str(), v->Get().c_str() );
		} else {
			Output( "Variable \"%s\" not found.", name.c_str() );
		}
	}
	
	void Get( const vector< Token > & tokens ) {
		if ( tokens.size() != 2 ) {
			Output( "usage: get <varname>" );
			return;
		}
		const string & name = tokens[1].valString;
		Atom aName( name.c_str() );
		if ( vars->lookup.count( aName ) ) {
			Var *v = vars->lookup[ aName ];
			Output( "%s = %s", name.c_str(), v->Get().c_str() );
		} else {
			Output( "Variable \"%s\" not found.", name.c_str() );
		}
	}
	
	void ListVars( const vector< Token > & tokens ) {
		for ( map< Atom, Var *>::iterator it = vars->lookup.begin(); it != vars->lookup.end(); ++it ){
			Output( "%s = %s", it->first.Str().c_str(), it->second->Get().c_str() );
		}
	}
	CommandFunc ListVarsCmd( "listvars", "list all defined vars", ListVars );

	void ReadVars( const vector< Token > & tokens ) {
		if ( tokens.size() > 2 ) {
			Output( "usage: readvars [filename]" );
			return;
		}
		reading = true;
		string fn = ( tokens.size() == 1 ? string("default") : tokens[1].valString ) + ".vars";
		File *f = FileOpenForRead( fn );
		while ( f && ! f->AtEnd() ) {
			string line = f->ReadLine();
			ExecuteCommand( line.c_str() );
		}
		delete f;
		reading = false;
	}	
	CommandFunc ReadVarsCmd( "readvars", "read and set vars from file", ReadVars );

	
	void WriteVars( const vector< Token > & tokens ) {
		if ( tokens.size() > 2 ) {
			Output( "usage: writevars [filename]" );
			return;
		}
		string fn;
		if ( tokens.size() == 2 ) {
			fn = tokens[1].valString + ".vars";			
		} else {
			fn = "default.vars";
		}
		File *f = FileOpenForWrite( fn );
		for( map< Atom, Var * >::iterator it = vars->lookup.begin(); it != vars->lookup.end(); ++it ) {
			string s = it->first.Str();
			Var *v = it->second;
			if ( v->Flags() & Var_Archive ) {
				stringstream ss ( stringstream::out );
				ss << "set " << s << " " << " " << v->Get();
				f->WriteLine( ss.str() );				
			}
		}
		delete f;		
	}
	CommandFunc WriteVarsCmd( "writevars", "write archive vars", WriteVars );

	void IncrVar( const vector< Token > &  tokens ) {
		if ( tokens.size() < 3 || tokens.size() > 4 ) {
			Output( "usage: incr <var> <incrBy> [upperBound]" );
			return;
		}
		const string & name = tokens[1].valString;
		Atom aName( name.c_str() );
		if ( vars->lookup.count( aName ) == 0 ) {
			Output( "Variable \"%s\" not found.", name.c_str() );
			return;
		}
		
		Var *v = vars->lookup[ aName ];
		const string & incrBy = tokens[2].valString;
		if ( tokens.size() == 3 ) {
			v->Incr( incrBy.c_str() );			
		} else {
			const string & upperBound = tokens[3].valString;
			v->Incr( incrBy.c_str(), upperBound.c_str() );
		}
		Output( "incr: %s = %s", name.c_str(), v->Get().c_str() );	
	}
	CommandFunc IncrVarCmd( "incr", "increment a var", IncrVar );
	
	void DecrVar( const vector< Token > &  tokens ) {
		if ( tokens.size() < 3 || tokens.size() > 4 ) {
			Output( "usage: decr <var> <decrBy> [lowerBound]" );
			return;
		}
		const string & name = tokens[1].valString;
		Atom aName( name.c_str() );
		if ( vars->lookup.count( aName ) == 0 ) {
			Output( "Variable \"%s\" not found.", name.c_str() );
			return;
		}
		
		Var *v = vars->lookup[ aName ];
		const string & decrBy = tokens[2].valString;
		if ( tokens.size() == 3 ) {
			v->Decr( decrBy.c_str() );			
		} else {
			const string & lowerBound = tokens[3].valString;
			v->Decr( decrBy.c_str(), lowerBound.c_str() );
		}
		Output( "decr: %s = %s", name.c_str(), v->Get().c_str() );	
	}
	CommandFunc DecrVarCmd( "decr", "decrement a var", DecrVar );

	void CycleVar( const vector< Token > & tokens ) {
		if ( tokens.size() < 4 ) {
			Output( "usage: cycle <var> <val0> <val1> ... <valn>" );
			return;
		}
		const string & name = tokens[1].valString;
		Atom aName( name.c_str() );
		if ( vars->lookup.count( aName ) == 0 ) {
			Output( "Variable \"%s\" not found.", name.c_str() );
			return;
		}

		Var *v = vars->lookup[ aName ];
		string curr = v->Get();
		vector< string > vals;
		int index = (int)tokens.size() - 2;
		for ( int i = 2; i < (int)tokens.size(); i++ ) {
			v->Set( tokens[ i ].valString.c_str() );
			vals.push_back( v->Get() );
			if ( curr == vals.back() ) {
				index = i - 2;
			}
		}
		index = ( index + 1 ) % ( (int)vals.size() );
		
		v->Set( vals[ index ].c_str() );
		Output( "cycle: %s = %s", name.c_str(), v->Get().c_str() );
	}
	CommandFunc CycleVarCmd( "cycle", "cycle a var though a small list of values", CycleVar );

	void ToggleVar( const vector< Token > & tokens ) {
		if ( tokens.size() != 2 ) {
			Output( "usage: toggle <var>" );
			return;
		}
		vector< Token > tok;
		tok.push_back( Token( "cycle" ) );
		tok.push_back( tokens[ 1 ] );
		tok.push_back( Token( "0" ) );
		tok.push_back( Token( "1" ) );
		CycleVar( tok );
	}
	CommandFunc ToggleVarCmd( "toggle", "toggle a variable between zero and nonzero", ToggleVar );
	
	void DefaultVar( const vector< Token > &  tokens ) {
		if ( tokens.size() != 2 ) {
			Output( "usage: default <var>" );
			return;
		}
		const string & name = tokens[1].valString;
		Atom aName( name.c_str() );
		if ( vars->lookup.count( aName ) == 0 ) {
			Output( "Variable \"%s\" not found.", name.c_str() );
			return;
		}
		
		Var *v = vars->lookup[ aName ];
		v->Default();
		Output( "default: %s = %s", name.c_str(), v->Get().c_str() );	
	}
	CommandFunc DefaultVarCmd( "default", "set a var to its default value", DefaultVar );
	
	
	void InitVar() {
		if ( vars == NULL ) {
			vars = new Vars;
			CreateCommandFunc( "set", "set a var", Set);
			CreateCommandFunc( "get", "get a var", Get);
		}
	}
	
}

namespace r3 {


	Var::Var( const char * varName, const char *varDesc, int varFlags ) 
	: name( varName ), desc( varDesc ), flags( varFlags ) {
		InitVar();
		if ( vars->lookup.count( name ) ) {
			Output( "r3::Var %s already exists!", name.Str().c_str() );
			return;
		}
		vars->lookup[ name ] = this;
	}

	// string
	
	string VarString::Get() const {
		return val;
	}
	
	void VarString::Set( const char * str ) {
		val = str;
	}
	
	// bool 
	
	string VarBool::Get() const {
		return val ? "1" : "0";
	}
		
	void VarBool::Set( const char * str ) {
		val = atoi( str ) != 0;
	}

	// int 
	
	string VarInteger::Get() const {
		char buf[32];
		r3Sprintf( buf, "%d", val );
		return string( buf );
	}

	void VarInteger::Set( const char * str ) {
		val = atoi( str );
	}
	
	void VarInteger::Incr( const char * incrBy, const char * upperBound ) {
		val += atoi( incrBy );
		if ( upperBound ) {
			val = min( val, (int)atoi( upperBound ) );
		}
	}
	
	void VarInteger::Decr( const char * decrBy, const char * lowerBound ) {
		val -= atoi( decrBy );
		if ( lowerBound ) {
			val = max( val, (int)atoi( lowerBound ) );
		}
	}
	
	
	// float 
	
	string VarFloat::Get() const {
		char buf[32];
		r3Sprintf( buf, "%f", val );
		return string( buf );
	}
	
	void VarFloat::Set( const char * str ) {
		val = atof( str );
	}
	
	void VarFloat::Incr( const char * incrBy, const char * upperBound ) {
		val += atof( incrBy );
		if ( upperBound ) {
			val = min( val, (float)atof( upperBound ) );
		}
	}
	
	void VarFloat::Decr( const char * decrBy, const char * lowerBound ) {
		val -= atof( decrBy );
		if ( lowerBound ) {
			val = max( val, (float)atof( lowerBound ) );
		}
	}

	
	// Vec2f
	
	string VarVec2f::Get() const {
		char buf[96];
		r3Sprintf( buf, "Vec2f( %f, %f )", val.x, val.y );
		return string( buf );
	}
	
	void VarVec2f::Set( const char * str ) {
		Vec2f v;
		if ( r3Sscanf( str, "Vec2f( %f, %f )", &v.x, &v.y ) == 2 ) {
			val = v;			
		} else {
			Output( "Failed to parse Vec2f from \"%s\".", str );
		}
	}
	
	
	// Vec3f
	
	string VarVec3f::Get() const {
		char buf[96];
		r3Sprintf( buf, "Vec3f( %f, %f, %f )", val.x, val.y, val.z );
		return string( buf );
	}
	
	void VarVec3f::Set( const char * str ) {
		Vec3f v;
		if ( r3Sscanf( str, "Vec3f( %f, %f, %f )", &v.x, &v.y, &v.z ) == 3 ) {
			val = v;			
		} else {
			Output( "Failed to parse Vec3f from \"%s\".", str );
		}
	}
	
	
	// Vec4f
	
	string VarVec4f::Get() const {
		char buf[96];
		r3Sprintf( buf, "Vec4f( %f, %f, %f, %f )", val.x, val.y, val.z, val.w );
		return string( buf );
	}
	
	void VarVec4f::Set( const char * str ) {
		Vec4f v;
		if ( r3Sscanf( str, "Vec4f( %f, %f, %f, %f )", &v.x, &v.y, &v.z, &v.w ) == 4 ) {
			val = v;			
		} else {
			Output( "Failed to parse Vec4f from \"%s\".", str );
		}
	}
	
	
	// Rotationf
	
	string VarRotationf::Get() const {
		char buf[96];
		Vec3f axis;
		float angle;
		val.GetValue( axis, angle ) ;
		r3Sprintf( buf, "Rotationf( %f, %f, %f, %f )", ToDegrees( angle ), axis.x, axis.y, axis.z );
		return string( buf );
	}
	
	void VarRotationf::Set( const char * str ) {
		float v[4];
		if ( r3Sscanf( str, "Rotationf( %f, %f, %f, %f )", v+0, v+1, v+2, v+3 ) == 4 ) {
			val.SetValue( Vec3f( v[1], v[2], v[3] ), ToRadians( v[0] ) );
		} else {
			Output( "Failed to parse Rotationf from \"%s\".", str );
		}
	}
	
	
	
	Var * FindVar( const char *varName ) {
		if ( vars->lookup.count( varName ) ) {
			return vars->lookup[ varName ];
		}
		return NULL;
	}

	
}



