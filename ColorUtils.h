#ifndef ColorUtils_DEFINED
#define ColorUtils_DEFINED

#include "GColor.h"
#include "GPixel.h"


/**
 * Convert a float in the range [0.0 ... 1.0] to an 8-bit integer.
 */
static inline int floatToPixelValue(float value) {
    assert(0 <= value && value <= 1);

    return GRoundToInt(value * 255);
}


/**
 * Convert a GColor to a GPixel that can actually be used.
 *
 * The color components are pre-multiplied by the alpha value before being
 * compacted.
 */
static inline GPixel colorToPixel(const GColor& color) {
    int alpha = floatToPixelValue(color.a);
    int red   = floatToPixelValue(color.r * color.a);
    int green = floatToPixelValue(color.g * color.a);
    int blue  = floatToPixelValue(color.b * color.a);

    return GPixel_PackARGB(alpha, red, green, blue);
}


#endif
