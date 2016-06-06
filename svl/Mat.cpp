/*
    File:           Mat.cpp

    Function:       Implements Mat.h

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott

*/

#include "Mat.h"

#include <cctype>
#include <cstring>
#include <cstdarg>
#include <iomanip>


// --- Mat Constructors & Destructors -----------------------------------------


Mat::Mat(Int rows, Int cols, ZeroOrOne k) : rows(rows), cols(cols)
{
    Assert(rows > 0 && cols > 0, "(Mat) illegal matrix size");

    data = new Real[rows * cols];

    MakeDiag(k);
}

Mat::Mat(Int rows, Int cols, Block k) : rows(rows), cols(cols)
{
    Assert(rows > 0 && cols > 0, "(Mat) illegal matrix size");

    data = new Real[rows * cols];

    MakeBlock(k);
}

Mat::Mat(Int rows, Int cols, double elt0, ...) : rows(rows), cols(cols)
// The double is hardwired here because it is the only type that will work
// with var args and C++ real numbers.
{
    Assert(rows > 0 && cols > 0, "(Mat) illegal matrix size");

    va_list ap;
    Int     i, j;

    data = new Real[rows * cols];
    va_start(ap, elt0);

    SetReal(data[0], elt0);

    for (i = 1; i < cols; i++)
        SetReal(Elt(0, i), va_arg(ap, double));

    for (i = 1; i < rows; i++)
        for (j = 0; j < cols; j++)
            SetReal(Elt(i, j), va_arg(ap, double));

    va_end(ap);
}

Mat::Mat(const Mat &m) : cols(m.cols)
{
    Assert(m.data != 0, "(Mat) Can't construct from null matrix");
    rows = m.Rows();

    UInt    elts = rows * cols;

    data = new Real[elts];
#ifdef VL_USE_MEMCPY
    memcpy(data, m.data, elts * sizeof(Real));
#else
    for (UInt i = 0; i < elts; i++)
        data[i] = m.data[i];
#endif
}

Mat::Mat(const Mat2 &m) : data(m.Ref()), rows(2 | VL_REF_FLAG), cols(2)
{
}

Mat::Mat(const Mat3 &m) : data(m.Ref()), rows(3 | VL_REF_FLAG), cols(3)
{
}

Mat::Mat(const Mat4 &m) : data(m.Ref()), rows(4 | VL_REF_FLAG), cols(4)
{
}


// --- Mat Assignment Operators -----------------------------------------------


Mat &Mat::operator = (const Mat &m)
{
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else
        Assert(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
    for (Int i = 0; i < Rows(); i++)
        SELF[i] = m[i];

    return(SELF);
}

Mat &Mat::operator = (const Mat2 &m)
{
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else
        Assert(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
    for (Int i = 0; i < Rows(); i++)
        SELF[i] = m[i];

    return(SELF);
}

Mat &Mat::operator = (const Mat3 &m)
{
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else
        Assert(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");
    for (Int i = 0; i < Rows(); i++)
        SELF[i] = m[i];

    return(SELF);
}

Mat &Mat::operator = (const Mat4 &m)
{
    if (!IsRef())
        SetSize(m.Rows(), m.Cols());
    else
        Assert(Rows() == m.Rows(), "(Mat::=) Matrix rows don't match");

    for (Int i = 0; i < Rows(); i++)
        SELF[i] = m[i];

    return(SELF);
}

Void Mat::SetSize(Int nrows, Int ncols)
{
	UInt	elts = nrows * ncols;
	Assert(nrows > 0 && ncols > 0, "(Mat::SetSize) Illegal matrix size.");
	UInt	oldElts = Rows() * Cols();

	if (IsRef())
	{
		// Abort! We don't allow this operation on references.
		_Error("(Mat::SetSize) Trying to resize a matrix reference");
	}

	rows = nrows;
	cols = ncols;

	// Don't reallocate if we already have enough storage
	if (elts <= oldElts)
		return;

	// Otherwise, delete old storage and reallocate
	delete[] data;
	data = 0;
	data = new Real[elts]; // may throw an exception
}

Void Mat::SetSize(const Mat &m)
{
    SetSize(m.Rows(), m.Cols());
}

Void Mat::MakeZero()
{
#ifdef VL_USE_MEMCPY
    memset(data, 0, sizeof(Real) * Rows() * Cols());
#else
    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i] = vl_zero;
#endif
}

Void Mat::MakeDiag(Real k)
{
    Int     i, j;

    for (i = 0; i < Rows(); i++)
        for (j = 0; j < Cols(); j++)
            if (i == j)
                Elt(i,j) = k;
            else
                Elt(i,j) = vl_zero;
}

Void Mat::MakeDiag()
{
    Int     i, j;

    for (i = 0; i < Rows(); i++)
        for (j = 0; j < Cols(); j++)
            Elt(i,j) = (i == j) ? vl_one : vl_zero;
}

Void Mat::MakeBlock(Real k)
{
    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i].MakeBlock(k);
}

Void Mat::MakeBlock()
{
    Int     i, j;

    for (i = 0; i < Rows(); i++)
        for (j = 0; j < Cols(); j++)
            Elt(i,j) = vl_one;
}


// --- Mat Assignment Operators -----------------------------------------------


Mat &Mat::operator += (const Mat &m)
{
    Assert(Rows() == m.Rows(), "(Mat::+=) matrix rows don't match");

    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i] += m[i];

    return(SELF);
}

