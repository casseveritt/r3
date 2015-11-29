/*
 array
 
 Copyright (c) 2010 Cass Everitt
 
 from glh library - 
 
 
 
    Copyright (C) 2000 Cass Everitt

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Cass Everitt - cass@r3.nu
*/

// Simple array template.
// Copyright (c) Cass W. Everitt 1999 

#ifndef __R3_ARRAY_H__
#define __R3_ARRAY_H__

namespace r3
{
  
  template <class T> class Array2
  {
  public:
	typedef T ValueType;

	Array2(int arrayWidth=1, int arrayHeight=1) 
    {
      width = arrayWidth;
      height = arrayHeight;
      data = new T [width * height];
      Clear(T());
    }
	
	Array2(const Array2<T> & t)
    {
      width = height = 0;
      data=0;
      (*this) = t;
    }
	
	// intentionally non-virtual 
	~Array2() { delete [] data; }
	
	const Array2 & operator = (const Array2<T> & t)
    {
      if( width != t.width || height != t.heigh ) {
		  SetSize( t.width, t.height );
	  }
	  int sz = width * height;
      for(int i = 0; i < sz; i++) {
		  data[i] = t.data[i];
	  } 
      return *this;
    }
	
	void SetSize(int arrayWidth, int arrayHeight)
	{
		if( width == arrayWidth && height == arrayHeight) {
			return;	
		} 
		delete [] data;
		width = arrayWidth;
		height = arrayHeight;
		data = new T [width * height];
		memset( data, 0, width * height * sizeof( T ) );
	}

	T & operator () (int i, int j) {
		return data[i + j * width];
	}
	
	const T & operator () (int i, int j) const {
		return data[i + j * width];
	}

	int Size(int i) const { 
		return ( (int *) & width )[i];
	}
	
	void Clear(const T & val) {
      int sz = width * height;
      for(int i = 0; i < sz; i++) {
		  data[i] = val;  
	  }     
	}

	void Copy(const Array2<T> & src, int iOffset = 0, int jOffset = 0,
		      int iSize = 0, int jSize = 0)
	{
		int io = iOffset;
		int jo = jOffset;
		if(iSize == 0) {
			iSize = src.Size(0);
		} 
		if(jSize == 0) {
			jSize = src.Size(1);
		}
		if( ( io + iSize ) > width ) {
			iSize = width - io;			
		}
		if( ( jo + jSize ) > height ) {
			jSize = height - jo;
		}
		if ( iSize < 1 || jSize < 1 ) {
			return;
		}
		for(int i=0; i < iSize; i++) {
			for(int j=0; j < jSize; j++) {
				(*this)(io+i, jo+j) = src(i,j);			
			}
		}
	}

	T * Ptr() { return data; }
	const T * Ptr() const { return data; }

  private:
	
	int width, height;
	T * data;
  };
  
}
#endif  // __R3_ARRAY_H__
