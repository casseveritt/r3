/*
 *  modelobj
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


#include "r3/modelobj.h"

#include "r3/common.h"
#include "r3/filesystem.h"
#include "r3/output.h"
#include "r3/parse.h"

#include <vector>
#include <map>

using namespace r3;
using namespace std;

namespace {

	enum Mode {
		Mode_INVALID,
		Mode_Vertex,
		Mode_Face,
		Mode_Failed
	};
	
	
	
	struct ParseState {
		ParseState() : mode( Mode_Face ), varying( 0 ), indexBase( 0 ) {}
		Mode mode;
		int varying;
		vector< Vec3f > v;
		vector< Vec2f > vt;
		vector< Vec3f > vn;
		int sizes[3];
		map< r3::uint64, int > unique;
		vector< float > vbdata;
		vector< r3::ushort> ibdata;
		int indexBase;

		void ChangeMode( Mode newMode ) {
			if ( mode == newMode ) {
				return;
			}
			if ( newMode == Mode_Vertex ) {
				v.clear();
				vn.clear();
				vt.clear();
				int components = 0;
				components += varying & Varying_PositionBit ? 3 : 0;
				components += varying & Varying_NormalBit ? 3 : 0;
				components += varying & Varying_TexCoord0Bit ? 2 : 0;
				if ( components ) {
					indexBase = (int)vbdata.size() / components;
				} else {
					indexBase = 0;
				}
				unique.clear();
			} else if ( newMode == Mode_Face ) {
				sizes[0] = (int)v.size();
				sizes[1] = (int)vt.size();
				sizes[2] = (int)vn.size();
			}
			mode = newMode;
		}
		
		void PushVertexBufferData( int vindex, int vtindex, int vnindex ) {
			if ( varying & Varying_PositionBit ) {
				Vec3f p = v[ vindex ];
				vbdata.push_back( p.x );
				vbdata.push_back( p.y );
				vbdata.push_back( p.z );
				//Output( "pushing vertex [%d] %f, %f, %f", (int)vbdata.size(), p.x, p.y, p.z );
			}
			if ( varying & Varying_NormalBit ) {
				Vec3f n = vn[ vnindex ];
				vbdata.push_back( n.x );
				vbdata.push_back( n.y );
				vbdata.push_back( n.z );
				//Output( "pushing normal [%d] %f, %f, %f", (int)vbdata.size(), n.x, n.y, n.z );
			}
			if ( varying & Varying_TexCoord0Bit ) {
				Vec2f t = vt[ vindex ];
				vbdata.push_back( t.x );
				vbdata.push_back( t.y );
			}
		}
		
		r3::ushort ProcessIndex( const Token & t ) {
			string s = t.valString;
			vector< int > ind;
			int bits[] = { Varying_PositionBit, Varying_TexCoord0Bit, Varying_NormalBit };
			int var = 0;
			while( s.size() > 0 ) {
				if ( ind.size() > 3 ) {
					break;
				}
				string t;
				while( s.size() > 0 && ( s[0] != '/' ) ) {
					t += s[0];
					s.erase( s.begin() );
				}
				if ( s.size() > 0 ) {
					s.erase( s.begin() );
				}
				float f;
				if ( StringToFloat( t, f ) ) {
					var |= bits[ ind.size() ];
					int i = f;
					if ( i < 0 ) {
						i += sizes[ ind.size() ];
					} else {
						i -= 1;
					}
					ind.push_back( i );
				} else {
					ind.push_back( 0 );
				}
			}
			if ( varying == 0 ) {
				varying = var;
			}
			if ( varying != var ) {
				Output( "modelobj: Varying not allowed to mismatch in face index." );
				mode = Mode_Failed;
			}
			while ( ind.size() < 3 ) {
				ind.push_back( 0 );
			}
			r3::uint64 r = 0;
			for ( int i = 0; i < (int)ind.size(); i++ ) {
				r |= ( r3::uint64( ind[i] ) & 0xfffff ) << ( i * 20 );
			}
			if ( unique.count( r ) == 0 ) {
				int sz = (int)unique.size();
				unique[ r ] = sz;
				PushVertexBufferData( ind[0], ind[1], ind[2] );
			}
			return unique[ r ] + indexBase;
		} 
		
		void ProcessLine( const vector< Token > & tokens ) {
			if ( mode == Mode_Failed ) {
				return;
			}
			if ( tokens.size() > 0 ) {
				// comment
				if ( tokens[0].valString[0] == '#' ) {
					return;
				} else if ( tokens[0].valString == "v" ) {	 // vertex position
					ChangeMode( Mode_Vertex );
					if ( tokens.size() < 4 || tokens.size() > 5 ) {
						mode = Mode_Failed;
					}
					Vec3f p;
					p.x = tokens[1].valNumber;
					p.y = tokens[2].valNumber;
					p.z = tokens[3].valNumber;
					if ( tokens.size() == 5 ) {
						p *= 1.0 / tokens[4].valNumber;
					}
					v.push_back( p );
				} else if ( tokens[0].valString == "vt" ) {	 // vertex texcoord
					ChangeMode( Mode_Vertex );
					if ( tokens.size() != 3 ) {
						mode = Mode_Failed;
					}
					Vec2f t;
					t.x = tokens[1].valNumber;
					t.y = tokens[2].valNumber;
					vt.push_back( t );
				} else if ( tokens[0].valString == "vn" ) {	 // vertex normal
					ChangeMode( Mode_Vertex );
					if ( tokens.size() != 4 ) {
						mode = Mode_Failed;
					}
					Vec3f n;
					n.x = tokens[1].valNumber;
					n.y = tokens[2].valNumber;
					n.z = tokens[3].valNumber;
					vn.push_back( n );
				} else if ( tokens[0].valString == "f" ) { // a face
					ChangeMode( Mode_Face );
					if ( tokens.size() < 4 ) {
						mode = Mode_Failed;
						return;
					}
					r3::ushort i0 = ProcessIndex( tokens[1] );
					r3::ushort i1 = ProcessIndex( tokens[2] );
					for ( int i = 3; i < (int)tokens.size(); i++ ) {
						r3::ushort i2 = ProcessIndex( tokens[i] );
						ibdata.push_back( i0 );
						ibdata.push_back( i1 );
						ibdata.push_back( i2 );
						//Output( "index [%d] = %d %d %d", (int)ibdata.size(), (int)i0, (int)i1, (int)i2 );
						i1 = i2; 
					}
				}
			}
		}
	};
	
}


namespace r3 {
	Model * CreateModelFromObjFile( const std::string & filename ) {
		File *file = FileOpenForRead( filename );
		ParseState ps;
		while ( file->AtEnd() == false && ps.mode != Mode_Failed ) {
			string line = file->ReadLine();
			vector< Token > tokens = TokenizeString( line.c_str() );
			ps.ProcessLine( tokens );
		}
		delete file;
		if ( ps.mode == Mode_Failed ) {
			return NULL;
		}
		Model *m = new Model( filename );
		m->SetPrimitive( GL_TRIANGLES );
		VertexBuffer & vb = m->GetVertexBuffer();
        // FIXME: Need to add AttributeArrays here, varying flags have been removed...
		//vb.SetVarying( ps.varying );
		//Output( "model %s varying = %d", filename.c_str(), ps.varying );
		vb.SetData( (int)ps.vbdata.size() * sizeof( float ), &ps.vbdata[0] );
		m->GetIndexBuffer().SetData( (int)ps.ibdata.size() * sizeof( r3::ushort ), &ps.ibdata[0] );
		return m;
	}
}
