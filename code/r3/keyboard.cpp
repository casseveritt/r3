/*
 * r3 virtual keyboard
 *
 * Copyright (c) 2010 Cass Everitt
 * All rights reserved.
 *
 */

#include "r3/bounds.h"
#include "r3/draw.h"
#include "r3/font.h"
#include "r3/var.h"
#include <GL/Regal.h>

#include "r3/keyboard.h"
#include "r3/keysymdef.h"


#include <math.h>
#include <vector>

using namespace std;
using namespace r3;

r3::VarString kbd_font( "kbd_font", "console font", Var_Archive, "LiberationMono-Regular.ttf" );
r3::VarInteger kbd_fontSize( "kbd_fontSize", "console font rasterization size", 0, 12 );
r3::VarFloat kbd_fontScale( "kbd_fontScale", "console font rendering scale", 0, 0.5f );


namespace {

	struct Key {
		Key( int k, float xmin, float ymin, float xmax, float ymax ) 
			: keysym( k ), bounds( xmin, ymin, xmax, ymax ) {}
		int keysym;
		Bounds2f bounds;
	};

	void AppendKeys( vector< Key > & keys, const char *str ) {
		while ( *str != 0 ) {
			keys.push_back( Key( *str, 0, 0, 1, 1 ) );
			str++;
		}
	}

	void MakeRow( vector< Key > & keys, float border = 0.1f ) {
		float width = 0;
		for ( int i = 0; i < (int)keys.size(); i++ ) {
			Key &k = keys[i];
			k.bounds += Vec2f( width, 0 ) - k.bounds.Min();
			width += k.bounds.Width() + 2 * border;
		}
	}

	void TranslateKeys( vector< Key > & keys, Vec2f t ) {
		for ( int i = 0; i < (int)keys.size(); i++ ) {
			keys[i].bounds += t;
		}
	}

	void ScaleKeys( vector< Key > & keys, Vec2f s ) {
		for ( int i = 0; i < (int)keys.size(); i++ ) {
			keys[i].bounds *= s;
		}
	}

	Bounds2f GetBounds( vector< Key > & keys ) {
		Bounds2f b;
		for ( int i = 0; i < (int)keys.size(); i++ ) {
			b.Add( keys[i].bounds );
		}
		return b;
	}

}

namespace r3 {
	
	struct SimpleKeyboard : public Keyboard  {
		
		SimpleKeyboard();
		virtual ~SimpleKeyboard();
		virtual void SetAspect( float aspect ) {}
		virtual void Draw();
		virtual void BeginFocus() {}
		virtual void FocusPosition( Vec2f pos ) {}
		virtual void EndFocus() {}
		
		// at the current position while focused...
		virtual void Activate(){}

		virtual void SetRect( const Rect & r );

		vector<Key> keys;
		Rect rect;
		float aspect;
		Font *font;	
		Texture2D *circle;

	};

	SimpleKeyboard::SimpleKeyboard() {

		circle = Texture2D::Create( "circle", TextureFormat_RGBA, 64, 64 );
		
		char *img = new char [ 64 * 64 * 4 ];
		for( int i = 0; i < 64; i++ ) {
			float fi = ( i + 0.5f ) - 32;
			for( int j = 0; j < 64; j++ ) {
				float fj = ( j + 0.5f ) - 32;
				float r = sqrtf( fi * fi + fj * fj );
				char * t = img + ( ( j * 64 + i ) * 4 );
				float f = max( min( 31.0f - r, 1.0f ), 0.0f );
				t[0] = t[1] = t[2] = char( 255 );
				t[3] = char( f * 255.f );
				
			}
		}
		circle->SetImage( 0, img );
		
		
		font = r3::CreateStbFont( kbd_font.GetVal(), "", (float)kbd_fontSize.GetVal() ); 			
		vector< vector< Key > > rows;
		
		
		// add all the rows
		{
			vector< Key > row;
			AppendKeys( row, "qwertyuiop" );
			MakeRow( row );
			rows.push_back( row );
		}
		{
			vector< Key > row;
			AppendKeys( row, "asdfghjkl" );
			MakeRow( row );
			rows.push_back( row );
		}
		{
			vector< Key > row;
			AppendKeys( row, "zxcvbnm" );
			MakeRow( row );
			rows.push_back( row );
		}
		{
			vector< Key > row;
			AppendKeys( row, " " );
			MakeRow( row );
			row[0].bounds.ur.x = 5.0f;
			rows.push_back( row );
		}


		// center the rows
		Bounds2f b;
		for( int i = 0; i < (int)rows.size(); i++ ) {
			b.Add( GetBounds( rows[i] ) );
		}
		for( int i = 0; i < (int)rows.size(); i++ ) {
			Bounds2f rb = GetBounds( rows[i] );
			TranslateKeys( rows[i], Vec2f( ( b.Width() - rb.Width() ) / 2.0f, 0 ) );
		}



		// dump all the rows into the keys vector...
		for ( int i = 0; i < (int)rows.size(); i++ ) {
			vector< Key > & row = rows[i];
			keys.insert( keys.end(), row.begin(), row.end() );
			TranslateKeys( keys, Vec2f( 0, 1.2f ) );
		}
		TranslateKeys( keys, Vec2f( 0, -1.2f ) );


		Bounds2f bounds = GetBounds( keys );
		aspect = bounds.Width() / bounds.Height();
		Rect r( 0, 0, 1, 1 );
		SetRect( r );
	}

