/*
 *  input
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
#include "r3/input.h"
#include "r3/command.h"
#include "r3/common.h"
#include "r3/filesystem.h"
#include "r3/output.h"

#include "r3/keysymdef.h"

#include <map>
#include <vector>
#include <iostream>
#include <sstream>

using namespace std;
using namespace r3;

namespace {
	
	struct Bindings {
		map< string, map< int, string > > groups;		
		vector< string > stack;
		map< string, int > stringToKey;
		map< int, string > keyToString;
		int k( const string & s ) {
			return stringToKey[ s ];				
		}
		const string & s( int k ) {
			return keyToString[ k ];
		}
		
	};
	Bindings *bindings = NULL;

	void InitStringToKey( map< string, int > & m2k, map< int, string > & m2s ) {
#define STRING_KEY( s, k ) m2k[ s ] = k; m2s[ k ] = s
		char buf[20];
		for ( int i = 0; i <= 255; i++ ) {
			if ( isgraph( i ) ){
				r3Sprintf( buf, "%c", (unsigned char)i );
				//sprintf_s( buf, ARRAY_ELEMENTS( buf ), "%c", (unsigned char)i );
				string str( buf );
				STRING_KEY( buf, i );
				//Output( "registering string %s as key %d\n", str.c_str(), i );
			} else {
				//Output( "skipping key %d\n", i );				
			}
		}
		
		STRING_KEY( "escape", XK_Escape );
		STRING_KEY( "tilde" , XK_asciitilde );
		STRING_KEY( "space" , XK_space  );
		STRING_KEY( "home"  , XK_Home );
		STRING_KEY( "left"  , XK_Left );
		STRING_KEY( "up"    , XK_Up );
		STRING_KEY( "right" , XK_Right );
		STRING_KEY( "down"  , XK_Down );
		STRING_KEY( "pgup"  , XK_Page_Up );
		STRING_KEY( "pgdn"  , XK_Page_Down );
		STRING_KEY( "end"   , XK_End  );
		
		
		STRING_KEY( "backspace"  , XK_BackSpace );
		STRING_KEY( "tab"        , XK_Tab );
		STRING_KEY( "enter"      , XK_Return );
		STRING_KEY( "scrolllock" , XK_Scroll_Lock );
		STRING_KEY( "delete"     , XK_Delete );
		
		STRING_KEY( "leftshift"   , XK_Shift_L );
		STRING_KEY( "rightshift"  , XK_Shift_R );
		STRING_KEY( "leftcontrol" , XK_Control_L );
		STRING_KEY( "rightcontrol", XK_Control_R );
		STRING_KEY( "capslock"    , XK_Caps_Lock );
		STRING_KEY( "shiftlock"   , XK_Shift_Lock );
		STRING_KEY( "leftmeta"    , XK_Meta_L );
		STRING_KEY( "rightmeta"   , XK_Meta_R );
		STRING_KEY( "leftalt"     , XK_Alt_L );
		STRING_KEY( "rightalt"    , XK_Alt_R );
		
		
		STRING_KEY( "f1", XK_F1 );
		STRING_KEY( "f2", XK_F2 );
		STRING_KEY( "f3", XK_F3 );
		STRING_KEY( "f4", XK_F4 );
		STRING_KEY( "f5", XK_F5 );
		STRING_KEY( "f6", XK_F6 );
		STRING_KEY( "f7", XK_F7 );
		STRING_KEY( "f8", XK_F8 );
		STRING_KEY( "f9", XK_F9 );
		STRING_KEY( "f10", XK_F10 );
		STRING_KEY( "f11", XK_F11 );
		STRING_KEY( "f12", XK_F12 );
	}
		
	// bind command
	void Bind( const vector< Token > & tokens ) {
		if ( tokens.size() == 3 ) {
			//Output( "set a default binding: %s to \"%s\"\n", tokens[1].valString.c_str(), tokens[2].valString.c_str() );
			bindings->groups["default"][ bindings->k( tokens[1].valString ) ] = tokens[ 2 ].valString;
		} else if ( tokens.size() == 4 ) {
			//Output( "set a binding in a particular group\n" );
			bindings->groups[ tokens[ 1 ].valString ][ bindings->k( tokens[ 2 ].valString ) ] = tokens[ 3 ].valString;
		} else {
			Output( "Invalid binding syntax." );
		}
	}	
	
	// unbind command
	void Unbind( const vector< Token > & tokens ) {
		if ( tokens.size() == 2 ) {
			int k = bindings->k( tokens[1].valString );
			if ( bindings->groups["default"].count( k ) ) {
				bindings->groups["default"].erase( k );
			}
		} else if ( tokens.size() == 3 ) {
			int k = bindings->k( tokens[2].valString );
			if ( bindings->groups.count( tokens[1].valString ) ) {
				if ( bindings->groups[ tokens[1].valString ].count( k ) ) {
					bindings->groups[ tokens[1].valString ].erase( k );
				}
			}
		}
	}
		
		
	// pushbind command
	void PushBind( const vector< Token > & tokens ) {
		if ( tokens.size() == 2 ) {
			Output( "push bind group - %s", tokens[1].valString.c_str() );
			bindings->stack.push_back( tokens[1].valString );
		}
	}	
	
	// popbind command
	void PopBind( const vector< Token > & tokens ) {
		if ( bindings->stack.size() > 0 ) {
			Output( "pop bind group - %s", bindings->stack.back().c_str() );
			bindings->stack.pop_back();
		}
	}	
		
	void WriteBindings( const string & group ) {
		if( bindings->groups.count( group ) ){
			map< int, string > & b = bindings->groups[ group ];
			string fn = "bg-" + group + ".bind";
			File *f = FileOpenForWrite( fn );
			if ( f ) {
				for( map< int, string >::iterator it = b.begin(); it != b.end(); ++it ) {
					int k = it->first;
					string s = bindings->s( k );
					string cmd = it->second;
					stringstream ss ( stringstream::out );
					ss << "bind " << group << " " << s << " \"" << cmd << "\"";
					f->WriteLine( ss.str() );
				}
			}
			delete f;
		} else {
			Output( "Can't write unknown bindings group \"%s\"", group.c_str() );
		}
	}
	
	// writebindings command
	void WriteBindings( const vector< Token > & tokens ) {
		if ( tokens.size() > 1 ) {
			WriteBindings( tokens[1].valString );
		} else {
			for ( map< string, map< int, string > > ::iterator it = bindings->groups.begin(); it != bindings->groups.end(); ++it ) {
				WriteBindings( it->first );
			}
		}
	}	
	
	void ReadBindings( const string & group ) {
		string fn = "bg-" + group + ".bind";
		File *f = FileOpenForRead( fn );
		while ( f && ! f->AtEnd() ) {
			string line = f->ReadLine();
			ExecuteCommand( line.c_str() );
		}
		delete f;
	}
	
	// readbindings command - only one group per command, but should be enough
	void ReadBindings( const vector< Token > & tokens ) {
		if ( tokens.size() == 2 ) {
			ReadBindings( tokens[1].valString );
		} else {
			Output( "usage: readbindings <groupname>" );
		}
	}	

	void ListBindings( const string & group ) {
		if( bindings->groups.count( group ) ){
			map< int, string > & b = bindings->groups[ group ];
			for( map< int, string >::iterator it = b.begin(); it != b.end(); ++it ) {
				int k = it->first;
				string s = bindings->s( k );
				string cmd = it->second;
				stringstream ss ( stringstream::out );
				ss << "bind " << group << " " << s << " \"" << cmd << "\"";
				Output( "%s", ss.str().c_str() );
			}
		} else {
			Output( "Unknown bindings group \"%s\"", group.c_str() );
		}
	}
	
	// listbindings command
	void ListBindings( const vector< Token > & tokens ) {
		if ( tokens.size() > 1 ) {
			ListBindings( tokens[1].valString );
		} else {
			for ( map< string, map< int, string > > ::iterator it = bindings->groups.begin(); it != bindings->groups.end(); ++it ) {
				Output( "Binding group %s", it->first.c_str());
				ListBindings( it->first );
			}
		}
	}	
	
	
	// Do we need separate bindings for keydown and keyup?
	vector< KeyListener * > keyListeners;
	struct DefaultKeyListener : public KeyListener {
		void OnKeyEvent( KeyEvent &keyEvent ) {
			if ( keyEvent.state == KeyState_Down ) {
				for ( int i = (int)bindings->stack.size(); i >= 0; i-- ) {
					string group = i > 0 ? bindings->stack[ i - 1 ] : "default";
					if ( bindings->groups.count( group ) > 0 ) {
						map< int, string > & b = bindings->groups[ group ];
						if ( b.count( keyEvent.key ) > 0 ) {
							ExecuteCommand( b[ keyEvent.key ].c_str() ); 
							return;
						}
					}
				}
			}
		}
	};
	DefaultKeyListener defaultKeyListener;

	vector< PointerListener * > pointerListeners;
	struct DefaultPointerListener : public PointerListener {
		void OnPointerEvent( PointerEvent &pointerEvent ) {
		}
	};
	DefaultPointerListener defaultPointerListener;

	vector< ReshapeListener * > reshapeListeners;
	struct DefaultReshapeListener : public ReshapeListener {
		void OnReshape( ReshapeEvent &reshapeEvent ) {}
	};
	DefaultReshapeListener defaultReshapeListener;

	int asciiToKey[128];
}

namespace r3 {
	
	void InitInput() {
		if( bindings == NULL ) {
			bindings = new Bindings;
			InitStringToKey( bindings->stringToKey, bindings->keyToString );
			CreateCommandFunc( "bind", "adds a key binding", Bind );
			CreateCommandFunc( "unbind", "removes a key binding or group", Unbind );
			CreateCommandFunc( "pushbind", "pushes a key binding group onto the stack", PushBind );
			CreateCommandFunc( "popbind", "pops a key binding group off the stack", PushBind );
			CreateCommandFunc( "writebindings", "write current bindings from disk", WriteBindings );
			CreateCommandFunc( "readbindings", "read bindings from a file", ReadBindings );
			CreateCommandFunc( "listbindings", "list bindings", ListBindings );
			for( int i = 0; i < sizeof( asciiToKey ) / sizeof( asciiToKey[0] ) ; i++ ) {
				asciiToKey[i] = i;
			}
			asciiToKey[ 0x08 ] = XK_BackSpace;
			asciiToKey[ 0x09 ] = XK_Tab;
			asciiToKey[ 0x0d ] = XK_Return;
			asciiToKey[ 0x1b ] = XK_Escape;
			asciiToKey[ 0x7f ] = XK_Delete;
#if __APPLE__ // swap backspace and delete
			asciiToKey[ 0x08 ] = XK_Delete;
			asciiToKey[ 0x7f ] = XK_BackSpace;
#endif			
			PushKeyListener( &defaultKeyListener );
			PushPointerListener( &defaultPointerListener );
			PushReshapeListener( &defaultReshapeListener );
		}
	}
	
	void PushKeyListener( KeyListener *listener ) {
		keyListeners.push_back( listener );
	}

	KeyListener *PopKeyListener() {
		assert( keyListeners.size() > 1 );
		KeyListener *ret = keyListeners.back();
		keyListeners.pop_back();
		return ret;
	}

	void CreateKeyEvent( int key, KeyStateEnum keyState ) {
		KeyEvent keyEvent( key, keyState );
		for ( int i = (int)keyListeners.size() - 1; i >= 0 && keyEvent.handled == false ; --i ) {
			keyListeners[ i ]->OnKeyEvent( keyEvent );
		}
	}

	void PushPointerListener( PointerListener *listener ) {
		pointerListeners.push_back( listener );
	}

	PointerListener *PopPointerListener() {
		assert( pointerListeners.size() > 1 );
		PointerListener *ret = pointerListeners.back();
		pointerListeners.pop_back();
		return ret;
	}

	void CreatePointerEvent( bool active, int x, int y ) {
		PointerEvent pointerEvent( active, x, y );
		for ( int i = (int)pointerListeners.size() - 1; i >= 0 && pointerEvent.handled == false; --i ) {
			pointerListeners[ i ]->OnPointerEvent( pointerEvent );
		}
	}

	void PushReshapeListener( ReshapeListener *listener ) {
		reshapeListeners.push_back( listener );
	}

	ReshapeListener *PopReshapeListener() {
		assert( reshapeListeners.size() > 1 );
		ReshapeListener *ret = reshapeListeners.back();
		reshapeListeners.pop_back();
		return ret;
	}

	void CreateReshapeEvent( int w, int h ) {
		ReshapeEvent reshapeEvent( w, h );
		for ( int i = (int)reshapeListeners.size() - 1; i >= 0 && reshapeEvent.handled == false; --i ) {
			reshapeListeners[ i ]->OnReshape( reshapeEvent );
		}
	}

	int AsciiToKey( unsigned char key ) {
		return asciiToKey[ key & 0x7f ];
	}

}

