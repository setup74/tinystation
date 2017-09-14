/*
 * NcodeFontDraw.cpp
 * Lee yongjae, setu74p@gmail.com, 2017-05-10.
 */

#include <pgmspace.h>
#include <OLEDDisplay.h>
#include "utf8ncode.h"
#include "NcodeFontDraw.h"


const char *
NcodeFontDraw::fontJumpEntry(const char *font, int c)
{
  if (font) {
    int first_char = pgm_read_byte_near(font + 4);
    int num_chars = pgm_read_byte_near(font + 5);
    if (c >= first_char && c < first_char + num_chars)
      return font + 6 + (c - first_char) * 7;
  }
  return NULL;
}

const char *
NcodeFontDraw::fontBitmap(const char *font, int c)
{
  if (font) {
    int first_char = pgm_read_byte_near(font + 4);
    int num_chars = pgm_read_byte_near(font + 5);
    const char *pj = font + 6 + (c - first_char) * 7 ;
    if (c >= first_char && c < first_char + num_chars) {
      int bitmap_offset = (((int)pgm_read_byte_near(pj + 0) << 8) + 
                            (int)pgm_read_byte_near(pj + 1));
      return font + 6 + num_chars * 7 + bitmap_offset;
    }
  }
  return NULL;
}

int
NcodeFontDraw::fontHeight(void)
{
  const char *font = (ncode_font)? ncode_font : ascii_font;
  if (font == NULL)
    return 0;

  /* returns font ascent + descent */
  return pgm_read_byte_near(font + 2) + pgm_read_byte_near(font + 3);
}

int
NcodeFontDraw::fontHeight(const char *font)
{
  return (font)? ((int)(signed char)pgm_read_byte_near(font + 2) +
                  (int)(signed char)pgm_read_byte_near(font + 3)) : 0;
}

int
NcodeFontDraw::fontAscent(const char *font)
{
  return (font)? (signed char)pgm_read_byte_near(font + 2) : 0;
}

int
NcodeFontDraw::fontDescent(const char *font)
{
  return (font)? (signed char)pgm_read_byte_near(font + 3) : 0;
}

int
NcodeFontDraw::fontBBox(const char *font, int c,
                        int *bbw_ret, int *bbh_ret, int *bbox_ret, int *bboy_ret)
  /* returns font char's bbox info found(1) or not(0) */
{
  const char *pj;
  if ((pj = fontJumpEntry(font, c))) {
    if (bbw_ret)  *bbw_ret  = (signed char)pgm_read_byte_near(pj + 3);
    if (bbh_ret)  *bbh_ret  = (signed char)pgm_read_byte_near(pj + 4);
    if (bbox_ret) *bbox_ret = (signed char)pgm_read_byte_near(pj + 5);
    if (bboy_ret) *bboy_ret = (signed char)pgm_read_byte_near(pj + 6);
    return 1;
  }
  return 0;
}

int
NcodeFontDraw::fontWidth(const char *font, int c)
{
  const char *pj;
  if ((pj = fontJumpEntry(font, c)))
    return pgm_read_byte_near(pj + 2/* dwidth field */);
  return 0;  
}

int
NcodeFontDraw::drawFont(OLEDDisplay *d, int ox, int oy, const char *font, int c)
  /* returns font width */
{
  const char *pj = fontJumpEntry(font, c);
  const char *bitmap = fontBitmap(font, c);
  if (pj == NULL || bitmap == NULL)
    return 0;

  int fascent = (signed char)pgm_read_byte_near(font + 2);
  int dwidth = (signed char)pgm_read_byte_near(pj + 2);
  int bbw = (signed char)pgm_read_byte_near(pj + 3);
  int bbh = (signed char)pgm_read_byte_near(pj + 4);
  int bbox = (signed char)pgm_read_byte_near(pj + 5);
  int bboy = (signed char)pgm_read_byte_near(pj + 6);
  int bx, x, y;
  
  for (y = 0; y < bbh; y++)
    for (bx = 0; bx < bbw; bx += 8) {
      int b = pgm_read_byte_near(bitmap++);
      for (x = bx ; x < bbw && x < bx + 8; x++, b <<= 1) 
        if ((b & 0x80))
          d->setPixel(ox + x + bbox, oy + fascent - bboy - bbh + y);
    }
      
  return dwidth;
}

