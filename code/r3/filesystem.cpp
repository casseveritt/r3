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

#include "r3/filesystem.h"

#include "r3/command.h"
#include "r3/http.h"
#include "r3/md5.h"
#include "r3/output.h"
#include "r3/parse.h"
#include "r3/thread.h"
#include "r3/r3time.h"
#include "r3/ujson.h"
#include "r3/var.h"

#if __APPLE__
# include <TargetConditionals.h>
#endif

#if __APPLE__ || ANDROID || __linux__
# include <unistd.h>
# include <dirent.h>
# include <sys/stat.h>
#endif

#if _WIN32
# include <Windows.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <deque>

using namespace std;
using namespace r3;

VarInteger f_numOpenFiles( "f_numOpenFiles", "Number of open files.", 0, 0 );

#if ANDROID
// copy a file out of the apk into the cache
extern bool appMaterializeFile( const char * filename );
VarString app_apkPath( "app_apkPath", "path to APK file", 0, "" );
double apkTimestamp;
#endif

namespace r3 {
	VarString f_basePath( "f_basePath", "path to the base directory", Var_ReadOnly, "");
	VarString f_cachePath( "f_cachePath", "path to writable cache directory", Var_ReadOnly, "" );
	VarString f_netPath( "f_netPath", "semicolon separated base urls", Var_ReadOnly, "http://home.xyzw.us/star3map/data/" );
  VarBool   f_cacheUpdated( "f_cacheUpdated", "tells us we need to restart at the next opportunity", Var_ReadOnly, false );
}

// Try to download a the file again if it's been this long (in seconds)
#define CACHE_REFRESH_INTERVAL 3600
// Wait at least this long after a cached file has been opened before refreshing it from the net (in seconds)
#define CACHE_FETCH_DELAY 15

namespace {
  File * CachedFileOpenForPrivateRead( const string & inFileName );
  
  string ComputeMd5Sum(File *file) {
    if( file == NULL || file->Size() == 0 ) {
      return "00000000000000000000000000000000000000";
    }
    unsigned char buf[4096];
    MD5Context ctx;
    MD5Init( &ctx );
    int pos = file->Tell();
    file->Seek( Seek_Begin, 0 );
    for(;;) {
      int sz = file->Read( buf, 1, sizeof(buf) );
      if( sz == 0 ) {
        break;
      }
      MD5Update( &ctx, buf, sz );
    }
    file->Seek( Seek_Begin, pos );
    MD5Final( buf, &ctx );
    char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    string sum;
    for( int i = 0; i < 16; i++ ) {
      sum += hex[ ( buf[i] >> 4 ) & 0xf ];
      sum += hex[ ( buf[i] >> 0 ) & 0xf ];
    }
    return sum;
  }
  
  struct ManifestInfo {
    ManifestInfo() : lastTry(0) {}
    string url;
    string md5;
    string etag;
    string lastModified;
    int lastTry;
  };
  
  Condition filesystemCond;
  Mutex filesystemMutex;
  map<string, ManifestInfo> manifest;
  
  ManifestInfo & GetManifestInfo( const string & filename ) {
    ScopedMutex scm( filesystemMutex, R3_LOC );
    return manifest[ filename ];
  }
  
  void WriteCacheManifest() {
    if( f_cachePath.GetVal().size() == 0 ) {
      return;
    }
    File * f = FileOpenForWrite( "CacheManifest.json" );
    if( f == NULL ) {
      return;
    }
    stringstream ss ( stringstream::out );
    ss << "{ ";
    string sep = "";
		for( map<string,ManifestInfo>::iterator it = manifest.begin(); it != manifest.end(); ++it ) {
      ManifestInfo &mi = it->second;
      ss << sep << endl;
      ss << "  \"" << it->first.c_str() << "\": {" << endl;
      bool prev = false;
      if( mi.url.size() ) {
        if( prev ) {
          ss << "," << endl;
        }
        prev = true;
        ss << "    \"url\": \"" << mi.url.c_str() << "\"";
      }
      if( mi.md5.size() ) {
        if( prev ) {
          ss << "," << endl;
        }
        prev = true;
        ss << "    \"md5\": \"" << mi.md5.c_str() << "\"";
      }
      if( mi.etag.size() ) {
        if( prev ) {
          ss << "," << endl;
        }
        prev = true;
        ss << "    \"etag\": \"" << mi.etag.c_str() << "\"";
      }
      if( mi.lastModified.size() ) {
        if( prev ) {
          ss << "," << endl;
        }
        prev = true;
        ss << "    \"Last-Modified\": \"" << mi.lastModified.c_str() << "\"";
      }
      if( mi.lastTry != 0 ) {
        if( prev ) {
          ss << "," << endl;
        }
        prev = true;
        ss << "    \"lastTry\": " << mi.lastTry;
      }
      if( prev ) {
        ss << endl;
      }
      ss << "  }";
      sep = ",";
		}
    ss << endl << "}" << endl;
    f->WriteLine( ss.str() );
		delete f;
  }
  
