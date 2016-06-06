/*
    File:           Vec.cpp

    Function:       Implements Vec.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/


#include "Vec.h"

#include <cctype>
#include <cstring>
#include <cstdarg>
#include <iomanip>


// --- Vec Constructors -------------------------------------------------------


Vec::Vec(Int n, ZeroOrOne k) : elts(n)
{
    Assert(n > 0,"(Vec) illegal vector size");

    data = new Real[n];

    MakeBlock(k);
}

Vec::Vec(Int n, Axis a) : elts(n)
{
    Assert(n > 0,"(Vec) illegal vector size");

    data = new Real[n];

    MakeUnit(a);
}

Vec::Vec(const Vec &v)
{
    Assert(v.data != 0, "(Vec) Can't construct from a null vector");

    elts = v.Elts();
    data = new Real[elts];

#ifdef VL_USE_MEMCPY
    memcpy(data, v.Ref(), sizeof(Real) * Elts());
#else
    for (Int i = 0; i < Elts(); i++)
        data[i] = v[i];
#endif
}

Vec::Vec(const Vec2 &v) : data(v.Ref()), elts(v.Elts() | VL_REF_FLAG)
{
}

Vec::Vec(const Vec3 &v) : data(v.Ref()), elts(v.Elts() | VL_REF_FLAG)
{
}

Vec::Vec(const Vec4 &v) : data(v.Ref()), elts(v.Elts() | VL_REF_FLAG)
{
}

Vec::Vec(Int n, double elt0, ...) : elts(n)
{
    Assert(n > 0,"(Vec) illegal vector size");

    va_list ap;
    Int     i = 1;

    data = new Real[n];
    va_start(ap, elt0);

    SetReal(data[0], elt0);

    while (--n)
        SetReal(data[i++], va_arg(ap, double));

    va_end(ap);
}

Vec::~Vec()
{
    Assert(elts != 0,"(Vec) illegal vector size");

    if (!IsRef())
        delete[] data;
}


// --- Vec Assignment Operators -----------------------------------------------


Vec &Vec::operator = (const Vec &v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else
        Assert(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");

#ifdef VL_USE_MEMCPY
    memcpy(data, v.data, sizeof(Real) * Elts());
#else
    for (Int i = 0; i < Elts(); i++)
        data[i] = v[i];
#endif

    return(SELF);
}

Vec &Vec::operator = (const Vec2 &v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else
        Assert(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");

    data[0] = v[0];
    data[1] = v[1];

    return(SELF);
}

Vec &Vec::operator = (const Vec3 &v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else
        Assert(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");

    data[0] = v[0];
    data[1] = v[1];
    data[2] = v[2];

    return(SELF);
}

Vec &Vec::operator = (const Vec4 &v)
{
    if (!IsRef())
        SetSize(v.Elts());
    else
        Assert(Elts() == v.Elts(), "(Vec::=) Vector sizes don't match");

    data[0] = v[0];
    data[1] = v[1];
    data[2] = v[2];
    data[3] = v[3];

    return(SELF);
}

Void Vec::SetSize(Int ni)
{
    Assert(ni > 0, "(Vec::SetSize) Illegal vector size");
    UInt    n = UInt(ni);
    
    if (!IsRef())
    {
        // Don't reallocate if we already have enough storage

        if (n <= elts)
        {
            elts = n;
            return;
        }

        // Otherwise, delete old storage

        delete[] data;

        elts = n;
        data = new Real[elts];
    }
    else
        Assert(false, "(Vec::SetSize) Can't resize a vector reference");
}

Vec &Vec::MakeZero()
{
#ifdef VL_USE_MEMCPY
    memset(data, 0, sizeof(Real) * Elts());
#else
    Int j;

    for (j = 0; j < Elts(); j++)
        data[j] = vl_zero;
#endif

    return(SELF);
}

Vec &Vec::MakeUnit(Int i, Real k)
{
    MakeZero();
    data[i] = k;

    return(SELF);
}

Vec &Vec::MakeBlock(Real k)
{
    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] = k;

    return(SELF);
}


// --- Vec In-Place operators -------------------------------------------------


Vec &Vec::operator += (const Vec &b)
{
    Assert(Elts() == b.Elts(), "(Vec::+=) vector sizes don't match");

    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] += b[i];

    return(SELF);
}

Vec &Vec::operator -= (const Vec &b)
{
    Assert(Elts() == b.Elts(), "(Vec::-=) vector sizes don't match");

    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] -= b[i];

    return(SELF);
}

Vec &Vec::operator *= (const Vec &b)
{
    Assert(Elts() == b.Elts(), "(Vec::*=) Vec sizes don't match");

    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] *= b[i];

    return(SELF);
}

Vec &Vec::operator *= (Real s)
{
    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] *= s;

    return(SELF);
}

Vec &Vec::operator /= (const Vec &b)
{
    Assert(Elts() == b.Elts(), "(Vec::/=) Vec sizes don't match");

    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] /= b[i];

    return(SELF);
}

Vec &Vec::operator /= (Real s)
{
    Int     i;

    for (i = 0; i < Elts(); i++)
        data[i] /= s;

    return(SELF);
}


// --- Vec Comparison Operators -----------------------------------------------


Bool operator == (const Vec &a, const Vec &b)
{
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        if (a[i] != b[i])
            return(0);

    return(1);
}

Bool operator != (const Vec &a, const Vec &b)
{
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        if (a[i] != b[i])
            return(1);

    return(0);
}


// --- Vec Arithmetic Operators -----------------------------------------------


Vec operator + (const Vec &a, const Vec &b)
{
    Assert(a.Elts() == b.Elts(), "(Vec::+) Vec sizes don't match");

    Vec     result(a.Elts());
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        result[i] = a[i] + b[i];

    return(result);
}

Vec operator - (const Vec &a, const Vec &b)
{
    Assert(a.Elts() == b.Elts(), "(Vec::-) Vec sizes don't match");

    Vec     result(a.Elts());
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        result[i] = a[i] - b[i];

    return(result);
}

Vec operator - (const Vec &v)
{
    Vec     result(v.Elts());
    Int     i;

    for (i = 0; i < v.Elts(); i++)
        result[i] = - v[i];

    return(result);
}

Vec operator * (const Vec &a, const Vec &b)
{
    Assert(a.Elts() == b.Elts(), "(Vec::*) Vec sizes don't match");

    Vec     result(a.Elts());
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        result[i] = a[i] * b[i];

    return(result);
}

Vec operator * (const Vec &v, Real s)
{
    Vec     result(v.Elts());
    Int     i;

    for (i = 0; i < v.Elts(); i++)
        result[i] = v[i] * s;

    return(result);
}

Vec operator / (const Vec &a, const Vec &b)
{
    Assert(a.Elts() == b.Elts(), "(Vec::/) Vec sizes don't match");

    Vec     result(a.Elts());
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        result[i] = a[i] / b[i];

    return(result);
}

Vec operator / (const Vec &v, Real s)
{
    Vec     result(v.Elts());
    Int     i;

    for (i = 0; i < v.Elts(); i++)
        result[i] = v[i] / s;

    return(result);
}

Real dot(const Vec &a, const Vec &b)
{
    Assert(a.Elts() == b.Elts(), "(Vec::dot) Vec sizes don't match");

    Real    sum = vl_zero;
    Int     i;

    for (i = 0; i < a.Elts(); i++)
        sum += a[i] * b[i];

    return(sum);
}

Vec operator * (Real s, const Vec &v)
{
    Vec     result(v.Elts());
    Int     i;

    for (i = 0; i < v.Elts(); i++)
        result[i] = v[i] * s;

    return(result);
}

Vec &Vec::Clamp(Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    Int i;

    for (i = 0; i < Elts(); i++)
        if (len(SELF[i]) < fuzz)
            SELF[i] = vl_zero;

    return(SELF);
}

Vec &Vec::Clamp()
{
    return(Clamp(1e-7));
}

Vec clamped(const Vec &v, Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    Vec result(v);

    return(result.Clamp(fuzz));
}

Vec clamped(const Vec &v)
{
    return(clamped(v, 1e-7));
}


// --- Vec Input & Output -----------------------------------------------------


ostream &operator << (ostream &s, const Vec &v)
{
    Int i, w;

    s << '[';

    if (v.Elts() > 0)
    {
        w = s.width();
        s << v[0];

        for (i = 1; i < v.Elts(); i++)
            s << ' ' << setw(w) << v[i];
    }

    s << ']';

    return(s);
}

inline Void CopyPartialVec(const Vec &u, Vec &v, Int numElts)
{
    for (Int i = 0; i < numElts; i++)
        v[i] = u[i];
}

istream &operator >> (istream &s, Vec &v)
{
    Int     size = 0;
    Vec     inVec(16);
    Char    c;

    //  Expected format: [a b c d ...]

    while (isspace(s.peek()))           //  chomp white space
        s.get(c);

    if (s.peek() == '[')
    {
        s.get(c);


        while (isspace(s.peek()))       //  chomp white space
            s.get(c);

        while (s.peek() != ']')         //  resize if needed
        {
            if (size == inVec.Elts())
            {
                Vec     holdVec(inVec);

                inVec.SetSize(size * 2);
                CopyPartialVec(holdVec, inVec, size);
            }

            s >> inVec[size++];         //  read an item

            if (!s)
            {
                _Warning("Couldn't read vector element");
                return(s);
            }

            while (isspace(s.peek()))   //  chomp white space
                s.get(c);
        }
        s.get(c);
    }
    else
    {
        s.clear(ios::failbit);
        _Warning("Error: Expected '[' while reading vector");
        return(s);
    }

    v.SetSize(size);
    CopyPartialVec(inVec, v, size);

    return(s);
}
