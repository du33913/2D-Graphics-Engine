#include "GPaint.h"
#include "GPixel.h"

#include "Blend.h"


uint64_t expand(uint32_t x) {
    uint64_t hi = x & 0xFF00FF00;  // the A and G components
    uint64_t lo = x & 0x00FF00FF;  // the R and B components
    return (hi << 24) | lo;
}

// turn 0xXX into 0x00XX00XX00XX00XX
uint64_t replicate(uint64_t x) {
    return (x << 48) | (x << 32) | (x << 16) | x;
}

// turn 0x..AA..CC..BB..DD into 0xAABBCCDD
uint32_t compact(uint64_t x) {
    return ((x >> 24) & 0xFF00FF00) | (x & 0xFF00FF);
}

uint32_t quad_mul_div255(uint32_t x, uint8_t invA) {
    uint64_t prod = expand(x) * invA;
    prod += replicate(128);			
    prod += (prod >> 8) & replicate(0xFF);
    prod >>= 8;
    return compact(prod);
}

unsigned div255(unsigned x) {
    x += 128;
    return x + (x >> 8) >> 8;
}


GPixel Blend_Clear(const GPixel source, const GPixel dest) {
    return GPixel_PackARGB(0, 0, 0, 0);
}


GPixel Blend_Src(const GPixel source, const GPixel dest) {
    return source;
}


GPixel Blend_Dst(const GPixel source, const GPixel dest) {
    return dest;
}


GPixel Blend_SrcOver(const GPixel source, const GPixel dest) {
    int sAlpha = GPixel_GetA(source);
    
    return source + quad_mul_div255(dest, 255 - sAlpha);
}


GPixel Blend_DstOver(const GPixel source, const GPixel dest) {
    int dAlpha = GPixel_GetA(dest);
    
    return dest + quad_mul_div255(source, 255 - dAlpha);
}


GPixel Blend_SrcIn(const GPixel source, const GPixel dest) {
    int dAlpha = GPixel_GetA(dest);
    
    return quad_mul_div255(source, dAlpha);
}


GPixel Blend_DstIn(const GPixel source, const GPixel dest) {
    int sAlpha = GPixel_GetA(source);
   
    return quad_mul_div255(dest, sAlpha);
}


GPixel Blend_SrcOut(const GPixel source, const GPixel dest) {
    int dAlpha = GPixel_GetA(dest);
    
    return quad_mul_div255(source, 255 - dAlpha);
}

GPixel Blend_DstOut(const GPixel source, const GPixel dest) {
    int sAlpha = GPixel_GetA(source);

    return quad_mul_div255(dest, 255 - sAlpha);
}


GPixel Blend_SrcATop(const GPixel source, const GPixel dest) {
    int sAlpha = GPixel_GetA(source);
    int sRed = GPixel_GetR(source);
    int sGreen = GPixel_GetG(source);
    int sBlue = GPixel_GetB(source);

    int dAlpha = GPixel_GetA(dest);
    int dRed = GPixel_GetR(dest);
    int dGreen = GPixel_GetG(dest);
    int dBlue = GPixel_GetB(dest);

    unsigned inv = 255 - sAlpha;

    int alpha = dAlpha;
    int red = div255(dAlpha * sRed) + div255(inv * dRed);
    int green = div255(dAlpha * sGreen) + div255(inv * dGreen);
    int blue = div255(dAlpha * sBlue) + div255(inv * dBlue);

    return GPixel_PackARGB(alpha, red, green, blue);
}


  GPixel Blend_DstATop(const GPixel source, const GPixel dest) {
    int sAlpha = GPixel_GetA(source);
    int sRed = GPixel_GetR(source);
    int sGreen = GPixel_GetG(source);
    int sBlue = GPixel_GetB(source);

    int dAlpha = GPixel_GetA(dest);
    int dRed = GPixel_GetR(dest);
    int dGreen = GPixel_GetG(dest);
    int dBlue = GPixel_GetB(dest);

    unsigned inv = 255 - dAlpha;

    int alpha = sAlpha;
    int red = div255(sAlpha * dRed) + div255(inv * sRed);
    int green = div255(sAlpha * dGreen) + div255(inv * sGreen);
    int blue = div255(sAlpha * dBlue) + div255(inv * sBlue);

    return GPixel_PackARGB(alpha, red, green, blue);
}


GPixel Blend_Xor(const GPixel source, const GPixel dest) {
    int sAlpha = GPixel_GetA(source);
    int sRed = GPixel_GetR(source);
    int sGreen = GPixel_GetG(source);
    int sBlue = GPixel_GetB(source);

    int dAlpha = GPixel_GetA(dest);
    int dRed = GPixel_GetR(dest);
    int dGreen = GPixel_GetG(dest);
    int dBlue = GPixel_GetB(dest);

    unsigned invS = 255 - sAlpha;
    unsigned invD = 255 - dAlpha;

    int alpha = div255(sAlpha * (255 - dAlpha) + dAlpha * (255 - sAlpha));

    //int alpha = sAlpha + dAlpha - 2 * div255(sAlpha * dAlpha);
    int red = div255((invD * sRed) + (invS * dRed));
    int green = div255((invD * sGreen) + (invS * dGreen));
    int blue = div255((invD * sBlue) + (invS * dBlue));
    return GPixel_PackARGB(alpha, red, green, blue);
}


typedef GPixel (*BlendProc)(GPixel, GPixel);


// If this matches the order of the GBlendMode enum, we can use the enum values
// as indices for this array.
BlendProc BLEND_PROCS[] = {
    Blend_Clear,
    Blend_Src,
    Blend_Dst,
    Blend_SrcOver,
    Blend_DstOver,
    Blend_SrcIn,
    Blend_DstIn,
    Blend_SrcOut,
    Blend_DstOut,
    Blend_SrcATop,
    Blend_DstATop,
    Blend_Xor,
};


BlendProc Blend_GetProc(const GBlendMode mode) {
    return Blend_PROCS[static_cast<int>(mode)];
}


/**
 * Since we are provided the source pixel in this case, we can make some
 * optimizations if desired. For now we just take the naive approach and return
 * the function corresponding to the given mode.
 */
BlendProc Blend_GetProc(const GBlendMode mode, const GPixel src) {
    return Blend_GetProc(mode);
}
