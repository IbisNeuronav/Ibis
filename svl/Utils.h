/*
    File:           Utils.h

    Function:       Various math definitions for VL

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __VL_MATH__
#define __VL_MATH__

#include <cstdlib>

// --- Inlines ----------------------------------------------------------------

// additions to arithmetic functions

#ifdef VL_HAS_IEEEFP
#include <ieeefp.h>
#define vl_is_finite(X) finite(X)
#elif defined (__GNUC__) && defined(__USE_MISC)
#define vl_is_finite(X) finite(X)
#else
#define vl_is_finite(X) (1)
#endif

#ifdef VL_HAS_DRAND
inline Double vl_rand()
{ return(drand48()); }
#else
#ifndef RAND_MAX
// we're on something totally sucky, like SunOS
#define RAND_MAX (Double(1 << 30) * 4.0 - 1.0)
#endif
inline Double vl_rand()
{ return(rand() / (RAND_MAX + 1.0)); }
#endif

#ifdef VL_HAS_ABSF
inline Float len(Float x)
{ return (fabsf(x)); }
#endif
inline Double len(Double x)
{ return (fabs(x)); }

inline Float sqrlen(Float r)
{ return(sqr(r)); }
inline Double sqrlen(Double r)
{ return(sqr(r)); }

inline Float mix(Float a, Float b, Float s)
{ return((1.0f - s) * a + s * b); }
inline Double mix(Double a, Double b, Double s)
{ return((1.0 - s) * a + s * b); }

inline Double sign(Double d)
{
    if (d < 0)
        return(-1.0);
    else
        return(1.0);
}

// useful routines

inline Bool IsPowerOfTwo(Int a)
{ return((a & -a) == a); };

inline Void SetReal(Float &a, Double b)
{ a = Float(b); }
inline Void SetReal(Double &a, Double b)
{ a = b; }

#endif
