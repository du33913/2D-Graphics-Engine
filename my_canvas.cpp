#include <stack>
#include <iostream>

#include "GBitmap.h"
#include "GBlitter.h"
#include "GCanvas.h"
#include "GColor.h"
#include "GMath.h"
#include "GMatrix.h"
#include "GPath.h"
#include "GPixel.h"
#include "GPoint.h"
#include "GRect.h"
#include "GScanConverter.h"
#include "GShader.h"

#include "Blend.h"
#include "Clipper.h"
#include "DoubleShader.cpp"
#include "ProxyShader.cpp"
#include "TriangleGradient.cpp"


/**
 * Convert a float in the range [0.0 ... 1.0] to an 8-bit integer.
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


class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {
        GMatrix identity = GMatrix();
        //identity.setIdentity();
        mCTMStack.push(identity);
    }

    void concat(const GMatrix& matrix) override {
        mCTMStack.top().preConcat(matrix);
    }

    /**
     * Draw a convex polygon to the canvas. The polygon is constructed by
     * forming edges between the provided points. The paint determines how the
     * new polygon is drawn with respect to the pixels already on the screen.
     */
    void drawConvexPolygon(const GPoint srcPoints[], int count, const GPaint& paint) override {

        // If the paint has a shader and we can't set its context, we can't
        // draw anything.
        if (paint.getShader() != nullptr
                && !paint.getShader()->setContext(mCTMStack.top())) {
            return;
        }

        std::vector<GPoint> holder;
        holder.resize(count); 
        GPoint* points = holder.data();
        // GPoint points[count];
        mCTMStack.top().mapPoints(points, srcPoints, count);

        GRect bounds = GRect::WH(fDevice.width(), fDevice.height());
        std::vector<Edge> holder2;
        holder2.resize(count * 3); 
        Edge* storage = holder2.data();
        //Edge storage[count * 3];
        Edge* edge = storage;

        for (int i = 0; i < count; ++i) {
            GPoint p0 = points[i];
            GPoint p1 = points[(i + 1) % count];

            edge = clipLine(p0, p1, bounds, edge);
        }

        int edgeCount = edge - storage;
        if (edgeCount == 0) {
            return;
        }

        assert(edgeCount >= 2);

        std::sort(storage, storage + edgeCount);

        int lastY = storage[edgeCount - 1].bottomY;

        // Set up our initial left and right boundary edges
        Edge left = storage[0];
        Edge right = storage[1];

        // Track index of next edge position
        int next = 2;

        float curY = left.topY;

        float leftX = left.curX;
        float rightX = right.curX;

        // Loop through all the possible y-coordinates that could be drawn
        while (curY < lastY) {
            drawRow(curY, GRoundToInt(leftX), GRoundToInt(rightX), paint);
            curY++;

            // After drawing, we check to see if we've completed either the
            // left or right edge. If we have, we replace it with the next
            // edge.

            if (curY >= left.bottomY) {
                left = storage[next];
                next++;

                leftX = left.curX;
            } else {
                leftX += left.dxdy;
            }

            if (curY >= right.bottomY) {
                right = storage[next];
                next++;

                rightX = right.curX;
            } else {
                rightX += right.dxdy;
            }
        }
    }

    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[],
        int count, const int indices[], const GPaint& paint) {

        int n = 0;

        GPoint points[3];
        GColor triColors[3];
        GPoint textures[3];
        GPaint newPaint = paint;

        for (int i = 0; i < count; ++i) {
            points[0] = verts[indices[n+0]];
            points[1] = verts[indices[n+1]];
            points[2] = verts[indices[n+2]];

            if (colors != NULL && texs != NULL) {
                triColors[0] = colors[indices[n+0]];
                triColors[1] = colors[indices[n+1]];
                triColors[2] = colors[indices[n+2]];

                textures[0] = texs[indices[n+0]];
                textures[1] = texs[indices[n+1]];
                textures[2] = texs[indices[n+2]];

                GMatrix T, P, invT, fin;
                T = GMatrix(textures[1].x() - textures[0].x(), textures[2].x() - textures[0].x(), textures[0].x(), 
                    textures[1].y() - textures[0].y(), textures[2].y() - textures[0].y(), textures[0].y());

                P = GMatrix(points[1].x() - points[0].x(), points[2].x() - points[0].x(), points[0].x(),
                    points[1].y() - points[0].y(), points[2].y() - points[0].y(), points[0].y());

                T.invert(&invT);

                fin = P * invT;

                ProxyShader *s0 =  new ProxyShader(paint.getShader(), fin);
                TriangleGradient *s1 =  new TriangleGradient(points[0], points[1], points[2], 
                    triColors[0], triColors[1], triColors[2]);

                DoubleShader *sBOTH = new DoubleShader(s0, s1);
                newPaint = GPaint(sBOTH);
                drawConvexPolygon(points, 3, newPaint);


            } else if (colors != NULL) {
                triColors[0] = colors[indices[n+0]];
                triColors[1] = colors[indices[n+1]];
                triColors[2] = colors[indices[n+2]];

                TriangleGradient *s1 =  new TriangleGradient(points[0], points[1], points[2], 
                    triColors[0], triColors[1], triColors[2]);
                
                newPaint = GPaint(s1);
                drawConvexPolygon(points, 3, newPaint);
            } else if (texs != NULL) {
                textures[0] = texs[indices[n+0]];
                textures[1] = texs[indices[n+1]];
                textures[2] = texs[indices[n+2]];

                GMatrix T, P, invT, fin;
                T = GMatrix(textures[1].x() - textures[0].x(), textures[2].x() - textures[0].x(), textures[0].x(), 
                    textures[1].y() - textures[0].y(), textures[2].y() - textures[0].y(), textures[0].y());

                P = GMatrix(points[1].x() - points[0].x(), points[2].x() - points[0].x(), points[0].x(),
                    points[1].y() - points[0].y(), points[2].y() - points[0].y(), points[0].y());

                T.invert(&invT);

                fin = P * invT;
                ProxyShader *s0 =  new ProxyShader(paint.getShader(), fin);
                newPaint = GPaint(s0);
                drawConvexPolygon(points, 3, newPaint);
            }

            n += 3;
        }
    }

    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4],
        int level, const GPaint& paint) {
        GPoint A = verts[0];
        GPoint B = verts[1];
        GPoint C = verts[2];
        GPoint D = verts[3];

        GPoint new_verts[4];
        int indices[6] = {0,1,3,1,2,3};

        for (int u = 0; u <= level; ++u) {
            for (int v = 0; v <= level; ++v) {
                float u1 = (float)u / (float)(level+1);
                float u2 = (float)(u+1) / (float)(level+1);

                float v1 = (float)v / (float)(level+1);
                float v2 = (float)(v+1) / (float)(level+1);

                new_verts[0]=makeNewPoint(A,B,C,D, u1, v1);
                new_verts[1]=makeNewPoint(A,B,C,D, u2, v1);
                new_verts[2]=makeNewPoint(A,B,C,D, u2, v2);
                new_verts[3]=makeNewPoint(A,B,C,D, u1, v2);

                GColor new_colors[4];
                GColor *cp = nullptr;
                GPoint new_texs[4];
                GPoint *pc = nullptr;

                if(colors){
                    new_colors[0] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u1, v1);
                    new_colors[1] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u2, v1);
                    new_colors[2] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u2, v2);
                    new_colors[3] = makeNewColor(colors[0], colors[1],colors[2],colors[3], u1, v2);

                    cp = new_colors;
                }

                if(texs){
                    new_texs[0] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u1, v1);
                    new_texs[1] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u2, v1);
                    new_texs[2] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u2, v2);
                    new_texs[3] = makeNewPoint(texs[0], texs[1], texs[2], texs[3], u1, v2);

                    pc = new_texs;
                }

                drawMesh(new_verts, cp, pc, 2, indices, paint);
            }
        }
    }

    GPoint makeNewPoint(GPoint A, GPoint B, GPoint C, GPoint D, float u, float v){
        return ((1.0f-u)*(1-v)*A) + ((1-v)*u*B) + (u*v*C) + ((1-u)*v*D);
    }

    GColor makeNewColor(GColor A, GColor B, GColor C, GColor D, float u, float v){
        return ((1-u)*(1-v)*A) + ((1-v)*u*B) + (u*v*C) + ((1-u)*v*D);
    }

    GPoint quadHelper(const GPoint src[], float t) {
        return (src[0] * (1.0f - t) * (1.0f - t)) + (2 * t * src[1] * (1.0f - t)) + (src[2] * t * t);
    }

    GPoint cubicHelper(const GPoint src[], float t) {
        return (src[0] * (1.0f - t) * (1.0f - t) * (1.0f - t)) + (3 * t * src[1] * (1.0f - t) * (1.0f - t)) + (3 * t * t * src[2] * (1.0f - t)) + (src[3] * t * t * t);
    }

    void drawPath(const GPath& path, const GPaint& paint) override {
        GMatrix matrix = mCTMStack.top();

        // If the paint has a shader and we can't set its context, we can't
        // draw anything.
        if (paint.getShader() != nullptr
                && !paint.getShader()->setContext(matrix)) {
            return;
        }

        std::vector<GPoint> holder3;
        // holder3.resize(20 * path.countPoints());

        // GPoint points[6 * path.countPoints()];

        int edgeCount = 0;
        GPath::Edger edger = GPath::Edger(path);

        GPath::Verb verb;
        do {
            GPoint nextPts[4];
            verb = edger.next(nextPts);

            if (verb == GPath::Verb::kLine) {
                holder3.push_back(nextPts[0]);
                holder3.push_back(nextPts[1]);
                edgeCount++;
            } else if (verb == GPath::Verb::kCubic) {
                GPoint E0 = nextPts[0] + (2 * nextPts[1]) + nextPts[2];
                GPoint E1 = nextPts[1] + (2 * nextPts[2]) + nextPts[3];
                GPoint E = {(std::max(abs(E0.x()), abs(E1.x()))), std::max(abs(E0.y()), abs(E1.y()))};
                int n = (int)ceil(sqrt((3 * E.length())/(4 * 0.25f)));
                for (int i = 0; i < n; ++i) {
                    holder3.push_back(cubicHelper(nextPts, (float)i/n));
                    holder3.push_back(cubicHelper(nextPts, (float)(i + 1)/n));
                    edgeCount++;
                }
            } else if (verb == GPath::Verb::kQuad) {
                GPoint E = (nextPts[0] + (2 * nextPts[1]) + nextPts[2]);
                int n = (int)ceil(sqrt(E.length() / 0.25f));
                for (int i = 0; i < n; ++i) {
                    holder3.push_back(quadHelper(nextPts, (float)i/n));
                    holder3.push_back(quadHelper(nextPts, (float)(i + 1)/n));
                    edgeCount++;
                }
            }
        } while (verb != GPath::Verb::kDone);

        GPoint* points = holder3.data();

        matrix.mapPoints(points, points, 2 * edgeCount);

        GRect bounds = GRect::WH(fDevice.width(), fDevice.height());

        std::vector<Edge> holder4;
        holder4.resize(edgeCount * 3); 
        Edge* storage = holder4.data();

        // Edge storage[3 * edgeCount];
        Edge* edge = storage;

        for (int i = 0; i < edgeCount; ++i) {
            GPoint p0 = points[2 * i];
            GPoint p1 = points[2 * i + 1];

            edge = clipLine(p0, p1, bounds, edge);
        }

        // Recalculate edge count after clipping
        edgeCount = edge - storage;
        if (edgeCount == 0) {
            return;
        }

        GBlitter blitter = GBlitter(fDevice, paint);

        GScanConverter::scanComplex(storage, edgeCount, blitter);
    }

    /**
     * Fill the entire canvas with a particular paint.
     */
    void drawPaint(const GPaint& paint) override {
        // GColor color = paint.getColor().pinToUnit();
        // GPixel pixel = colorToPixel(color);

        // for (int y = 0; y < fDevice.height(); ++y) {
        //     for (int x = 0; x < fDevice.width(); ++x) {
        //         GPixel* addr = fDevice.getAddr(x, y);
        //         *addr = pixel;
        //     }
        // }

        GRect bounds = GRect::WH(fDevice.width(), fDevice.height());
        drawRect(bounds, paint);
    }

    /**
     * Draw a rectangular area by filling it with the provided paint.
     */
    void drawRect(const GRect& rect, const GPaint& paint) override {
        GPoint points[4] = {
            {rect.left(), rect.top()},
            {rect.right(), rect.top()},
            {rect.right(), rect.bottom()},
            {rect.left(), rect.bottom()}
        };

        drawConvexPolygon(points, 4, paint);
    }

    void restore() override {
        mCTMStack.pop();
    }

    void save() override {
        GMatrix current = mCTMStack.top();
        GMatrix copy(
            current[0], current[1], current[2],
            current[3], current[4], current[5]);

        mCTMStack.push(copy);
    }

