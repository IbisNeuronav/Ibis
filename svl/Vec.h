/*
    File:           Vec.h

    Function:       Defines a generic resizeable vector.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Vec__
#define __Vec__

class Vec2;
class Vec3;
class Vec4;


// --- Vec Class --------------------------------------------------------------

class Vec
{
public:

    // Constructors

                Vec();                          // Null vector
    explicit    Vec(Int n);                     // n-element vector
                Vec(Int n, double elt0, ...);   // e.g. Vec(3, 1.1, 2.0, 3.4)
                Vec(Int n, Real *data);         // Vector reference
                Vec(const Vec &v);              // Copy constructor
                Vec(const Vec2 &v);             // reference to a Vec2
                Vec(const Vec3 &v);             // reference to a Vec3
                Vec(const Vec4 &v);             // reference to a Vec4
                Vec(Int n, ZeroOrOne);          // Zero or all-ones vector
                Vec(Int n, Axis a);             // Unit vector
               ~Vec();                          // Destructor

    // Accessor functions

    Int         Elts() const;

    Real        &operator [] (Int i);
    Real        operator [] (Int i) const;

    Void        SetSize(Int n);                 // Resize the vector
    Real        *Ref() const;                   // Return pointer to data

    // Assignment operators

    Vec         &operator =  (const Vec &v);    // v = a etc.
    Vec         &operator =  (ZeroOrOne k);
    Vec         &operator =  (Axis a);
    Vec         &operator =  (const Vec2 &v);
    Vec         &operator =  (const Vec3 &v);
    Vec         &operator =  (const Vec4 &v);

    // In-Place operators

    Vec         &operator += (const Vec &v);
    Vec         &operator -= (const Vec &v);
    Vec         &operator *= (const Vec &v);
    Vec         &operator *= (Real s);
    Vec         &operator /= (const Vec &v);
    Vec         &operator /= (Real s);

    //  Vector initialisers

    Vec         &MakeZero();
    Vec         &MakeUnit(Int i, Real k = vl_one);
    Vec         &MakeBlock(Real k = vl_one);

    Vec         &Normalise();                   // Normalise vector
    Vec         &Clamp(Real fuzz);
    Vec         &Clamp();

    Bool        IsRef() const { return((elts & VL_REF_FLAG) != 0); };

    // Private...

protected:

    Real        *data;
    UInt        elts;
};


// --- Vec Comparison Operators -----------------------------------------------

Bool            operator == (const Vec &a, const Vec &b);
Bool            operator != (const Vec &a, const Vec &b);


// --- Vec Arithmetic Operators -----------------------------------------------

Vec             operator + (const Vec &a, const Vec &b);
Vec             operator - (const Vec &a, const Vec &b);
Vec             operator - (const Vec &v);
Vec             operator * (const Vec &a, const Vec &b);
Vec             operator * (const Vec &v, Real s);
Vec             operator / (const Vec &a, const Vec &b);
Vec             operator / (const Vec &v, Real s);
Vec             operator * (Real s, const Vec &v);

Real            dot(const Vec &a, const Vec &b);// v . a
inline Real     len(const Vec &v);              // || v ||
inline Real     sqrlen(const Vec &v);           // v . v
inline Vec      norm(const Vec &v);             // v / || v ||
inline Void     normalise(Vec &v);              // v = norm(v)

Vec             clamped(const Vec &v, Real fuzz);
Vec             clamped(const Vec &v);


// --- Vec Input & Output -----------------------------------------------------

std::ostream         &operator << (std::ostream &s, const Vec &v);
std::istream         &operator >> (std::istream &s, Vec &v);


// --- Sub-vector functions ---------------------------------------------------

inline Vec      sub(const Vec &v, Int start, Int length);
inline Vec      first(const Vec &v, Int length);
inline Vec      last(const Vec &v, Int length);


// --- Vec inlines ------------------------------------------------------------

inline Vec::Vec() : data(0), elts(0)
{
}

inline Vec::Vec(Int n) : elts(n)
{
    Assert(n > 0,"(Vec) illegal vector size");

    data = new Real[n];
}

inline Vec::Vec(Int n, Real *data) : data(data), elts(n | VL_REF_FLAG)
{
}

inline Int Vec::Elts() const
{
    return(elts & VL_REF_MASK);
}

inline Real &Vec::operator [] (Int i)
{
    CheckRange(i, 0, Elts(), "Vec::[i]");

    return(data[i]);
}

inline Real Vec::operator [] (Int i) const
{
    CheckRange(i, 0, Elts(), "Vec::[i]");

    return(data[i]);
}

inline Real *Vec::Ref() const
{
    return(data);
}

inline Vec &Vec::operator = (ZeroOrOne k)
{
    MakeBlock(k);

    return(SELF);
}

inline Vec &Vec::operator = (Axis a)
{
    MakeUnit(a);

    return(SELF);
}

inline Real len(const Vec &v)
{
    return(sqrt(dot(v, v)));
}

inline Real sqrlen(const Vec &v)
{
    return(dot(v, v));
}

inline Vec norm(const Vec &v)
{
    Assert(sqrlen(v) > 0.0, "normalising length-zero vector");
    return(v / len(v));
}

inline Void normalise(Vec &v)
{
    v /= len(v);
}

inline Vec sub(const Vec &v, Int start, Int length)
{
    Assert(start >= 0 && length > 0 && start + length <= v.Elts(),
        "(sub(Vec)) illegal subset of vector");

    return(Vec(length, v.Ref() + start));
}

inline Vec first(const Vec &v, Int length)
{
    Assert(length > 0 && length <= v.Elts(),
        "(first(Vec)) illegal subset of vector");

    return(Vec(length, v.Ref()));
}

inline Vec last(const Vec &v, Int length)
{
    Assert(length > 0 && length <= v.Elts(),
        "(last(Vec)) illegal subset of vector");

    return(Vec(length, v.Ref() + v.Elts() - length));
}

inline Vec &Vec::Normalise()
{
    Assert(sqrlen(SELF) > 0.0, "normalising length-zero vector");
    SELF /= len(SELF);
    return(SELF);
}


#endif

