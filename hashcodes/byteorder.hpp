#ifndef __BYTEORDER_H__
#define __BYTEORDER_H__

// バイトオーダ変換関連
//
// The MIT License (MIT)
//
// Copyright (c) <2014> chromabox <chromarockjp@gmail.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../include/typedef.hpp"

#if defined(__linux__) || defined(__CYGWIN__)
//#define _BSD_SOURCE
#include <endian.h>
#elif defined(__MINGW32__)
#define htobe32(x) __builtin_bswap32 (x)
#define htobe64(x) __builtin_bswap64 (x)
#endif	// __MINGW32__

inline uint32_t left_rotate32(uint32_t x, size_t n)
{
    return (x<<n) ^ (x>> (32-n));
}

#endif // __BYTEORDER_H__