  void ReadCacheManifest() {
    vector< unsigned char > data;
    bool success = FileReadToMemory( "CacheManifest.json",  data );
    if( !success || data.size() == 0 ) {
      Output( "Failed to read the CacheManifest.json file." );
      return;
    }
    
    ujson::Json * root =  ujson::Decode( reinterpret_cast<const char *>(&data[0]), int( data.size() ) );
    if( root && root->GetType() == ujson::Type_Object ) {
      map<string, ujson::Json *> & m = root->m;
      for( map<string, ujson::Json *>::iterator i = m.begin(); i != m.end(); ++i ) {
        if( i->second == NULL || i->second->GetType() != ujson::Type_Object ) {
          continue;
        }
        ManifestInfo mi;
        map<string, ujson::Json *> & f = i->second->m;
        string fn = i->first;
        if( f.count( "url" ) && f["url"]->GetType() == ujson::Type_String ) {
          mi.url = f["url"]->s;
        }
        if( f.count( "md5" ) && f["md5"]->GetType() == ujson::Type_String ) {
          mi.md5 = f["md5"]->s;
        }
        if( f.count( "etag" ) && f["etag"]->GetType() == ujson::Type_String ) {
          mi.etag = f["etag"]->s;
        }
        if( f.count( "Last-Modified" ) && f["Last-Modified"]->GetType() == ujson::Type_String ) {
          mi.lastModified = f["Last-Modified"]->s;
        }
        if( f.count( "lastTry" ) && f["lastTry"]->GetType() == ujson::Type_Number ) {
          mi.lastTry = f["lastTry"]->n;
        }
        GetManifestInfo( i->first ) = mi;
      }
    }
    
  }
  
  void WriteCacheManifest_cmd( const vector< Token > & tokens ) {
    WriteCacheManifest();
	}
	CommandFunc WriteCacheManifestCmd( "writecachemanifest", "write the cache manifest file", WriteCacheManifest_cmd );
  
	string NormalizePathSeparator( const string inPath ) {
		string path = inPath;
		for ( int i = 0; i < (int)path.size(); i++ ) {
			if ( path[i] == '\\' )  {
				path[i] = '/';
			}
		}
		return path;
	}
  
#if ! _WIN32
	double getFileTimestamp( const char *path ) {
		struct stat s;
		double ts = 0.0;
		if ( 0 == stat( path, &s ) )  {
#if ANDROID || __linux__
			ts = double( s.st_mtime );
#else
			ts = double( s.st_mtimespec.tv_sec );
#endif
		}
		//Output( "Timestamp is %lf for %s", ts, path );
		return ts;
	}
#endif
	
#if _WIN32
	FILE *Fopen( const char *filename, const char *mode ) {
		FILE *fp;
		fopen_s( &fp, filename, mode );
		if ( fp ) {
			f_numOpenFiles.SetVal( f_numOpenFiles.GetVal() + 1 );
		}
		return fp;
	}
#else
	FILE *Fopen( const char *filename, const char *mode ) {
		FILE *fp;
		fp = fopen( filename, mode );
		f_numOpenFiles.SetVal( f_numOpenFiles.GetVal() + 1 );
		return fp;
	}
#endif
  void Fclose( FILE * fp ) {
    f_numOpenFiles.SetVal( f_numOpenFiles.GetVal() - 1 );
    fclose( fp );
  }
  
  
  class StdCFile : public r3::File {
  public:
    FILE *fp;
    bool unlinkOnDestruction;
    bool write;
    bool internal;
    string name;
    StdCFile( const string & filename, bool filewrite )
    : fp( 0 )
    , unlinkOnDestruction( false )
    , write( filewrite )
    , internal( false )
    , name( filename )
    {}
    