int
NcodeFontDraw::fontWidthUc(int uc)
{
  int c, w = 0;
  if (isHangleUc(uc) && advanced_ncode_render) {
    int c_cho = ucToNcodeCho(uc);
    int c_jung = ucToNcodeJung(uc);
    int c_jong = ucToNcodeJong(uc);
    int bbw, bbox;
    int xmin = 0, xmax = 0;
    int wcho = 0, wjung = 0, wjong = 0, bjong = 0;

    /* shift up/down to make font bound box to be middle */
    if (c_cho && c_cho != 0xa1 && fontBBox(ncode_font, c_cho, &bbw, NULL, &bbox, NULL)) {
      wcho = fontWidth(ncode_font, c_cho);
      if (bbox < xmin) xmin = bbox;
      if (bbox + bbw > xmax) xmax = bbox + bbw;
    }
    if (c_jung && c_jung != 0xb5 && fontBBox(ncode_font, c_jung, &bbw, NULL, &bbox, NULL)) {
      wjung = fontWidth(ncode_font, c_jung);
      if (wcho + bbox < xmin) xmin = wcho + bbox;
      if (wcho + bbox + bbw > xmax) xmax = wcho + bbox + bbw;
    }
    if (c_jong && fontBBox(ncode_font, c_jong, &bbw, NULL, &bbox, NULL)) {
      wjong = fontWidth(ncode_font, c_jong);
      bjong = wcho + ((wjung >= 2)? (wjung * 3 + 2) / 4 : wjung);
      if (bjong + bbox < xmin) xmin = bjong + bbox;
      if (bjong + bbox + bbw > xmax) xmax = bjong + bbox + bbw;
    }

    w = -xmin;  /* align using bounding box */   
    w += wcho + wjung + wjong;
    if (w < (xmax -xmin))
      w = xmax - xmin;  /* align  using bounding box */
  }
  else if (isHangleUc(uc)) {
    w += fontWidth(ncode_font, ucToNcodeCho(uc));
    w += fontWidth(ncode_font, ucToNcodeJung(uc));
    if ((c = ucToNcodeJong(uc)))
      w += fontWidth(ncode_font, c);
  }
  else {
    w += fontWidth(ascii_font, uc);
  }
  return w;
}

