/*
 * NcodeFontDraw.h - Draw ACF Node2 Hangul Font on utf8 string
 * Lee Yongjae, setup74p@gmail.com, 2017-04-28.
 */

#ifndef __NCODE_FONT_DRAW_H__
#define __NCODE_FONT_DRAW_H__

#include <OLEDDisplay.h>

class NcodeFontDraw {
private:
  const char *ascii_font;   /* ASCII ACF data */
  const char *ncode_font;   /* NCODE ACF data */
  int advanced_ncode_render;

  int isHangleUc(int uc) {
    return (uc >= 44032 && uc <= 55195);
  }
  int ucToNcodeCho(int uc) {
    return isHangleUc(uc)? (uc - 44032) / 588 + 0xa2 : 0;
  }
  int ucToNcodeJung(int uc) {
    return isHangleUc(uc)? (uc - 44032) % 588 / 28 + 0xb6 : 0;
  }
  int ucToNcodeJong(int uc) {
    int n;
    return (isHangleUc(uc) && (n = (uc - 44032) % 588 % 28))? n - 1 + 0xcb  : 0;
  }

  const char *fontJumpEntry(const char *font, int c);
  const char *fontBitmap(const char *font, int c);

  int fontHeight(void);
  int fontHeight(const char *font);
  int fontAscent(const char *font);
  int fontDescent(const char *font);
  int fontBBox(const char *font, int c, 
               int *bbx_ret, int *bbh_ret, int *bbox_ret, int *bboy_ret);
      /* returns font char's bbox info found(1) or not(0) */
  int fontWidth(const char *font, int c);
  int drawFont(OLEDDisplay *d, int ox, int oy, const char *font, int c);

  int fontWidthUc(int uc);
  int drawFontUc(OLEDDisplay *d, int ox, int oy, int uc);

public:
  static const int TEXT_ALIGN_LEFT   = 0;
  static const int TEXT_ALIGN_CENTER = 1;
  static const int TEXT_ALIGN_RIGHT  = 2;

  NcodeFontDraw(const char *_ascii_font, const char *_ncode_font, int _advanced_ncode_render = 1) {
    ascii_font = _ascii_font;
    ncode_font = _ncode_font;
    advanced_ncode_render = _advanced_ncode_render;
  }
  void setFont(const char *_ascii_font, const char *_ncode_font) {
    ascii_font = _ascii_font;
    ncode_font = _ncode_font;
  }

  void drawString(OLEDDisplay *d, int x, int y,
                  int textAlign, const char *utf8_str);
  void drawStringMaxWidth(OLEDDisplay *d, int x, int y,
                  int textAlign, int maxWidth, const char *utf8_str);

};

#endif  /* __NCODE_FONT_DRAW_H__ */

