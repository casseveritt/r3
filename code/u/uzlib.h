/*
 *  uzlib - tiny zlib
 *
 */

/* 
 Copyright (c) 2011 Cass Everitt
 All rights reserved.
 
 Initially ported from Sean Barrett's public domain image
 encode/decode software at nothings.org.

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

#ifndef __R3_UZLIB_H__
#define __R3_UZLIB_H__

#include <vector>

#if ! R3_UZLIB_IMPLEMENTATION 

namespace r3 {

    void Deflate( std::vector< char > & ov, char const * ibuffer, int ilen, int quality );
    void Inflate( std::vector< char > & ov, char const * ibuffer, int ilen );

}

#else 

#include <assert.h>
#include <string.h>

namespace {

    typedef unsigned char uint8;
    typedef unsigned short uint16;
    typedef   signed short  int16;
    typedef unsigned int   uint32;
    typedef   signed int    int32;
# if ! ANDROID
    typedef unsigned int   uint;
# endif
    
    // public domain zlib decode    v0.2  Sean Barrett 2006-11-18
    //    simple implementation
    //      - all input must be provided in an upfront buffer
    //      - all output is written to a single output buffer (can malloc/realloc)
    //    performance
    //      - fast huffman
    
    // fast-way is faster to check than jpeg huffman, but slow way is slower
#define ZFAST_BITS  9 // accelerate all cases in default tables
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)
    
    // zlib-style huffman encoding
    // (jpegs packs from left, zlib from right, so can't share code)
    struct zhuffman {
        uint16 fast[1 << ZFAST_BITS];
        uint16 firstcode[16];
        int maxcode[17];
        uint16 firstsymbol[16];
        uint8  size[288];
        uint16 value[288]; 
    };
    
    int bitreverse16( int n ) {
        n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
        n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
        n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
        n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
        return n;
    }
    
    int bit_reverse( int v, int bits ) {
        assert(bits <= 16);
        // to bit reverse n bits, reverse 16 and shift
        // e.g. 11 bits, bit reverse and shift away 5
        return bitreverse16( v ) >> ( 16 - bits );
    }
    
    int zbuild_huffman( zhuffman *z, uint8 *sizelist, int num ) {
        int i,k=0;
        int code, next_code[16], sizes[17];
        
        // DEFLATE spec for generating codes
        memset(sizes, 0, sizeof(sizes));
        memset(z->fast, 255, sizeof(z->fast));
        for (i=0; i < num; ++i) 
            ++sizes[sizelist[i]];
        sizes[0] = 0;
        for (i=1; i < 16; ++i)
            assert(sizes[i] <= (1 << i));
        code = 0;
        for (i=1; i < 16; ++i) {
            next_code[i] = code;
            z->firstcode[i] = (uint16) code;
            z->firstsymbol[i] = (uint16) k;
            code = (code + sizes[i]);
            if (sizes[i])
                if (code-1 >= (1 << i)) return 0; // e("bad codelengths","Corrupt JPEG");
            z->maxcode[i] = code << (16-i); // preshift for inner loop
            code <<= 1;
            k += sizes[i];
        }
        z->maxcode[16] = 0x10000; // sentinel
        for (i=0; i < num; ++i) {
            int s = sizelist[i];
            if (s) {
                int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
                z->size[c] = (uint8)s;
                z->value[c] = (uint16)i;
                if (s <= ZFAST_BITS) {
                    int k = bit_reverse(next_code[s],s);
                    while (k < (1 << ZFAST_BITS)) {
                        z->fast[k] = (uint16) c;
                        k += (1 << s);
                    }
                }
                ++next_code[s];
            }
        }
        return 1;
    }
    
    
	// zlib-from-memory implementation for PNG reading
    //    because PNG allows splitting the zlib stream arbitrarily,
    //    and it's annoying structurally to have PNG call ZLIB call PNG,
    //    we require PNG read all the IDATs and combine them into a single
    //    memory buffer
    
    struct zbuf {
        uint8 *zbuffer, *zbuffer_end;
        int num_bits;
        uint32 code_buffer;
        
        char *zout;
        char *zout_start;
        char *zout_end;
        int   z_expandable;
        
        zhuffman z_length, z_distance;
    };
    
    int zget8( zbuf *z ) {
        if (z->zbuffer >= z->zbuffer_end) return 0;
        return *z->zbuffer++;
    }
    
    void fill_bits( zbuf *z ) {
        do {
            assert(z->code_buffer < (1U << z->num_bits));
            z->code_buffer |= zget8(z) << z->num_bits;
            z->num_bits += 8;
        } while (z->num_bits <= 24);
    }
    
    unsigned int zreceive( zbuf *z, int n ) {
        unsigned int k;
        if (z->num_bits < n) fill_bits(z);
        k = z->code_buffer & ((1 << n) - 1);
        z->code_buffer >>= n;
        z->num_bits -= n;
        return k;   
    }
    
    int zhuffman_decode( zbuf *a, zhuffman *z ) {
        int b,s,k;
        if (a->num_bits < 16) fill_bits(a);
        b = z->fast[a->code_buffer & ZFAST_MASK];
        if (b < 0xffff) {
            s = z->size[b];
            a->code_buffer >>= s;
            a->num_bits -= s;
            return z->value[b];
        }
        
        // not resolved by fast table, so compute it the slow way
        // use jpeg approach, which requires MSbits at top
        k = bit_reverse(a->code_buffer, 16);
        for (s=ZFAST_BITS+1; ; ++s)
            if (k < z->maxcode[s])
                break;
        if (s == 16) return -1; // invalid code!
        // code size is s, so:
        b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
        assert(z->size[b] == s);
        a->code_buffer >>= s;
        a->num_bits -= s;
        return z->value[b];
    }
    
    int expand( zbuf *z, int n )  { // need to make room for n bytes
        char *q;
        int cur, limit;
        if (!z->z_expandable) return 0; // e("output buffer limit","Corrupt PNG");
        cur   = (int) (z->zout     - z->zout_start);
        limit = (int) (z->zout_end - z->zout_start);
        while (cur + n > limit)
            limit *= 2;
        q = (char *) realloc(z->zout_start, limit);
        if (q == NULL) return 0; // e("outofmem", "Out of memory");
        z->zout_start = q;
        z->zout       = q + cur;
        z->zout_end   = q + limit;
        return 1;
    }
    
    
    int length_base[31] = {
        3,4,5,6,7,8,9,10,11,13,
        15,17,19,23,27,31,35,43,51,59,
        67,83,99,115,131,163,195,227,258,0,0 };
    
    int length_extra[31]= 
    { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };
    
    int dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
        257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};
    
    int dist_extra[32] =
    { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    
    int parse_huffman_block( zbuf *a ) {
        for(;;) {
            int z = zhuffman_decode(a, &a->z_length);
            if (z < 256) {
                if (z < 0) return 0; // e("bad huffman code","Corrupt PNG"); // error in huffman codes
                if (a->zout >= a->zout_end) if (!expand(a, 1)) return 0;
                *a->zout++ = (char) z;
            } else {
                uint8 *p;
                int len,dist;
                if (z == 256) return 1;
                z -= 257;
                len = length_base[z];
                if (length_extra[z]) len += zreceive(a, length_extra[z]);
                z = zhuffman_decode(a, &a->z_distance);
                if (z < 0) return 0; // e("bad huffman code","Corrupt PNG");
                dist = dist_base[z];
                if (dist_extra[z]) dist += zreceive(a, dist_extra[z]);
                if (a->zout - a->zout_start < dist) return 0; // e("bad dist","Corrupt PNG");
                if (a->zout + len > a->zout_end) if (!expand(a, len)) return 0;
                p = (uint8 *) (a->zout - dist);
                while (len--)
                    *a->zout++ = *p++;
            }
        }
    }
    
    int compute_huffman_codes( zbuf *a ) {
        static uint8 length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
        zhuffman z_codelength;
        uint8 lencodes[286+32+137];//padding for maximum single op
        uint8 codelength_sizes[19];
        int i,n;
        
        int hlit  = zreceive(a,5) + 257;
        int hdist = zreceive(a,5) + 1;
        int hclen = zreceive(a,4) + 4;
        
        memset(codelength_sizes, 0, sizeof(codelength_sizes));
        for (i=0; i < hclen; ++i) {
            int s = zreceive(a,3);
            codelength_sizes[length_dezigzag[i]] = (uint8) s;
        }
        if (!zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;
        
        n = 0;
        while (n < hlit + hdist) {
            int c = zhuffman_decode(a, &z_codelength);
            assert(c >= 0 && c < 19);
            if (c < 16)
                lencodes[n++] = (uint8) c;
            else if (c == 16) {
                c = zreceive(a,2)+3;
                memset(lencodes+n, lencodes[n-1], c);
                n += c;
            } else if (c == 17) {
                c = zreceive(a,3)+3;
                memset(lencodes+n, 0, c);
                n += c;
            } else {
                assert(c == 18);
                c = zreceive(a,7)+11;
                memset(lencodes+n, 0, c);
                n += c;
            }
        }
        if (n != hlit+hdist) return 0; // e("bad codelengths","Corrupt PNG");
        if (!zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
        if (!zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
        return 1;
    }
    
    int parse_uncompressed_block( zbuf *a ) {
        uint8 header[4];
        int len,nlen,k;
        if (a->num_bits & 7)
            zreceive(a, a->num_bits & 7); // discard
        // drain the bit-packed data into header
        k = 0;
        while (a->num_bits > 0) {
            header[k++] = (uint8) (a->code_buffer & 255); // wtf this warns?
            a->code_buffer >>= 8;
            a->num_bits -= 8;
        }
        assert(a->num_bits == 0);
        // now fill header the normal way
        while (k < 4)
            header[k++] = (uint8) zget8(a);
        len  = header[1] * 256 + header[0];
        nlen = header[3] * 256 + header[2];
        if (nlen != (len ^ 0xffff)) return 0; // e("zlib corrupt","Corrupt PNG");
        if (a->zbuffer + len > a->zbuffer_end) return 0; // e("read past buffer","Corrupt PNG");
        if (a->zout + len > a->zout_end)
            if (!expand(a, len)) return 0;
        memcpy(a->zout, a->zbuffer, len);
        a->zbuffer += len;
        a->zout += len;
        return 1;
    }
    
    int parse_zlib_header(zbuf *a) {
        int cmf   = zget8(a);
        int cm    = cmf & 15;
        /* int cinfo = cmf >> 4; */
        int flg   = zget8(a);
        if ((cmf*256+flg) % 31 != 0) return 0; // e("bad zlib header","Corrupt PNG"); // zlib spec
        if (flg & 32) return 0; // e("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
        if (cm != 8) return 0; // e("bad compression","Corrupt PNG"); // DEFLATE required for png
        // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
        return 1;
    }
    
    uint8 default_length[288], default_distance[32];
    
    void init_defaults()
    {
        int i;   // use <= to match clearly with spec
        for (i=0; i <= 143; ++i)     default_length[i]   = 8;
        for (   ; i <= 255; ++i)     default_length[i]   = 9;
        for (   ; i <= 279; ++i)     default_length[i]   = 7;
        for (   ; i <= 287; ++i)     default_length[i]   = 8;
        
        for (i=0; i <=  31; ++i)     default_distance[i] = 5;
    }
    
    
    int parse_zlib( zbuf *a, int parse_header ) {
        int final, type;
        if (parse_header)
            if (!parse_zlib_header(a)) return 0;
        a->num_bits = 0;
        a->code_buffer = 0;
        do {
            final = zreceive(a,1);
            type = zreceive(a,2);
            if (type == 0) {
                if (!parse_uncompressed_block(a)) return 0;
            } else if (type == 3) {
                return 0;
            } else {
                if (type == 1) {
                    // use fixed code lengths
                    if (!default_distance[31]) init_defaults();
                    if (!zbuild_huffman(&a->z_length  , default_length  , 288)) return 0;
                    if (!zbuild_huffman(&a->z_distance, default_distance,  32)) return 0;
                } else {
                    if (!compute_huffman_codes(a)) return 0;
                }
                if (!parse_huffman_block(a)) return 0;
            }
        } while (!final);
        return 1;
    }
    
    int do_zlib( zbuf *a, char *obuf, int olen, int exp, int parse_header ) {
        a->zout_start = obuf;
        a->zout       = obuf;
        a->zout_end   = obuf + olen;
        a->z_expandable = exp;
        
        return parse_zlib(a, parse_header);
    }
    
    char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
    {
        zbuf a;
        char *p = (char *) malloc(initial_size);
        if (p == NULL) return NULL;
        a.zbuffer = (uint8 *) buffer;
        a.zbuffer_end = (uint8 *) buffer + len;
        if (do_zlib(&a, p, initial_size, 1, 1)) {
            if (outlen) *outlen = (int) (a.zout - a.zout_start);
            return a.zout_start;
        } else {
            free(a.zout_start);
            return NULL;
        }
    }
    
    char *stbi_zlib_decode_malloc(char const *buffer, int len, int *outlen)
    {
        return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
    }
        
    int stbi_zlib_decode_buffer( char *obuffer, int olen, char const *ibuffer, int ilen )
    {
        zbuf a;
        a.zbuffer = (uint8 *) ibuffer;
        a.zbuffer_end = (uint8 *) ibuffer + ilen;
        if (do_zlib(&a, obuffer, olen, 0, 1))
            return (int) (a.zout - a.zout_start);
        else
            return -1;
    }
    
    
}