    virtual ~StdCFile() {
      if ( fp ) {
        Fclose( fp );
        if( unlinkOnDestruction > 0 ) {
          unlink( name.c_str() );
        }
        if( write ) {
          size_t loc = name.find( f_cachePath.GetVal() );
          if( loc != string::npos ) {
            string fn = name.substr( name.rfind('/') + 1 );
            if( internal == false && fn != "CacheManifest.json" ) {
              StdCFile * file = reinterpret_cast<StdCFile *>( CachedFileOpenForPrivateRead( fn ) );
              file->internal = true;
              string md5 = ComputeMd5Sum( file );
              delete file;
              GetManifestInfo( fn ).md5 = md5;
              //Output( "md5 for %s = %s", fn.c_str(), md5.c_str() );
              //WriteCacheManifest();
            }
          }
        }
      }
    }
    virtual int Read( void *data, int size, int nitems ) {
      return (int)fread( data, (size_t)size, (size_t)nitems, fp );
    }
    
    virtual int Write( const void *data, int size, int nitems ) {
      return (int)fwrite( data, size, nitems, fp );
    }
    
    virtual void Seek( SeekEnum whence, int offset ) {
      fseek( fp, offset, (int)whence );
    }
    
    virtual int Tell() {
      return (int)ftell( fp );
    }
    
    virtual int Size() {
      int curPos = Tell();
      Seek( Seek_End, 0 );
      int size = Tell();
      Seek( Seek_Begin, curPos );
      return size;
    }
    
    virtual bool AtEnd() {
      return feof( fp ) != 0;
    }
    
    virtual double GetModifiedTime() {
      struct stat s;
      fstat( fileno( fp ), & s );
#if ANDROID || __linux__
      return double( s.st_mtime );
#else
      return double( s.st_mtimespec.tv_sec );
#endif
    }
    
  };
  
  bool FindDirectory( VarString & path, const char * dirName ) {
    string dirname = dirName;
#if ! _WIN32
    // figure out current working directory
    if ( path.GetVal().size() > 0 ) {
      DIR *dir = opendir( path.GetVal().c_str() );
      if ( ! dir ) {
        return false;
      }
    } else {
      Output( "Searching for \"%s\" directory.", dirName );
      char ccwd[256];
      getcwd( ccwd, sizeof( ccwd ));
      string cwd = ccwd;
      for( int i = 0; i < 10; i++ ) {
        Output( "cwd = %s", cwd.c_str() );
        DIR *dir = opendir( ( cwd + string("/") + dirname ).c_str() );
        if ( dir ) {
          cwd += "/" + dirname + "/";
          path.SetVal( cwd );
          Output( "Found \"%s\": %s", dirName, cwd.c_str() );
          closedir( dir );
          break;
        } else {
          cwd += "/..";
        }
      }
    }
#else
    WIN32_FIND_DATAA findData;
    // figure out current working directory
    if ( path.GetVal().size() > 0 ) {
      HANDLE hDir = FindFirstFileA( path.GetVal().c_str(), &findData );
      if ( hDir == INVALID_HANDLE_VALUE ) {
        return false;
      }
    } else {
      //Output( "Searching for base directory." );
      char ccwd[256];
      GetCurrentDirectoryA( sizeof( ccwd ), ccwd );
      string cwd = ccwd;
      for( int i = 0; i < 10; i++ ) {
        //Output( "cwd = %s", cwd );
        HANDLE hDir = FindFirstFileA( ( cwd + "/" + dirname ).c_str(), &findData );
        if ( hDir != INVALID_HANDLE_VALUE ) {
          cwd += "/" + dirname + "/";
          cwd = NormalizePathSeparator( cwd );
          path.SetVal( cwd );
          //Output( "Found base: %s", base.c_str() );
          FindClose( hDir );
          break;
        } else {
          cwd += "/..";
        }
      }
    }
#endif
    return true;
  }
  
