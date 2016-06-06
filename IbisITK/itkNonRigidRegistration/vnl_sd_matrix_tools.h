#ifndef __vnl_sd_matrix_tools_h
#define __vnl_sd_matrix_tools_h

#include <vnl/vnl_matrix.h>
#include <vector>

/**
 * A set of non-obvious matrix functions. Includes the equivalent of
 * Matlab's logm and expm.
 *
 * \author Vincent Arsigny, INRIA, Olivier Commowick, INRIA, and Tom Vercauteren, MKT
 */

namespace sdtools
{

/**
 * Computation of the square root of a reasonnable matrix.
 * Essential part in the general computation of the matrix logarithm.
 * Based on a variant (product form) of the classical Denman-Beavers (DB) iteration.
 **/
template <class T>
vnl_matrix<T> GetSquareRoot(const vnl_matrix<T> & m, const T precision, vnl_matrix<T> & resultM);

/**
 * Final part of the computation of the log. Estimates the log
 * with a Pade approximation for a matrix m such that \|m-Id\| <= 0.5.
 **/
template <class T>
vnl_matrix<T> GetPadeLogarithm(const vnl_matrix<T> & m, const int numApprox);

/**
 * Computation of the matrix logarithm. Algo: inverse scaling
 * and squaring, variant proposed by Cheng et al., SIAM Matrix Anal., 2001.
 **/
template <class T>
vnl_matrix<T> GetLogarithm(const vnl_matrix<T> & m, const T square_root_precision = 1e-11, const int numApprox = 1);

/**
 * Computation of the matrix exponential. Algo: classical scaling
 * and squaring, as in Matlab. See Higham, SIAM Matr. Anal., 2004.
 */
template <class T>
vnl_matrix<T> GetExponential(const vnl_matrix<T> & m, const int numApprox = 3);

/**
 * Computation of the Log-Euclidean barycenter of matrices, ie the
 * exponential of the arithmetic mean of their logarithms.
 **/
template <class T>
vnl_matrix<T> GetLogEuclideanBarycenter(const std::vector<vnl_matrix<T> > & matrices, const std::vector<T> & weights);

/**
 * Computation of the group barycenter of matrices, ie the left-,
 * right- and inverse-invariant barycenter. It is similar to the Log-Euclidean
 * barycenter. Its computation is iterative, whereas there is a closed
 * form for the LE barycenter.
 **/
template <class T>
vnl_matrix<T> GetGroupBarycenter(const std::vector<vnl_matrix<T> > & matrices, const std::vector<T> & weights,
                                 const T precision = 0.000001);

/**
 * Computation of the Arithmetic barycenter of matrices, ie the
 * arithmetic mean of the matrices!
 **/
template <class T>
vnl_matrix<T> GetArithmeticBarycenter(const std::vector<vnl_matrix<T> > & matrices, const std::vector<T> & weights);

} // end namespace

#include "vnl_sd_matrix_tools.hxx"

#endif
