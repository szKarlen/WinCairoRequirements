#ifndef _CAIRO_DWRITE_COMMON_H_
#define _CAIRO_DWRITE_COMMON_H_

// Windows header files
#include <windows.h>
#include <vector>
#include "cairo.h"

#include <unordered_map>
#include <iostream>
#include <vector>
#include <algorithm>

UINT* getFontsList();

void cairo_begin_custom_fonts_update();

void cairo_end_custom_fonts_update();

#endif /* _CAIRO_DWRITE_COMMON_H_ */