  bool MakeDirectory( const char * dirName ) {
    assert( dirName );
    vector<Token> tokens = TokenizeString( dirName, "/" );
    Output( "In MakeDirectory( \"%s\" )", dirName );
    string dirname = dirName[0] == '/' ? "/" : "";
    for ( int i = 0; i < (int)tokens.size(); i++ ) {
      dirname += tokens[i].valString;
      //Output( "Checking dir %s", dirname.c_str() );
#if ! _WIN32
      if ( getFileTimestamp( dirname.c_str() ) == 0.0 ) {
        Output( "Attempting to create dir %s", dirname.c_str() );
        int ret = mkdir( dirname.c_str(), 0777 );
        if ( ret != 0 ) {
          Output( "Failed to create dir %s", dirname.c_str() );
          return false;
        }
      }
#else
      WIN32_FIND_DATAA findData;
      // figure out current working directory
      HANDLE hDir = FindFirstFileA( dirname.c_str(), &findData );
      if ( hDir == INVALID_HANDLE_VALUE ) {
        Output( "Attempting to create dir %s", dirname.c_str() );
        CreateDirectoryA( dirname.c_str(), NULL );
        HANDLE hDir = FindFirstFileA( dirname.c_str(), &findData );
        if ( hDir == INVALID_HANDLE_VALUE ) {
          Output( "Failed to create dir %s", dirname.c_str() );
          return false;
        }
      }
#endif
      dirname += "/";
    }
    //Output( "Success in MakeDirectory for %s", dirname.c_str() );
    return true;
  }
  
  struct FileFetchEntry {
    FileFetchEntry() : notBefore( 0.0 ) {}
    FileFetchEntry( const string & file_name, double not_before ) : filename( file_name ), notBefore( not_before ) {}
    string filename;
    double notBefore;
  };
  
  deque<FileFetchEntry> fetchQueue;
  set<string> fetchSet;
  
  void PushFileFetch( const string & filename, double notBefore = 0.0 ) {
    ScopedMutex scm( filesystemMutex, R3_LOC );
    FileFetchEntry ffe( filename, notBefore );
    if( fetchSet.count( ffe.filename ) ) {
      Output( "PushFileFetch: Already have %s", ffe.filename.c_str() );
      return;
    }
    fetchSet.insert( filename );
    fetchQueue.push_back( ffe );
  }
  
  bool PopFileFetch( FileFetchEntry & ffe ) {
    ScopedMutex scm( filesystemMutex, R3_LOC );
    if( fetchQueue.size() == 0 ) {
      return false;
    }
    ffe = fetchQueue.front();
    fetchQueue.pop_front();
    fetchSet.erase( ffe.filename );
    return true;
  }
  
  
  File * CachedFileOpenForWrite( const string & inFileName, ManifestInfo & mi ) {
    string path = f_cachePath.GetVal();
    if( path.size() == 0 ) {
      return NULL;
    }
    string filename = NormalizePathSeparator( inFileName );
    string dir = path;
    if( filename.rfind('/') != string::npos ) {
      dir += filename.substr( 0, filename.rfind('/') );
      if ( MakeDirectory( dir.c_str() ) == false ) {
        return NULL;
      }
    }
    string fn = path + filename;
    FILE * fp = Fopen( fn.c_str(), "wb" );
    Output( "Opening file %s for write %s", fn.c_str(), fp ? "succeeded" : "failed" );
    if ( fp ) {
      StdCFile * F = new StdCFile( fn, true );
      F->fp = fp;
      GetManifestInfo( filename ) = mi;
      return F;
    }
    return NULL;
  }
  
  double lastCacheRefresh = 0;
  void RefreshCache() {
    map<string,ManifestInfo> m;
    {
      ScopedMutex scm( filesystemMutex, R3_LOC );
      m = manifest;
    }
    double t = GetTime();
    lastCacheRefresh = t;
    for( map<string,ManifestInfo>::iterator i = m.begin(); i != m.end(); ++i ) {
      PushFileFetch( i->first, t );
    }
  }
  
  struct NetCacheThread : public Thread {
    NetCacheThread() : Thread( "NetCache" ) {}
    
		string UrlToFilename( const string & url ) {
			string s = url.substr( url.rfind('/') ).substr( 1 );
			return s;
		}
		
