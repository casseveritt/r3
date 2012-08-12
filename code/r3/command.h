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

#ifndef __R3_COMMAND_H__
#define __R3_COMMAND_H__

#include "r3/atom.h"
#include "r3/parse.h"
#include <string>

namespace r3 {
	
	class Command {
		Atom name;
		std::string helpText;
	public:
		Command( const char *cmdName, const char *cmdHelpText );
		virtual ~Command();
		Atom Name() const {
			return name;
		}
		std::string HelpText() const {
			return helpText;
		}
		virtual void Execute( const std::vector< Token > & tokens ) = 0;
	};
	
	Command * GetCommand( const char *cmdName );

	// CommandFunc calls a function
	class CommandFunc : public Command {
		void (*func)( const std::vector< Token > & );
	public:
		CommandFunc( const char *cmdName, const char *cmdHelpText, void( *cmdFunc )( const std::vector< Token > & ) ) 
		: Command( cmdName, cmdHelpText ), func( cmdFunc ) {
		}
		virtual void Execute( const std::vector< Token > & tokens ) {
			func( tokens );
		}
	};
	
	inline void CreateCommandFunc( const char *cmdName, const char *cmdHelpText, void (*cmdFunc)( const std::vector< Token > & ) ) {
		new CommandFunc( cmdName, cmdHelpText, cmdFunc );
	}
	

	// CommandObject calls a method of an object instance
	template <typename T>
	class CommandObject : public Command {
		T *obj;
		void (T::*method)( const std::vector< Token > & );
	public:
		CommandObject( const char *cmdName, const char *cmdHelpText, T * cmdObj, void( *cmdMethod )( const std::vector< Token > & ) ) 
		: Command( cmdName, cmdHelpText ), obj( cmdObj ), method( cmdMethod ) {
		}
		virtual void Execute( const std::vector< Token > & tokens ) {			
			obj->*method( tokens );
		}		
	};
	
	// helpers for creating and destroying CommandObject instances
	template <typename T>
	inline void CreateCommandObject( const char *cmdName, const char *cmdHelpText, T *cmdObj, void (T::*cmdMethod)( const std::vector< Token > & ) ) {
		new CommandObject<T>( cmdName, cmdHelpText, cmdObj, cmdMethod );
	}
	
	inline void DestroyCommand( const char *cmdName ) {
		delete GetCommand( cmdName );
	}
	
	void ExecuteCommand( const char * cmd );

}

#endif // __R3_COMMAND_H__