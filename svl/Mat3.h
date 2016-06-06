/*
    File:           Mat3.h

    Function:       Defines a 3 x 3 matrix.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
*/

#ifndef __Mat3__
#define __Mat3__

#include "Vec3.h"


// --- Mat3 Class -------------------------------------------------------------


class Vec4;

class Mat3
{
public:

    // Constructors

                Mat3();
                Mat3(Real a, Real b, Real c,
                     Real d, Real e, Real f,
                     Real g, Real h, Real i);
                Mat3(const Mat3 &m);
                Mat3(ZeroOrOne k);
                Mat3(Block k);

    // Accessor functions

    Int         Rows() const { return(3); };
    Int         Cols() const { return(3); };

    Vec3        &operator [] (Int i);
    const Vec3  &operator [] (Int i) const;

    Real        *Ref() const;               // Return pointer to data

    // Assignment operators

    Mat3        &operator =  (const Mat3 &m);
    Mat3        &operator =  (ZeroOrOne k);
    Mat3        &operator =  (Block k);
    Mat3        &operator += (const Mat3 &m);
    Mat3        &operator -= (const Mat3 &m);
    Mat3        &operator *= (const Mat3 &m);
    Mat3        &operator *= (Real s);
    Mat3        &operator /= (Real s);

    // Comparison operators

    Bool        operator == (const Mat3 &m) const;  // M == N?
    Bool        operator != (const Mat3 &m) const;  // M != N?

    // Arithmetic operators

    Mat3        operator + (const Mat3 &m) const;   // M + N
    Mat3        operator - (const Mat3 &m) const;   // M - N
    Mat3        operator - () const;                // -M
    Mat3        operator * (const Mat3 &m) const;   // M * N
    Mat3        operator * (Real s) const;          // M * s
    Mat3        operator / (Real s) const;          // M / s

    // Initialisers

    Void        MakeZero();                 // Zero matrix
    Void        MakeDiag(Real k = vl_one);  // I
    Void        MakeBlock(Real k = vl_one); // all elts = k

    // Vector Transforms

    Mat3&       MakeRot(const Vec3 &axis, Real theta);
    Mat3&       MakeRot(const Vec4 &q);     // Rotate by quaternion
    Mat3&       MakeScale(const Vec3 &s);

    // Homogeneous Transforms

    Mat3&       MakeHRot(Real theta);       // Rotate by theta rads
    Mat3&       MakeHScale(const Vec2 &s);  // Scale by s
    Mat3&       MakeHTrans(const Vec2 &t);  // Translation by t

    // Private...

protected:

    Vec3        row[3];
};


// --- Matrix operators -------------------------------------------------------

inline Vec3     &operator *= (Vec3 &v, const Mat3 &m);      // v *= m
inline Vec3     operator * (const Mat3 &m, const Vec3 &v);  // m * v
inline Vec3     operator * (const Vec3 &v, const Mat3 &m);  // v * m
inline Mat3     operator * (const Real s, const Mat3 &m);   // s * m

Mat3            trans(const Mat3 &m);                   // Transpose
Real            trace(const Mat3 &m);                   // Trace
Mat3            adj(const Mat3 &m);                     // Adjoint
Real            det(const Mat3 &m);                     // Determinant
Mat3            inv(const Mat3 &m);                     // Inverse
Mat3            oprod(const Vec3 &a, const Vec3 &b);    // Outer product

// The xform functions help avoid dependence on whether row or column
// vectors are used to represent points and vectors.
inline Vec3     xform(const Mat3 &m, const Vec3 &v); // Transform of v by m
inline Vec2     xform(const Mat3 &m, const Vec2 &v); // Hom. xform of v by m
inline Mat3     xform(const Mat3 &m, const Mat3 &n); // Xform v -> m(n(v))

std::ostream         &operator << (std::ostream &s, const Mat3 &m);
std::istream         &operator >> (std::istream &s, Mat3 &m);


// --- Inlines ----------------------------------------------------------------

inline Mat3::Mat3()
{
}

inline Vec3 &Mat3::operator [] (Int i)
{
    CheckRange(i, 0, 3, "(Mat3::[i]) index out of range");
    return(row[i]);
}

inline const Vec3 &Mat3::operator [] (Int i) const
{
    CheckRange(i, 0, 3, "(Mat3::[i]) index out of range");
    return(row[i]);
}

inline Real *Mat3::Ref() const
{
    return((Real *) row);
}

inline Mat3::Mat3(ZeroOrOne k)
{
    MakeDiag(k);
}

inline Mat3::Mat3(Block k)
{
    MakeBlock((ZeroOrOne) k);
}

inline Mat3 &Mat3::operator = (ZeroOrOne k)
{
    MakeDiag(k);

    return(SELF);
}

inline Mat3 &Mat3::operator = (Block k)
{
    MakeBlock((ZeroOrOne) k);

    return(SELF);
}

inline Mat3 operator *  (const Real s, const Mat3 &m)
{
    return(m * s);
}

inline Vec3 operator * (const Mat3 &m, const Vec3 &v)
{
    Vec3 result;

    result[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2];
    result[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2];
    result[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2];

    return(result);
}

inline Vec3 operator * (const Vec3 &v, const Mat3 &m)
{
    Vec3 result;

    result[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0];
    result[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1];
    result[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2];

    return(result);
}

inline Vec3 &operator *= (Vec3 &v, const Mat3 &m)
{
    Real t0, t1;

    t0   = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0];
    t1   = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1];
    v[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2];
    v[0] = t0;
    v[1] = t1;

    return(v);
}

#ifdef VL_ROW_ORIENT
inline Vec2 xform(const Mat3 &m, const Vec2 &v)
{ return(proj(Vec3(v, 1.0) * m)); }
inline Vec3 xform(const Mat3 &m, const Vec3 &v)
{ return(v * m); }
inline Mat3 xform(const Mat3 &m, const Mat3 &n)
{ return(n * m); }
#else
inline Vec2 xform(const Mat3 &m, const Vec2 &v)
{ return(proj(m * Vec3(v, 1.0))); }
inline Vec3 xform(const Mat3 &m, const Vec3 &v)
{ return(m * v); }
inline Mat3 xform(const Mat3 &m, const Mat3 &n)
{ return(m * n); }
#endif

#endif
