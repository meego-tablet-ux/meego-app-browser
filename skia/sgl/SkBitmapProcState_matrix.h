
#define TRANSLATE_NOFILTER_NAME MAKENAME(_nofilter_translate)
#define SCALE_NOFILTER_NAME     MAKENAME(_nofilter_scale)
#define SCALE_FILTER_NAME       MAKENAME(_filter_scale)
#define AFFINE_NOFILTER_NAME    MAKENAME(_nofilter_affine)
#define AFFINE_FILTER_NAME      MAKENAME(_filter_affine)
#define PERSP_NOFILTER_NAME     MAKENAME(_nofilter_persp)
#define PERSP_FILTER_NAME       MAKENAME(_filter_persp)

#define PACK_FILTER_X_NAME  MAKENAME(_pack_filter_x)
#define PACK_FILTER_Y_NAME  MAKENAME(_pack_filter_y)

#ifndef PREAMBLE
    #define PREAMBLE(state)
    #define PREAMBLE_PARAM_X
    #define PREAMBLE_PARAM_Y
    #define PREAMBLE_ARG_X
    #define PREAMBLE_ARG_Y
#endif

#ifndef PREAMBLE_TRANS
    #define PREAMBLE_TRANS(state)
#endif

static void TRANSLATE_NOFILTER_NAME(const SkBitmapProcState& s,
                                    uint32_t xy[], int count, int x, int y)
{
    SkASSERT((s.fInvType & ~SkMatrix::kTranslate_Mask) == 0);

    PREAMBLE_TRANS(s);

    x += SkScalarFloor(s.fInvMatrix->getTranslateX());
    y += SkScalarFloor(s.fInvMatrix->getTranslateY());

    *xy++ = (uint32_t)TILEY_TRANS(y, (s.fBitmap->height() - 1));
    
    unsigned maxX = s.fBitmap->width() - 1;
    int i;
    uint16_t* xx = (uint16_t*)xy;
    for (i = (count >> 2); i > 0; --i)
    {
        *xx++ = (uint16_t)TILEX_TRANS(x, maxX); x++;
        *xx++ = (uint16_t)TILEX_TRANS(x, maxX); x++;
        *xx++ = (uint16_t)TILEX_TRANS(x, maxX); x++;
        *xx++ = (uint16_t)TILEX_TRANS(x, maxX); x++;
    }
    for (i = (count & 3); i > 0; --i)
    {
        *xx++ = (uint16_t)TILEX_TRANS(x, maxX); x++;
    }
}

static void SCALE_NOFILTER_NAME(const SkBitmapProcState& s,
                                uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);

    PREAMBLE(s);
    // we store y, x, x, x, x, x

    // invert x+half, y+half and convert to fixed
    SkFixed fx = s.fInvSy * y + s.fInvTyPlusHalf;
    *xy++ = TILEY_PROCF(fx, (s.fBitmap->height() - 1));
    // invert X
    SkFixed dx = s.fInvSx;
    fx = dx * x + s.fInvTxPlusHalf;
    unsigned maxX = s.fBitmap->width() - 1;

#ifdef CHECK_FOR_DECAL
    // test if we don't need to apply the tile proc
    if ((unsigned)(fx >> 16) <= maxX &&
        (unsigned)((fx + dx * (count - 1)) >> 16) <= maxX) {
        decal_nofilter_scale(xy, fx, dx, count);
    } else
#endif
    {
        int i;
#if 0
        uint16_t* xx = (uint16_t*)xy;
        for (i = (count >> 2); i > 0; --i) {
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
        }
        for (i = (count & 3); i > 0; --i) {
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
        }
#else
        for (i = (count >> 2); i > 0; --i) {
            unsigned a, b;
            a = TILEX_PROCF(fx, maxX); fx += dx;
            b = TILEX_PROCF(fx, maxX); fx += dx;
#ifdef SK_CPU_BENDIAN
            *xy++ = (a << 16) | b;
#else
            *xy++ = (b << 16) | a;
#endif
            a = TILEX_PROCF(fx, maxX); fx += dx;
            b = TILEX_PROCF(fx, maxX); fx += dx;
#ifdef SK_CPU_BENDIAN
            *xy++ = (a << 16) | b;
#else
            *xy++ = (b << 16) | a;
#endif
        }
        uint16_t* xx = (uint16_t*)xy;
        for (i = (count & 3); i > 0; --i) {
            *xx++ = TILEX_PROCF(fx, maxX); fx += dx;
        }
#endif
    }
}