namespace r3 {
    
    void Inflate( std::vector< char > & ov, char const * ibuffer, int ilen ) {
        int olen;
        char *obuffer = stbi_zlib_decode_malloc( ibuffer, ilen, &olen );
        ov.resize( olen );
        memcpy( &ov[0], obuffer, olen );
        free( obuffer );
    }

}

namespace {

    
    // stretchy buffer; stbi__sbpush() == vector<>::push_back() -- stbi__sbcount() == vector<>::size()
#define stbi__sbraw(a) ((int *) (a) - 2)
#define stbi__sbm(a)   stbi__sbraw(a)[0]
#define stbi__sbn(a)   stbi__sbraw(a)[1]
    
#define stbi__sbneedgrow(a,n)  ((a)==0 || stbi__sbn(a)+n >= stbi__sbm(a))
#define stbi__sbmaybegrow(a,n) (stbi__sbneedgrow(a,(n)) ? stbi__sbgrow(a,n) : 0)
#define stbi__sbgrow(a,n)  stbi__sbgrowf((void **) &(a), (n), sizeof(*(a)))
    
#define stbi__sbpush(a, v)      (stbi__sbmaybegrow(a,1), (a)[stbi__sbn(a)++] = (v))
#define stbi__sbcount(a)        ((a) ? stbi__sbn(a) : 0)
#define stbi__sbfree(a)         ((a) ? free(stbi__sbraw(a)),0 : 0)
    