int
NcodeFontDraw::drawFontUc(OLEDDisplay *d, int ox, int oy, int uc)
  /* returns font width */
{
  int c, w = 0;
  if (isHangleUc(uc) && advanced_ncode_render) {
    int c_cho = ucToNcodeCho(uc);
    int c_jung = ucToNcodeJung(uc);
    int c_jong = ucToNcodeJong(uc);
    int bbw, bbh, bbox, bboy;
    int fascent = fontAscent(ncode_font);
    int fdescent = fontDescent(ncode_font);
    int fheight = fascent + fdescent;
    int ymin = fascent;
    int ymax = -fdescent;
    int xmin = 0, xmax = 0;
    int wcho = 0, wjung = 0, wjong = 0, bjong = 0;

    /* shift up/down to make font bound box to be middle */
    if (c_cho && c_cho != 0xa1 && fontBBox(ncode_font, c_cho, &bbw, &bbh, &bbox, &bboy)) {
      wcho = fontWidth(ncode_font, c_cho);
      if (bbox < xmin) xmin = bbox;
      if (bbox + bbw > xmax) xmax = bbox + bbw;
      if (bboy < ymin) ymin = bboy;
      if (bboy + bbh > ymax) ymax = bboy + bbh;
    }
    if (c_jung && c_jung != 0xb5 && fontBBox(ncode_font, c_jung, &bbw, &bbh, &bbox, &bboy)) {
      wjung = fontWidth(ncode_font, c_jung);
      if (wcho + bbox < xmin) xmin = wcho + bbox;
      if (wcho + bbox + bbw > xmax) xmax = wcho + bbox + bbw;
      if (bboy < ymin) ymin = bboy;
      if (bboy + bbh > ymax) ymax = bboy + bbh;
    }
    if (c_jong && fontBBox(ncode_font, c_jong, &bbw, &bbh, &bbox, &bboy)) {
      wjong = fontWidth(ncode_font, c_jong);
      bjong = wcho + ((wjung >= 2)? (wjung * 3 + 2) / 4 : wjung);
      if (bjong + bbox < xmin) xmin = bjong + bbox;
      if (bjong + bbox + bbw > xmax) xmax = bjong + bbox + bbw;
      if (bboy < ymin) ymin = bboy;
      if (bboy + bbh > ymax) ymax = bboy + bbh;
    }
    if (ymin < ymax)
      oy += (((ymax + ymin) - (fascent - fdescent)) + 1) / 3;

    w = -xmin;  /* align using bounding box */   
    w += drawFont(d, ox + w, oy, ncode_font, c_cho);
    w += drawFont(d, ox + w, oy, ncode_font,c_jung);
    if (c_jong)
      w += drawFont(d, ox - xmin + bjong, oy, ncode_font, c_jong);
    if (w < (xmax -xmin))
      w = xmax - xmin;  /* align  using bounding box */
  }
  else if (isHangleUc(uc)) {
    w += drawFont(d, ox, oy, ncode_font, ucToNcodeCho(uc));
    w += drawFont(d, ox + w, oy, ncode_font, ucToNcodeJung(uc));
    if ((c = ucToNcodeJong(uc)))
      w += drawFont(d, ox + w, oy, ncode_font, c);
  }
  else {
    w += drawFont(d, ox, oy, ascii_font, uc);
  }
  return w;
}

void
NcodeFontDraw::drawString(OLEDDisplay *d, int x, int y,
                          int textAlign, const char *utf8_str)
{
  const char *s;
  int w, n, uc;

  w = 0;
  for (s = utf8_str; *s && (n = utf8_to_utf32((const unsigned char *)s, &uc)); s += n)
    w += fontWidthUc(uc);

  if (textAlign == TEXT_ALIGN_CENTER)     x -= w / 2;
  else if (textAlign == TEXT_ALIGN_RIGHT) x -= w;
  else /* textAlign == TEXT_ALIGN_LEFT */ x -= 0;

  for (s = utf8_str; *s && (n = utf8_to_utf32((const unsigned char *)s, &uc)); s += n)
    x += drawFontUc(d, x, y, uc);
}

void
NcodeFontDraw::drawStringMaxWidth(OLEDDisplay *d, int base_x, int base_y,
                          int textAlign, int maxWidth, const char *utf8_str)
{
  const char *s, *p, *b, *e;
  int fw, w, n, m, uc;
  int x, line_height;

  line_height = fontHeight();
  line_height += line_height / 5;  /* add 20% space */

  b = utf8_str;
  w = 0;
  for (s = utf8_str; *s && (n = utf8_to_utf32((const unsigned char *)s, &uc)); s += n) { 
    fw = fontWidthUc(uc);
    if (w + fw >= maxWidth)
      e = s;  /* line overflow; draw just before s */
    else {
      w += fw;      
      if (*(s + n) == '\0')
        e = s + n;  /* string ends; draw including s */
      else
        continue;
    }
    
    /* draw string b~e */
    if (textAlign == TEXT_ALIGN_CENTER)     x = base_x - w / 2;
    else if (textAlign == TEXT_ALIGN_RIGHT) x = base_x - w;
    else /* textAlign == TEXT_ALIGN_LEFT */ x = base_x;
    for (p = b; p < e && (m = utf8_to_utf32((const unsigned char *)p, &uc)); p += m)
      x += drawFontUc(d, x, base_y, uc);
      
    b = e;
    w = fw;  /* for overflow case only */
    base_y += line_height;
  }
}