// note: we could special-case on a matrix which is skewed in X but not Y.
// this would require a more general setup thatn SCALE does, but could use
// SCALE's inner loop that only looks at dx

static void AFFINE_NOFILTER_NAME(const SkBitmapProcState& s,
                                 uint32_t xy[], int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);
    
    PREAMBLE(s);
    SkPoint srcPt;
    s.fInvProc(*s.fInvMatrix,
               SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
    
    SkFixed fx = SkScalarToFixed(srcPt.fX);
    SkFixed fy = SkScalarToFixed(srcPt.fY);
    SkFixed dx = s.fInvSx;
    SkFixed dy = s.fInvKy;
    int maxX = s.fBitmap->width() - 1;
    int maxY = s.fBitmap->height() - 1;
    
    for (int i = count; i > 0; --i) {
        *xy++ = (TILEY_PROCF(fy, maxY) << 16) | TILEX_PROCF(fx, maxX);
        fx += dx; fy += dy;
    }
}

static void PERSP_NOFILTER_NAME(const SkBitmapProcState& s,
                                uint32_t* SK_RESTRICT xy,
                                int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kPerspective_Mask);
    
    PREAMBLE(s);
    int maxX = s.fBitmap->width() - 1;
    int maxY = s.fBitmap->height() - 1;
    
    SkPerspIter   iter(*s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf, count);
    
    while ((count = iter.next()) != 0) {
        const SkFixed* SK_RESTRICT srcXY = iter.getXY();
        while (--count >= 0) {
            *xy++ = (TILEY_PROCF(srcXY[1], maxY) << 16) |
                     TILEX_PROCF(srcXY[0], maxX);
            srcXY += 2;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

static inline uint32_t PACK_FILTER_Y_NAME(SkFixed f, unsigned max,
                                          SkFixed one PREAMBLE_PARAM_Y) {
    unsigned i = TILEY_PROCF(f, max);
    i = (i << 4) | TILEY_LOW_BITS(f, max);
    return (i << 14) | (TILEY_PROCF((f + one), max));
}

static inline uint32_t PACK_FILTER_X_NAME(SkFixed f, unsigned max,
                                          SkFixed one PREAMBLE_PARAM_X) {
    unsigned i = TILEX_PROCF(f, max);
    i = (i << 4) | TILEX_LOW_BITS(f, max);
    return (i << 14) | (TILEX_PROCF((f + one), max));
}

static void SCALE_FILTER_NAME(const SkBitmapProcState& s,
                              uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);

    PREAMBLE(s);
    // compute our two Y values up front
    {
        unsigned maxY = s.fBitmap->height() - 1;
        SkFixed fy = s.fInvSy * y + s.fInvTyPlusHalf - (s.fFilterOneY >> 1);
        *xy++ = PACK_FILTER_Y_NAME(fy, maxY, s.fFilterOneY PREAMBLE_ARG_Y);
    }

    unsigned maxX = s.fBitmap->width() - 1;
    SkFixed one = s.fFilterOneX;
    SkFixed dx = s.fInvSx;
    SkFixed fx = dx * x + s.fInvTxPlusHalf - (one >> 1);

#ifdef CHECK_FOR_DECAL
    // test if we don't need to apply the tile proc
    if (dx > 0 &&
            (unsigned)(fx >> 16) <= maxX &&
            (unsigned)((fx + dx * (count - 1)) >> 16) < maxX) {
        decal_filter_scale(xy, fx, dx, count);
    } else
#endif
    {
        do {
            *xy++ = PACK_FILTER_X_NAME(fx, maxX, one PREAMBLE_ARG_X);
            fx += dx;
        } while (--count != 0);
    }
}

static void AFFINE_FILTER_NAME(const SkBitmapProcState& s,
                               uint32_t xy[], int count, int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kAffine_Mask);
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask |
                             SkMatrix::kScale_Mask |
                             SkMatrix::kAffine_Mask)) == 0);
    
    PREAMBLE(s);
    SkPoint srcPt;
    s.fInvProc(*s.fInvMatrix,
               SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &srcPt);
    
    SkFixed oneX = s.fFilterOneX;
    SkFixed oneY = s.fFilterOneY;
    SkFixed fx = SkScalarToFixed(srcPt.fX) - (oneX >> 1);
    SkFixed fy = SkScalarToFixed(srcPt.fY) - (oneY >> 1);
    SkFixed dx = s.fInvSx;
    SkFixed dy = s.fInvKy;
    unsigned maxX = s.fBitmap->width() - 1;
    unsigned maxY = s.fBitmap->height() - 1;
    
    do {
        *xy++ = PACK_FILTER_Y_NAME(fy, maxY, oneY PREAMBLE_ARG_Y);
        fy += dy;
        *xy++ = PACK_FILTER_X_NAME(fx, maxX, oneX PREAMBLE_ARG_X);
        fx += dx;
    } while (--count != 0);
}