	SimpleKeyboard::~SimpleKeyboard() {
		delete font;
	}


	void SimpleKeyboard::SetRect( const Rect & r ) {
		Bounds2f bounds = GetBounds( keys );

		TranslateKeys( keys, -bounds.ll );

		float a = ( r.Width() / r.Height() ) / aspect;

		if ( a > 1 ) {
			ScaleKeys( keys, Vec2f( r.Width() / ( a * bounds.Width() ), r.Height() / bounds.Height() ) );
		} else {
			ScaleKeys( keys, Vec2f( r.Width() / bounds.Width(), a * r.Height() / bounds.Height() ) );
		}
		TranslateKeys( keys, r.ll );
		//bounds = GetBounds( keys );
	
	}

	void SimpleKeyboard::Draw()	{

		glColor4f( .4f, .4f, .4f, 1 );
		
		Bounds2f b = GetBounds( keys );
        glBegin( GL_LINE_STRIP );
		glVertex2f( b.ll.x, b.ll.y );
		glVertex2f( b.ur.x, b.ll.y );
		glVertex2f( b.ur.x, b.ur.y );
		glVertex2f( b.ll.x, b.ur.y );
		glVertex2f( b.ll.x, b.ll.y );
		glEnd();
		
		circle->Bind( 0 );
		circle->Enable( 0 );
        glEnable( GL_BLEND );

		glColor4f( .4f, .4f, .4f, 1 );
			
        glBegin( GL_QUADS );
		for ( int i = 0; i < (int)keys.size(); i++ ) {
			Bounds2f b = keys[i].bounds;
			float radius = 8;
			float fi[4];
			float fj[4];
			float ft[4];
			
			fi[0] = b.ll.x;
			fi[3] = b.ur.x;
			if ( b.Width() > radius * 2 ) {
				fi[1] = fi[0] + radius;
				fi[2] = fi[3] - radius;
			} else {
				fi[1] = fi[2] = b.Mid().x;
			}
			
			fj[0] = b.ll.y;
			fj[3] = b.ur.y;
			if ( b.Height() > radius * 2 ) {
				fj[1] = fj[0] + radius;
				fj[2] = fj[3] - radius;
			} else {
				fj[1] = fj[2] = b.Mid().y;
			}
			
			ft[0] = 0.0f;
			ft[1] = 0.5f;
			ft[2] = 0.5f;
			ft[3] = 1.0f;				
			
					
			for( int i = 0; i < 3; i++ ) {
				for( int j = 0; j < 3; j++ ) {
                    glMultiTexCoord2f( GL_TEXTURE0, ft[ i + 0 ], ft[ j + 0 ] );
					glVertex2f( fi[ i + 0 ], fj[ j + 0 ] );
					glMultiTexCoord2f( GL_TEXTURE0, ft[ i + 1 ], ft[ j + 0 ] );
					glVertex2f( fi[ i + 1 ], fj[ j + 0 ] );
					glMultiTexCoord2f( GL_TEXTURE0, ft[ i + 1 ], ft[ j + 1 ] );
					glVertex2f( fi[ i + 1 ], fj[ j + 1 ] );
					glMultiTexCoord2f( GL_TEXTURE0, ft[ i + 0 ], ft[ j + 1 ] );
					glVertex2f( fi[ i + 0 ], fj[ j + 1 ] );
				}				
			}
		}
        glEnd();
        glDisable( GL_BLEND );
		circle->Disable( 0 );
		
		float s = kbd_fontScale.GetVal();
		glColor4f( .4f, 0, 0, 1 );

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );
		for ( int i = 0; i < (int)keys.size(); i++ ) {
			Bounds2f b = keys[i].bounds;
			string str;
			str.push_back( keys[i].keysym );
			float descent = font->GetDescent() * s;
			font->Print( str, b.Mid().x, b.Min().y - descent, s, Align_Mid, Align_Min );
		}
        glDisable( GL_BLEND );
	}


	Keyboard * CreateSimpleKeyboard() {
		return new SimpleKeyboard();
	}
	
}

