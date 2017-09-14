/*
 * utf8ncode.c - UTF8 Hangul to Ncode Encoding
 * Lee yongjae, setup74@gmail.com, 2017-04-24.
 */

/*

ncode (n-byte 3BeulSik hangul code) version 2.0

  ncode is a hangul encoding scheme for 3-BeulSik Hangul input/output. 
  ncode should not be exposed to users without notice that this code is not
  desiged for general information exchanging purpose, but only for internal
  representation of hangul, for input/output convience.

ncode encoding:

  -------------------------------------------------------------------------
  | a0     | a1 f1  | a2 K   | a3 Kk  | a4 N   | a5 T   | a6 Tt  | a7 R   |
  | a8 M   | a9 P   | aa Pp  | ab S   | ac Ss  | ad O*  | ae C   | af Cc  |
  | b0 Ch  | b1 Kh  | b2 Th  | b3 Ph  | b4 H   | b5 f2  | b6 a   | b7 ae  |
  | b8 ya  | b9 yae | ba eo  | bb e   | bc yeo | bd ye  | be o   | bf wa  |
  | c0 wae | c1 oe  | c2 yo  | c3 u   | c4 weo | c5 we  | c6 wi  | c7 yu  |
  | c8 eu  | c9 yi  | ca i   | cb k   | cc kk  | cd ks  | ce n   | cf nc  |
  | d0 nh  | d1 t   | d2 l   | d3 lk  | d4 lm  | d5 lp  | d6 ls  | d7 lth |
  | d8 lph | d9 lh  | da m   | db p   | dc ps  | dd s   | de ss  | df ng  |
  | e0 c   | e1 ch  | e2 kh  | e3 th  | e4 ph  | e5 h   |        |        | 
  -------------------------------------------------------------------------
  * f1 is ChoSeong-fill code and f2 is JungSeong-fill code.
  * O* is null string in hangul roman expression.

ncode sequence for each combined hangul:

  [ch] [ju]      : normal hangul without JongSeong
  [ch] [ju] [jo] : normal hangul JongSeong
  [ch] [f2]      : ChoSeong-only
  [f1] [ju]      : JungSeong-only
  [f1] [f2] [jo] : JongSeong-only
  [f1] [ju] [jo] : JungSeong-and-JongSeong-only
  [ch] [f2] [jo] : ChoSeong-and-JongSeong-only

  * f1 : ChoSeong-fill  (0xa1)
  * ch : ChoSeong       (0xa2-0xb4)
  * f2 : JungSeong-fill (0xb5)
  * ju : JungSeong      (0xb6-0xca)
  * jo : JongSeong      (0xcb-0xe5)

versions:

  There was ncode version 1 used before 1996.1.27 which has only one fill
  code for ChoSeong-fill at 0xa0 without f1 nor f2. The version is upgraded
  to version 1.1 which has f1 and f2 but the location of f1 is 0xa0 and
  f2 is 0xe4. In this version 2.0, f1 and f2 is move to 0xa1 and 0xb5 and
  others are shifted for this.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utf8ncode.h"


/* Unicode Hangul Johap code: 44032~55195 (0xac00~0xd79b) */
/* (initial:0~18) x 588 + (medial:0~20) x 28 + (final:0~27) + 44032 */

/* Unicode Hangul Jamo code: 0x1100~0x11ff */
/* See: https://en.wikipedia.org/wiki/Hangul_Jamo_(Unicode_block) */


int
utf8_to_utf32(const unsigned char *utf8_str, int *utf32_ret)
  /* returns number of utf8 chars consumes */
{
  const unsigned char *p = utf8_str;
  int c, i, b, n;

  c = 0;

  if ((b = (unsigned char)utf8_str[0]) == 0)
    return 0;  /* return 0 at end of string */

  /* check first byte */
  if      ((b & 0x80) == 0x00) { n = 1; c = b & 0x7f; } 
  else if ((b & 0xe0) == 0xc0) { n = 2; c = b & 0x1f; }
  else if ((b & 0xf0) == 0xe0) { n = 3; c = b & 0x0f; }
  else if ((b & 0xf8) == 0xf0) { n = 4; c = b & 0x07; }
  else {
      /* invalid utf8; consume 1 byte */
      *utf32_ret = b;  /* NEED TO MARK INVALID CHAR */
      return 1;
  }

  /* get remaining byte */
  for (i = 1; i < n && (b = utf8_str[i]) && (b & 0xc0) == 0x80; i++)
    c = (c << 6) + (b & 0x3f);

  if (i != n) {
    /* invalid utf8; consume i byte */
    *utf32_ret = c;  /* NEED TO MARK INVALID CHAR */
    return i;
  }

  *utf32_ret = c;
  return n;
}

int
utf8_to_n3f(const unsigned char *utf8_str, unsigned char *n3f_buf, int n3f_buf_size)
  /* returns strlen of n3f_buf */
{
  const unsigned char *s;
  unsigned char *p, *p_limit;
  int uc, n;

  p = n3f_buf;
  p_limit = n3f_buf + n3f_buf_size - 4;
  
  for (s = utf8_str; *s && p < p_limit &&
                     (n = utf8_to_utf32(utf8_str, &uc)) > 0; s += n) {
    if (uc >= 1 && uc <= 127) {
      /* unicode ASCII */
      if (p + 1 >= p_limit)
        break;  /* buffer overflow */
      *p++ = (char)uc;
    }
    else if (uc >= 44032 && uc <= 55195) {
      /* unicode johap hangul */
      /* (initial:0~18) x 588 + (medial:0~20) x 28 + (final:0~27) + 44032 */
      if (p + 2 >= p_limit)
        break;  /* buffer overflow */
      uc -= 44032;
      *p++ = (char)((uc / 588) + 0xa2);
      uc %= 588;
      *p++ = (char)((uc / 28) + 0xb6);
      uc %= 28; 
      if (uc) {
        if (p + 1 >= p_limit)
          break;  /* buffer overflow */
        /* NOTE: uc=0 for no jong-sung chars */
        *p++ = (char)(uc - 1 + 0xcb); 
      }
    }
    /* unicode hangul jamo */
    else if (uc >= 0x1100 && uc <= 0x1112) {
      *p++ = (char)(uc - 0x1100) + 0xa2;
    }
    else if (uc >= 0x1161 && uc <= 0x1175) {
      *p++ = 0xa1;
      *p++ = (char)(uc - 0x1161) + 0xb6;
    }
    else if (uc >= 0x11a8 && uc <= 0x11c2) {
      *p++ = 0xa1;
      *p++ = 0xb5;
      *p++ = (char)(uc - 0x11a8) + 0xcb;
    } 
    else {
      /* unicode unhandled range */
      if (p + 1 >= p_limit)
        break;  /* buffer overflow */
      *p++ = '?';
    }
  }

  *p = '\0';
  return p - n3f_buf;
}