Mat &Mat::operator -= (const Mat &m)
{
    Assert(Rows() == m.Rows(), "(Mat::-=) matrix rows don't match");

    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i] -= m[i];

    return(SELF);
}

Mat &Mat::operator *= (const Mat &m)
{
    Assert(Cols() == m.Cols(), "(Mat::*=) matrix columns don't match");

    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i] = SELF[i] * m;

    return(SELF);
}

Mat &Mat::operator *= (Real s)
{
    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i] *= s;

    return(SELF);
}

Mat &Mat::operator /= (Real s)
{
    Int     i;

    for (i = 0; i < Rows(); i++)
        SELF[i] /= s;

    return(SELF);
}


// --- Mat Comparison Operators -----------------------------------------------


Bool operator == (const Mat &m, const Mat &n)
{
    Assert(n.Rows() == m.Rows(), "(Mat::==) matrix rows don't match");

    Int     i;

    for (i = 0; i < m.Rows(); i++)
        if (m[i] != n[i])
            return(0);

    return(1);
}

Bool operator != (const Mat &m, const Mat &n)
{
    Assert(n.Rows() == m.Rows(), "(Mat::!=) matrix rows don't match");

    Int     i;

    for (i = 0; i < m.Rows(); i++)
        if (m[i] != n[i])
            return(1);

    return(0);
}


// --- Mat Arithmetic Operators -----------------------------------------------


Mat operator + (const Mat &m, const Mat &n)
{
    Assert(n.Rows() == m.Rows(), "(Mat::+) matrix rows don't match");

    Mat result(m.Rows(), m.Cols());
    Int     i;

    for (i = 0; i < m.Rows(); i++)
        result[i] = m[i] + n[i];

    return(result);
}

Mat operator - (const Mat &m, const Mat &n)
{
    Assert(n.Rows() == m.Rows(), "(Mat::-) matrix rows don't match");

    Mat result(m.Rows(), m.Cols());
    Int     i;

    for (i = 0; i < m.Rows(); i++)
        result[i] = m[i] - n[i];

    return(result);
}

Mat operator - (const Mat &m)
{
    Mat result(m.Rows(), m.Cols());
    Int     i;

    for (i = 0; i < m.Rows(); i++)
        result[i] = -m[i];

    return(result);
}

Mat operator * (const Mat &m, const Mat &n)
{
    Assert(m.Cols() == n.Rows(), "(Mat::*m) matrix cols don't match");

    Mat result(m.Rows(), n.Cols());
    Int     i;

    for (i = 0; i < m.Rows(); i++)
        result[i] = m[i] * n;

    return(result);
}

Vec operator * (const Mat &m, const Vec &v)
{
    Assert(m.Cols() == v.Elts(), "(Mat::*v) matrix and vector sizes don't match");

    Int     i;
    Vec result(m.Rows());

    for (i = 0; i < m.Rows(); i++)
        result[i] = dot(v, m[i]);

    return(result);
}

Mat operator * (const Mat &m, Real s)
{
    Int     i;
    Mat result(m.Rows(), m.Cols());

    for (i = 0; i < m.Rows(); i++)
        result[i] = m[i] * s;

    return(result);
}

Mat operator / (const Mat &m, Real s)
{
    Int     i;
    Mat result(m.Rows(), m.Cols());

    for (i = 0; i < m.Rows(); i++)
        result[i] = m[i] / s;

    return(result);
}


// --- Mat Mat-Vec Functions --------------------------------------------------


Vec operator * (const Vec &v, const Mat &m)         // v * m
{
    Assert(v.Elts() == m.Rows(), "(Mat::v*m) vector/matrix sizes don't match");

    Vec     result(m.Cols(), vl_zero);
    Int     i;

    for (i = 0; i < m.Rows(); i++)
        result += m[i] * v[i];

    return(result);
}


// --- Mat Special Functions --------------------------------------------------


Mat trans(const Mat &m)
{
    Int     i,j;
    Mat result(m.Cols(), m.Rows());

    for (i = 0; i < m.Rows(); i++)
        for (j = 0; j < m.Cols(); j++)
            result.Elt(j,i) = m.Elt(i,j);

    return(result);
}

Real trace(const Mat &m)
{
    Int     i;
    Real    result = vl_0;

    for (i = 0; i < m.Rows(); i++)
        result += m.Elt(i,i);

    return(result);
}

Mat &Mat::Clamp(Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    Int i;

    for (i = 0; i < Rows(); i++)
        SELF[i].Clamp(fuzz);

    return(SELF);
}

Mat &Mat::Clamp()
{
    return(Clamp(1e-7));
}

