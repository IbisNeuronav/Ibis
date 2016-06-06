/*
    File:           Mat4.cpp

    Function:       Implements Mat4.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/

#include "Mat4.h"
#include <cctype>
#include <iomanip>


Mat4::Mat4(Real a, Real b, Real c, Real d,
           Real e, Real f, Real g, Real h,
           Real i, Real j, Real k, Real l,
           Real m, Real n, Real o, Real p)
{
    row[0][0] = a;  row[0][1] = b;  row[0][2] = c;  row[0][3] = d;
    row[1][0] = e;  row[1][1] = f;  row[1][2] = g;  row[1][3] = h;
    row[2][0] = i;  row[2][1] = j;  row[2][2] = k;  row[2][3] = l;
    row[3][0] = m;  row[3][1] = n;  row[3][2] = o;  row[3][3] = p;
}

Mat4::Mat4(const Mat4 &m)
{
    row[0] = m[0];
    row[1] = m[1];
    row[2] = m[2];
    row[3] = m[3];
}


Mat4 &Mat4::operator = (const Mat4 &m)
{
    row[0] = m[0];
    row[1] = m[1];
    row[2] = m[2];
    row[3] = m[3];

    return(SELF);
}

Mat4 &Mat4::operator += (const Mat4 &m)
{
    row[0] += m[0];
    row[1] += m[1];
    row[2] += m[2];
    row[3] += m[3];

    return(SELF);
}

Mat4 &Mat4::operator -= (const Mat4 &m)
{
    row[0] -= m[0];
    row[1] -= m[1];
    row[2] -= m[2];
    row[3] -= m[3];

    return(SELF);
}

Mat4 &Mat4::operator *= (const Mat4 &m)
{
    SELF = SELF * m;

    return(SELF);
}

Mat4 &Mat4::operator *= (Real s)
{
    row[0] *= s;
    row[1] *= s;
    row[2] *= s;
    row[3] *= s;

    return(SELF);
}

Mat4 &Mat4::operator /= (Real s)
{
    row[0] /= s;
    row[1] /= s;
    row[2] /= s;
    row[3] /= s;

    return(SELF);
}


Bool Mat4::operator == (const Mat4 &m) const
{
    return(row[0] == m[0] && row[1] == m[1] && row[2] == m[2] && row[3] == m[3]);
}

Bool Mat4::operator != (const Mat4 &m) const
{
    return(row[0] != m[0] || row[1] != m[1] || row[2] != m[2] || row[3] != m[3]);
}


Mat4 Mat4::operator + (const Mat4 &m) const
{
    Mat4 result;

    result[0] = row[0] + m[0];
    result[1] = row[1] + m[1];
    result[2] = row[2] + m[2];
    result[3] = row[3] + m[3];

    return(result);
}

Mat4 Mat4::operator - (const Mat4 &m) const
{
    Mat4 result;

    result[0] = row[0] - m[0];
    result[1] = row[1] - m[1];
    result[2] = row[2] - m[2];
    result[3] = row[3] - m[3];

    return(result);
}

Mat4 Mat4::operator - () const
{
    Mat4 result;

    result[0] = -row[0];
    result[1] = -row[1];
    result[2] = -row[2];
    result[3] = -row[3];

    return(result);
}

Mat4 Mat4::operator * (const Mat4 &m) const
{
#define N(x,y) row[x][y]
#define M(x,y) m[x][y]
#define R(x,y) result[x][y]

    Mat4 result;

    R(0,0) = N(0,0) * M(0,0) + N(0,1) * M(1,0) + N(0,2) * M(2,0) + N(0,3) * M(3,0);
    R(0,1) = N(0,0) * M(0,1) + N(0,1) * M(1,1) + N(0,2) * M(2,1) + N(0,3) * M(3,1);
    R(0,2) = N(0,0) * M(0,2) + N(0,1) * M(1,2) + N(0,2) * M(2,2) + N(0,3) * M(3,2);
    R(0,3) = N(0,0) * M(0,3) + N(0,1) * M(1,3) + N(0,2) * M(2,3) + N(0,3) * M(3,3);

    R(1,0) = N(1,0) * M(0,0) + N(1,1) * M(1,0) + N(1,2) * M(2,0) + N(1,3) * M(3,0);
    R(1,1) = N(1,0) * M(0,1) + N(1,1) * M(1,1) + N(1,2) * M(2,1) + N(1,3) * M(3,1);
    R(1,2) = N(1,0) * M(0,2) + N(1,1) * M(1,2) + N(1,2) * M(2,2) + N(1,3) * M(3,2);
    R(1,3) = N(1,0) * M(0,3) + N(1,1) * M(1,3) + N(1,2) * M(2,3) + N(1,3) * M(3,3);

    R(2,0) = N(2,0) * M(0,0) + N(2,1) * M(1,0) + N(2,2) * M(2,0) + N(2,3) * M(3,0);
    R(2,1) = N(2,0) * M(0,1) + N(2,1) * M(1,1) + N(2,2) * M(2,1) + N(2,3) * M(3,1);
    R(2,2) = N(2,0) * M(0,2) + N(2,1) * M(1,2) + N(2,2) * M(2,2) + N(2,3) * M(3,2);
    R(2,3) = N(2,0) * M(0,3) + N(2,1) * M(1,3) + N(2,2) * M(2,3) + N(2,3) * M(3,3);

    R(3,0) = N(3,0) * M(0,0) + N(3,1) * M(1,0) + N(3,2) * M(2,0) + N(3,3) * M(3,0);
    R(3,1) = N(3,0) * M(0,1) + N(3,1) * M(1,1) + N(3,2) * M(2,1) + N(3,3) * M(3,1);
    R(3,2) = N(3,0) * M(0,2) + N(3,1) * M(1,2) + N(3,2) * M(2,2) + N(3,3) * M(3,2);
    R(3,3) = N(3,0) * M(0,3) + N(3,1) * M(1,3) + N(3,2) * M(2,3) + N(3,3) * M(3,3);

    return(result);

#undef N
#undef M
#undef R
}

Mat4 Mat4::operator * (Real s) const
{
    Mat4 result;

    result[0] = row[0] * s;
    result[1] = row[1] * s;
    result[2] = row[2] * s;
    result[3] = row[3] * s;

    return(result);
}

Mat4 Mat4::operator / (Real s) const
{
    Mat4 result;

    result[0] = row[0] / s;
    result[1] = row[1] / s;
    result[2] = row[2] / s;
    result[3] = row[3] / s;

    return(result);
}

Vec4 operator * (const Mat4 &m, const Vec4 &v)          // m * v
{
    Vec4 result;

    result[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + v[3] * m[0][3];
    result[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + v[3] * m[1][3];
    result[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + v[3] * m[2][3];
    result[3] = v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m[3][2] + v[3] * m[3][3];

    return(result);
}

Vec4 operator * (const Vec4 &v, const Mat4 &m)          // v * m
{
    Vec4 result;

    result[0] = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0];
    result[1] = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1];
    result[2] = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2];
    result[3] = v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3];

    return(result);
}

Vec4 &operator *= (Vec4 &v, const Mat4 &m)              // v *= m
{
    Real    t0, t1, t2;

    t0   = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + v[3] * m[3][0];
    t1   = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + v[3] * m[3][1];
    t2   = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + v[3] * m[3][2];
    v[3] = v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m[2][3] + v[3] * m[3][3];
    v[0] = t0;
    v[1] = t1;
    v[2] = t2;

    return(v);
}

Mat4 trans(const Mat4 &m)
{
#define M(x,y) m[x][y]
#define R(x,y) result[x][y]

    Mat4 result;

    R(0,0) = M(0,0); R(0,1) = M(1,0); R(0,2) = M(2,0); R(0,3) = M(3,0);
    R(1,0) = M(0,1); R(1,1) = M(1,1); R(1,2) = M(2,1); R(1,3) = M(3,1);
    R(2,0) = M(0,2); R(2,1) = M(1,2); R(2,2) = M(2,2); R(2,3) = M(3,2);
    R(3,0) = M(0,3); R(3,1) = M(1,3); R(3,2) = M(2,3); R(3,3) = M(3,3);

    return(result);

#undef M
#undef R
}

Mat4 adj(const Mat4 &m)
{
    Mat4    result;

    result[0] =  cross(m[1], m[2], m[3]);
    result[1] = -cross(m[0], m[2], m[3]);
    result[2] =  cross(m[0], m[1], m[3]);
    result[3] = -cross(m[0], m[1], m[2]);

    return(result);
}

Real trace(const Mat4 &m)
{
    return(m[0][0] + m[1][1] + m[2][2] + m[3][3]);
}

Real det(const Mat4 &m)
{
    return(dot(m[0], cross(m[1], m[2], m[3])));
}

Mat4 inv(const Mat4 &m)
{
    Real    mDet;
    Mat4    adjoint;
    Mat4    result;

    adjoint = adj(m);               // Find the adjoint
    mDet = dot(adjoint[0], m[0]);

    Assert(mDet != 0, "(Mat4::inv) matrix is non-singular");

    result = trans(adjoint);
    result /= mDet;

    return(result);
}

Mat4 oprod(const Vec4 &a, const Vec4 &b)
// returns outerproduct of a and b:  a * trans(b)
{
    Mat4    result;

    result[0] = a[0] * b;
    result[1] = a[1] * b;
    result[2] = a[2] * b;
    result[3] = a[3] * b;

    return(result);
}

Void Mat4::MakeZero()
{
    Int     i;

    for (i = 0; i < 16; i++)
        ((Real *) row)[i] = vl_zero;
}

Void Mat4::MakeDiag(Real k)
{
    Int     i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            if (i == j)
                row[i][j] = k;
            else
                row[i][j] = vl_zero;
}

Void Mat4::MakeBlock(Real k)
{
    Int     i;

    for (i = 0; i < 16; i++)
        ((Real *) row)[i] = k;
}

ostream &operator << (ostream &s, const Mat4 &m)
{
    Int w = s.width();

    return(s << '[' << m[0] << endl << setw(w) << m[1] << endl
         << setw(w) << m[2] << endl << setw(w) << m[3] << ']' << endl);
}

istream &operator >> (istream &s, Mat4 &m)
{
    Mat4    result;
    Char    c;

    // Expected format: [[1 2 3] [4 5 6] [7 8 9]]
    // Each vector is a column of the matrix.

    while (s >> c && isspace(c))        // ignore leading white space
        ;

    if (c == '[')
    {
        s >> result[0] >> result[1] >> result[2] >> result[3];

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


Mat4& Mat4::MakeHRot(const Vec4 &q)
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

    MakeDiag();

    i2 *= q[0];
    j2 *= q[1];
    k2 *= q[2];

#if VL_ROW_ORIENT
    row[0][0] = 1 - j2 - k2;  row[0][1] = ij + rk   ;  row[0][2] = ik - rj;
    row[1][0] = ij - rk    ;  row[1][1] = 1 - i2- k2;  row[1][2] = jk + ri;
    row[2][0] = ik + rj    ;  row[2][1] = jk - ri   ;  row[2][2] = 1 - i2 - j2;
#else
    row[0][0] = 1 - j2 - k2;  row[0][1] = ij - rk   ;  row[0][2] = ik + rj;
    row[1][0] = ij + rk    ;  row[1][1] = 1 - i2- k2;  row[1][2] = jk - ri;
    row[2][0] = ik - rj    ;  row[2][1] = jk + ri   ;  row[2][2] = 1 - i2 - j2;
#endif

    return(SELF);
}

Mat4& Mat4::MakeHRot(const Vec3 &axis, Real theta)
{
    Real        s;
    Vec4        q;

    theta /= 2.0;
    s = sin(theta);

    q[0] = s * axis[0];
    q[1] = s * axis[1];
    q[2] = s * axis[2];
    q[3] = cos(theta);

    MakeHRot(q);

    return(SELF);
}

Mat4& Mat4::MakeHScale(const Vec3 &s)
{
    MakeDiag();

    row[0][0] = s[0];
    row[1][1] = s[1];
    row[2][2] = s[2];

    return(SELF);
}

Mat4& Mat4::MakeHTrans(const Vec3 &t)
{
    MakeDiag();

#ifdef VL_ROW_ORIENT
    row[3][0] = t[0];
    row[3][1] = t[1];
    row[3][2] = t[2];
#else
    row[0][3] = t[0];
    row[1][3] = t[1];
    row[2][3] = t[2];
#endif

    return(SELF);
}

Mat4& Mat4::Transpose()
{
    row[0][1] = row[1][0]; row[0][2] = row[2][0]; row[0][3] = row[3][0];
    row[1][0] = row[0][1]; row[1][2] = row[2][1]; row[1][3] = row[3][1];
    row[2][0] = row[0][2]; row[2][1] = row[1][2]; row[2][3] = row[3][2];
    row[3][0] = row[0][3]; row[3][1] = row[1][3]; row[3][2] = row[2][3];

    return(SELF);
}

Mat4& Mat4::AddShift(const Vec3 &t)
{
#ifdef VL_ROW_ORIENT
    row[3][0] += t[0];
    row[3][1] += t[1];
    row[3][2] += t[2];
#else
    row[0][3] += t[0];
    row[1][3] += t[1];
    row[2][3] += t[2];
#endif

    return(SELF);
}