		void Run() {
      int count = 0;
			while( ++count ) {
        filesystemCond.Wait();
        FileFetchEntry ffe;
        double t = GetTime();
        if( PopFileFetch( ffe ) == false ) {
          if( ( t - lastCacheRefresh ) > CACHE_REFRESH_INTERVAL ) {
            RefreshCache();
          }
          continue;
        }
        string file;
        {
          if( t < ffe.notBefore ) {
            PushFileFetch( ffe.filename, ffe.notBefore );
            continue;
          }
          file = ffe.filename;
          if( file.size() == 0 ) {
            continue;
          }
        }
        vector<Token> urls = TokenizeString( f_netPath.GetVal().c_str(), ";" );
        ManifestInfo mi = GetManifestInfo( file );
        if( mi.url == "local" ) {
          continue;
        }
        if( ( t - mi.lastTry ) < CACHE_REFRESH_INTERVAL ) {
          Output( "NetCache - skipping %s, last try only %.0lf minutes ago", file.c_str(), (t - mi.lastTry ) / 60 );
          continue;
        }
        Output( "NetCache looking for %s", file.c_str() );
        mi.lastTry = t;
        for( int i = 0; i < urls.size(); i++ ) {
          string url = urls[i].valString;
          Output( "NetCache trying %s -  %s", url.c_str(), file.c_str() );
          vector<uchar> data;
          map<string,string> header;
          if( url == mi.url && mi.etag.size() ) {
            header["etag"] = mi.etag;
          }
          if( UrlReadToMemory( url + '/' + file, data, header ) ) {
            mi.url = url;
            if( header.count( "Last-Modified" ) ) {
              mi.lastModified = header["Last-Modified"];
            }
            if( header.count( "etag" ) ) {
              mi.etag = header["etag"];
              if( mi.etag.size() && mi.etag[0] == '"' ) {
                mi.etag = mi.etag.substr( 1, mi.etag.size() - 2 );
              }
            }
            mi.lastTry = GetTime();
            File * f = CachedFileOpenForWrite( file, mi );
            f->Write( &data[0], 1, (int)data.size() );
            f_cacheUpdated.SetVal( true );
            delete f;
            break;
          }
        }
        GetManifestInfo( file ) = mi;
      }
    }
	};
	
	NetCacheThread netCacheThread;
  
  File * CachedFileOpenForPrivateRead( const string & inFileName ) {
    string path = f_cachePath.GetVal();
    if( path.size() == 0 ) {
      return NULL;
    }
    string filename = NormalizePathSeparator( inFileName );
    if( filename.size() && filename[0] == '/' ) {
      return NULL;
    }
    string dir = path;
    if( filename.rfind('/') != string::npos ) {
      dir += filename.substr( 0, filename.rfind('/') );
      if ( MakeDirectory( dir.c_str() ) == false ) {
        return NULL;
      }
    }
    string fn = path + filename;
    FILE * fp = Fopen( fn.c_str(), "rb" );
    Output( "Opening file %s for read %s", fn.c_str(), fp ? "succeeded" : "failed" );
    if ( fp ) {
      StdCFile * F = new StdCFile( fn, true );
      F->fp = fp;
      return F;
    }
    return NULL;
  }

  File * CachedFileOpenForRead( const string & inFileName ) {
    File *fp = CachedFileOpenForPrivateRead( inFileName );
    if( inFileName != "CacheManifest.json" ) {
      string filename = NormalizePathSeparator( inFileName );
      PushFileFetch( filename, GetTime() + CACHE_FETCH_DELAY );
    }
    return fp;
  }
  
}

namespace r3 {
  
  void InitFilesystem() {
// for debugging "missing file" resilience
//    if ( FindDirectory( f_basePath, "bass" ) == false ) {
    if ( FindDirectory( f_basePath, "base" ) == false ) {
      Output( "exiting due to invalid %s: %s", f_basePath.Name().Str().c_str(), f_basePath.GetVal().c_str() );
      exit( 1 );
    }
    if( f_cachePath.GetVal().size() == 0 ) {
      f_cachePath.SetVal( f_basePath.GetVal() + "/../cache/" );
    }
    if( MakeDirectory( f_cachePath.GetVal().c_str() ) == false ) {
      exit( 1 );
    }
    lastCacheRefresh = GetTime() + 120.0;
    ReadCacheManifest();
    netCacheThread.Start();
  }
  
