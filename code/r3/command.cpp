/*
 *  command
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

#include "r3/command.h"
#include "r3/output.h"
#include "r3/var.h"
#include <map>

using namespace std;
using namespace r3;

namespace {
	
	// We have to do this to have static initialization of commands works.
	
	struct Commands {
		Commands() {
			// Output("Constructing Commands singleton.\n" );
		}
		map<Atom, Command *> lookup;
	};
	
	Commands * commands = NULL;
	void InitCommand() {
		if ( commands == NULL ) {
			commands = new Commands;
		}
	}
    
	
}

namespace r3 {
		
	Command::Command( const char *cmdName, const char *cmdHelpText ) 
	: name( cmdName ), helpText( cmdHelpText ) {
		InitCommand();
		//Output( "Registering command %s (%d): %s - %s\n", name.Str().c_str(), name.Val(), cmdName, cmdHelpText );
		commands->lookup[ name ] = this;
	}
	
	Command::~Command() {
		commands->lookup.erase( name );
	}
	
	Command *GetCommand( const char *cmdName ) {
		Atom name = FindAtom( cmdName );
		if ( commands->lookup.count( name ) ) {
			return commands->lookup[ name ];
		}
		return NULL;
	}
	
	void ExecuteCommand( const char *cmd ) {
		Output("executing command: \"%s\"", cmd );
		vector< Token > tokens = TokenizeString( cmd );
		if ( tokens.size() == 0 ) {
			return;
		}

		/*
		if ( tokens.size() == 2 && tokens[0].valString == "toggle" && tokens[1].valString == "app_showSatellites" ) {
			OutputDebug( "Did this button really get pressed?" );
		}
		*/
		
		// try command first
		Command *c = GetCommand( tokens[0].valString.c_str() );
		if ( c ) {
			c->Execute( tokens );
			return;
		}

		// then try var
		if ( FindVar( tokens[0].valString.c_str() )  ){
			string newcmd("_et ");
			newcmd += cmd;
			newcmd[0] = tokens.size() == 1 ? 'g' : 's';
			ExecuteCommand( newcmd.c_str() );
			return;
		}
		
		Output( "Command not found." );
	}
	
}

extern VarInteger f_numOpenFiles;

namespace {

	// quit command
	void Quit( const vector< Token > & tokens ) {
		ExecuteCommand( "writebindings" );
		ExecuteCommand( "writevars" );
		ExecuteCommand( "appquit" );
	}	
	CommandFunc QuitCmd( "quit", "calls exit(0)", Quit );

	// listCommands command
	void ListCommands( const vector< Token > & tokens ) {
		map< Atom, Command * > &m = commands->lookup;
		for( map< Atom, Command * >::iterator it = m.begin(); it != m.end(); ++it ) {
			Output( "%s - %s\n", it->first.Str().c_str(), it->second->HelpText().c_str() );
		}
	}
	CommandFunc ListCommandsCmd( "listcommands", "lists registered commands", ListCommands );
	
}


