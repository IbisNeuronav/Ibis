/*
    File:           Mat4.h

    Function:       Defines a 4 x 4 matrix.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Mat4__
#define __Mat4__

#include "Vec3.h"
#include "Vec4.h"


// --- Mat4 Class -------------------------------------------------------------

class Mat4
{
public:

    // Constructors

                Mat4();
                Mat4(Real a, Real b, Real c, Real d,
                     Real e, Real f, Real g, Real h,
                     Real i, Real j, Real k, Real l,
                     Real m, Real n, Real o, Real p);
                Mat4(const Mat4 &m);
                Mat4(ZeroOrOne k);
                Mat4(Block k);

    // Accessor functions

    Int          Rows() const { return(4); };
    Int          Cols() const { return(4); };

    Vec4        &operator [] (Int i);
    const Vec4  &operator [] (Int i) const;

    Real        *Ref() const;

    // Assignment operators

    Mat4        &operator =  (const Mat4 &m);
    Mat4        &operator =  (ZeroOrOne k);
    Mat4        &operator =  (Block k);
    Mat4        &operator += (const Mat4 &m);
    Mat4        &operator -= (const Mat4 &m);
    Mat4        &operator *= (const Mat4 &m);
    Mat4        &operator *= (Real s);
    Mat4        &operator /= (Real s);

    // Comparison operators

    Bool        operator == (const Mat4 &m) const;  // M == N?
    Bool        operator != (const Mat4 &m) const;  // M != N?

    // Arithmetic operators

    Mat4        operator + (const Mat4 &m) const;   // M + N
    Mat4        operator - (const Mat4 &m) const;   // M - N
    Mat4        operator - () const;                // -M
    Mat4        operator * (const Mat4 &m) const;   // M * N
    Mat4        operator * (Real s) const;          // M * s
    Mat4        operator / (Real s) const;          // M / s

    // Initialisers

    Void        MakeZero();                         // Zero matrix
    Void        MakeDiag(Real k = vl_one);          // I
    Void        MakeBlock(Real k = vl_one);         // all elts = k

    // Homogeneous Transforms

    Mat4&       MakeHRot(const Vec3 &axis, Real theta);
                                    // Rotate by theta radians about axis
    Mat4&       MakeHRot(const Vec4 &q);    // Rotate by quaternion
    Mat4&       MakeHScale(const Vec3 &s);  // Scale by components of s

    Mat4&       MakeHTrans(const Vec3 &t);  // Translation by t

    Mat4&       Transpose();                // transpose in place
    Mat4&       AddShift(const Vec3 &t);    // Concatenate shift

    // Private...

protected:

    Vec4        row[4];
};


// --- Matrix operators -------------------------------------------------------

Vec4            operator * (const Mat4 &m, const Vec4 &v);  // m * v
Vec4            operator * (const Vec4 &v, const Mat4 &m);  // v * m
Vec4            &operator *= (Vec4 &a, const Mat4 &m);      // v *= m
inline Mat4     operator * (Real s, const Mat4 &m);         // s * m

Mat4            trans(const Mat4 &m);               // Transpose
Real            trace(const Mat4 &m);               // Trace
Mat4            adj(const Mat4 &m);                 // Adjoint
Real            det(const Mat4 &m);                 // Determinant
Mat4            inv(const Mat4 &m);                 // Inverse
Mat4            oprod(const Vec4 &a, const Vec4 &b);
                                                    // Outer product

// The xform functions help avoid dependence on whether row or column
// vectors are used to represent points and vectors.
inline Vec4     xform(const Mat4 &m, const Vec4 &v); // Transform of v by m
inline Vec3     xform(const Mat4 &m, const Vec3 &v); // Hom. xform of v by m
inline Mat4     xform(const Mat4 &m, const Mat4 &n); // Xform v -> m(n(v))

std::ostream         &operator << (std::ostream &s, const Mat4 &m);
std::istream         &operator >> (std::istream &s, Mat4 &m);


// --- Inlines ----------------------------------------------------------------

inline Mat4::Mat4()
{
}

inline Vec4 &Mat4::operator [] (Int i)
{
    CheckRange(i, 0, 4, "(Mat4::[i]) index out of range");
    return(row[i]);
}

inline const Vec4 &Mat4::operator [] (Int i) const
{
    CheckRange(i, 0, 4, "(Mat4::[i]) index out of range");
    return(row[i]);
}

inline Real *Mat4::Ref() const
{
    return((Real *) row);
}

inline Mat4::Mat4(ZeroOrOne k)
{
    MakeDiag(k);
}

inline Mat4::Mat4(Block k)
{
    MakeBlock((ZeroOrOne) k);
}

inline Mat4 &Mat4::operator = (ZeroOrOne k)
{
    MakeDiag(k);

    return(SELF);
}

inline Mat4 &Mat4::operator = (Block k)
{
    MakeBlock((ZeroOrOne) k);

    return(SELF);
}

inline Mat4 operator * (Real s, const Mat4 &m)
{
    return(m * s);
}

#ifdef VL_ROW_ORIENT
inline Vec3 xform(const Mat4 &m, const Vec3 &v)
{ return(proj(Vec4(v, 1.0) * m)); }
inline Vec4 xform(const Mat4 &m, const Vec4 &v)
{ return(v * m); }
inline Mat4 xform(const Mat4 &m, const Mat4 &n)
{ return(n * m); }
#else
inline Vec3 xform(const Mat4 &m, const Vec3 &v)
{ return(proj(m * Vec4(v, 1.0))); }
inline Vec4 xform(const Mat4 &m, const Vec4 &v)
{ return(m * v); }
inline Mat4 xform(const Mat4 &m, const Mat4 &n)
{ return(m * n); }
#endif

#endif