private:
    const GBitmap fDevice;

    std::stack<GMatrix> mCTMStack;

    void drawRow(int y, int xLeft, int xRight, const GPaint& paint) {
        xLeft = std::max(0, xLeft);
        xRight = std::min(fDevice.width(), xRight);

        BlendProc blendProc = Blend_GetProc(paint.getBlendMode());

        GShader* shader = paint.getShader();
        if (shader == nullptr) {
            GColor color = paint.getColor().pinToUnit();
            GPixel source = colorToPixel(color);

            for (int x = xLeft; x < xRight; ++x) {
                GPixel* addr = fDevice.getAddr(x, y);
                *addr = blendProc(source, *addr);
            }
        } else {
            if (!shader->setContext(mCTMStack.top())) {
                return;
            }

            int count = xRight - xLeft;
            std::vector<GPixel> holder5;
            holder5.resize(count); 
            GPixel* shaded = holder5.data();
            //GPixel shaded[count];
            if (count > 0) {
                shader->shadeRow(xLeft, y, count, shaded);
            }

            for (int x = xLeft; x < xRight; ++x) {
                GPixel* addr = fDevice.getAddr(x, y);
                *addr = blendProc(shaded[x - xLeft], *addr);
            }
        }
    }
};


std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    if (!device.pixels()) {
        return nullptr;
    }

    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    // as fancy as you like
    // ...
    GPaint paint = {GColor::RGBA(1,1,1,1)};
    GPoint points [3] = {{0,0},{1,1},{3,3}};
    canvas->drawConvexPolygon(points, 3, paint);

    //canvas->drawPaint(paint);

    return "trouble in paradise";
}