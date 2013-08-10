/*
 *  console
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


#include "r3/console.h"
#include "r3/command.h"
#include "r3/common.h"
#include "r3/filesystem.h"
#include "r3/input.h"
#include "r3/output.h"
#include "r3/thread.h"
#include "r3/var.h"
#include "r3/font.h"
#include "r3/draw.h"
#include <GL/Regal.h>

#include <stdio.h>

using namespace std;
using namespace r3;

r3::VarString con_font( "con_font", "console font", Var_Archive, "LiberationMono-Regular.ttf" );
r3::VarInteger con_fontSize( "con_fontSize", "console font rasterization size", Var_Archive, 12 );
r3::VarFloat con_fontScale( "con_fontScale", "console font rendering scale", Var_Archive, 0.35 );
r3::VarInteger con_scrollBuffer( "con_scrollBuffer", "console scrollback buffer size", Var_Archive, 1000 );
r3::VarInteger con_historySize( "con_historySize", "console history size", Var_Archive, 1000 );
r3::VarFloat con_opacity( "con_opacity", "opacity of the console display", Var_Archive, 0.5f );

r3::Console *r3::console;	

namespace {

   r3::Font *font;	
	Mutex conMutex;
	
	void InitConsoleBindings() {
		InitInput();
		ExecuteCommand( "bind ` activateconsole" );
	}

	// ActivateConsole command
	void ActivateConsole( const vector< Token > & tokens ) {
		fprintf(stderr, "Activating console!\n" );
		if ( console != NULL ) {
			console->Activate();
		}
	}	
	CommandFunc ActivateConsoleCmd( "activateconsole", "activates the console to acquire keyboard input", ActivateConsole );
	
	// DeactivateConsole command
	void DeactivateConsole( const vector< Token > & tokens ) {
		fprintf(stderr, "Deactivating console!\n" );
		if ( console != NULL ) {
			console->Deactivate();
		}
	}	
	CommandFunc DeactivateConsoleCmd( "deactivateconsole", "deactivates the console and restores previous keyboard input handler", DeactivateConsole );
	
	
	void AttemptCompletion( string & commandLine, int & cursorPos ) {
		// pad cmdline for simplicity
		string cmdLine = " " + commandLine + " ";
		int pos = cursorPos + 1;
		// back the pos up by one if it is directly in front of an arg 
		if ( cmdLine[ pos ] == ' ' && cmdLine[ pos - 1 ] != ' ' ) {
			pos--;
		}
		
		int bgn = pos;
		int end = pos;
		
		while( cmdLine[ bgn - 1 ] != ' ' ) {
			bgn--;
		}
		while( cmdLine[ end ] != ' ' ) {
			end++;
		}
		
		int len = end - bgn;
		if ( len == 0 ) {
			return;
		}
		
		string arg = cmdLine.substr( bgn, len );
		// switch back to the non-padded version
		bgn --;
		end --;
		//Output( "Arg to attempt completion on: %s (%d, %d)\n", arg.c_str(), bgn, len );

		vector< string > matches;
		for ( int i = 0; i < GetAtomTableSize(); i++ ) {
			string candidate = GetAtom( i ).Str();
			if ( candidate.compare( 0, len, arg ) == 0 ) {
				matches.push_back( candidate );
			}
		}

		if ( matches.size() > 0 ) {
			sort( matches.begin(), matches.end() );
			if ( matches.size() > 1 ) {
				Output( "Found %d matches:", (int)matches.size() );
			}
			int minLen = 100000;
			for ( int i = 0; i < matches.size(); i++ ) {
				minLen = min( minLen, (int)matches[ i ].size() );
				if ( matches.size() > 1 ) {
					Output( "  %s", matches[ i ].c_str() );
				}
			}
			for( int i = (int)arg.size(); i < minLen; i++ ) {
				bool mismatch = false;
				for ( int j = 1; j < matches.size(); j++ ) {
					if ( matches[0][i] != matches[j][i] ) {
						mismatch = true;
						break;
					}					
				}
				if ( mismatch ) {
					break;
				}
				arg.push_back( matches[0][i] );
			}
			//Output( "replace old arg with \"%s\"\n", arg.c_str() );
			commandLine.replace( bgn, len, arg);
			cursorPos = bgn + (int)arg.size();
			
		} else {
			Output( "No matches found." );
		}

		
	}

	void ReadHistory( const vector< Token > & tokens ) {
		if ( console ) {
			console->ReadHistory();
		}
	}
	CommandFunc ReadHistoryCmd( "readhistory", "read console history", ReadHistory );

	void WriteHistory( const vector< Token > & tokens ) {
		if ( console ) {
			console->WriteHistory();
		}
	}
	CommandFunc WriteHistoryCmd( "writehistory", "write console history", WriteHistory );
}

extern VarInteger r_windowWidth;
extern VarInteger r_windowHeight;


namespace r3 {
	
	void InitConsole() {
		console = new Console();
	}
    
    void ShutdownConsole() {
        delete console;
        console = NULL;
    }
	
	Console::Console() : cursorPos( 0 ), historyPos( 0 ) {
		//fprintf(stderr,"Constructing console singleton\n");
		InitConsoleBindings();
		history.push_back( "" );
		outputBufferPos = 0;
		active = false;
        ReadHistory();
	}
    
    Console::~Console() {
        WriteHistory();
    }
	
	void Console::Activate() {
		if ( ! active ) {
			active = true;
			PushKeyListener( this );
			PushPointerListener( this );
		}
	}
	
	void Console::Deactivate() {
		if ( active ) {
			active = false;
			KeyListener *kl = PopKeyListener();
			assert( kl == this );
			PointerListener *pl = PopPointerListener();
			assert( pl == this );
		}
	}
	
	void Console::AppendOutput( const string & str ) {
		ScopedMutex scm( conMutex, R3_LOC );
		outputBuffer.push_back( str );
		if ( outputBuffer.size() > con_scrollBuffer.GetVal() ) {
			outputBuffer.pop_front();
		}		
	}

	void Console::ReadHistory() {
		File *file = FileOpenForRead( "history.txt" );
		if ( file ) {
			history.clear();
			while( file->AtEnd() == false ) {
				history.push_back( file->ReadLine() );
			}
			history.push_back( "" );
			delete file;
			historyPos = (int)history.size() - 1;
		}
	}

	void Console::WriteHistory() {
		File *file = FileOpenForWrite( "history.txt" );
		if ( file == NULL ) {
			Output( "Failed to open history.txt for write." );
			return;
		}
		for ( int i = 0; i < (int)history.size(); i++ ) {
			if( history[i].size() > 0 ) {
				file->WriteLine( history[i] );
			}
		}
		delete file;
	}
	
	void Console::OnKeyEvent( KeyEvent &keyEvent ) {
		//Output( "Got console key event! key = %d, keyState = %s\n", key, keyState == KeyState_Up ? "up" : "down" );
		keyEvent.handled = true;
		if ( keyEvent.state == KeyState_Up ) {
			return;
		}

		if ( keyEvent.key == '`' ) {
			ExecuteCommand( "deactivateconsole" );
			return;
		} else if ( keyEvent.key == XK_BackSpace ) {
			if ( cursorPos > 0 ) {
				cursorPos--;
				commandLine.erase( cursorPos, 1 );
			}
		} else if ( keyEvent.key == XK_Delete ) {
			if ( cursorPos > 0 && cursorPos < commandLine.size() ) {
				commandLine.erase( cursorPos, 1 );
			}
		} else if ( keyEvent.key == XK_Return ) {
			AppendOutput( "> " + commandLine );
			ExecuteCommand( commandLine.c_str() );
			history.pop_back();
			history.push_back( commandLine );
			history.push_back( "" );
			while( con_historySize.GetVal() > 0 && history.size() > con_historySize.GetVal() ) {
				history.pop_front();
			}
			commandLine.clear();
			cursorPos = 0;
			historyPos = (int)history.size() - 1;
		} else if ( keyEvent.key == XK_Up || keyEvent.key == XK_Down ) {
			historyPos -= keyEvent.key == XK_Up ? 1 : -1;
			historyPos = std::max<int>( 0, std::min<int>( (int)historyPos, (int)history.size() - 1 ) );
			if ( history.size() > 0 ) {
				commandLine = history[ historyPos ];
				cursorPos = (int)commandLine.size();			
			}
		} else if ( keyEvent.key == XK_Left ) {
			cursorPos--;
			cursorPos = std::max<int>( cursorPos, 0 );
		} else if ( keyEvent.key == XK_Right ) {
			cursorPos++;
			cursorPos = std::min<int>( cursorPos, (int)commandLine.size() );
		} else if ( keyEvent.key == XK_Page_Up ) {
			outputBufferPos = min( (int)outputBuffer.size(), outputBufferPos + 1 );
		} else if ( keyEvent.key == XK_Page_Down ) {
			outputBufferPos = max( 0, outputBufferPos - 1 );
		} else if ( keyEvent.key == XK_Tab ) {
			AttemptCompletion( commandLine, cursorPos );
		} else if ( keyEvent.key >= 0x20 && keyEvent.key <= 0x7f ){ // printable characters
			char c[2] = { keyEvent.key, 0 };
			commandLine.insert( cursorPos, c );
			cursorPos++;
			outputBufferPos = 0;
		}
	
		//Output( "> %s_%s\n", commandLine.substr(0, cursorPos ).c_str(), commandLine.substr( cursorPos, string::npos ).c_str() );
		// else actually process the keyboard input
	}

	void Console::OnPointerEvent( PointerEvent &pointerEvent ) {
	}

	
	void Console::Draw() {
		if ( font == NULL ) {
			font = r3::CreateStbFont( con_font.GetVal(), "", con_fontSize.GetVal() ); 			
		}

		ScopedMutex scm( conMutex, R3_LOC );

		if ( ! IsActive() ) {
			return;
		}
		
		int border = 10;
		
		int w = r_windowWidth.GetVal();
		int h = r_windowHeight.GetVal();
		int conW = w - 2 * border;
		int conH = ( h - ( h >> 2 ) ) - border;
		float s = con_fontScale.GetVal();

		static Bounds2f bO;
		static float yAdvance = 0;
		static float sCache = 0;
		if ( s != sCache ) {
			bO = font->GetStringDimensions( "O", s );
			Bounds2f bb = font->GetStringDimensions( "|", s );
			sCache = s;
			yAdvance = std::max<int>( bO.Height(), bb.Height() );
		}
		
		int x0 = border;
		int y = ( h >> 2) + border;
		
		glColor4ub( 16, 16, 16, con_opacity.GetVal() * 255 );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );

		DrawQuad( x0, y, x0 + conW, y + conH );
		
		string cl = "> " + commandLine;
		int cp = cursorPos + 2;

		//Bounds2f b = font->GetStringDimensions( cl, s );
		Bounds2f b2 = font->GetStringDimensions( cl.substr(0, cp ), s );
		
		glColor4ub( 255, 255, 255, 192 );
		font->Print( cl, x0, y, s );
		
        
		glColor4ub( 255, 255, 255, 64 );
		DrawQuad( x0 + b2.Width(), y, x0 + b2.Width() + bO.Width(), y + bO.Height() );
		
		y += yAdvance;

		glColor4ub( 255, 255, 255, 128 );
		for ( int i = (int)outputBuffer.size() - 1 - outputBufferPos; i >= 0 && y < h; i-- ) {
			string & line = outputBuffer[ i ];
			font->Print( line, x0, y, s );
			y += yAdvance;
		}
        glDisable( GL_BLEND );
	}
	
}