Mat clamped(const Mat &m, Real fuzz)
//  clamps all values of the matrix with a magnitude
//  smaller than fuzz to zero.
{
    Mat result(m);

    return(result.Clamp(fuzz));
}

Mat clamped(const Mat &m)
{
    return(clamped(m, 1e-7));
}

Mat oprod(const Vec &a, const Vec &b)
// returns outerproduct of a and b:  a * trans(b)
{
    Mat result;
    Int     i;

    result.SetSize(a.Elts(), b.Elts());
    for (i = 0; i < a.Elts(); i++)
        result[i] = a[i] * b;

    return(result);
}


// --- Mat Input & Output -----------------------------------------------------


ostream &operator << (ostream &s, const Mat &m)
{
    Int i, w = s.width();

    s << '[';
    for (i = 0; i < m.Rows() - 1; i++)
        s << setw(w) << m[i] << endl;
    s << setw(w) << m[i] << ']' << endl;
    return(s);
}

inline Void CopyPartialMat(const Mat &m, Mat &n, Int numRows)
{
    for (Int i = 0; i < numRows; i++)
        n[i] = m[i];
}

istream &operator >> (istream &s, Mat &m)
{
    Int     size = 1;
    Char    c;
    Mat     inMat;

    //  Expected format: [row0 row1 row2 ...]

    while (isspace(s.peek()))           //  chomp white space
        s.get(c);

    if (s.peek() == '[')
    {
        Vec     row;

        s.get(c);

        s >> row;
        inMat.SetSize(2 * row.Elts(), row.Elts());
        inMat[0] = row;

        while (isspace(s.peek()))       //  chomp white space
            s.get(c);

        while (s.peek() != ']')         //  resize if needed
        {
            if (size == inMat.Rows())
            {
                Mat     holdMat(inMat);

                inMat.SetSize(size * 2, inMat.Cols());
                CopyPartialMat(holdMat, inMat, size);
            }

            s >> row;           //  read a row
            inMat[size++] = row;

            if (!s)
            {
                _Warning("Couldn't read matrix row");
                return(s);
            }

            while (isspace(s.peek()))   //  chomp white space
                s.get(c);
        }
        s.get(c);
    }
    else
    {
        s.clear(ios::failbit);
        _Warning("Error: Expected '[' while reading matrix");
        return(s);
    }

    m.SetSize(size, inMat.Cols());
    CopyPartialMat(inMat, m, size);

    return(s);
}

// --- Matrix Inversion -------------------------------------------------------

#if !defined(CL_CHECKING) && !defined(VL_CHECKING)
// we #define away pAssertEps if it is not used, to avoid
// compiler warnings.
#define pAssertEps
#endif

Mat inv(const Mat &m, Real *determinant, Real pAssertEps)
// matrix inversion using Gaussian pivoting
{
    Assert(m.IsSquare(), "(inv) Matrix not square");

    Int             i, j, k;
    Int             n = m.Rows();
    Real            t, pivot, det;
    Real            max;
    Mat         A(m);
    Mat         B(n, n, vl_I);

    // ---------- Forward elimination ---------- ------------------------------

    det = vl_1;

    for (i = 0; i < n; i++)             // Eliminate in column i, below diag
    {
        max = -1.0;

        for (k = i; k < n; k++)         // Find a pivot for column i
            if (len(A[k][i]) > max)
            {
                max = len(A[k][i]);
                j = k;
            }

        Assert(max > pAssertEps, "(inv) Matrix not invertible");

        if (j != i)                     // Swap rows i and j
        {
            for (k = i; k < n; k++)
                Swap(A.Elt(i, k), A.Elt(j, k));
            for (k = 0; k < n; k++)
                Swap(B.Elt(i, k), B.Elt(j, k));

            det = -det;
        }

        pivot = A.Elt(i, i);
        Assert(abs(pivot) > pAssertEps, "(inv) Matrix not invertible");
        det *= pivot;

        for (k = i + 1; k < n; k++)     // Only do elements to the right of the pivot
            A.Elt(i, k) /= pivot;

        for (k = 0; k < n; k++)
            B.Elt(i, k) /= pivot;

        // We know that A(i, i) will be set to 1, so don't bother to do it

        for (j = i + 1; j < n; j++)
        {                               // Eliminate in rows below i
            t = A.Elt(j, i);            // We're gonna zero this guy
            for (k = i + 1; k < n; k++) // Subtract scaled row i from row j
                A.Elt(j, k) -= A.Elt(i, k) * t; // (Ignore k <= i, we know they're 0)
            for (k = 0; k < n; k++)
                B.Elt(j, k) -= B.Elt(i, k) * t;
        }
    }

    // ---------- Backward elimination ---------- -----------------------------

    for (i = n - 1; i > 0; i--)         // Eliminate in column i, above diag
    {
        for (j = 0; j < i; j++)         // Eliminate in rows above i
        {
            t = A.Elt(j, i);            // We're gonna zero this guy
            for (k = 0; k < n; k++)     // Subtract scaled row i from row j
                B.Elt(j, k) -= B.Elt(i, k) * t;
        }
    }
    if (determinant)
        *determinant = det;
    return(B);
}

