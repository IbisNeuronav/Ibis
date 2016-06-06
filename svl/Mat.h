/*
    File:           Mat.h

    Function:       Defines a generic resizeable matrix.

    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2001, Andrew Willmott
 */

#ifndef __Mat__
#define __Mat__

#include "Vec.h"

class Mat2;
class Mat3;
class Mat4;


// --- Mat Class --------------------------------------------------------------

class Mat
{
public:

    // Constructors

                Mat();                          // Null matrix
                Mat(Int rows, Int cols);        // Uninitialised matrix
                Mat(Int rows, Int cols, Double elt0 ...);   // Mat(2, 2, 1.0, 2.0, 3.0, 4.0)
                Mat(Int nrows, Int ncols, Real *ndata);     // Create reference matrix
                Mat(const Mat &m);              // Copy constructor
                Mat(const Mat2 &m);             // reference to a Mat2
                Mat(const Mat3 &m);             // reference to a Mat3
                Mat(const Mat4 &m);             // reference to a Mat4
                Mat(Int rows, Int cols, ZeroOrOne k); // I * k
                Mat(Int rows, Int cols, Block k); // block matrix (m[i][j] = k)

                ~Mat();

    // Accessor methods

    Int         Rows() const { return(rows & VL_REF_MASK); };
    Int         Cols() const { return(cols); };

    Vec         operator [] (Int i);            // Indexing by row
    Vec         operator [] (Int i) const;      // Indexing by row

    Real        &Elt(Int i, Int j);             // Indexing by elt
    Real        Elt(Int i, Int j) const;

    Void        SetSize(Int nrows, Int ncols);
    Void        SetSize(const Mat &m);
    Bool        IsSquare() const
                { return((rows & VL_REF_MASK) == cols); };

    Real        *Ref() const;                   // Return pointer to data

    // Assignment operators

    Mat         &operator =  (const Mat &m);    // Assignment of a matrix
    Mat         &operator =  (ZeroOrOne k);     // Set to k * I...
    Mat         &operator =  (Block k);         // Set to a block matrix...
    Mat         &operator =  (const Mat2 &m);
    Mat         &operator =  (const Mat3 &m);
    Mat         &operator =  (const Mat4 &m);

    // In-Place Operators

    Mat         &operator += (const Mat &m);
    Mat         &operator -= (const Mat &m);
    Mat         &operator *= (const Mat &m);
    Mat         &operator *= (Real s);
    Mat         &operator /= (Real s);

    //  Matrix initialisers

    Void        MakeZero();
    Void        MakeDiag(Real k);
    Void        MakeDiag();
    Void        MakeBlock(Real k);
    Void        MakeBlock();

    Mat         &Clamp(Real fuzz);
    Mat         &Clamp();

    // Private...

protected:

    Real        *data;
    UInt        rows;
    UInt        cols;

    Bool        IsRef() { return((rows & VL_REF_FLAG) != 0); };
};


// --- Mat Comparison Operators -----------------------------------------------

Bool            operator == (const Mat &m, const Mat &n);
Bool            operator != (const Mat &m, const Mat &n);


// --- Mat Arithmetic Operators -----------------------------------------------

Mat             operator + (const Mat &m, const Mat &n);
Mat             operator - (const Mat &m, const Mat &n);
Mat             operator - (const Mat &m);
Mat             operator * (const Mat &m, const Mat &n);
Mat             operator * (const Mat &m, Real s);
inline Mat      operator * (Real s, const Mat &m);
Mat             operator / (const Mat &m, Real s);

Vec             operator * (const Mat &m, const Vec &v);
Vec             operator * (const Vec &v, const Mat &m);

Mat             trans(const Mat &m);                // Transpose
Real            trace(const Mat &m);                // Trace
Mat             inv(const Mat &m, Real *determinant = 0, Real pEps = 1e-20);
                                                    // Inverse
Mat             oprod(const Vec &a, const Vec &b);  // Outer product

Mat             clamped(const Mat &m, Real fuzz);
Mat             clamped(const Mat &m);


// --- Mat Input & Output -----------------------------------------------------

std::ostream         &operator << (std::ostream &s, const Mat &m);
std::istream         &operator >> (std::istream &s, Mat &m);


// --- Mat Inlines ------------------------------------------------------------

inline Mat::Mat() : data(0), rows(0), cols(0)
{
}

inline Mat::Mat(Int rows, Int cols) : rows(rows), cols(cols)
{
    Assert(rows > 0 && cols > 0, "(Mat) illegal matrix size");

    data = new Real[rows * cols];
}

inline Mat::Mat(Int nrows, Int ncols, Real *ndata) :
    data(ndata), rows(nrows | VL_REF_FLAG), cols(ncols)
{
}

inline Vec Mat::operator [] (Int i)
{
    CheckRange(i, 0, Rows(), "(Mat::[i]) i index out of range");

    return(Vec(cols, data + i * cols));
}

inline Vec Mat::operator [] (Int i) const
{
    CheckRange(i, 0, Rows(), "(Mat::[i]) i index out of range");

    return(Vec(cols, data + i * cols));
}

inline Real &Mat::Elt(Int i, Int j)
{
    CheckRange(i, 0, Rows(), "(Mat::e(i,j)) i index out of range");
    CheckRange(j, 0, Cols(), "(Mat::e(i,j)) j index out of range");

    return(data[i * cols + j]);
}

inline Real Mat::Elt(Int i, Int j) const
{
    CheckRange(i, 0, Rows(), "(Mat::e(i,j)) i index out of range");
    CheckRange(j, 0, Cols(), "(Mat::e(i,j)) j index out of range");

    return(data[i * cols + j]);
}

inline Real *Mat::Ref() const
{
    return(data);
}

inline Mat operator * (Real s, const Mat &m)
{
    return(m * s);
}

inline Mat &Mat::operator = (ZeroOrOne k)
{
    MakeDiag(k);

    return(SELF);
}

inline Mat &Mat::operator = (Block k)
{
    MakeBlock((ZeroOrOne) k);

    return(SELF);
}

inline Mat::~Mat()
{
    if (!IsRef())
        delete[] data;
}

#endif
