//
//  ujson.h
//  ujson
//
//  Created by Cass Everitt on 8/11/12.
//  Copyright (c) 2012 n/a. All rights reserved.
//

#ifndef ujson_ujson_h
#define ujson_ujson_h

#include <string>
#include <map>
#include <vector>

namespace ujson {
    
    struct Json {
    public:
        typedef std::string S;
        typedef std::map< S, Json * > M;
        typedef std::vector< Json *  > V;
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
        
        Json( Type tv = Type_Invalid ) : t( tv ) {}
        Json( const std::string & sv ) : t( Type_String ), s( sv ) {} 
        Json( const char * sv ) : t( Type_String ), s( sv ) {} 
        Json( double nv ) : t( Type_Number ), n( nv ) {}
        Json( bool bv ) : t( Type_Bool ), b( bv ) {} 
        
        ~Json() {}
        Type GetType() const { return t; }
        
        // map methods
        Json * & LookupAdd( const std::string & key ) {
            return m[key];
        }
        Json * Lookup( const std::string & key ) {
            return m.find( key )->second;
        }
        const Json * Lookup( const std::string & key ) const {
            return m.find( key )->second;
        }
        //
        
        // vector methods
        int Size() const {
            return (int)v.size();
        }
        //

        const Type t;
        M m;
        V v;
        S s;
        double n;
        bool b;
    };
    
        
    void Encode( const Json *json, bool pretty, std::string & str );
    Json * Decode(const char * bytes, int nbytes = 0 );
    void Delete( Json *json );
    
}


#if UJSON_IMPLEMENTATION

#include <stdlib.h>
#include <sstream>

#include <stdio.h> // tmp debugging
using namespace ujson;

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
    
    std::string unescape_string( const std::string & s ) {
        std::string u;
        bool escaped = false;
        for( int i = 0; i < (int)s.size(); i++ ) {
            char c = s[ i ];
            if( escaped ) {
                if( c == 'n' || c == 'u' ) { // complete this list
                    u += '\\';
                }
                u += c;
                escaped = false;
                continue;
            }
            if( c == '\\' ) {
                escaped = true;
                continue;
            }
            u += c;
        }
        return u;
    }    
    
    struct parse_state {
        parse_state( const char * bv, int nb ) : bytes( bv ), nbytes( nb ), line( 0 ), success( true ) {
            if( nbytes == 0 ) {
                nbytes = strlen( bytes );
            }
            start_bytes = bytes;
            start_nbytes = nbytes;            
        }
        bool at_end() {
            return success == false || nbytes == 0 || bytes == NULL;
        }
        void advance( int i ) {
            // assert nbytes >= i
            nbytes -= i;
            bytes += i;
        }
        void newline() {
            line++;
        }
        void fail() {
            success = false;
        }
        const char * start_bytes;
        int start_nbytes;
        const char * bytes;
        int nbytes;
        int line;
        bool success;
    };
    
    void skip_whitespace( parse_state & p ) {
        while( p.at_end() == false ) {
            switch( p.bytes[0] ) {
                case '\n':
                    p.line++;
                case '\r':
                case ' ':
                case '\t':
                    p.advance( 1 );
                    break;
                default:
                    return;                    
            }
        }
    }

    Json * parse_json_node( parse_state & p );
    
    
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
    
    void parse_string( parse_state & p, std::string & str ) {
        if( p.bytes[0] != '"' ) {
            p.fail();
            return;
        }
        p.advance( 1 );
        while( p.at_end() == false ) {
            char c = p.bytes[0];
            p.advance( 1 );
            if( is_control( c ) ) {
                p.fail();
                return;
            }
            if( c == '\\' ) {
                c = p.bytes[0];
                p.advance( 1 );
                switch( c ) {
                    case 'b': str += '\b'; continue;
                    case 'f': str += '\f'; continue;
                    case 'n': str += '\n'; continue;
                    case 'r': str += '\r'; continue;
                    case 't': str += '\t'; continue;
                    case '"':
                    case '\\':
                    case '/':
                        str += c;
                        continue;
                        break;
                    case 'u':
                        if( p.nbytes < 4 ) {
                            p.fail();
                            return;
                        }
                        str += '\\';
                        str += 'u';
                        str += p.bytes[0];
                        str += p.bytes[1];
                        str += p.bytes[2];
                        str += p.bytes[3];
                        p.advance( 4 );
                        break;
                    default:
                        p.fail();
                        return;
                }
            }
            if( c == '"' ) {
                break;
            }
            int nb = utf8_bytes( c );
            if( nb == 0 || nb > p.nbytes ) {
                p.fail();
                return;
            }
            str += c;
            for( int i = 1; i < nb; i++ ) {
                char c = p.bytes[0];
                p.advance( 1 );
                unsigned char u = *(unsigned char *)&c;
                if( ( u & 0xc0 ) != 0x80 ) {
                    p.fail();
                    return;
                }
                str += c;
            }
        }
    }

    
    Json * parse_json_object( parse_state & p ) {
        p.advance( 1 );
        Json * j = new Json( Json::Type_Object );
        for(;;) {
            skip_whitespace( p );
            if( p.bytes[0] == '}' ) {
                break;
            }
            std::string member;
            parse_string( p, member );
            if( p.success == false ) {
                return j;
            }
            skip_whitespace( p );
            if( p.bytes[0] != ':' ) {
                p.fail();
                return j;
            }
            p.advance(1);
            skip_whitespace( p );
            Json *jj = parse_json_node( p );
            if( jj ) {
                j->m[ member ] = jj;
            }
            if( p.success == false ) {
                break;
            }
            skip_whitespace( p );
            if( p.bytes[0] == ',' ) {
                p.advance( 1 );
            } else if( p.bytes[0] == '}' ) {
                p.advance( 1 );
                break;
            } else {
                p.fail();
            }
        }
        return j;
    }
    
    Json * parse_json_array( parse_state & p ) {
        p.advance( 1 );
        Json * j = new Json( Json::Type_Array );
        for(;;) {
            skip_whitespace( p );
            if( p.bytes[0] == ']' ) {
                break;
            }
            Json *jj = parse_json_node( p );
            if( jj ) {
                j->v.push_back( jj );
            }
            if( p.success == false ) {
                break;
            }
            skip_whitespace( p );
            if( p.bytes[0] == ',' ) {
                p.advance( 1 );
            } else if( p.bytes[0] == ']' ) {
                p.advance( 1 );
                break;
            } else {
                p.fail();
            }
        }
        return j;
    }

    enum string_parse_state {
        SPS_normal,
        SPS_escape,
        SPS_unicode,
    };
    
    Json * parse_json_string( parse_state & p ) {
        Json * j = new Json("");
        parse_string( p, j->s );
        return j;
    }
    
    // number, bool, or null
    Json * parse_json_other( parse_state & p ) {
        if( p.nbytes >= 4 && strncmp( p.bytes, "null", 4 ) == 0 ) {
            p.advance( 4 );
            return new Json( Json::Type_Null );
        } else if( p.nbytes >= 4 && strncmp( p.bytes, "true", 4 ) == 0 ) {
            p.advance( 4 );
            return new Json( true );
        } else if( p.nbytes >= 5 && strncmp( p.bytes, "false", 5 ) == 0 ) {
            p.advance( 5 );
            return new Json( false );
        } else {
            char * end;
            double d = strtod( (const char *)p.bytes, &end );
            int len = end - p.bytes;
            if( len > 0 ) {
                p.advance( len );
                return new Json( d );
            }
        }
        return NULL;
    }

    Json * parse_json_node( parse_state & p ) {
        skip_whitespace( p );
        if( p.at_end() ) {
            p.fail();
            return new Json( Json::Type_Invalid );
        }
        // json node can be any of { object, array, string, number, bool }

        Json * j;
        switch( p.bytes[0] ) {
            case '{':
                j = parse_json_object( p );
                break;
            case '[':
                j = parse_json_array( p );
                break;
            case '"':
                j = parse_json_string( p );
                break;
            default:
                j = parse_json_other( p );
                break;
        }
        if( j == NULL ) {
            p.fail();
            return new Json( Json::Type_Invalid );
        }
        return j;
    }
        
}

