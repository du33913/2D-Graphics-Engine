#include "GShader.h"
#include "GMatrix.h"
#include "MathUtils.h"
#include <vector>


class TriangleGradient : public GShader {
public:
   TriangleGradient(GPoint p0, GPoint p1, GPoint p2, GColor c0, GColor c1, GColor c2){
      fP0 = p0;
      fP1 = p1;
      fP2 = p2;
      fC0 = c0;
      fC1 = c1;
      fC2 = c2;
   }

   bool isOpaque() override{
     return false;
   }

   // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
   bool setContext(const GMatrix& ctm) override{

    GPoint U = fP1-fP0;
    GPoint V = fP2-fP0;
    fLocalMatrix[0] = U.x();
    fLocalMatrix[1] = V.x();
    fLocalMatrix[2] = fP0.x();
    fLocalMatrix[3] = U.y();
    fLocalMatrix[4] = V.y();
    fLocalMatrix[5] = fP0.y();

    dc0 = fC1 - fC0;
    dc1 = fC2 - fC0;

    fLocalMatrix.invert(&gInverse);
    fInverse = GMatrix();

    if (!ctm.invert(&fInverse)) {
        return false;
    }

    ctm.invert(&fInverse);

    P = gInverse;
    P.preConcat(fInverse);
    return true;

}

  /**
   *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
   *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
   *  can hold at least [count] entries.
   */
   void shadeRow(int x, int y, int count, GPixel row[]) override{
    assert(count > 0);
    // GPoint local = fInverse.mapXY(x + 0.5, y + 0.5);
    // C[0] = P’x * DC1 + P’y * DC2 + C0

    GPoint pts[1]{ x + 0.5, y + 0.5 };
    fInverse.mapPoints(pts, 1);
    gInverse.mapPoints(pts, 1);

    float fx = pts[0].fX;
    float fy = pts[0].fY;
    
    //  float fx = gInverse.mapXY(local.fX, local.fY).fX;
    //  float fy = gInverse.mapXY(local.fX, local.fY).fY;
    GColor c = (fx * dc0) + (fy * dc1) + fC0;
    c.a = std::max(0.0f, std::min(1.0f, c.a));
    c.r = std::max(0.0f, std::min(1.0f, c.r));
    c.g = std::max(0.0f, std::min(1.0f, c.g));
    c.b = std::max(0.0f, std::min(1.0f, c.b));

    row[0] = colorToPixel(c);

    GColor dc = (P[0]*dc0) + (P[3]*dc1);


    for (int i = 1; i < count; ++i){
        //C[i] = C[i - 1] + DC
        GColor nc = c + dc;

        nc.a = clamp(nc.a, 0.0f, 1.0f);
        nc.r = clamp(nc.r, 0.0f, 1.0f);
        nc.g = clamp(nc.g, 0.0f, 1.0f);
        nc.b = clamp(nc.b, 0.0f, 1.0f);

    	row[i] = colorToPixel(nc);

        c = nc;
    }
}

private:
  GPoint fP0;
  GPoint fP1;
  GPoint fP2;

  GColor fC0;
  GColor fC1;
  GColor fC2;

  GColor dc0, dc1;

  GMatrix fLocalMatrix;
  GMatrix fInverse;
  GMatrix gInverse;
  GMatrix P;
  /**
   * 
   */
  int floatToPixelValue(float value) {
    assert(0 <= value && value <= 1);

    return GRoundToInt(value * 255);
    }

    /**
    * Convert a GColor to a GPixel that can actually be used.
    *
    * The color components are pre-multiplied by the alpha value before being
    * compacted.
    */
    GPixel colorToPixel(const GColor& color) {
        int alpha = floatToPixelValue(color.a);
        int red   = floatToPixelValue(color.r * color.a);
        int green = floatToPixelValue(color.g * color.a);
        int blue  = floatToPixelValue(color.b * color.a);

        return GPixel_PackARGB(alpha, red, green, blue);
    }

};