/*
 * utf8ncode.h - UTF8 Hangul to Ncode Encoding
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


#ifndef __UTF8NCODE_H__
#define __UTF8NCODE_H__

#ifdef __cplusplus
extern "C" {
#endif


/* Unicode Hangul Johap code: 44032~55195 (0xac00~0xd79b) */
/* (initial:0~18) x 588 + (medial:0~20) x 28 + (final:0~27) + 44032 */

/* Unicode Hangul Jamo code: 0x1100~0x11ff */
/* See: https://en.wikipedia.org/wiki/Hangul_Jamo_(Unicode_block) */


int utf8_to_utf32(const unsigned char *utf8_str, int *utf32_ret);
  /* returns number of utf8 chars consumes */

int utf8_to_ncode(const unsigned char *utf8_str, unsigned char *n_buf, int n_buf_size);
  /* returns strlen of n_buf */


#ifdef __cplusplus
}
#endif

#endif  /* __UTF8NCODE_H__ */