static void PERSP_FILTER_NAME(const SkBitmapProcState& s,
                              uint32_t* SK_RESTRICT xy, int count,
                              int x, int y) {
    SkASSERT(s.fInvType & SkMatrix::kPerspective_Mask);
    
    PREAMBLE(s);
    unsigned maxX = s.fBitmap->width() - 1;
    unsigned maxY = s.fBitmap->height() - 1;
    SkFixed oneX = s.fFilterOneX;
    SkFixed oneY = s.fFilterOneY;
    
    SkPerspIter   iter(*s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf, count);
    
    while ((count = iter.next()) != 0) {
        const SkFixed* SK_RESTRICT srcXY = iter.getXY();
        do {
            *xy++ = PACK_FILTER_Y_NAME(srcXY[1] - (oneY >> 1), maxY,
                                       oneY PREAMBLE_ARG_Y);
            *xy++ = PACK_FILTER_X_NAME(srcXY[0] - (oneX >> 1), maxX,
                                       oneX PREAMBLE_ARG_X);
            srcXY += 2;
        } while (--count != 0);
    }
}

static SkBitmapProcState::MatrixProc MAKENAME(_Procs)[] = {
    TRANSLATE_NOFILTER_NAME,
    TRANSLATE_NOFILTER_NAME,    // No need to do filtering if the matrix is no
                                // more complex than identity/translate.
    SCALE_NOFILTER_NAME,
    SCALE_FILTER_NAME,
    AFFINE_NOFILTER_NAME,
    AFFINE_FILTER_NAME,
    PERSP_NOFILTER_NAME,
    PERSP_FILTER_NAME
};

#undef MAKENAME
#undef TILEX_PROCF
#undef TILEY_PROCF
#ifdef CHECK_FOR_DECAL
    #undef CHECK_FOR_DECAL
#endif

#undef TILEX_TRANS
#undef TILEY_TRANS

#undef TRANSLATE_NOFILTER_NAME
#undef SCALE_NOFILTER_NAME
#undef SCALE_FILTER_NAME
#undef AFFINE_NOFILTER_NAME
#undef AFFINE_FILTER_NAME
#undef PERSP_NOFILTER_NAME
#undef PERSP_FILTER_NAME

#undef PREAMBLE
#undef PREAMBLE_PARAM_X
#undef PREAMBLE_PARAM_Y
#undef PREAMBLE_ARG_X
#undef PREAMBLE_ARG_Y

#undef PREAMBLE_TRANS

#undef TILEX_LOW_BITS
#undef TILEY_LOW_BITS
