/*
 bounds

 Copyright (c) 2010 Cass Everitt

 from glh library - 
 
 Copyright (c) 2000-2009 Cass Everitt
 Copyright (c) 2000 NVIDIA Corporation
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

// Author:  Cass W. Everitt

#ifndef __R3_BOUNDS_H__
#define __R3_BOUNDS_H__

#include <algorithm>
#include "r3/linear.h"

namespace r3 {

	struct Rect {
		Rect() : ll( 0, 0 ), ur( 0, 0 ) {}
		Rect( const Vec2f & lowerLeft, const Vec2f & upperRight ) : ll( lowerLeft ), ur( upperRight ) {}
		Rect( float x0, float y0, float x1, float y1 ) : ll( x0, y0 ), ur( x1, y1 ) {}

		Vec2f & Min() { return ll; }
		Vec2f & Max() { return ur; }
		const Vec2f & Min() const { return ll; }
		const Vec2f & Max() const { return ur; }

		float Width() const { return ur.x - ll.x; }
		float Height() const { return ur.y - ll.y; }
		Vec2f Mid() const { return ( ur + ll ) * 0.5f; }

		// negative inset grows bounds
		void Inset( float f ) {
			Vec2f v( f, f );
			ll += v;
			ur -= v;
		}

		const Rect & operator *=( float scale );
		const Rect & operator *=( const Vec2f & scale );
		const Rect & operator +=( const Vec2f & trans );

		Vec2f ll;
		Vec2f ur;
	};

	inline Rect operator * ( const Rect & rect, float scale ) {
		Rect ret = rect;
		ret.ll *= scale;
		ret.ur *= scale;
		return ret;
	}

	inline Rect operator * ( const Rect & rect, const Vec2f & scale ) {
		Rect ret = rect;
		ret.ll *= scale;
		ret.ur *= scale;
		return ret;
	}

	inline Rect operator + ( const Rect & rect, const Vec2f & trans ) {
		Rect ret = rect;
		ret.ll += trans;
		ret.ur += trans;
		return ret;
	}

	inline const Rect & Rect :: operator *= ( float scale ) {
		*this = *this * scale;
		return *this;
	}

	inline const Rect & Rect :: operator *= ( const Vec2f & scale ) {
		*this = *this * scale;
		return *this;
	}

	inline const Rect & Rect :: operator += ( const Vec2f & trans ) {
		*this = *this + trans;
		return *this;
	}


	struct Bounds2f : public Rect {
		Bounds2f() { Clear(); }
		Bounds2f( const Vec2f &pmin, const Vec2f &pmax ) : Rect( pmin, pmax ) {}
		Bounds2f( float x0, float y0, float x1, float y1 ) : Rect( x0, y0, x1, y1 ) {}
			
		void Set( float x0, float y0, float x1, float y1 ) {
			*(Rect *)this = Rect( x0, y0, x1, y1 );
		}
		
		void Clear() { ll = Vec2f( 1e20f, 1e20f );  ur = Vec2f( -1e20f, -1e20f ); }
		bool IsClear() const { return ll.x > ur.x || ll.y > ur.y; }

		bool IsInside( const Vec2f & p ) {
			return ll.x <= p.x && ll.y <= p.y && ur.x >= p.x && ur.y >= p.y;
		}

		void Add( const Vec2f v ) {
			if ( IsClear() ) {
				ll = ur = v;
				return;
			}
			ll.x = v.x < ll.x ? v.x : ll.x;
			ll.y = v.y < ll.y ? v.y : ll.y; 
			ur.x = v.x > ur.x ? v.x : ur.x;
			ur.y = v.y > ur.y ? v.y : ur.y;
		}
		
		void Add( const Bounds2f & b ) {
			if ( b.IsClear() ) {
				return;
			}
			Add( b.ur );
			Add( b.ll );
		}
		
	};

	inline Bounds2f Intersection( const Bounds2f & a, const Bounds2f & b ) {
		Bounds2f r;
		r.Min() = Max( a.Min(), b.Min() );
		r.Max() = Min( a.Max(), b.Max() );
		return r;
	}
	
	struct OrientedBounds2f {
		OrientedBounds2f() : empty( true ) {}
		bool empty;
		Vec2f vert[4];
	};
	
	inline bool Intersect( const OrientedBounds2f & a, const OrientedBounds2f & b ) {
		int ind[] = { 0, 1, 2, 3, 0 };

		// do any segments intersect ?
		for ( int i = 0; i < 4; i++ ) {
			LineSegment2f sa( a.vert[ ind[ i ] ], a.vert[ ind[ i + 1 ] ] );
			for ( int j = 0; j < 4; j++ ) {
				LineSegment2f sb( b.vert[ ind[ j ] ], b.vert[ ind[ j + 1 ] ] );
				if ( Intersect( sa, sb ) ) {
					return true;
				}
			}
		}

		// is a completely inside b ?
		bool inside = true;
		for ( int i = 0; i < 4 && inside ; i++ ) {
			LineSegment2f sb( b.vert[ ind[ i ] ], b.vert[ ind[ i + 1 ] ] );
			Vec3f pl = sb.GetPlane();
			for ( int j = 0; j < 4 && inside ; j++ ) {
				inside = pl.Dot( Vec3f( a.vert[ j ].x, a.vert[ j ].y, 1 ) ) > 0.0f;
			}
		}
		if ( inside ) {
			return true;
		}
		
		// is b completely inside a ?
		inside = true;
		for ( int i = 0; i < 4 && inside ; i++ ) {
			LineSegment2f sa( a.vert[ ind[ i ] ], a.vert[ ind[ i + 1 ] ] );
			Vec3f pl = sa.GetPlane();
			for ( int j = 0; j < 4 && inside ; j++ ) {
				inside = pl.Dot( Vec3f( b.vert[ j ].x, b.vert[ j ].y, 1 ) ) > 0.0f;
			}
		}
		if ( inside ) {
			return true;
		}
		
		
		return false;
	}
	
	
}

#endif // __R3_BOUNDS_H__

