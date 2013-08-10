/*
 Copyright (c) 2012-2013 Cass Everitt
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
 
 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//  Created by Cass Everitt on 8/11/12.


#ifndef R3_UJSON_H
#define R3_UJSON_H

#include <string>
#include <map>
#include <vector>

namespace r3 {
  namespace ujson {
    
    enum Type {
      Type_Invalid,
      Type_Object,
      Type_Array,
      Type_String,
      Type_Number,
      Type_Bool,
      Type_Null,
      Type_MAX
    };
    
    struct Json {
    public:
      typedef std::string S;
      typedef std::map< S, Json * > M;
      typedef std::vector< Json *  > V;
      
      Json( Type tv = Type_Null ) : t( tv ) {}
      Json( const std::string & sv ) : t( Type_String ), s( sv ) {}
      Json( const char * sv ) : t( Type_String ), s( sv ) {}
      Json( double nv ) : t( Type_Number ), n( nv ) {}
      Json( bool bv ) : t( Type_Bool ), b( bv ) {}
      
      ~Json() {}
      
      Json & operator= ( const Json & rhs );
      
      Type GetType() const { return t; }
      
      size_t Size() const {
        switch( t ) {
          case Type_Object:  return m.size();
          case Type_Array:   return v.size();
          case Type_String:
          case Type_Number:
          case Type_Bool:
          case Type_Null:    return 1;
          default:
            break;
        }
        return 0;
      }
      
      void GetMemberNames( std::vector<std::string> & name ) {
        name.resize( m.size() );
        int i = 0;
        for( M::iterator it = m.begin(); it != m.end(); ++it ) {
          name[ i++ ] = it->first;
        }
      }
      
      void Add( const std::string & name, Json * val );
      
      // square brackets construct members if necessary
      Json & operator[] ( const std::string & name );
      Json & operator[] ( size_t index );
      
      // parens never construct members, they are for inspection only
      Json & operator() ( const std::string & name );
      Json & operator() ( size_t index );
      
      const Json & operator() ( const std::string & name ) const;
      const Json & operator() ( size_t index ) const;
      
      Type t;
      M m;
      V v;
      S s;
      double n;
      bool b;
    };
    
    void Encode( const Json *json, bool pretty, std::string & str );
    void Delete( Json *json );

    struct ParseHandler {
      virtual void BeginObject( const std::string & key ) = 0;
      virtual void BeginArray( const std::string & key ) = 0;
      virtual void End() = 0;
      virtual void String( const std::string & key, const std::string & val ) = 0;
      virtual void Number( const std::string & key, double n ) = 0;
      virtual void Boolean( const std::string & key, bool b ) = 0;
      virtual void Null( const std::string & key ) = 0;
    };

    struct ParseTreeConstructor : public ParseHandler {
      ParseTreeConstructor() {}
      
      void Add( const std::string & key, Json * j ) {
        if( nodeStack.size() == 0 ) {
          nodeStack.push_back( j );
          return;
        }
        Json & n = *nodeStack.back();
        n.Add( key, j );
        if( j->GetType() == Type_Object || j->GetType() == Type_Array ) {
          nodeStack.push_back( j );
        }
      }
      
      virtual void BeginObject( const std::string & key ) {
        Add( key, new Json( Type_Object ) );
      }
      virtual void BeginArray( const std::string & key ) {
        Add( key, new Json( Type_Array ) );
      }
      virtual void End() {
        if( nodeStack.size() > 1 ) {
          nodeStack.pop_back();
        }
      }
      virtual void String( const std::string & key, const std::string & val ) {
        Add( key, new Json( val ) );
      }
      virtual void Number( const std::string & key,  double n ) {
        Add( key, new Json( n ) );
      }
      virtual void Boolean( const std::string & key,  bool b ) {
        Add( key, new Json( b ) );
      }
      virtual void Null( const std::string & key ) {
        Add( key, new Json( Type_Null ) );
      }
      
      std::vector<Json *> nodeStack;
    };
    
    struct Input {
      Input() : offset( 0 ), end( false ) {}
      Input( const char * b, int nb ) : offset( 0 ), end( false )
      {
        Append( b, nb );
      }
      void Append( const char * b, int nb ) {
        if( end ) {
          return; // no more data
        }
        if( nb == 0 && b ) {
          const char * b2 = b;
          while( *b2++ ) {
            nb++;
          }
        }
        if( b == NULL ) {
          nb = 0;
        }
        bytes.insert( bytes.end(), b, b + nb );
      }
      void End() { end = true; bytes.push_back( ' ' ); }
      size_t Size() const { return bytes.size() - offset; }
      void Advance( size_t adv ) {
        if( Size() >= adv ) {
          offset += adv;
        }
        if( offset > 128 ) {
          bytes.erase( bytes.begin() + 0, bytes.begin() + offset );
          offset = 0;
        }
      }
      const char *Ptr() const { return &bytes[offset]; }
      char operator[](int idx) const { return bytes[offset+idx]; }
      
      std::vector<char> bytes;
      int offset;
      bool end;
    };

    enum ParseState {
      ParseState_Invalid = 0,
      ParseState_Whitespace,
      ParseState_Colon,
      ParseState_String,
      ParseState_Node,
      ParseState_Object,
      ParseState_ObjectMemberRequired,
      ParseState_ObjectMemberSepRequired,
      ParseState_ObjectMember,
      ParseState_Array,
      ParseState_ArrayElementRequired,
      ParseState_ArrayElementSepRequired,
      ParseState_PrimitiveString,
      ParseState_PrimitiveOther
    };
    
    enum StringParseState {
      StringParseState_Normal = 0,
      StringParseState_AdditionalBytes1,
      StringParseState_AdditionalBytes2,
      StringParseState_AdditionalBytes3,
      StringParseState_AdditionalBytes4,
      StringParseState_AdditionalBytes5,
      StringParseState_AdditionalHexBytes1,
      StringParseState_AdditionalHexBytes2,
      StringParseState_AdditionalHexBytes3,
      StringParseState_AdditionalHexBytes4,
      StringParseState_Escaped
    };
    
    struct ParseContext {
      
      ParseContext( ParseHandler & h )
      : handler( h )
      , stringState( StringParseState_Normal )
      , inputNeeded( 1 )
      , line( 0 )
      , success( true ) {
        stateStack.push_back( ParseState_Node );
        stateStack.push_back( ParseState_Whitespace );
      }

      ParseContext( ParseHandler & h, const char * bv, int nb )
      : handler( h )
      , stringState( StringParseState_Normal )
      , input( bv, nb )
      , inputNeeded( 1 )
      , line( 0 )
      , success( true ) {
        stateStack.push_back( ParseState_Node );
        stateStack.push_back( ParseState_Whitespace );
      }

      bool Failed() const {
        return success == false;
      }
      
      void Append( const char * bv, int nb ) {
        input.Append( bv,  nb );
      }
      
      bool HasInput() {
        return success && input.Size() >= inputNeeded;
      }
      
      void __push( ParseState s ) {
        stateStack.push_back( s );
      }
      void __pop() {
        if( stateStack.size() < 1 ) {
          ; //error
        } else {
          stateStack.pop_back();
        }
      }
      size_t __depth() const {
        return stateStack.size();
      }
      ParseState __state( int depth = 0 ) const {
        if( ( (int)stateStack.size() - 1 ) < depth ) {
          return ParseState_Invalid;
        }
        return stateStack[ stateStack.size() - 1 - depth ];
      }
      
      void __newline() {
        line++;
      }
      
      void __fail() {
        success = false;
      }
      
      ParseHandler & handler;
      std::vector<ParseState> stateStack;
      std::vector<int> countStack;
      std::vector<std::string> stringStack;
      StringParseState stringState;
      Input input;
      size_t inputNeeded;
      int line;
      bool success;
    };

    void Decode( ParseContext & ps );
    inline Json * Decode( const char * bytes, int nbytes = 0, bool * success = NULL, int * line = NULL ) {
      ParseTreeConstructor h;
      
      ParseContext ps( h, bytes, nbytes );
      ps.input.End();
      
      if( ps.HasInput() == false ) {
        return NULL;
      }

      Decode( ps );
              
      if( success != NULL ) {
        *success = ps.success;
      }
      if( line != NULL ) {
        *line = ps.line;
      }
      return h.nodeStack.size() > 0 ? h.nodeStack[0] : NULL;
    }
    
  } // namespace ujson
} // namespace r3


#if UJSON_IMPLEMENTATION

#include <stdlib.h>
#include <sstream>

using namespace r3::ujson;

namespace {
  
  std::string escape_string( const std::string & s ) {
    std::string e;
    for( int i = 0; i < (int)s.size(); i++ ) {
      char c = s[ i ];
      switch( c ) {
        case '\b':  e += '\\'; e += 'b'; break;
        case '\f':  e += '\\'; e += 'f'; break;
        case '\n':  e += '\\'; e += 'n'; break;
        case '\r':  e += '\\'; e += 'r'; break;
        case '\t':  e += '\\'; e += 't'; break;
        case '/':
        case '"':
        case '\\':
          e += '\\';
          e += c;
          break;
        default:
          e += c;
      }
    }
    return e;
  }
  
  bool is_whitespace( char c ) {
    switch( c ) {
      case '\n':
      case '\r':
      case ' ':
      case '\t': return true;
      default: break;
    }
    return false;
  }
  
  bool is_control( char c ) {
    return c >= 0 && c < 32;
  }
  
  int utf8_bytes( char c ) {
    unsigned char u = *(unsigned char *)&c;
    if( ( u & 0x80 ) == 0 ) {
      return 1;
    } else if ( ( u & 0xe0 ) == 0xc0 ) {
      return 2;
    } else if ( ( u & 0xf0 ) == 0xe0 ) {
      return 3;
    } else if ( ( u & 0xf8 ) == 0xf0 ) {
      return 4;
    } else if ( ( u & 0xfc ) == 0xf8 ) {
      return 5;
    } else if ( ( u & 0xfe ) == 0xfc ) {
      return 6;
    }
    return 0;
  }
  
  
  void parse_whitespace( ParseContext & p ) {
    while( p.HasInput() ) {
      if( is_whitespace( p.input[0] ) ) {
        if( p.input[0] == '\n' ) {
          p.__newline();
        }
        p.input.Advance(1);
      } else {
        p.__pop();
        return;
      }
    }    
  }

  void parse_string( ParseContext & p ) {
    std::string & str = p.stringStack.back();
    while( p.HasInput() ) {
      switch( p.stringState ) {
        case StringParseState_Normal:
        {
          char c = p.input[0];
          p.input.Advance(1);
          int nb = utf8_bytes( c );
          if( c == '\\' ) {
            p.stringState = StringParseState_Escaped;
            break;
          } else if ( c == '"') {
            //fprintf( stderr, "Parsed string: %s\n", p.stringStack.back().c_str() );
            p.__pop();
            return;
          } else if( nb < 2 && is_control( c ) ) {
            p.__fail();
            return;
          }
          str += c;
          if( nb > 1 ) {
            p.stringState = (StringParseState)((int)StringParseState_AdditionalBytes1 + (nb - 1));
          }
        }
          break;
        case StringParseState_AdditionalBytes1:
        case StringParseState_AdditionalBytes2:
        case StringParseState_AdditionalBytes3:
        case StringParseState_AdditionalBytes4:
        case StringParseState_AdditionalBytes5:
        {
          char c = p.input[0];
          unsigned char u = *(unsigned char *)&c;
          if( ( u & 0xc0 ) != 0x80 ) {
            p.__fail();
            return;
          }
          str += c;
          p.input.Advance(1);
          p.stringState = (StringParseState)((int)p.stringState - 1);
        }
          break;
        case StringParseState_AdditionalHexBytes1:
        case StringParseState_AdditionalHexBytes2:
        case StringParseState_AdditionalHexBytes3:
        case StringParseState_AdditionalHexBytes4:
        {
          char c = p.input[0];
          if( ( c < '0' && '9' < c ) && ( c < 'a' && 'f' < c ) ) {
            p.__fail();
            return;
          }
          str += c;
          p.input.Advance(1);
          if( p.stringState == StringParseState_AdditionalHexBytes1 ) {
            p.stringState = StringParseState_Normal;
          } else {
            p.stringState = (StringParseState)((int)p.stringState - 1);
          }
        }
          break;
        case StringParseState_Escaped:
        {
          char c = p.input[0];
          p.input.Advance( 1 );
          p.stringState = StringParseState_Normal;
          switch( c ) {
            case 'b': str += '\b'; break;
            case 'f': str += '\f'; break;
            case 'n': str += '\n'; break;
            case 'r': str += '\r'; break;
            case 't': str += '\t'; break;
            case '"':
            case '\\':
            case '/': str += c; break;
            case 'u':
              str += '\\';
              str += 'u';
              p.stringState = StringParseState_AdditionalHexBytes4;
              break;
            default:
              p.__fail();
              return;
          }
        }
          break;
        default:
          p.__fail();
          return;
      }
    }
  }

  void parse_node( ParseContext & p ) {
    char c = p.input[0];
    p.__pop();
    switch( c ) {
      case '{':
        p.__push( ParseState_Object );
        p.__push( ParseState_Whitespace );
        p.countStack.push_back( 0 );
        p.input.Advance(1);
        p.handler.BeginObject( p.stringStack.size() > 0 ? p.stringStack.back() : "" );
        break;
      case '[':
        p.__push( ParseState_Array );
        p.__push( ParseState_Whitespace );
        p.countStack.push_back( 0 );
        p.input.Advance(1);
        p.handler.BeginArray( p.stringStack.size() > 0 ? p.stringStack.back() : "" );
        break;
      case '"':
        p.__push( ParseState_PrimitiveString );
        p.__push( ParseState_String );
        p.stringStack.push_back( "" );
        p.input.Advance(1);
        break;
      default:
        p.__push( ParseState_PrimitiveOther );
        break;
    }
  }
  
  void parse_object( ParseContext & p ) {
    if( p.HasInput() == false ) {
      return;
    }
    char c = p.input[0];
    switch( c ) {
      case '"':
        switch( p.__state() ) {
          case ParseState_ObjectMemberSepRequired: p.__fail(); return;
          default: break;
        }
        p.stringStack.push_back( "" );
        p.countStack.back()++;
        p.input.Advance(1);
        p.__pop();
        p.__push( ParseState_ObjectMemberSepRequired );
        p.__push( ParseState_Whitespace );
        p.__push( ParseState_ObjectMember );
        p.__push( ParseState_Node );
        p.__push( ParseState_Whitespace );
        p.__push( ParseState_Colon );
        p.__push( ParseState_Whitespace );
        p.__push( ParseState_String );
        break;
      case ',':
        switch( p.__state() ) {
          case ParseState_Object:
          case ParseState_ObjectMemberRequired: p.__fail(); return;
          default: break;
        }
        if( p.countStack.back() == 0 ) {
          p.__fail();
          return;
        }
        p.input.Advance(1);
        p.__pop();
        p.__push( ParseState_ObjectMemberRequired );
        p.__push( ParseState_Whitespace );
        break;
        
      case '}':
        switch( p.__state() ) {
          case ParseState_ObjectMemberRequired: p.__fail(); return;
          default: break;
        }
        p.input.Advance(1);
        p.countStack.pop_back();
        p.__pop(); // object
        p.handler.End();
        break;
        
      default:
        p.__fail();
        return;
    }
  }
  
  void parse_array( ParseContext & p ) {
    if( p.HasInput() == false ) {
      return;
    }
    switch( p.input[0] ) {
      case ',':
        switch( p.__state() ) {
          case ParseState_Array:
          case ParseState_ArrayElementRequired: p.__fail(); return;
          default: break;
        }
        if( p.countStack.back() == 0 ) {
          p.__fail();
          return;
        }
        p.input.Advance(1);
        p.__pop();
        p.__push( ParseState_ArrayElementRequired );
        p.__push( ParseState_Whitespace );
        break;
        
      case ']':
        switch( p.__state() ) {
          case ParseState_ArrayElementRequired: p.__fail(); return;
          default: break;
        }
        p.countStack.pop_back();
        p.input.Advance(1);
        p.__pop(); // array
        p.handler.End();
        break;
        
      default:
        switch( p.__state() ) {
          case ParseState_ArrayElementSepRequired: p.__fail(); return;
          default: break;
        }
        p.countStack.back()++;
        p.__pop();
        p.__push( ParseState_ArrayElementSepRequired );
        p.__push( ParseState_Whitespace );
        p.__push( ParseState_Node );
        break;
    }
  }

  void parse_primitive_string( ParseContext & p ) {
    if( p.stringStack.size() == 0 ) {
      p.__fail();
      return;
    }
    std::string str = p.stringStack.back();
    p.stringStack.pop_back();
    std::string member;
    if( p.stringStack.size() > 0 ) {
      member = p.stringStack.back();
    }
    p.handler.String( member, str );
    p.__pop(); // string primitive
  }

  void parse_primitive_other( ParseContext & p ) {
    bool foundDelimiter = false;
    size_t i = 0;
    while( foundDelimiter == false && i < p.input.Size() ) {
      char c = p.input[i];
      foundDelimiter = is_whitespace( c ) || c == ',' || c == ']' || c == '}';
      i++;
    }
    if( foundDelimiter == false ) {
      p.inputNeeded = p.input.Size() + 1;
      return;
    } else {
      p.inputNeeded = 1;
    }
    
    std::string member;
    if( p.stringStack.size() > 0 ) {
      member = p.stringStack.back();
    }
    if( p.input.Size() >= 4 && strncmp( p.input.Ptr(), "null", 4 ) == 0 ) {
      p.input.Advance( 4 );
      p.handler.Null( member );
      p.__pop();
    } else if( p.input.Size() >= 4 && strncmp( p.input.Ptr(), "true", 4 ) == 0 ) {
      p.input.Advance( 4 );
      p.handler.Boolean( member, true );
      p.__pop();
    } else if( p.input.Size() >= 5 && strncmp( p.input.Ptr(), "false", 5 ) == 0 ) {
      p.input.Advance( 5 );
      p.handler.Boolean( member, false );
      p.__pop();
    } else {
      char * end;
      double d = strtod( p.input.Ptr(), &end );
      size_t len = end - p.input.Ptr();
      if( len > 0 ) {
        p.inputNeeded = 1;
        p.input.Advance( len );
        p.handler.Number( member, d );
        p.__pop();
      } else {
        p.__fail();
      }
    }
  }
  
  void parse( ParseContext & p ) {
    while( p.HasInput() && p.__depth() > 0) {
      switch( p.__state() ) {
        case ParseState_Whitespace:
          parse_whitespace( p );
          break;
        case ParseState_Colon:
          if( p.input[0] == ':' ) {
            p.input.Advance(1);
            p.__pop();
          } else {
            p.__fail();
          }
          break;
        case ParseState_String:
          parse_string( p );
          break;
        case ParseState_Node:
          parse_node( p );
          break;
        case ParseState_Object:
        case ParseState_ObjectMemberRequired:
        case ParseState_ObjectMemberSepRequired:
          parse_object( p );
          break;
        case ParseState_ObjectMember:
          p.stringStack.pop_back();
          p.__pop();
          break;
        case ParseState_Array:
        case ParseState_ArrayElementRequired:
        case ParseState_ArrayElementSepRequired:
          parse_array( p );
          break;
        case ParseState_PrimitiveString:
          parse_primitive_string( p );
          break;
        case ParseState_PrimitiveOther:
          parse_primitive_other( p );
          break;
        default:
          p.__fail();
          break;
      }
    }
  }
  
  
  Json invalid_node( Type_Invalid );
}

namespace r3 {
  namespace ujson {
    
    Json & Json::operator= ( const Json & rhs ) {
      if( this != &invalid_node ) {
        t = rhs.t;
        m = rhs.m;
        v = rhs.v;
        s = rhs.s;
        n = rhs.n;
        b = rhs.b;
      }
      return *this;
    }
    
    void Json::Add( const std::string & name, Json * val ) {
      if( t == Type_Object && m.count( name ) == 0 ) {
        m[name] = val;
      } else if( t == Type_Array ) {
        v.push_back( val );
      }
    }
    
    // square brackets construct members if necessary
    Json & Json::operator[] ( const std::string & name ) {
      if( t == Type_Object ) {
        Json * & j = m[name];
        if( j == NULL ) {
          j = new Json();
        }
        return *j;
      }
      return invalid_node;
    }
    
    Json & Json::operator[] ( size_t index ) {
      if( t == Type_Array ) {
        if( index >= v.size() ) {
          size_t old_size = v.size();
          v.resize( index + 1 );
          for( size_t i = old_size; i <= index; i++ ) {
            v[ i ] = new Json();
          }
        }
        return *v[ index ];
      }
      return invalid_node;
    }
    
    // parens never construct members, they are for inspection only
    Json & Json::operator() ( const std::string & name ) {
      if( t == Type_Object ) {
        M::iterator it = m.find( name );
        if( it != m.end() ) {
          return *it->second;
        }
      }
      return invalid_node;
    }
    
    Json & Json::operator() ( size_t index ) {
      if( t == Type_Array ) {
        if( index < v.size() ) {
          return *v[ index ];
        }
      }
      return invalid_node;
    }
    
    const Json & Json::operator() ( const std::string & name ) const {
      if( t == Type_Object ) {
        M::const_iterator it = m.find( name );
        if( it != m.end() ) {
          return *it->second;
        }
      }
      return invalid_node;
    }
    
    const Json & Json::operator() ( size_t index ) const {
      if( t == Type_Array ) {
        if( index < v.size() ) {
          return *v[ index ];
        }
      }
      return invalid_node;
    }
    
    void Encode( const Json *json, std::string s, std::stringstream & ss ) {
      if ( json == NULL ) {
        if( s.size() != 1 ) { ss << s; }
        ss << "null";
        return;
      }
      switch( json->GetType() ) {
        case Type_Object:
        {
          const std::map<std::string, Json *> & m = json->m;
          size_t sz = m.size();
          int i = 0;
          ss << "{" << std::endl;
          std::string s2 = s + "  ";
          for( std::map<std::string, Json *>::const_iterator it = m.begin(); it != m.end(); ++it ) {
            ss << s2 << '"' << escape_string( it->first ) << '"' << " : ";
            Encode( it->second, s2, ss );
            i++;
            if ( i != sz ) {
              ss << ",";
            }
            ss << std::endl;
          }
          ss << s << "}";
        }
          break;
        case Type_Array:
        {
          const std::vector<Json *> & v = json->v;
          size_t sz = v.size();
          
          bool can_inline = sz < 9;
          for( int i = 0; can_inline && i < sz; i++ ) {
            bool multi = ( v[i] != NULL ) && ( v[i]->GetType() == Type_Object || v[i]->GetType() == Type_Array );
            can_inline = ! multi;
          }
          
          if( can_inline ) {
            ss << "[ ";
            for( int i = 0; i < sz; i++ ) {
              Encode( v[ i ], "", ss );
              if( i < ( sz - 1 ) ) {
                ss << ", ";
              }
            }
            ss << " ]";
          } else {
            ss << "[" << std::endl;
            std::string s2 = s + "  ";
            for( int i = 0; i < sz; i++ ) {
              ss << s2;
              Encode( v[ i ], s2, ss );
              if( i < ( sz - 1 ) ) {
                ss << ",";
              }
              ss << std::endl;
            }
            ss << s << "]";
          }
        }
          break;
        case Type_String:
        {
          const std::string & str = json->s;
          ss << s << '"' << escape_string( str ) << '"';
        }
          break;
        case Type_Number:
        {
          ss << s << json->n;
        }
          break;
        case Type_Bool:
        {
          ss << s << ( json->b ? "true" : "false" );
        }
          break;
        case Type_Null:
        {
          ss << s << "null";
        }
        default:
          break;
      }
    }
    
    void Encode( const Json *json, bool pretty, std::string & str ) {
      std::stringstream ss ( std::stringstream::out );
      ss.precision( 17 ); // lossless double
      Encode( json, pretty ? "" : "-", ss );
      str = ss.str();
    }
    
    void Decode( ParseContext & ps) {
      parse( ps );
    }
    
    void Delete( Json *json ) {
      if( json ) {
        for( Json::M::iterator i = json->m.begin(); i != json->m.end(); ++i ) {
          Delete( i->second );
        }
        for( size_t i = 0; i < json->v.size(); i++ ) {
          Delete( json->v[i] );
        }
      }
      delete json;
    }
    
  } // namespace ujson
} // namespace r3

#endif

#endif
