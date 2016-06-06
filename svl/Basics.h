/*
    File:           Basics.h

    Function:       Basic definitions for all files. Contains type
                    definitions, assertion and debugging facilities, and
                    miscellaneous useful template functions.

                    This is a cut-down version for SVL.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

    Notes:          This header is affected by the following defines:

                    VL_CHECKING     - Include code for assertions,
                                      range errors and warnings.
                    VL_FLOAT        - Use floats for real numbers. (Doubles
                                      are the default.)
                    VL_NO_BOOL      - There is no bool type.
                    VL_NO_TF        - true and false are not predefined.
*/

#ifndef __Basics__
#define __Basics__

#include "SVLConfig.h"
#include <iostream>
#include <cmath>


// --- Basic types -------------------------------------------------------------

typedef void            Void;
typedef float           Float;
typedef double          Double;
typedef char            Char;
typedef int             Short;
typedef int             Int;
typedef long            Long;
typedef unsigned char   Byte;
typedef unsigned int    UInt;

#ifndef VL_FLOAT
typedef Double          Real;
#else
typedef Float           Real;
#endif

#define SELF (*this)    // A syntactic convenience.


// --- Boolean type ------------------------------------------------------------

// X11 #defines 'Bool' -- typical.

#ifdef Bool
#undef Bool
#endif

#ifndef VL_NO_BOOL
// if the compiler implements the bool type...
typedef bool Bool;
#else
// if not, make up our own.
class Bool
{
public:

    Bool() : val(0) {};
    Bool(Int b) : val(b) {};

    operator Int() { return val; };

private:
    Int val;
};
#ifdef VL_NO_TF
enum {false, true};
#endif
#endif


// --- Assertions and Range checking -------------------------------------------

#define _Error(e)               _Assert(false, e, __FILE__, __LINE__)
#define _Warning(w)             _Expect(false, w, __FILE__, __LINE__)

#if defined(VL_CHECKING)
#define Assert(b, e)            _Assert(b, e, __FILE__, __LINE__)
    // Assert that b is true. e is an error message to be printed if b
    // is false.
#define Expect(b, w)            _Expect(b, w, __FILE__, __LINE__)
    // Prints warning w if b is false
#define CheckRange(i, l, u, r)  _CheckRange(i, l, u, r, __FILE__, __LINE__)
    // Checks whether i is in the range [lowerBound, upperBound).
#else
#define Assert(b, e)
#define Expect(b, w)
#define CheckRange(a, l, u, r)
#endif

Void _Assert(Int condition, const Char *errorMessage, const Char *file, Int line);
Void _Expect(Int condition, const Char *warningMessage, const Char *file, Int line);
Void _CheckRange(Int i, Int lowerBound, Int upperBound, const Char *rangeMessage,
        const Char *file, Int line);


// --- Inlines -----------------------------------------------------------------

template<class Value>
    inline Value Min(Value x, Value y)
    {
        if (x <= y)
            return(x);
        else
            return(y);
    };

template<class Value>
    inline Value Max(Value x, Value y)
    {
        if (x >= y)
            return(x);
        else
            return(y);
    };

template<class Value>
    inline Void Swap(Value &x, Value &y)
    {
        Value t;

        t = x;
        x = y;
        y = t;
    };

template<class Value>
    inline Value Mix(Value x, Value y, Real s)
    {
        return(x + (y - x) * s);
    };

template<class Value>
    inline Value Clip(Value x, Value min, Value max)
    {
        if (x < min)
            return(min);
        else if (x > max)
            return(max);
        else
            return(x);
    };

template<class Value>
    inline Value sqr(Value x)
    {
        return(x * x);
    };

#endif
