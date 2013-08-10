/*
 *  filesystem
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

#ifndef __R3_FILESYSTEM_H__
#define __R3_FILESYSTEM_H__

#include "r3/common.h"
#include <string>
#include <vector>

namespace r3 {

	void InitFilesystem();
  void ShutdownFilesystem();
	
	enum SeekEnum {
		Seek_Begin,
		Seek_Curr,
		Seek_End
	};

	// will need to convert many of these ints to 64 bit types at some point
	class File {
	public:
		virtual ~File() {}
		virtual int Read( void *data, int size, int nitems ) = 0;
		virtual int Write( const void *data, int size, int nitems ) = 0;
		virtual void Seek( SeekEnum whence, int offset ) = 0;
		virtual int Tell() = 0;
		virtual int Size() = 0;
		virtual bool AtEnd() = 0; 
		virtual double GetModifiedTime() = 0;

		// convenience
		std::string ReadLine();
		void WriteLine( const std::string & str );
	};

	File * FileOpenForWrite( const std::string & filename );
	File * FileOpenForRead( const std::string & filename );
    void FileDelete( const std::string & filename );
	
	bool FileReadToMemory( const std::string & filename, std::vector< uchar > & data );
	
}

#endif // __R3_FILESYSTEM_H__