/*
    File:           Mat2.cpp

    Function:       Implements Mat2.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/

#include "Mat2.h"
#include <cctype>
#include <iomanip>


Bool Mat2::operator == (const Mat2 &m) const
{
    return(row[0] == m[0] && row[1] == m[1]);
}

Bool Mat2::operator != (const Mat2 &m) const
{
    return(row[0] != m[0] || row[1] != m[1]);
}


Real det(const Mat2 &m)
{
    return(dot(m[0], cross(m[1])));
}

Mat2 inv(const Mat2 &m)
{
    Real            mDet;
    Mat2            result;

    result[0][0] =  m[1][1]; result[0][1] = -m[0][1];
    result[1][0] = -m[1][0]; result[1][1] =  m[0][0];

    mDet = m[0][0] * result[0][0] + m[0][1] * result[1][0];
    Assert(mDet != 0.0, "(Mat2::inv) matrix is non-singular");
    result /= mDet;

    return(result);
}

Mat2 oprod(const Vec2 &a, const Vec2 &b)
// returns outerproduct of a and b:  a * trans(b)
{
    Mat2    result;

    result[0] = a[0] * b;
    result[1] = a[1] * b;

    return(result);
}

ostream &operator << (ostream &s, const Mat2 &m)
{
    Int w = s.width();

    return(s << '[' << m[0] << endl << setw(w) << m[1] << ']' << endl);
}

istream &operator >> (istream &s, Mat2 &m)
{
    Mat2    result;
    Char    c;

    // Expected format: [[1 2] [3 4]]
    // Each vector is a row of the row matrix.

    while (s >> c && isspace(c))        // ignore leading white space
        ;

    if (c == '[')
    {
        s >> result[0] >> result[1];

        if (!s)
        {
            cerr << "Expected number while reading matrix\n";
            return(s);
        }

        while (s >> c && isspace(c))
            ;

        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Expected ']' while reading matrix\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Expected '[' while reading matrix\n";
        return(s);
    }

    m = result;
    return(s);
}


Mat2 &Mat2::MakeRot(Real theta)
{
    Real    c, s;

    SetReal(s, sin(theta));
    SetReal(c, cos(theta));

#ifdef VL_ROW_ORIENT
    row[0][0] =  c; row[0][1] = s;
    row[1][0] = -s; row[1][1] = c;
#else
    row[0][0] = c; row[0][1] = -s;
    row[1][0] = s; row[1][1] =  c;
#endif

    return(SELF);
}

Mat2 &Mat2::MakeScale(const Vec2 &s)
{
    row[0][0] = s[0]; row[0][1] = vl_0;
    row[1][0] = vl_0; row[1][1] = s[1];

    return(SELF);
}