    void *stbi__sbgrowf(void **arr, int increment, int itemsize) {
        int m = *arr ? 2*stbi__sbm(*arr)+increment : increment+1;
        void *p = realloc(*arr ? stbi__sbraw(*arr) : 0, itemsize * m + sizeof(int)*2);
        assert(p);
        if (p) {
            if (!*arr) ((int *) p)[1] = 0;
            *arr = (void *) ((int *) p + 2);
            stbi__sbm(*arr) = m;
        }
        return *arr;
    }
    
    unsigned char *stbi__zlib_flushf(unsigned char *data, unsigned int *bitbuffer, int *bitcount) {
        while (*bitcount >= 8) {
            stbi__sbpush(data, (unsigned char) *bitbuffer);
            *bitbuffer >>= 8;
            *bitcount -= 8;
        }
        return data;
    }
    
    int stbi__zlib_bitrev(int code, int codebits) {
        int res=0;
        while (codebits--) {
            res = (res << 1) | (code & 1);
            code >>= 1;
        }
        return res;
    }
    
    unsigned int stbi__zlib_countm(unsigned char *a, unsigned char *b, int limit) {
        int i;
        for (i=0; i < limit && i < 258; ++i)
            if (a[i] != b[i]) break;
        return i;
    }
    
    unsigned int stbi__zhash(unsigned char *data) {
        uint32 hash = data[0] + (data[1] << 8) + (data[2] << 16);
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;
        return hash;
    }
    
#define stbi__zlib_flush() (out = stbi__zlib_flushf(out, &bitbuf, &bitcount))
#define stbi__zlib_add(code,codebits) \
(bitbuf |= (code) << bitcount, bitcount += (codebits), stbi__zlib_flush())
#define stbi__zlib_huffa(b,c)  stbi__zlib_add(stbi__zlib_bitrev(b,c),c)
    // default huffman tables
#define stbi__zlib_huff1(n)  stbi__zlib_huffa(0x30 + (n), 8)
#define stbi__zlib_huff2(n)  stbi__zlib_huffa(0x190 + (n)-144, 9)
#define stbi__zlib_huff3(n)  stbi__zlib_huffa(0 + (n)-256,7)
#define stbi__zlib_huff4(n)  stbi__zlib_huffa(0xc0 + (n)-280,8)
#define stbi__zlib_huff(n)  ((n) <= 143 ? stbi__zlib_huff1(n) : (n) <= 255 ? stbi__zlib_huff2(n) : (n) <= 279 ? stbi__zlib_huff3(n) : stbi__zlib_huff4(n))
#define stbi__zlib_huffb(n) ((n) <= 143 ? stbi__zlib_huff1(n) : stbi__zlib_huff2(n))
    
#define stbi__ZHASH   16384
    
