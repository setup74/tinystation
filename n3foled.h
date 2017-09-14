/*
 * n3foled.h - Draw utf8 string with n3f font on OLEDDisplay
 * Lee Yongjae, setup74p@gmail.com, 2017-04-28.
 */

/* y is 0 at top and 13 at bottom */


#ifndef __N3FOLED_H__
#define __N3FOLED_H__

#include <OLEDDisplay.h>

void n3f_drawString(OLEDDisplay *d, int x, int y, char* utf8_str);
void n3f_drawStringMaxWidth(OLEDDisplay *d, int x, int y, int maxWidth, char* utf8_str);

#endif  /* __N3FOLED_H__ */

