/*
    File:           Vec2.h

    Function:       Defines a length-2 vector.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Vec2__
#define __Vec2__


// --- Vec2 Class -------------------------------------------------------------


class Vec2
{
public:

    // Constructors

                Vec2();
                Vec2(Real x, Real y);       // (x, y)
                Vec2(const Vec2 &v);        // Copy constructor
                Vec2(ZeroOrOne k);          // v[i] = vl_zero
                Vec2(Axis k);               // v[k] = 1

    // Accessor functions

    Real        &operator [] (Int i);
    const Real  &operator [] (Int i) const;

    Int         Elts() const { return(2); };
    Real        *Ref() const;                       // Return ptr to data

    // Assignment operators

    Vec2        &operator =  (const Vec2 &a);
    Vec2        &operator =  (ZeroOrOne k);
    Vec2        &operator =  (Axis k);

    Vec2        &operator += (const Vec2 &a);
    Vec2        &operator -= (const Vec2 &a);
    Vec2        &operator *= (const Vec2 &a);
    Vec2        &operator *= (Real s);
    Vec2        &operator /= (const Vec2 &a);
    Vec2        &operator /= (Real s);

    // Comparison operators

    Bool        operator == (const Vec2 &a) const;  // v == a?
    Bool        operator != (const Vec2 &a) const;  // v != a?

    // Arithmetic operators

    Vec2        operator + (const Vec2 &a) const;   // v + a
    Vec2        operator - (const Vec2 &a) const;   // v - a
    Vec2        operator - () const;                // -v
    Vec2        operator * (const Vec2 &a) const;   // v * a (vx * ax, ...)
    Vec2        operator * (Real s) const;          // v * s
    Vec2        operator / (const Vec2 &a) const;   // v / a (vx / ax, ...)
    Vec2        operator / (Real s) const;          // v / s

    // Initialisers

    Vec2        &MakeZero();                        // Zero vector
    Vec2        &MakeUnit(Int i, Real k = vl_one);  // I[i]
    Vec2        &MakeBlock(Real k = vl_one);        // All-k vector

    Vec2        &Normalise();                       // normalise vector

    // Private...

protected:

    Real            elt[2];
};


// --- Vec operators ----------------------------------------------------------

inline Vec2     operator * (Real s, const Vec2 &v); // s * v
inline Real     dot(const Vec2 &a, const Vec2 &b);  // v . a
inline Real     len(const Vec2 &v);                 // || v ||
inline Real     sqrlen(const Vec2 &v);              // v . v
inline Vec2     norm(const Vec2 &v);                // v / || v ||
inline Void     normalise(Vec2 &v);                 // v = norm(v)
inline Vec2     cross(const Vec2 &v);               // cross prod.

std::ostream &operator << (std::ostream &s, const Vec2 &v);
std::istream &operator >> (std::istream &s, Vec2 &v);


// --- Inlines ----------------------------------------------------------------

inline Real &Vec2::operator [] (Int i)
{
    CheckRange(i, 0, 2, "(Vec2::[i]) index out of range");
    return(elt[i]);
}

inline const Real &Vec2::operator [] (Int i) const
{
    CheckRange(i, 0, 2, "(Vec2::[i]) index out of range");
    return(elt[i]);
}

inline Vec2::Vec2()
{
}

inline Vec2::Vec2(Real x, Real y)
{
    elt[0] = x;
    elt[1] = y;
}

inline Vec2::Vec2(const Vec2 &v)
{
    elt[0] = v[0];
    elt[1] = v[1];
}

inline Real *Vec2::Ref() const
{
    return((Real *) elt);
}

inline Vec2 &Vec2::operator = (const Vec2 &v)
{
    elt[0] = v[0];
    elt[1] = v[1];

    return(SELF);
}

inline Vec2 &Vec2::operator += (const Vec2 &v)
{
    elt[0] += v[0];
    elt[1] += v[1];

    return(SELF);
}

inline Vec2 &Vec2::operator -= (const Vec2 &v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];

    return(SELF);
}

inline Vec2 &Vec2::operator *= (const Vec2 &v)
{
    elt[0] *= v[0];
    elt[1] *= v[1];

    return(SELF);
}

inline Vec2 &Vec2::operator *= (Real s)
{
    elt[0] *= s;
    elt[1] *= s;

    return(SELF);
}

inline Vec2 &Vec2::operator /= (const Vec2 &v)
{
    elt[0] /= v[0];
    elt[1] /= v[1];

    return(SELF);
}

inline Vec2 &Vec2::operator /= (Real s)
{
    elt[0] /= s;
    elt[1] /= s;

    return(SELF);
}

inline Vec2 Vec2::operator + (const Vec2 &a) const
{
    Vec2 result;

    result[0] = elt[0] + a[0];
    result[1] = elt[1] + a[1];

    return(result);
}

inline Vec2 Vec2::operator - (const Vec2 &a) const
{
    Vec2 result;

    result[0] = elt[0] - a[0];
    result[1] = elt[1] - a[1];

    return(result);
}

inline Vec2 Vec2::operator - () const
{
    Vec2 result;

    result[0] = -elt[0];
    result[1] = -elt[1];

    return(result);
}

inline Vec2 Vec2::operator * (const Vec2 &a) const
{
    Vec2 result;

    result[0] = elt[0] * a[0];
    result[1] = elt[1] * a[1];

    return(result);
}

inline Vec2 Vec2::operator * (Real s) const
{
    Vec2 result;

    result[0] = elt[0] * s;
    result[1] = elt[1] * s;

    return(result);
}

inline Vec2 operator * (Real s, const Vec2 &v)
{
    return(v * s);
}

inline Vec2 Vec2::operator / (const Vec2 &a) const
{
    Vec2 result;

    result[0] = elt[0] / a[0];
    result[1] = elt[1] / a[1];

    return(result);
}

inline Vec2 Vec2::operator / (Real s) const
{
    Vec2 result;

    result[0] = elt[0] / s;
    result[1] = elt[1] / s;

    return(result);
}

inline Real dot(const Vec2 &a, const Vec2 &b)
{
    return(a[0] * b[0] + a[1] * b[1]);
}

inline Vec2 cross(const Vec2 &a)
{
    Vec2 result;

    result[0] =  a[1];
    result[1] = -a[0];

    return(result);
}

inline Real len(const Vec2 &v)
{
    return(sqrt(dot(v, v)));
}

inline Real sqrlen(const Vec2 &v)
{
    return(dot(v, v));
}

inline Vec2 norm(const Vec2 &v)
{
    Assert(sqrlen(v) > 0.0, "normalising length-zero vector");
    return(v / len(v));
}

inline Void normalise(Vec2 &v)
{
    v /= len(v);
}

inline Vec2 &Vec2::MakeUnit(Int i, Real k)
{
    if (i == 0)
    { elt[0] = k; elt[1] = vl_zero; }
    else if (i == 1)
    { elt[0] = vl_zero; elt[1] = k; }
    else
        _Error("(Vec2::Unit) illegal unit vector");
    return(SELF);
}

inline Vec2 &Vec2::MakeZero()
{
    elt[0] = vl_zero; elt[1] = vl_zero;
    return(SELF);
}

inline Vec2 &Vec2::MakeBlock(Real k)
{
    elt[0] = k; elt[1] = k;
    return(SELF);
}

inline Vec2 &Vec2::Normalise()
{
    Assert(sqrlen(SELF) > 0.0, "normalising length-zero vector");
    SELF /= len(SELF);
    return(SELF);
}


inline Vec2::Vec2(ZeroOrOne k)
{
    elt[0] = k;
    elt[1] = k;
}

inline Vec2::Vec2(Axis k)
{
    MakeUnit(k, vl_one);
}

inline Vec2 &Vec2::operator = (ZeroOrOne k)
{
    elt[0] = k; elt[1] = k;

    return(SELF);
}

inline Vec2 &Vec2::operator = (Axis k)
{
    MakeUnit(k, vl_1);

    return(SELF);
}

inline Bool Vec2::operator == (const Vec2 &a) const
{
    return(elt[0] == a[0] && elt[1] == a[1]);
}

inline Bool Vec2::operator != (const Vec2 &a) const
{
    return(elt[0] != a[0] || elt[1] != a[1]);
}

#endif
