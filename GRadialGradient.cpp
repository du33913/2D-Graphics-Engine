#include <math.h>

#include "GColor.h"
#include "GMatrix.h"
#include "GPixel.h"
#include "GPoint.h"
#include "GShader.h"

#include "MathUtils.h"


class GRadialGradient : public GShader {
public:
    GRadialGradient(GPoint center, float radius, const GColor colors[], int count, TileMode tile) {
        fCenter = center;
        fRadius = radius;

        fColors = (GColor*) malloc(count * sizeof(GColor));
        memcpy(fColors, colors, count * sizeof(GColor));
        fColorCount = count;
        fTile = tile;
    }


    bool isOpaque() {
        return false;
    }


    bool setContext(const GMatrix& ctm) {
        return ctm.invert(&fLocalMatrix);
    }

    GPixel colorToPixel(const GColor& color) {
        int alpha = floatToPixelValue(color.a);
        int red   = floatToPixelValue(color.r * color.a);
        int green = floatToPixelValue(color.g * color.a);
        int blue  = floatToPixelValue(color.b * color.a);

        return GPixel_PackARGB(alpha, red, green, blue);
    }

    /**
     * Convert a float in the range [0.0 ... 1.0] to an 8-bit integer.
     */
    int floatToPixelValue(float value) {
        assert(0 <= value && value <= 1);

        return GRoundToInt(value * 255);
    }


    void shadeRow(int x, int y, int count, GPixel row[]) {
        for (int i = 0; i < count; ++i) {
            // GPoint point = fLocalMatrix.mapXY(x + i, y);

            GPoint pts[1]{ x + i, y };
            fLocalMatrix.mapPoints(pts, 1);
            GPoint point = pts[0];

            float dx = point.fX - fCenter.fX;
            float dy = point.fY - fCenter.fY;
            float distance = sqrtf(dx * dx + dy * dy);

            float t = distance / fRadius;

            if (fTile == TileMode::kRepeat) {
                t = t - floor(t);
            } else if (fTile == TileMode::kMirror) {
                t *= 0.5;
                t = t - floor(t);
                if (t > .5) {
                    t = 1 - t;
                }
                t *= 2;
            }

            t = clamp(t, 0.0f, 1.0f);

            if (t == 0) {
                row[i] = colorToPixel(fColors[0].pinToUnit());
            } else if (t == 1) {
                row[i] = colorToPixel(fColors[fColorCount - 1].pinToUnit());
            } else {
                if (fColorCount == 1) {
                    row[i] = colorToPixel(fColors[0].pinToUnit());
                } else {
                    int index = floor(t * (fColorCount - 1));
                    float span = 1.0f / (fColorCount - 1);
                    float start = index * span;

                    GColor c1 = fColors[index].pinToUnit();
                    GColor c2 = fColors[index + 1].pinToUnit();

                    t = clamp((t - start) / span, 0.0f, 1.0f);

                    GColor color = GColor::RGBA(
                        c1.r * (1 - t) + c2.r * t,
                        c1.g * (1 - t) + c2.g * t,
                        c1.b * (1 - t) + c2.b * t,
                        c1.a * (1 - t) + c2.a * t);    

                    row[i] = colorToPixel(color);
                }
            }
        }
    }

private:
    GPoint fCenter;
    float fRadius;

    GColor* fColors;
    int fColorCount;

    TileMode fTile;

    GMatrix fLocalMatrix;
};