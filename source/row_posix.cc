/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "row.h"

extern "C" {

#if defined(__x86_64__)

// 64 bit linux gcc version

void FastConvertYUVToRGB32Row(const uint8* y_buf,  // rdi
                              const uint8* u_buf,  // rsi
                              const uint8* v_buf,  // rdx
                              uint8* rgb_buf,      // rcx
                              int width) {         // r8
  asm(
"1:"
  "movzb  (%1),%%r10\n"
  "lea    1(%1),%1\n"
  "movzb  (%2),%%r11\n"
  "lea    1(%2),%2\n"
  "movq   2048(%5,%%r10,8),%%xmm0\n"
  "movzb  (%0),%%r10\n"
  "movq   4096(%5,%%r11,8),%%xmm1\n"
  "movzb  0x1(%0),%%r11\n"
  "paddsw %%xmm1,%%xmm0\n"
  "movq   (%5,%%r10,8),%%xmm2\n"
  "lea    2(%0),%0\n"
  "movq   (%5,%%r11,8),%%xmm3\n"
  "paddsw %%xmm0,%%xmm2\n"
  "paddsw %%xmm0,%%xmm3\n"
  "shufps $0x44,%%xmm3,%%xmm2\n"
  "psraw  $0x6,%%xmm2\n"
  "packuswb %%xmm2,%%xmm2\n"
  "movq   %%xmm2,0x0(%3)\n"
  "lea    8(%3),%3\n"
  "sub    $0x2,%4\n"
  "ja     1b\n"
  :
  : "r"(y_buf),  // %0
    "r"(u_buf),  // %1
    "r"(v_buf),  // %2
    "r"(rgb_buf),  // %3
    "r"(width),  // %4
    "r" (_kCoefficientsRgbY)  // %5
  : "memory", "r10", "r11", "xmm0", "xmm1", "xmm2", "xmm3"
);
}

#elif defined(__i386__)
// 32 bit gcc version

void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width);
  asm(
  ".text\n"
#if defined(OSX) || defined(IOS)
  ".globl _FastConvertYUVToRGB32Row\n"
"_FastConvertYUVToRGB32Row:\n"
#else
  ".global FastConvertYUVToRGB32Row\n"
"FastConvertYUVToRGB32Row:\n"
#endif
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"

"1:"
  "movzbl (%edi),%eax\n"
  "lea    1(%edi),%edi\n"
  "movzbl (%esi),%ebx\n"
  "lea    1(%esi),%esi\n"
  "movq   _kCoefficientsRgbY+2048(,%eax,8),%mm0\n"
  "movzbl (%edx),%eax\n"
  "paddsw _kCoefficientsRgbY+4096(,%ebx,8),%mm0\n"
  "movzbl 0x1(%edx),%ebx\n"
  "movq   _kCoefficientsRgbY(,%eax,8),%mm1\n"
  "lea    2(%edx),%edx\n"
  "movq   _kCoefficientsRgbY(,%ebx,8),%mm2\n"
  "paddsw %mm0,%mm1\n"
  "paddsw %mm0,%mm2\n"
  "psraw  $0x6,%mm1\n"
  "psraw  $0x6,%mm2\n"
  "packuswb %mm2,%mm1\n"
  "movntq %mm1,0x0(%ebp)\n"
  "lea    8(%ebp),%ebp\n"
  "sub    $0x2,%ecx\n"
  "ja     1b\n"
  "popa\n"
  "ret\n"
);

#else
// C reference code that mimic the YUV assembly.
#define packuswb(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))
#define paddsw(x, y) (((x) + (y)) < -32768 ? -32768 : \
    (((x) + (y)) > 32767 ? 32767 : ((x) + (y))))

static inline void YuvPixel(uint8 y,
                            uint8 u,
                            uint8 v,
                            uint8* rgb_buf) {

  int b = _kCoefficientsRgbY[256+u][0];
  int g = _kCoefficientsRgbY[256+u][1];
  int r = _kCoefficientsRgbY[256+u][2];
  int a = _kCoefficientsRgbY[256+u][3];

  b = paddsw(b, _kCoefficientsRgbY[512+v][0]);
  g = paddsw(g, _kCoefficientsRgbY[512+v][1]);
  r = paddsw(r, _kCoefficientsRgbY[512+v][2]);
  a = paddsw(a, _kCoefficientsRgbY[512+v][3]);

  b = paddsw(b, _kCoefficientsRgbY[y][0]);
  g = paddsw(g, _kCoefficientsRgbY[y][1]);
  r = paddsw(r, _kCoefficientsRgbY[y][2]);
  a = paddsw(a, _kCoefficientsRgbY[y][3]);

  b >>= 6;
  g >>= 6;
  r >>= 6;
  a >>= 6;

  *reinterpret_cast<uint32*>(rgb_buf) = (packuswb(b)) |
                                        (packuswb(g) << 8) |
                                        (packuswb(r) << 16) |
                                        (packuswb(a) << 24);
}

void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  for (int x = 0; x < width; x += 2) {
    uint8 u = u_buf[x >> 1];
    uint8 v = v_buf[x >> 1];
    uint8 y0 = y_buf[x];
    YuvPixel(y0, u, v, rgb_buf);
    if ((x + 1) < width) {
      uint8 y1 = y_buf[x + 1];
      YuvPixel(y1, u, v, rgb_buf + 4);
    }
    rgb_buf += 8;  // Advance 2 pixels.
  }
}
#endif

}  // extern "C"
