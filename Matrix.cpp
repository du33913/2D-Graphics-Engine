#include <math.h>

#include "GMatrix.h"
#include "GPoint.h"


GMatrix::GMatrix() {
    this->fMat[0] = 1;
    this->fMat[1] = 0;
    this->fMat[2] = 0;
    this->fMat[3] = 0;
    this->fMat[4] = 1;
    this->fMat[5] = 0;
}


GMatrix GMatrix::Translate(float tx, float ty) {
    return GMatrix(1, 0, tx, 0, 1, ty);
}


GMatrix GMatrix::Scale(float sx, float sy) {
    return GMatrix(sx, 0, 0, 0, sy, 0);
}

GMatrix GMatrix::Rotate(float radians) {
    return GMatrix(cos(radians), -sin(radians), 0, sin(radians), cos(radians), 0);
}


GMatrix GMatrix::Concat(const GMatrix& secundo, const GMatrix& primo) {
    return GMatrix(
        primo[0] * secundo[0] + primo[3] * secundo[1],
        primo[1] * secundo[0] + primo[4] * secundo[1],
        primo[2] * secundo[0] + primo[5] * secundo[1] + secundo[2],
        primo[0] * secundo[3] + primo[3] * secundo[4],
        primo[1] * secundo[3] + primo[4] * secundo[4],
        primo[2] * secundo[3] + primo[5] * secundo[4] + secundo[5]);
}


bool GMatrix::invert(GMatrix* inverse) const {
    float a = this->fMat[0];
    float b = this->fMat[1];
    float c = this->fMat[2];
    float d = this->fMat[3];
    float e = this->fMat[4];
    float f = this->fMat[5];

    float determinant = a * e - b * d;
    if (determinant == 0) {
        return false;
    }

    float divisor = 1 / determinant;

    inverse->fMat[0] = e * divisor;
    inverse->fMat[1] = -b * divisor;
    inverse->fMat[2] = -(c * e - b * f) * divisor;
    inverse->fMat[3] = -d * divisor;
    inverse->fMat[4] = a * divisor;
    inverse->fMat[5] = (c * d - a * f) * divisor;

    return true;
}


void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
    for (int i = 0; i < count; ++i) {
        GPoint point = src[i];

        float x0 = point.x();
        float y0 = point.y();

        float x = this->fMat[0] * x0 + this->fMat[1] * y0 + this->fMat[2];
        float y = this->fMat[3] * x0 + this->fMat[4] * y0 + this->fMat[5];

        dst[i] = {x, y};
    }
}
