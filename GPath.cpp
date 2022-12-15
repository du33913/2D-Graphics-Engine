#include "GMatrix.h"
#include "GPath.h"
#include "GPoint.h"
#include "GRect.h"

#include "MathUtils.h"

GPoint quadHelper(const GPoint src[], float t) {
    return (src[0] * (1.0f - t) * (1.0f - t)) + (2 * t * src[1] * (1.0f - t)) + (src[2] * t * t);
}

GPoint cubicHelper(const GPoint src[], float t) {
    return (src[0] * (1.0f - t) * (1.0f - t) * (1.0f - t)) + (3 * t * src[1] * (1.0f - t) * (1.0f - t)) + (3 * t * t * src[2] * (1.0f - t)) + (src[3] * t * t * t);
}

void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t) {
    dst[0] = src[0];
    dst[1] = ((1.0f - t) * src[0]) + (t * src[1]);
    dst[5] = ((1.0f - t) * src[2]) + (t * src[3]);
    dst[6] = src[3];
    GPoint middle = ((1.0f - t) * src[1]) + (t * src[2]);
    dst[2] = ((1.0f - t) * dst[1]) + (t * middle);
    dst[4] = ((1.0f - t) * middle) + (t * dst[5]);
    dst[3] = ((1.0f - t) * dst[2]) + (t * dst[4]);
}


void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t) {
   dst[0] = src[0];
   dst[1] = ((1.0f - t) * src[0]) + (t * src[1]);
   dst[3] = ((1.0f - t) * src[1]) + (t * src[2]);
   dst[4] = src[2];
   dst[2] = ((1.0f - t) * dst[1]) + (t * dst[3]);
 }


GPath& GPath::addCircle(GPoint center, float radius, Direction dir) {
    float a = 1.00005519;
    float b = 0.55342686;
    float c = 0.99873585;

    GMatrix m = GMatrix::Translate(center.x(), center.y()) * GMatrix::Scale(radius, radius);

    if (dir == Direction::kCW_Direction) {
        this->moveTo(m * GPoint{a, 0});
        this->cubicTo(m * GPoint{c, -b}, m * GPoint{b, -c}, m * GPoint{0, -a});
        this->cubicTo(m * GPoint{-b, -c}, m * GPoint{-c, -b}, m * GPoint{-a, 0});
        this->cubicTo(m * GPoint{-c, b}, m * GPoint{-b, c}, m * GPoint{0, a});
        this->cubicTo(m * GPoint{b, c}, m * GPoint{c, b}, m * GPoint{a, 0});
    } else {
        this->moveTo(m * GPoint{a, 0});
        this->cubicTo(m * GPoint{c, b}, m * GPoint{b, c}, m * GPoint{0, a});
        this->cubicTo(m * GPoint{-b, c}, m * GPoint{-c, b}, m * GPoint{-a, 0});
        this->cubicTo(m * GPoint{-c, -b}, m * GPoint{-b, -c}, m * GPoint{0, -a});
        this->cubicTo(m * GPoint{b, -c}, m * GPoint{c, -b}, m * GPoint{a, 0});
    }
    
    return *this;
   
    // GRect rect = GRect::LTRB(
    //     center.x() - radius / 2,
    //     center.y() - radius / 2,
    //     center.x() + radius / 2,
    //     center.y() + radius/ 2);

    // return this->addRect(rect, dir);
}


GPath& GPath::addPolygon(const GPoint pts[], int count) {
    if (count <= 1) { return *this; }

    this->moveTo(pts[0]);
    for (int i = 1; i < count; ++i) {
        this->lineTo(pts[i]);
    }

    return *this;
}


GPath& GPath::addRect(const GRect& rect, Direction dir) {
    this->moveTo({rect.left(), rect.top()});

    if (dir == Direction::kCW_Direction) {
        this->lineTo({rect.right(), rect.top()});
        this->lineTo({rect.right(), rect.bottom()});
        this->lineTo({rect.left(), rect.bottom()});
    } else {
        this->lineTo({rect.left(), rect.bottom()});
        this->lineTo({rect.right(), rect.bottom()});
        this->lineTo({rect.right(), rect.top()});
    }

    return *this;
}


GRect GPath::bounds() const {
    int count = this->fPts.size();

    if (count == 0) {
        return GRect::WH(0, 0);
    }

    std::vector<float> holder1;
    holder1.resize(count); 
    float* xVals = holder1.data();

    std::vector<float> holder2;
    holder2.resize(count); 
    float* yVals = holder1.data();

    // float xVals[count], yVals[count];
    for (int i = 0; i < count; ++i) {
        xVals[i] = fPts[i].fX;
        yVals[i] = fPts[i].fY;
    }

    return GRect::LTRB(
        manyMin(xVals, count),
        manyMin(yVals, count),
        manyMax(xVals, count),
        manyMax(yVals, count));
}


void GPath::transform(const GMatrix& matrix) {
    matrix.mapPoints(this->fPts.data(), this->fPts.data(), this->fPts.size());
}
