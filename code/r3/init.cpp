/*
 *  init
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
#include "r3/console.h"
#include "r3/filesystem.h"
#include "r3/output.h"
#include "r3/var.h"

#if R3_HAS_GL
#include "r3/buffer.h"
#include "r3/draw.h"
#include "r3/font.h"
#include "r3/model.h"
#include "r3/shader.h"
#include "r3/texture.h"
#endif

using namespace std;

namespace r3 {

    void Init( int argc, const char **argv );
    
  void Init( int argc, const char **argv ) {
    vector< string > commands;
    string command;
    vector< string > av;
    for ( int i = 0; i < argc; i++ ) {
      av.push_back( string( argv[ i ] ) );
      if ( av.back()[0] ==  '+' ) {
        commands.push_back( command );
        av.back().erase( 0, 1 );
        command = av.back();
      } else {
        command += " ";
        command += av.back();
      }
    }
    commands.push_back( command );

    Output( "begin Command Line directives ----------------" );
    for ( int i = 1; i < (int)commands.size(); i++ ) {
      ExecuteCommand( commands[ i ].c_str() );
    }
    Output( "end   Command Line directives ----------------" );
    InitFilesystem();
    extern VarString f_basePath;
    if ( f_basePath.GetVal().size() == 0 ) {
      OutputDebug( "Unable to initialize f_basePath, exiting." );			
    } else {
      Output( "f_basePath = %s", f_basePath.GetVal().c_str() );
    }
    InitInput();
    ExecuteCommand( "readbindings default" );
    ExecuteCommand( "readvars" );
#if R3_HAS_GL
    InitBuffer();
    InitDraw();
    InitFont();
    InitModel();
    InitShader();
    InitTexture();
    InitConsole();
#endif
  }

    void Shutdown();
    
  void Shutdown() {
#if R3_HAS_GL
    ShutdownConsole();
    ShutdownFont();
    ShutdownModel();
    ShutdownTexture();
    ShutdownShader();
    ShutdownDraw();
    ShutdownBuffer();
#endif
  }

}

