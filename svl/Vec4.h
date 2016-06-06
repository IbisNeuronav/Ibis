/*
    File:           Vec4.h

    Function:       Defines a length-4 vector.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Vec4__
#define __Vec4__

#include "Vec3.h"


// --- Vec4 Class -------------------------------------------------------------

class Vec4
{
public:

    // Constructors

                Vec4();
                Vec4(Real x, Real y, Real z, Real w);   // [x, y, z, w]
                Vec4(const Vec4 &v);                    // Copy constructor
                Vec4(const Vec3 &v, Real w);            // Hom. 3D vector
                Vec4(ZeroOrOne k);
                Vec4(Axis k);

    // Accessor functions

    Int         Elts() const { return(4); };

    Real        &operator [] (Int i);
    const Real  &operator [] (Int i) const;

    Real        *Ref() const;                   // Return pointer to data

    // Assignment operators

    Vec4        &operator =  (const Vec4 &a);
    Vec4        &operator =  (ZeroOrOne k);
    Vec4        &operator =  (Axis k);
    Vec4        &operator += (const Vec4 &a);
    Vec4        &operator -= (const Vec4 &a);
    Vec4        &operator *= (const Vec4 &a);
    Vec4        &operator *= (Real s);
    Vec4        &operator /= (const Vec4 &a);
    Vec4        &operator /= (Real s);

    // Comparison operators

    Bool        operator == (const Vec4 &a) const;  // v == a ?
    Bool        operator != (const Vec4 &a) const;  // v != a ?

    // Arithmetic operators

    Vec4        operator + (const Vec4 &a) const;   // v + a
    Vec4        operator - (const Vec4 &a) const;   // v - a
    Vec4        operator - () const;                // -v
    Vec4        operator * (const Vec4 &a) const;   // v * a (vx * ax, ...)
    Vec4        operator * (Real s) const;          // v * s
    Vec4        operator / (const Vec4 &a) const;   // v / a (vx / ax, ...)
    Vec4        operator / (Real s) const;          // v / s


    // Initialisers

    Vec4        &MakeZero();                        // Zero vector
    Vec4        &MakeUnit(Int i, Real k = vl_one);  // kI[i]
    Vec4        &MakeBlock(Real k = vl_one);        // All-k vector

    Vec4        &Normalise();                       // normalise vector

    // Private...

protected:

    Real        elt[4];
};


// --- Vec operators ----------------------------------------------------------

inline Vec4     operator * (Real s, const Vec4 &v); // Left mult. by s
inline Real     dot(const Vec4 &a, const Vec4 &b);  // v . a
inline Real     len(const Vec4 &v);                 // || v ||
inline Real     sqrlen(const Vec4 &v);              // v . v
inline Vec4     norm(const Vec4 &v);                // v / || v ||
inline Void     normalise(Vec4 &v);                 // v = norm(v)
Vec4            cross(const Vec4 &a, const Vec4 &b, const Vec4 &c);
                                                    // 4D cross prod.
Vec3            proj(const Vec4 &v);                // hom. projection

std::ostream &operator << (std::ostream &s, const Vec4 &v);
std::istream &operator >> (std::istream &s, Vec4 &v);


// --- Inlines ----------------------------------------------------------------

inline Real &Vec4::operator [] (Int i)
{
    CheckRange(i, 0, 4, "(Vec4::[i]) index out of range");
    return(elt[i]);
}

inline const Real &Vec4::operator [] (Int i) const
{
    CheckRange(i, 0, 4, "(Vec4::[i]) index out of range");
    return(elt[i]);
}


inline Vec4::Vec4()
{
}

inline Vec4::Vec4(Real x, Real y, Real z, Real w)
{
    elt[0] = x;
    elt[1] = y;
    elt[2] = z;
    elt[3] = w;
}

inline Vec4::Vec4(const Vec4 &v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = v[3];
}

inline Vec4::Vec4(const Vec3 &v, Real w)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = w;
}

inline Real *Vec4::Ref() const
{
    return((Real *) elt);
}

inline Vec4 &Vec4::operator = (const Vec4 &v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
    elt[3] = v[3];

    return(SELF);
}

inline Vec4 &Vec4::operator += (const Vec4 &v)
{
    elt[0] += v[0];
    elt[1] += v[1];
    elt[2] += v[2];
    elt[3] += v[3];

    return(SELF);
}

inline Vec4 &Vec4::operator -= (const Vec4 &v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];
    elt[2] -= v[2];
    elt[3] -= v[3];

    return(SELF);
}

inline Vec4 &Vec4::operator *= (const Vec4 &v)
{
    elt[0] *= v[0];
    elt[1] *= v[1];
    elt[2] *= v[2];
    elt[3] *= v[3];

    return(SELF);
}

inline Vec4 &Vec4::operator *= (Real s)
{
    elt[0] *= s;
    elt[1] *= s;
    elt[2] *= s;
    elt[3] *= s;

    return(SELF);
}

inline Vec4 &Vec4::operator /= (const Vec4 &v)
{
    elt[0] /= v[0];
    elt[1] /= v[1];
    elt[2] /= v[2];
    elt[3] /= v[3];

    return(SELF);
}

inline Vec4 &Vec4::operator /= (Real s)
{
    elt[0] /= s;
    elt[1] /= s;
    elt[2] /= s;
    elt[3] /= s;

    return(SELF);
}


inline Vec4 Vec4::operator + (const Vec4 &a) const
{
    Vec4 result;

    result[0] = elt[0] + a[0];
    result[1] = elt[1] + a[1];
    result[2] = elt[2] + a[2];
    result[3] = elt[3] + a[3];

    return(result);
}

inline Vec4 Vec4::operator - (const Vec4 &a) const
{
    Vec4 result;

    result[0] = elt[0] - a[0];
    result[1] = elt[1] - a[1];
    result[2] = elt[2] - a[2];
    result[3] = elt[3] - a[3];

    return(result);
}

inline Vec4 Vec4::operator - () const
{
    Vec4 result;

    result[0] = -elt[0];
    result[1] = -elt[1];
    result[2] = -elt[2];
    result[3] = -elt[3];

    return(result);
}

inline Vec4 Vec4::operator * (const Vec4 &a) const
{
    Vec4 result;

    result[0] = elt[0] * a[0];
    result[1] = elt[1] * a[1];
    result[2] = elt[2] * a[2];
    result[3] = elt[3] * a[3];

    return(result);
}

inline Vec4 Vec4::operator * (Real s) const
{
    Vec4 result;

    result[0] = elt[0] * s;
    result[1] = elt[1] * s;
    result[2] = elt[2] * s;
    result[3] = elt[3] * s;

    return(result);
}

inline Vec4 Vec4::operator / (const Vec4 &a) const
{
    Vec4 result;

    result[0] = elt[0] / a[0];
    result[1] = elt[1] / a[1];
    result[2] = elt[2] / a[2];
    result[3] = elt[3] / a[3];

    return(result);
}

inline Vec4 Vec4::operator / (Real s) const
{
    Vec4 result;

    result[0] = elt[0] / s;
    result[1] = elt[1] / s;
    result[2] = elt[2] / s;
    result[3] = elt[3] / s;

    return(result);
}

inline Vec4 operator * (Real s, const Vec4 &v)
{
    return(v * s);
}

inline Vec4 &Vec4::MakeZero()
{
    elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = vl_zero; elt[3] = vl_zero;
    return(SELF);
}

inline Vec4 &Vec4::MakeBlock(Real k)
{
    elt[0] = k; elt[1] = k; elt[2] = k; elt[3] = k;
    return(SELF);
}

inline Vec4 &Vec4::Normalise()
{
    Assert(sqrlen(SELF) > 0.0, "normalising length-zero vector");
    SELF /= len(SELF);
    return(SELF);
}

inline Vec4::Vec4(ZeroOrOne k)
{
    MakeBlock(k);
}

inline Vec4::Vec4(Axis k)
{
    MakeUnit(k, vl_1);
}

inline Vec4 &Vec4::operator = (ZeroOrOne k)
{
    MakeBlock(k);

    return(SELF);
}

inline Vec4 &Vec4::operator = (Axis k)
{
    MakeUnit(k, vl_1);

    return(SELF);
}


inline Real dot(const Vec4 &a, const Vec4 &b)
{
    return(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
}

inline Real len(const Vec4 &v)
{
    return(sqrt(dot(v, v)));
}

inline Real sqrlen(const Vec4 &v)
{
    return(dot(v, v));
}

inline Vec4 norm(const Vec4 &v)
{
    Assert(sqrlen(v) > 0.0, "normalising length-zero vector");
    return(v / len(v));
}

inline Void normalise(Vec4 &v)
{
    v /= len(v);
}

#endif
