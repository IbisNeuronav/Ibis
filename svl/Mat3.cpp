/*
    File:           Mat3.cpp

    Function:       Implements Mat3.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/

#include "Mat3.h"
#include "Vec4.h"
#include <cctype>
#include <iomanip>


Mat3::Mat3(Real a, Real b, Real c,
           Real d, Real e, Real f,
           Real g, Real h, Real i)
{
    row[0][0] = a;  row[0][1] = b;  row[0][2] = c;
    row[1][0] = d;  row[1][1] = e;  row[1][2] = f;
    row[2][0] = g;  row[2][1] = h;  row[2][2] = i;
}

Mat3::Mat3(const Mat3 &m)
{
    row[0] = m[0];
    row[1] = m[1];
    row[2] = m[2];
}

Mat3 &Mat3::operator = (const Mat3 &m)
{
    row[0] = m[0];
    row[1] = m[1];
    row[2] = m[2];

    return(SELF);
}

Mat3 &Mat3::operator += (const Mat3 &m)
{
    row[0] += m[0];
    row[1] += m[1];
    row[2] += m[2];

    return(SELF);
}

Mat3 &Mat3::operator -= (const Mat3 &m)
{
    row[0] -= m[0];
    row[1] -= m[1];
    row[2] -= m[2];

    return(SELF);
}

Mat3 &Mat3::operator *= (const Mat3 &m)
{
    SELF = SELF * m;

    return(SELF);
}

Mat3 &Mat3::operator *= (Real s)
{
    row[0] *= s;
    row[1] *= s;
    row[2] *= s;

    return(SELF);
}

Mat3 &Mat3::operator /= (Real s)
{
    row[0] /= s;
    row[1] /= s;
    row[2] /= s;

    return(SELF);
}


Bool Mat3::operator == (const Mat3 &m) const
{
    return(row[0] == m[0] && row[1] == m[1] && row[2] == m[2]);
}

Bool Mat3::operator != (const Mat3 &m) const
{
    return(row[0] != m[0] || row[1] != m[1] || row[2] != m[2]);
}


Mat3 Mat3::operator + (const Mat3 &m) const
{
    Mat3 result;

    result[0] = row[0] + m[0];
    result[1] = row[1] + m[1];
    result[2] = row[2] + m[2];

    return(result);
}

Mat3 Mat3::operator - (const Mat3 &m) const
{
    Mat3 result;

    result[0] = row[0] - m[0];
    result[1] = row[1] - m[1];
    result[2] = row[2] - m[2];

    return(result);
}

Mat3 Mat3::operator - () const
{
    Mat3 result;

    result[0] = -row[0];
    result[1] = -row[1];
    result[2] = -row[2];

    return(result);
}

Mat3 Mat3::operator * (const Mat3 &m) const
{
#define N(x,y) row[x][y]
#define M(x,y) m[x][y]
#define R(x,y) result[x][y]

    Mat3 result;

    R(0,0) = N(0,0) * M(0,0) + N(0,1) * M(1,0) + N(0,2) * M(2,0);
    R(0,1) = N(0,0) * M(0,1) + N(0,1) * M(1,1) + N(0,2) * M(2,1);
    R(0,2) = N(0,0) * M(0,2) + N(0,1) * M(1,2) + N(0,2) * M(2,2);

    R(1,0) = N(1,0) * M(0,0) + N(1,1) * M(1,0) + N(1,2) * M(2,0);
    R(1,1) = N(1,0) * M(0,1) + N(1,1) * M(1,1) + N(1,2) * M(2,1);
    R(1,2) = N(1,0) * M(0,2) + N(1,1) * M(1,2) + N(1,2) * M(2,2);

    R(2,0) = N(2,0) * M(0,0) + N(2,1) * M(1,0) + N(2,2) * M(2,0);
    R(2,1) = N(2,0) * M(0,1) + N(2,1) * M(1,1) + N(2,2) * M(2,1);
    R(2,2) = N(2,0) * M(0,2) + N(2,1) * M(1,2) + N(2,2) * M(2,2);

    return(result);

#undef N
#undef M
#undef R
}

Mat3 Mat3::operator * (Real s) const
{
    Mat3 result;

    result[0] = row[0] * s;
    result[1] = row[1] * s;
    result[2] = row[2] * s;

    return(result);
}

Mat3 Mat3::operator / (Real s) const
{
    Mat3 result;

    result[0] = row[0] / s;
    result[1] = row[1] / s;
    result[2] = row[2] / s;

    return(result);
}

Mat3 trans(const Mat3 &m)
{
#define M(x,y) m[x][y]
#define R(x,y) result[x][y]

    Mat3 result;

    R(0,0) = M(0,0); R(0,1) = M(1,0); R(0,2) = M(2,0);
    R(1,0) = M(0,1); R(1,1) = M(1,1); R(1,2) = M(2,1);
    R(2,0) = M(0,2); R(2,1) = M(1,2); R(2,2) = M(2,2);

    return(result);

#undef M
#undef R
}

Mat3 adj(const Mat3 &m)
{
    Mat3    result;

    result[0] = cross(m[1], m[2]);
    result[1] = cross(m[2], m[0]);
    result[2] = cross(m[0], m[1]);

    return(result);
}


Real trace(const Mat3 &m)
{
    return(m[0][0] + m[1][1] + m[2][2]);
}

Real det(const Mat3 &m)
{
    return(dot(m[0], cross(m[1], m[2])));
}

Mat3 inv(const Mat3 &m)
{
    Real    mDet;
    Mat3    adjoint;
    Mat3    result;

    adjoint = adj(m);
    mDet = dot(adjoint[0], m[0]);

    Assert(mDet != 0, "(Mat3::inv) matrix is non-singular");

    result = trans(adjoint);
    result /= mDet;

    return(result);
}

Mat3 oprod(const Vec3 &a, const Vec3 &b)
// returns outerproduct of a and b:  a * trans(b)
{
    Mat3    result;

    result[0] = a[0] * b;
    result[1] = a[1] * b;
    result[2] = a[2] * b;

    return(result);
}

Void Mat3::MakeZero()
{
    Int     i;

    for (i = 0; i < 9; i++)
        ((Real*) row)[i] = vl_zero;
}

Void Mat3::MakeDiag(Real k)
{
    Int     i, j;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            if (i == j)
                row[i][j] = k;
            else
                row[i][j] = vl_zero;
}

Void Mat3::MakeBlock(Real k)
{
    Int     i;

    for (i = 0; i < 9; i++)
        ((Real *) row)[i] = k;
}

ostream &operator << (ostream &s, const Mat3 &m)
{
    Int     w = s.width();

    return(s << '[' << m[0] << endl << setw(w) << m[1] << endl << setw(w)
           << m[2] << ']' << endl);
}

istream &operator >> (istream &s, Mat3 &m)
{
    Mat3    result;
    Char    c;

    // Expected format: [[1 2 3] [4 5 6] [7 8 9]]
    // Each vector is a column of the matrix.

    while (s >> c && isspace(c))        // ignore leading white space
        ;

    if (c == '[')
    {
        s >> result[0] >> result[1] >> result[2];

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


Mat3 &Mat3::MakeRot(const Vec4 &q)
{
    Real    i2 =  2 * q[0],
            j2 =  2 * q[1],
            k2 =  2 * q[2],
            ij = i2 * q[1],
            ik = i2 * q[2],
            jk = j2 * q[2],
            ri = i2 * q[3],
            rj = j2 * q[3],
            rk = k2 * q[3];

    i2 *= q[0];
    j2 *= q[1];
    k2 *= q[2];

#if VL_ROW_ORIENT
    row[0][0] = 1 - j2 - k2;  row[0][1] = ij + rk    ;  row[0][2] = ik - rj;
    row[1][0] = ij - rk    ;  row[1][1] = 1 - i2 - k2;  row[1][2] = jk + ri;
    row[2][0] = ik + rj    ;  row[2][1] = jk - ri    ;  row[2][2] = 1 - i2 - j2;
#else
    row[0][0] = 1 - j2 - k2;  row[0][1] = ij - rk    ;  row[0][2] = ik + rj;
    row[1][0] = ij + rk    ;  row[1][1] = 1 - i2 - k2;  row[1][2] = jk - ri;
    row[2][0] = ik - rj    ;  row[2][1] = jk + ri    ;  row[2][2] = 1 - i2 - j2;
#endif

    return(SELF);
}

Mat3 &Mat3::MakeRot(const Vec3 &axis, Real theta)
{
    Real            s;
    Vec4            q;

    theta /= 2.0;
    s = sin(theta);

    q[0] = s * axis[0];
    q[1] = s * axis[1];
    q[2] = s * axis[2];
    q[3] = cos(theta);

    MakeRot(q);

    return(SELF);
}

Mat3 &Mat3::MakeScale(const Vec3 &s)
{
    MakeZero();

    row[0][0] = s[0];
    row[1][1] = s[1];
    row[2][2] = s[2];

    return(SELF);
}

Mat3 &Mat3::MakeHRot(Real theta)
{
    Real    c, s;

    MakeDiag();

    s = sin(theta);
    c = cos(theta);

#ifdef VL_ROW_ORIENT
    row[0][0] =  c; row[0][1] =  s;
    row[1][0] = -s; row[1][1] =  c;
#else
    row[0][0] =  c; row[0][1] = -s;
    row[1][0] =  s; row[1][1] =  c;
#endif

    return(SELF);
}

Mat3 &Mat3::MakeHScale(const Vec2 &s)
{
    MakeDiag();

    row[0][0] = s[0];
    row[1][1] = s[1];

    return(SELF);
}

Mat3 &Mat3::MakeHTrans(const Vec2 &t)
{
    MakeDiag();

#ifdef VL_ROW_ORIENT
    row[2][0] = t[0];
    row[2][1] = t[1];
#else
    row[0][2] = t[0];
    row[1][2] = t[1];
#endif

    return(SELF);
}
