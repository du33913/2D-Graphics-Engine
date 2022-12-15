#include "GBitmap.h"
#include "GMatrix.h"
#include "GShader.h"


class BitmapShader : public GShader {
public:
    BitmapShader(const GBitmap& bitmap, const GMatrix& localMatrix, GShader::TileMode tile)
        : fSourceBitmap(bitmap)
        , fLocalMatrix(localMatrix)
        , fTile(tile) {}

    bool isOpaque() override {
        return false;
    }

    bool setContext(const GMatrix& ctm) override {
        GMatrix tmp = ctm * fLocalMatrix;

        return tmp.invert(&fInverse);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        // GPoint local = fInverse.mapXY(x + 0.5f, y + 0.5f);

        GPoint pts[1]{ x + 0.5f, y + 0.5f };
        fInverse.mapPoints(pts,pts, 1);

        GPoint local = pts[0];

        for (int i = 0; i < count; ++i) {
            int sourceX = GFloorToInt(local.fX);
            int sourceY = GFloorToInt(local.fY);

            if (fTile == TileMode::kRepeat) {
                sourceX %= fSourceBitmap.width();
                if (sourceX < 0) {
                    sourceX += fSourceBitmap.width();
                }

                sourceY %= fSourceBitmap.height();
                if (sourceY < 0) {
                    sourceY += fSourceBitmap.height();
                }
            } else if (fTile == TileMode::kMirror) {
                float x1 = local.fX / fSourceBitmap.width();
                float y1 = local.fY / fSourceBitmap.height();

                x1 *= .5;
                x1 = x1 - floor(x1);
                if (x1 > .5) {
                    x1 = 1 - x1;
                }
                x1 *= 2;

                y1 *= .5;
                y1 = y1 - floor(y1);
                if (y1 > .5) {
                    y1 = 1 - y1;
                }
                y1 *= 2;

                sourceX = GFloorToInt(x1 * fSourceBitmap.width());
                sourceY = GFloorToInt(y1 * fSourceBitmap.height());
            }

            // Clamp values
            sourceX = std::max(0, std::min(fSourceBitmap.width() - 1, sourceX));
            sourceY = std::max(0, std::min(fSourceBitmap.height() - 1, sourceY));

            row[i] = *fSourceBitmap.getAddr(sourceX, sourceY);

            local.fX += fInverse[0];
            local.fY += fInverse[3];
        }
    }

private:
    GBitmap fSourceBitmap;
    GMatrix fInverse;
    GMatrix fLocalMatrix;
    TileMode fTile;
};


std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localInv, GShader::TileMode tile) {
    if (!bitmap.pixels()) {
        return nullptr;
    }

    return std::unique_ptr<GShader>(new BitmapShader(bitmap, localInv, tile));
}
