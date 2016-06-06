/*
    File:           Vec3.h

    Function:       Defines a length-3 vector.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Vec3__
#define __Vec3__

#include "Vec2.h"


// --- Vec3 Class -------------------------------------------------------------

class Vec3
{
public:

    // Constructors

	Vec3();
	Vec3(Real x, Real y, Real z);   // [x, y, z]
	Vec3(const Vec3 &v);            // Copy constructor
	Vec3(const Vec2 &v, Real w);    // Hom. 2D vector
	Vec3(ZeroOrOne k);
	Vec3(Axis a);
	Vec3( const Real * a );

    // Accessor functions

    Int         Elts() const { return(3); };

    Real        &operator [] (Int i);
    const Real  &operator [] (Int i) const;

    Real        *Ref() const;                   // Return pointer to data

    // Assignment operators

    Vec3        &operator =  (const Vec3 &a);
	Vec3		&operator =  (const Real *a);    
	Vec3        &operator =  (ZeroOrOne k);
    Vec3        &operator += (const Vec3 &a);
    Vec3        &operator -= (const Vec3 &a);
    Vec3        &operator *= (const Vec3 &a);
    Vec3        &operator *= (Real s);
    Vec3        &operator /= (const Vec3 &a);
    Vec3        &operator /= (Real s);

    // Comparison operators

    Bool        operator == (const Vec3 &a) const;  // v == a?
    Bool        operator != (const Vec3 &a) const;  // v != a?
    Bool        operator <  (const Vec3 &a) const; // v <  a?
    Bool        operator >= (const Vec3 &a) const; // v >= a?

    // Arithmetic operators

    Vec3        operator + (const Vec3 &a) const;   // v + a
    Vec3        operator - (const Vec3 &a) const;   // v - a
    Vec3        operator - () const;                // -v
    Vec3        operator * (const Vec3 &a) const;   // v * a (vx * ax, ...)
    Vec3        operator * (Real s) const;          // v * s
    Vec3        operator / (const Vec3 &a) const;   // v / a (vx / ax, ...)
    Vec3        operator / (Real s) const;          // v / s

    // Initialisers

    Vec3        &MakeZero();                        // Zero vector
    Vec3        &MakeUnit(Int i, Real k = vl_one);  // I[i]
    Vec3        &MakeBlock(Real k = vl_one);        // All-k vector

    Vec3        &Normalise();                       // normalise vector

	void         Interp( const Vec3 & a, const Vec3 & b, Real ratio );

protected:

    Real elt[3];
};


// --- Vec operators ----------------------------------------------------------

inline Vec3     operator * (Real s, const Vec3 &v); // s * v
inline Real     dot(const Vec3 &a, const Vec3 &b);  // v . a
inline Real     len(const Vec3 &v);                 // || v ||
inline Real     sqrlen(const Vec3 &v);              // v . v
inline Vec3     norm(const Vec3 &v);                // v / || v ||
inline Void     normalise(Vec3 &v);                 // v = norm(v)
inline Vec3     cross(const Vec3 &a, const Vec3 &b);// a x b
inline Vec2     proj(const Vec3 &v);                // hom. projection
inline Vec3     interp( const Vec3 & a, const Vec3 & b, Real ratio );

std::ostream &operator << (std::ostream &s, const Vec3 &v);
std::istream &operator >> (std::istream &s, Vec3 &v);


// --- Inlines ----------------------------------------------------------------

inline Real &Vec3::operator [] (Int i)
{
    CheckRange(i, 0, 3, "(Vec3::[i]) index out of range");
    return(elt[i]);
}

inline const Real &Vec3::operator [] (Int i) const
{
    CheckRange(i, 0, 3, "(Vec3::[i]) index out of range");
    return(elt[i]);
}

inline Vec3::Vec3()
{
}

inline Vec3::Vec3(Real x, Real y, Real z)
{
    elt[0] = x;
    elt[1] = y;
    elt[2] = z;
}

inline Vec3::Vec3(const Vec3 &v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];
}

inline Vec3::Vec3(const Vec2 &v, Real w)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = w;
}

inline Vec3::Vec3( const Real * a )
{
	elt[0] = a[0];
	elt[1] = a[1];
	elt[2] = a[2];
}

inline Real *Vec3::Ref() const
{
    return((Real *) elt);
}

inline Vec3 &Vec3::operator = (const Vec3 &v)
{
    elt[0] = v[0];
    elt[1] = v[1];
    elt[2] = v[2];

    return(SELF);
}

inline Vec3 &Vec3::operator =  (const Real *a)
{
    elt[0] = a[0];
    elt[1] = a[1];
    elt[2] = a[2];
    return(SELF);
}

inline Vec3 &Vec3::operator += (const Vec3 &v)
{
    elt[0] += v[0];
    elt[1] += v[1];
    elt[2] += v[2];

    return(SELF);
}

inline Vec3 &Vec3::operator -= (const Vec3 &v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];
    elt[2] -= v[2];

    return(SELF);
}

inline Vec3 &Vec3::operator *= (const Vec3 &a)
{
    elt[0] *= a[0];
    elt[1] *= a[1];
    elt[2] *= a[2];

    return(SELF);
}

inline Vec3 &Vec3::operator *= (Real s)
{
    elt[0] *= s;
    elt[1] *= s;
    elt[2] *= s;

    return(SELF);
}

inline Vec3 &Vec3::operator /= (const Vec3 &a)
{
    elt[0] /= a[0];
    elt[1] /= a[1];
    elt[2] /= a[2];

    return(SELF);
}

inline Vec3 &Vec3::operator /= (Real s)
{
    elt[0] /= s;
    elt[1] /= s;
    elt[2] /= s;

    return(SELF);
}

inline Vec3 Vec3::operator + (const Vec3 &a) const
{
    Vec3 result;

    result[0] = elt[0] + a[0];
    result[1] = elt[1] + a[1];
    result[2] = elt[2] + a[2];

    return(result);
}

inline Vec3 Vec3::operator - (const Vec3 &a) const
{
    Vec3 result;

    result[0] = elt[0] - a[0];
    result[1] = elt[1] - a[1];
    result[2] = elt[2] - a[2];

    return(result);
}

inline Vec3 Vec3::operator - () const
{
    Vec3 result;

    result[0] = -elt[0];
    result[1] = -elt[1];
    result[2] = -elt[2];

    return(result);
}

inline Vec3 Vec3::operator * (const Vec3 &a) const
{
    Vec3 result;

    result[0] = elt[0] * a[0];
    result[1] = elt[1] * a[1];
    result[2] = elt[2] * a[2];

    return(result);
}

inline Vec3 Vec3::operator * (Real s) const
{
    Vec3 result;

    result[0] = elt[0] * s;
    result[1] = elt[1] * s;
    result[2] = elt[2] * s;

    return(result);
}

inline Vec3 Vec3::operator / (const Vec3 &a) const
{
    Vec3 result;

    result[0] = elt[0] / a[0];
    result[1] = elt[1] / a[1];
    result[2] = elt[2] / a[2];

    return(result);
}

inline Vec3 Vec3::operator / (Real s) const
{
    Vec3 result;

    result[0] = elt[0] / s;
    result[1] = elt[1] / s;
    result[2] = elt[2] / s;

    return(result);
}

inline Vec3 operator * (Real s, const Vec3 &v)
{
    return(v * s);
}

inline Vec3 &Vec3::MakeUnit(Int n, Real k)
{
    if (n == 0)
    { elt[0] = k; elt[1] = vl_zero; elt[2] = vl_zero; }
    else if (n == 1)
    { elt[0] = vl_zero; elt[1] = k; elt[2] = vl_zero; }
    else if (n == 2)
    { elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = k; }
    else
        _Error("(Vec3::Unit) illegal unit vector");

    return(SELF);
}

inline Vec3 &Vec3::MakeZero()
{
    elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = vl_zero;

    return(SELF);
}

inline Vec3 &Vec3::MakeBlock(Real k)
{
    elt[0] = k; elt[1] = k; elt[2] = k;

    return(SELF);
}

inline Vec3 &Vec3::Normalise()
{
    Assert(sqrlen(SELF) > 0.0, "normalising length-zero vector");
    SELF /= len(SELF);

    return(SELF);
}

inline void Vec3::Interp( const Vec3 & a, const Vec3 & b, Real ratio )
{
	Vec3 diff( b - a );
	diff *= ratio;
	(*this) = a + diff;
}

inline Vec3::Vec3(ZeroOrOne k)
{
    elt[0] = k; elt[1] = k; elt[2] = k;
}

inline Vec3 &Vec3::operator = (ZeroOrOne k)
{
    elt[0] = k; elt[1] = k; elt[2] = k;

    return(SELF);
}

inline Vec3::Vec3(Axis a)
{
    MakeUnit(a, vl_one);
}


inline Bool Vec3::operator == (const Vec3 &a) const
{
    return(elt[0] == a[0] && elt[1] == a[1] && elt[2] == a[2]);
}

inline Bool Vec3::operator != (const Vec3 &a) const
{
    return(elt[0] != a[0] || elt[1] != a[1] || elt[2] != a[2]);
}

inline Bool Vec3::operator < (const Vec3 &a) const
{
    return(elt[0] < a[0] && elt[1] < a[1] && elt[2] < a[2]);
}

inline Bool Vec3::operator >= (const Vec3 &a) const
{
    return(elt[0] >= a[0] && elt[1] >= a[1] && elt[2] >= a[2]);
}


inline Real dot(const Vec3 &a, const Vec3 &b)
{
    return(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

inline Real len(const Vec3 &v)
{
    return(sqrt(dot(v, v)));
}

inline Real sqrlen(const Vec3 &v)
{
    return(dot(v, v));
}

inline Vec3 norm(const Vec3 &v)
{
    Assert(sqrlen(v) > 0.0, "normalising length-zero vector");
    return(v / len(v));
}

inline Void normalise(Vec3 &v)
{
    v /= len(v);
}

inline Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    Vec3 result;

    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];

    return(result);
}

inline Vec2 proj(const Vec3 &v)
{
    Vec2 result;

    Assert(v[2] != 0, "(Vec3/proj) last elt. is zero");

    result[0] = v[0] / v[2];
    result[1] = v[1] / v[2];

    return(result);
}

inline Vec3 interp( const Vec3 & a, const Vec3 & b, Real ratio )
{
	Vec3 result;

	Vec3 diff( b );
	diff -= a;
	result = a + diff * ratio;
	
	return result;
}

#endif
