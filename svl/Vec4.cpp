/*
    File:           Vec4.cpp

    Function:       Implements Vec4.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/


#include "Vec4.h"
#include <cctype>
#include <iomanip>


Vec4 &Vec4::MakeUnit(Int n, Real k)
{
    if (n == 0)
    { elt[0] = k; elt[1] = vl_zero; elt[2] = vl_zero; elt[3] = vl_zero; }
    else if (n == 1)
    { elt[0] = vl_zero; elt[1] = k; elt[2] = vl_zero; elt[3] = vl_zero; }
    else if (n == 2)
    { elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = k; elt[3] = vl_zero; }
    else if (n == 3)
    { elt[0] = vl_zero; elt[1] = vl_zero; elt[2] = vl_zero; elt[3] = k; }
    else
        _Error("(Vec4::MakeUnit) illegal unit vector");

    return(SELF);
}

Bool Vec4::operator == (const Vec4 &a) const
{
    return(elt[0] == a[0] && elt[1] == a[1] && elt[2] == a[2] && elt[3] == a[3]);
}

Bool Vec4::operator != (const Vec4 &a) const
{
    return(elt[0] != a[0] || elt[1] != a[1] || elt[2] != a[2] || elt[3] != a[3]);
}

Vec4 cross(const Vec4 &a, const Vec4 &b, const Vec4 &c)
{
    Vec4 result;
    // XXX can this be improved? Look at assembly.
#define ROW(i)       a[i], b[i], c[i]
#define DET(i,j,k)   dot(Vec3(ROW(i)), cross(Vec3(ROW(j)), Vec3(ROW(k))))

    result[0] =  DET(1,2,3);
    result[1] = -DET(0,2,3);
    result[2] =  DET(0,1,3);
    result[3] = -DET(0,1,2);

    return(result);

#undef ROW
#undef DET
}

Vec3 proj(const Vec4 &v)
{
    Vec3 result;

    Assert(v[3] != 0, "(Vec4/proj) last elt. is zero");

    result[0] = v[0] / v[3];
    result[1] = v[1] / v[3];
    result[2] = v[2] / v[3];

    return(result);
}


ostream &operator << (ostream &s, const Vec4 &v)
{
    Int w = s.width();

    return(s << '[' << v[0] << ' ' << setw(w) << v[1] << ' '
        << setw(w) << v[2] << ' ' << setw(w) << v[3] << ']');
}

istream &operator >> (istream &s, Vec4 &v)
{
    Vec4    result;
    Char    c;

    // Expected format: [1 2 3 4]

    while (s >> c && isspace(c))
        ;

    if (c == '[')
    {
        s >> result[0] >> result[1] >> result[2] >> result[3];

        if (!s)
        {
            cerr << "Error: Expected number while reading vector\n";
            return(s);
        }

        while (s >> c && isspace(c))
            ;

        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Error: Expected ']' while reading vector\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Error: Expected '[' while reading vector\n";
        return(s);
    }

    v = result;
    return(s);
}