namespace ujson {    

    void Encode( const Json *json, std::string s, std::stringstream & ss ) {
        if ( json == NULL ) {
            if( s.size() != 1 ) { ss << s; } 
            ss << "null";
            return;
        }
        switch( json->GetType() ) {
            case Json::Type_Object:
            {
                const std::map<std::string, Json *> & m = json->m;
                int sz = m.size();
                int i = 0;
                ss << "{" << std::endl;
                std::string s2 = s + "  ";
                for( std::map<std::string, Json *>::const_iterator it = m.begin(); it != m.end(); ++it ) {
                    //const std::string & key = it->first;
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
            case Json::Type_Array:
            {
                const std::vector<Json *> & v = json->v;
                int sz = v.size();
                
                bool can_inline = sz < 9;
                for( int i = 0; can_inline && i < sz; i++ ) {
                    bool multi = ( v[i] != NULL ) && ( v[i]->GetType() == Json::Type_Object || v[i]->GetType() == Json::Type_Array );
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
            case Json::Type_String:
            {
                const std::string & str = json->s;
                ss << s << '"' << escape_string( str ) << '"';
            }
                break;
            case Json::Type_Number:
            {
                ss << s << json->n;
            }
                break;
            case Json::Type_Bool:
            {
                ss << s << ( json->b ? "true" : "false" );
            }
                break;
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
    
    Json * Decode(const char * bytes, int nbytes) {
        //printf( "ujson::Decode() called.\n" );
        parse_state p( bytes, nbytes );
        return parse_json_node( p );
    }
    void Delete( Json *json ) {
        //printf( "ujson::Delete() called.\n" );
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
    
}

#endif

#endif