    unsigned char * stbi_zlib_compress(unsigned char *data, int data_len, int *out_len, int quality) {
        static unsigned short lengthc[] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258, 259 };
        static unsigned char  lengtheb[]= { 0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0 };
        static unsigned short distc[]   = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577, 32768 };
        static unsigned char  disteb[]  = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };
        unsigned int bitbuf=0;
        int i,j, bitcount=0;
        unsigned char *out = NULL;
        unsigned char **hash_table[stbi__ZHASH]; // 64KB on the stack!
        if (quality < 5) quality = 5;
        
        stbi__sbpush(out, 0x78);   // DEFLATE 32K window
        stbi__sbpush(out, 0x5e);   // FLEVEL = 1
        stbi__zlib_add(1,1);  // BFINAL = 1
        stbi__zlib_add(1,2);  // BTYPE = 1 -- fixed huffman
        
        for (i=0; i < stbi__ZHASH; ++i)
            hash_table[i] = NULL;
        
        i=0;
        while (i < data_len-3) {
            // hash next 3 bytes of data to be compressed 
            int h = stbi__zhash(data+i)&(stbi__ZHASH-1), best=3;
            unsigned char *bestloc = 0;
            unsigned char **hlist = hash_table[h];
            int n = stbi__sbcount(hlist);
            for (j=0; j < n; ++j) {
                if (hlist[j]-data > i-32768) { // if entry lies within window
                    int d = stbi__zlib_countm(hlist[j], data+i, data_len-i);
                    if (d >= best) best=d,bestloc=hlist[j];
                }
            }
            // when hash table entry is too long, delete half the entries
            if (hash_table[h] && stbi__sbn(hash_table[h]) == 2*quality) {
                memcpy(hash_table[h], hash_table[h]+quality, sizeof(hash_table[h][0])*quality);
                stbi__sbn(hash_table[h]) = quality;
            }
            stbi__sbpush(hash_table[h],data+i);
            
            if (bestloc) {
                // "lazy matching" - check match at *next* byte, and if it's better, do cur byte as literal
                h = stbi__zhash(data+i+1)&(stbi__ZHASH-1);
                hlist = hash_table[h];
                n = stbi__sbcount(hlist);
                for (j=0; j < n; ++j) {
                    if (hlist[j]-data > i-32767) {
                        int e = stbi__zlib_countm(hlist[j], data+i+1, data_len-i-1);
                        if (e > best) { // if next match is better, bail on current match
                            bestloc = NULL;
                            break;
                        }
                    }
                }
            }
            
            if (bestloc) {
                int d = data+i - bestloc; // distance back
                assert(d <= 32767 && best <= 258);
                for (j=0; best > lengthc[j+1]-1; ++j);
                stbi__zlib_huff(j+257);
                if (lengtheb[j]) stbi__zlib_add(best - lengthc[j], lengtheb[j]);
                for (j=0; d > distc[j+1]-1; ++j);
                stbi__zlib_add(stbi__zlib_bitrev(j,5),5);
                if (disteb[j]) stbi__zlib_add(d - distc[j], disteb[j]);
                i += best;
            } else {
                stbi__zlib_huffb(data[i]);
                ++i;
            }
        }
        // write out final bytes
        for (;i < data_len; ++i)
            stbi__zlib_huffb(data[i]);
        stbi__zlib_huff(256); // end of block
        // pad with 0 bits to byte boundary
        while (bitcount)
            stbi__zlib_add(0,1);
        
        for (i=0; i < stbi__ZHASH; ++i)
            (void) stbi__sbfree(hash_table[i]);
        
        {
            // compute adler32 on input
            unsigned int i=0, s1=1, s2=0, blocklen = data_len % 5552;
            int j=0;
            while (j < data_len) {
                for (i=0; i < blocklen; ++i) s1 += data[j+i], s2 += s1;
                s1 %= 65521, s2 %= 65521;
                j += blocklen;
                blocklen = 5552;
            }
            stbi__sbpush(out, (unsigned char) (s2 >> 8));
            stbi__sbpush(out, (unsigned char) s2);
            stbi__sbpush(out, (unsigned char) (s1 >> 8));
            stbi__sbpush(out, (unsigned char) s1);
        }
        *out_len = stbi__sbn(out);
        // make returned pointer freeable
        memmove(stbi__sbraw(out), out, *out_len);
        return (unsigned char *) stbi__sbraw(out);
    }    
    
}

namespace r3 {
    
    void Deflate( std::vector< char > & ov, char const * ibuffer, int ilen, int quality ) {
        int olen;
        unsigned char *obuffer = stbi_zlib_compress( (unsigned char *)ibuffer, ilen, &olen, quality );
        ov.resize( olen );
        memcpy( &ov[0], obuffer, olen );
        free( obuffer );
    }
    
}

#endif // R3_UZLIB_IMPLEMENTATION    

#endif // __R3_UZLIB_H__
