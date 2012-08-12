#ifndef __UBASE64_H__
#define __UBASE64_H__

#include <vector>

#if UBASE64_IMPLEMENTATION
namespace {
    typedef unsigned char byte;    
    
    char idx2char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int  char2idx[] = {
        //   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19           
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //   0
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //  20
        -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, //  40
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, //  60
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, //  80
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, // 100
        49, 50, 51, -1, -1, -1, -1, -1                                                  // 120
    };
    
    struct U {        
        union {
            struct {
                byte b2;  
                byte b1;  
                byte b0;  
            };
            struct {
                unsigned int h3 : 6;
                unsigned int h2 : 6;
                unsigned int h1 : 6;
                unsigned int h0 : 6;
            };
        };
    };
    
    void EncodeChunk( char * enc, const byte *dec, int chars ) {
        U u;
        enc[2] = enc[3] = '=';
        switch( chars ) {
            case 3:
                u.b2 = dec[2];
            case 2:
                u.b1 = dec[1];
            case 1:
                u.b0 = dec[0];
            default:
                break;                    
        }
        switch ( chars ) {
            case 3:
                enc[3] = idx2char[ u.h3 ];
            case 2:
                enc[2] = idx2char[ u.h2 ];
            case 1:
                enc[1] = idx2char[ u.h1 ];
                enc[0] = idx2char[ u.h0 ];
            default:
                break;
        }
    }

    //  0123456.0123456.0123456.
    //  01234.01234.01234.01234.
    int DecodeChunk( byte * b, const byte *enc ) {
        char ce[4];
        char cd[3];
        U u;
        int count = 3;
        if( enc[3] == '=' ) {
            count--;
        }
        if( enc[2] == '=' ) {
            count--;
        }
        switch ( count ) {
            case 3:
                u.h3 = char2idx[ enc[3] ];
            case 2:
                u.h2 = char2idx[ enc[2] ];
            case 1:
                u.h1 = char2idx[ enc[1] ];
                u.h0 = char2idx[ enc[0] ];
            default:
                break;
        }
        switch( count ) {
            case 3:
                cd[2] = b[2] = u.b2;
            case 2:
                cd[1] = b[1] = u.b1;
            case 1:
                cd[0] = b[0] = u.b0;
            default:
                break;
        }
        memcpy( ce, enc, 4 );
        return count;
    }
    
    
    

}

namespace ubase64 {
    
    void Encode( std::vector<byte> &output, const std::vector<byte> &input ) {
        int s = (int)input.size();
        int c = 0;
        char enc[4];
        while( s > c ) {
            int chars = std::min( s - c, 3 );
            EncodeChunk( enc, & input[ c ], chars );
            output.push_back( enc[0] );
            output.push_back( enc[1] );
            output.push_back( enc[2] );
            output.push_back( enc[3] );            
            c += 3;
        }
    }
    
    void Decode( std::vector<byte> &output, const std::vector<byte> &input ) {
        for( int c = 0; c < (int) input.size(); c += 4 ) {
            byte dec[3];
            int count = DecodeChunk( dec, & input[ c ] );
            for( int i = 0; i < count; i++ ) {
                output.push_back( dec[i] );
            }
        }
    }
    
}
#endif

#endif // __R3_BASE64_H__