  void ShutdownFilesystem() {
    WriteCacheManifest();
  }
  
  void TickFilesystem() {
    filesystemCond.Broadcast();
  }
  
  bool CacheUpdated() { return f_cacheUpdated.GetVal(); }
  
  File * FileOpenForWrite( const string & inFileName ) {
    ManifestInfo mi;
    mi.url = "local";
    File * f = CachedFileOpenForWrite( inFileName, mi );
    return f;
  }
  
  File * FileOpenForRead( const string & inFileName ) {
    string filename = NormalizePathSeparator( inFileName );
    {
      File *F = CachedFileOpenForRead( inFileName );
      if( F ) {
        return F;
      }
    }
    // then check in base
    {
      string fn = f_basePath.GetVal() + filename;
      FILE * fp = Fopen( fn.c_str(), "rb" );
      Output( "Opening file %s for read %s", fn.c_str(), fp ? "succeeded" : "failed" );
      if ( fp ) {
        StdCFile * F = new StdCFile( fn, false);
        F->fp = fp;
        return F;
      }
    }
#if ANDROID
    // try to materialize the file, then look for it again in the base path...
    if ( f_basePath.GetVal().size() > 0 ) {
      if ( appMaterializeFile( filename.c_str() ) ) {
        Output( "Materialized file %s from apk.", filename.c_str() );
      } else {
        Output( "Materialize failed." );
      }
      string fn = f_basePath.GetVal() + filename;
      FILE * fp = Fopen( fn.c_str(), "rb" );
      Output( "Opening file %s for read %s", fn.c_str(), fp ? "succeeded" : "failed" );
      if ( fp ) {
        StdCFile * F = new StdCFile(fn, false);
        F->fp = fp;
        F->unlinkOnDestruction = true;
        return F;
      }
    }
#endif
    // then give up
    return NULL;
  }
  
  
  void FileDelete( const string & fileName ) {
    string filename = NormalizePathSeparator( fileName );
    vector<string> paths;
    if ( f_cachePath.GetVal().size() > 0 ) {
      paths.push_back( f_cachePath.GetVal() );
    }
    paths.push_back( f_basePath.GetVal() );
    Output( "Trying to delete file %s", fileName.c_str() );
    vector<Token> tokens = TokenizeString( filename.c_str(), "/" );
    for ( int i = 0; i < (int)paths.size(); i++ ) {
      string & path = paths[i];
      if ( path.size() > 0 ) {
        string dir = path;
        for ( int j = 0; j < (int)tokens.size() - 1; j++ ) {
          dir += tokens[j].valString;
          dir += "/";
        }
        if ( MakeDirectory( dir.c_str() ) == false ) {
          continue;
        }
        string fn = path + filename;
        FILE * fp = Fopen( fn.c_str(), "wb" );
        bool found = fp != NULL;
        Fclose( fp );
        
        if( found ) {
          Output( "Deleting file %s", fn.c_str() );
          unlink( fn.c_str() );
          return;
        }
      }
    }
  }
  
  
  
  bool FileReadToMemory( const string & inFileName, vector< unsigned char > & data ) {
    string filename = NormalizePathSeparator( inFileName );
    File *f = FileOpenForRead( filename );
    if ( f == NULL ) {
      data.clear();
      return false;
    }
    int sz = f->Size();
    data.resize( sz );
    int bytesRead = f->Read( &data[0], 1, sz );
    assert( sz == bytesRead && sz == data.size() );
    delete f;
    return true;
  }
  
  
  string File::ReadLine() {
    string ret;
    char s[256];
    int l;
    do {
      l = Read( s, 1, sizeof( s ) );
      int end = l;
      for ( int i = 0; i < l; i++ ) {
        if ( s[i] == '\n' ) {
          // We take care not seek with a zero offset,
          // because it clears the eof flag.
          int o = i + 1 - l;
          if ( o != 0 ) {
            Seek( Seek_Curr, o );
          }
          l = 0;
          end = i;
          break;
        }
      }
      ret = ret + string( s, s + end );
    } while( l );
    return ret;
  }
  
  void File::WriteLine( const string & str ) {
    string s = str + "\n";
    Write( s.c_str(), 1, (int)s.size() );
  }
  
}

