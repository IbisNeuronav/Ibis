/*
    File:           Mat2.h

    Function:       Defines a 2 x 2 matrix.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Mat2__
#define __Mat2__

#include "Vec2.h"


// --- Mat2 Class -------------------------------------------------------------

class Mat2
{
public:

    // Constructors

                Mat2();
                Mat2(Real a, Real b, Real c, Real d);   // Create from rows
                Mat2(const Mat2 &m);                    // Copy constructor
                Mat2(ZeroOrOne k);
                Mat2(Block k);

    // Accessor functions

    Int         Rows() const { return(2); };
    Int         Cols() const { return(2); };

    Vec2        &operator [] (Int i);
    const Vec2  &operator [] (Int i) const;

    Real        *Ref() const;               // Return pointer to data

    // Assignment operators

    Mat2        &operator =  (const Mat2 &m);
    Mat2        &operator =  (ZeroOrOne k);
    Mat2        &operator =  (Block k);
    Mat2        &operator += (const Mat2 &m);
    Mat2        &operator -= (const Mat2 &m);
    Mat2        &operator *= (const Mat2 &m);
    Mat2        &operator *= (Real s);
    Mat2        &operator /= (Real s);

    // Comparison operators

    Bool        operator == (const Mat2 &m) const;  // M == N?
    Bool        operator != (const Mat2 &m) const;  // M != N?

    // Arithmetic operators

    Mat2        operator + (const Mat2 &m) const;   // M + N
    Mat2        operator - (const Mat2 &m) const;   // M - N
    Mat2        operator - () const;                // -M
    Mat2        operator * (const Mat2 &m) const;   // M * N
    Mat2        operator * (Real s) const;          // M * s
    Mat2        operator / (Real s) const;          // M / s

    // Initialisers

    Void        MakeZero();                         // Zero matrix
    Void        MakeDiag(Real k = vl_one);          // I
    Void        MakeBlock(Real k = vl_one);         // all elts=k

    // Vector Transformations

    Mat2&       MakeRot(Real theta);
    Mat2&       MakeScale(const Vec2 &s);

    // Private...

protected:

    Vec2        row[2];     // Rows of the matrix
};


// --- Matrix operators -------------------------------------------------------

inline Vec2     &operator *= (Vec2 &v, const Mat2 &m);      // v *= m
inline Vec2     operator * (const Mat2 &m, const Vec2 &v);  // m * v
inline Vec2     operator * (const Vec2 &v, const Mat2 &m);  // v * m
inline Mat2     operator * (Real s, const Mat2 &m);         // s * m

inline Mat2     trans(const Mat2 &m);               // Transpose
inline Real     trace(const Mat2 &m);               // Trace
inline Mat2     adj(const Mat2 &m);                 // Adjoint
Real            det(const Mat2 &m);                 // Determinant
Mat2            inv(const Mat2 &m);                 // Inverse
Mat2            oprod(const Vec2 &a, const Vec2 &b);
                                                    // Outer product

// The xform functions help avoid dependence on whether row or column
// vectors are used to represent points and vectors.
inline Vec2     xform(const Mat2 &m, const Vec2 &v); // Transform of v by m
inline Mat2     xform(const Mat2 &m, const Mat2 &n); // xform v -> m(n(v))

std::ostream         &operator << (std::ostream &s, const Mat2 &m);
std::istream         &operator >> (std::istream &s, Mat2 &m);


// --- Inlines ----------------------------------------------------------------

inline Vec2 &Mat2::operator [] (Int i)
{
    CheckRange(i, 0, 2, "(Mat2::[i]) index out of range");
    return(row[i]);
}

inline const Vec2 &Mat2::operator [] (Int i) const
{
    CheckRange(i, 0, 2, "(Mat2::[i]) index out of range");
    return(row[i]);
}

inline Real *Mat2::Ref() const
{
    return((Real*) row);
}

inline Mat2::Mat2()
{
}

inline Mat2::Mat2(Real a, Real b, Real c, Real d)
{
    row[0][0] = a;  row[0][1] = b;
    row[1][0] = c;  row[1][1] = d;
}

inline Mat2::Mat2(const Mat2 &m)
{
    row[0] = m[0];
    row[1] = m[1];
}


inline Void Mat2::MakeZero()
{
    row[0][0] = vl_zero; row[0][1] = vl_zero;
    row[1][0] = vl_zero; row[1][1] = vl_zero;
}

inline Void Mat2::MakeDiag(Real k)
{
    row[0][0] = k;          row[0][1] = vl_zero;
    row[1][0] = vl_zero;    row[1][1] = k;
}

inline Void Mat2::MakeBlock(Real k)
{
    row[0][0] = k; row[0][1] = k;
    row[1][0] = k; row[1][1] = k;
}

inline Mat2::Mat2(ZeroOrOne k)
{
    MakeDiag(k);
}

inline Mat2::Mat2(Block k)
{
    MakeBlock((ZeroOrOne) k);
}

inline Mat2 &Mat2::operator = (ZeroOrOne k)
{
    MakeDiag(k);

    return(SELF);
}

inline Mat2 &Mat2::operator = (Block k)
{
    MakeBlock((ZeroOrOne) k);

    return(SELF);
}

inline Mat2 &Mat2::operator = (const Mat2 &m)
{
    row[0] = m[0];
    row[1] = m[1];

    return(SELF);
}

inline Mat2 &Mat2::operator += (const Mat2 &m)
{
    row[0] += m[0];
    row[1] += m[1];

    return(SELF);
}

inline Mat2 &Mat2::operator -= (const Mat2 &m)
{
    row[0] -= m[0];
    row[1] -= m[1];

    return(SELF);
}

inline Mat2 &Mat2::operator *= (const Mat2 &m)
{
    SELF = SELF * m;

    return(SELF);
}

inline Mat2 &Mat2::operator *= (Real s)
{
    row[0] *= s;
    row[1] *= s;

    return(SELF);
}

inline Mat2 &Mat2::operator /= (Real s)
{
    row[0] /= s;
    row[1] /= s;

    return(SELF);
}


inline Mat2 Mat2::operator + (const Mat2 &m) const
{
    Mat2 result;

    result[0] = row[0] + m[0];
    result[1] = row[1] + m[1];

    return(result);
}

inline Mat2 Mat2::operator - (const Mat2 &m) const
{
    Mat2 result;

    result[0] = row[0] - m[0];
    result[1] = row[1] - m[1];

    return(result);
}

inline Mat2 Mat2::operator - () const
{
    Mat2 result;

    result[0] = -row[0];
    result[1] = -row[1];

    return(result);
}

inline Mat2 Mat2::operator * (const Mat2 &m) const
{
#define N(x,y) row[x][y]
#define M(x,y) m.row[x][y]
#define R(x,y) result[x][y]

    Mat2 result;

    R(0,0) = N(0,0) * M(0,0) + N(0,1) * M(1,0);
    R(0,1) = N(0,0) * M(0,1) + N(0,1) * M(1,1);
    R(1,0) = N(1,0) * M(0,0) + N(1,1) * M(1,0);
    R(1,1) = N(1,0) * M(0,1) + N(1,1) * M(1,1);

    return(result);

#undef N
#undef M
#undef R
}

inline Mat2 Mat2::operator * (Real s) const
{
    Mat2 result;

    result[0] = row[0] * s;
    result[1] = row[1] * s;

    return(result);
}

inline Mat2 Mat2::operator / (Real s) const
{
    Mat2 result;

    result[0] = row[0] / s;
    result[1] = row[1] / s;

    return(result);
}

inline Mat2  operator *  (Real s, const Mat2 &m)
{
    return(m * s);
}

inline Vec2 operator * (const Mat2 &m, const Vec2 &v)
{
    Vec2 result;

    result[0] = m[0][0] * v[0] + m[0][1] * v[1];
    result[1] = m[1][0] * v[0] + m[1][1] * v[1];

    return(result);
}

inline Vec2 operator * (const Vec2 &v, const Mat2 &m)
{
    Vec2 result;

    result[0] = v[0] * m[0][0] + v[1] * m[1][0];
    result[1] = v[0] * m[0][1] + v[1] * m[1][1];

    return(result);
}

inline Vec2 &operator *= (Vec2 &v, const Mat2 &m)
{
    Real t;

    t    = v[0] * m[0][0] + v[1] * m[1][0];
    v[1] = v[0] * m[0][1] + v[1] * m[1][1];
    v[0] = t;

    return(v);
}


inline Mat2 trans(const Mat2 &m)
{
    Mat2 result;

    result[0][0] = m[0][0]; result[0][1] = m[1][0];
    result[1][0] = m[0][1]; result[1][1] = m[1][1];

    return(result);
}

inline Real trace(const Mat2 &m)
{
    return(m[0][0] + m[1][1]);
}

inline Mat2 adj(const Mat2 &m)
{
    Mat2 result;

    result[0] =  cross(m[1]);
    result[1] = -cross(m[0]);

    return(result);
}

#ifdef VL_ROW_ORIENT
inline Vec2 xform(const Mat2 &m, const Vec2 &v)
{ return(v * m); }
inline Mat2 xform(const Mat2 &m, const Mat2 &n)
{ return(n * m); }
#else
inline Vec2 xform(const Mat2 &m, const Vec2 &v)
{ return(m * v); }
inline Mat2 xform(const Mat2 &m, const Mat2 &n)
{ return(m * n); }
#endif

#endif
