#ifndef __vnl_sd_matrix_tools_txx
#define __vnl_sd_matrix_tools_txx

#include "vnl_sd_matrix_tools.h"

#include <exception>
#include <vnl/vnl_math.h>
#include <vnl/algo/vnl_determinant.h>
#include <vnl/algo/vnl_matrix_inverse.h>

namespace sdtools
{

template <class T>
vnl_matrix<T>
GetInverse(const vnl_matrix<T> & m)
{
  // Compute the matrix inverse. For dimensions less than 4,
  // this is basically a copy of the code in vn_inverse (explicit inversion) but avoids
  // some matrix copies. For arbitrary dimensions, we call vnl_matrix_inverse
  // which in turn use svd.
  const unsigned int n = m.rows();

  switch( n )
    {
    case 1:
      {
      return vnl_matrix<T>(n, n, static_cast<T>(1.0) / m(0, 0) );
      }
    case 2:
      {
      const T       invdet = static_cast<T>(1.0) / vnl_determinant(m[0], m[1]);
      vnl_matrix<T> invmat(n, n);
      invmat(0, 0) =  m(1, 1) * invdet;
      invmat(0, 1) = -m(0, 1) * invdet;
      invmat(1, 0) = -m(1, 0) * invdet;
      invmat(1, 1) =  m(0, 0) * invdet;
      return invmat;
      }
    case 3:
      {
      const T       invdet = static_cast<T>(1.0) / vnl_determinant(m[0], m[1], m[2]);
      vnl_matrix<T> invmat(n, n);
      invmat(0, 0) = (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1) ) * invdet;
      invmat(0, 1) = (m(2, 1) * m(0, 2) - m(2, 2) * m(0, 1) ) * invdet;
      invmat(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1) ) * invdet;
      invmat(1, 0) = (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2) ) * invdet;
      invmat(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0) ) * invdet;
      invmat(1, 2) = (m(1, 0) * m(0, 2) - m(1, 2) * m(0, 0) ) * invdet;
      invmat(2, 0) = (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0) ) * invdet;
      invmat(2, 1) = (m(0, 1) * m(2, 0) - m(0, 0) * m(2, 1) ) * invdet;
      invmat(2, 2) = (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0) ) * invdet;
      return invmat;
      }
    case 4:
      {
      const T       invdet = static_cast<T>(1.0) / vnl_determinant(m[0], m[1], m[2], m[3]);
      vnl_matrix<T> invmat(n, n);
      invmat(0, 0) = ( m(1, 1) * m(2, 2) * m(3, 3) - m(1, 1) * m(2, 3) * m(3, 2) - m(2, 1) * m(1, 2) * m(3, 3)
                       + m(2,
                           1)
                       * m(1, 3) * m(3, 2) + m(3, 1) * m(1, 2) * m(2, 3) - m(3, 1) * m(1, 3) * m(2, 2) ) * invdet;
      invmat(0, 1) = (-m(0, 1) * m(2, 2) * m(3, 3) + m(0, 1) * m(2, 3) * m(3, 2) + m(2, 1) * m(0, 2) * m(3, 3)
                      - m(2,
                          1) * m(0, 3) * m(3, 2) - m(3, 1) * m(0, 2) * m(2, 3) + m(3, 1) * m(0, 3) * m(2, 2) ) * invdet;
      invmat(0, 2) = ( m(0, 1) * m(1, 2) * m(3, 3) - m(0, 1) * m(1, 3) * m(3, 2) - m(1, 1) * m(0, 2) * m(3, 3)
                       + m(1,
                           1)
                       * m(0, 3) * m(3, 2) + m(3, 1) * m(0, 2) * m(1, 3) - m(3, 1) * m(0, 3) * m(1, 2) ) * invdet;
      invmat(0, 3) = (-m(0, 1) * m(1, 2) * m(2, 3) + m(0, 1) * m(1, 3) * m(2, 2) + m(1, 1) * m(0, 2) * m(2, 3)
                      - m(1,
                          1) * m(0, 3) * m(2, 2) - m(2, 1) * m(0, 2) * m(1, 3) + m(2, 1) * m(0, 3) * m(1, 2) ) * invdet;
      invmat(1, 0) = (-m(1, 0) * m(2, 2) * m(3, 3) + m(1, 0) * m(2, 3) * m(3, 2) + m(2, 0) * m(1, 2) * m(3, 3)
                      - m(2,
                          0) * m(1, 3) * m(3, 2) - m(3, 0) * m(1, 2) * m(2, 3) + m(3, 0) * m(1, 3) * m(2, 2) ) * invdet;
      invmat(1, 1) = ( m(0, 0) * m(2, 2) * m(3, 3) - m(0, 0) * m(2, 3) * m(3, 2) - m(2, 0) * m(0, 2) * m(3, 3)
                       + m(2,
                           0)
                       * m(0, 3) * m(3, 2) + m(3, 0) * m(0, 2) * m(2, 3) - m(3, 0) * m(0, 3) * m(2, 2) ) * invdet;
      invmat(1, 2) = (-m(0, 0) * m(1, 2) * m(3, 3) + m(0, 0) * m(1, 3) * m(3, 2) + m(1, 0) * m(0, 2) * m(3, 3)
                      - m(1,
                          0) * m(0, 3) * m(3, 2) - m(3, 0) * m(0, 2) * m(1, 3) + m(3, 0) * m(0, 3) * m(1, 2) ) * invdet;
      invmat(1, 3) = ( m(0, 0) * m(1, 2) * m(2, 3) - m(0, 0) * m(1, 3) * m(2, 2) - m(1, 0) * m(0, 2) * m(2, 3)
                       + m(1,
                           0)
                       * m(0, 3) * m(2, 2) + m(2, 0) * m(0, 2) * m(1, 3) - m(2, 0) * m(0, 3) * m(1, 2) ) * invdet;
      invmat(2, 0) = ( m(1, 0) * m(2, 1) * m(3, 3) - m(1, 0) * m(2, 3) * m(3, 1) - m(2, 0) * m(1, 1) * m(3, 3)
                       + m(2,
                           0)
                       * m(1, 3) * m(3, 1) + m(3, 0) * m(1, 1) * m(2, 3) - m(3, 0) * m(1, 3) * m(2, 1) ) * invdet;
      invmat(2, 1) = (-m(0, 0) * m(2, 1) * m(3, 3) + m(0, 0) * m(2, 3) * m(3, 1) + m(2, 0) * m(0, 1) * m(3, 3)
                      - m(2,
                          0) * m(0, 3) * m(3, 1) - m(3, 0) * m(0, 1) * m(2, 3) + m(3, 0) * m(0, 3) * m(2, 1) ) * invdet;
      invmat(2, 2) = ( m(0, 0) * m(1, 1) * m(3, 3) - m(0, 0) * m(1, 3) * m(3, 1) - m(1, 0) * m(0, 1) * m(3, 3)
                       + m(1,
                           0)
                       * m(0, 3) * m(3, 1) + m(3, 0) * m(0, 1) * m(1, 3) - m(3, 0) * m(0, 3) * m(1, 1) ) * invdet;
      invmat(2, 3) = (-m(0, 0) * m(1, 1) * m(2, 3) + m(0, 0) * m(1, 3) * m(2, 1) + m(1, 0) * m(0, 1) * m(2, 3)
                      - m(1,
                          0) * m(0, 3) * m(2, 1) - m(2, 0) * m(0, 1) * m(1, 3) + m(2, 0) * m(0, 3) * m(1, 1) ) * invdet;
      invmat(3, 0) = (-m(1, 0) * m(2, 1) * m(3, 2) + m(1, 0) * m(2, 2) * m(3, 1) + m(2, 0) * m(1, 1) * m(3, 2)
                      - m(2,
                          0) * m(1, 2) * m(3, 1) - m(3, 0) * m(1, 1) * m(2, 2) + m(3, 0) * m(1, 2) * m(2, 1) ) * invdet;
      invmat(3, 1) = ( m(0, 0) * m(2, 1) * m(3, 2) - m(0, 0) * m(2, 2) * m(3, 1) - m(2, 0) * m(0, 1) * m(3, 2)
                       + m(2,
                           0)
                       * m(0, 2) * m(3, 1) + m(3, 0) * m(0, 1) * m(2, 2) - m(3, 0) * m(0, 2) * m(2, 1) ) * invdet;
      invmat(3, 2) = (-m(0, 0) * m(1, 1) * m(3, 2) + m(0, 0) * m(1, 2) * m(3, 1) + m(1, 0) * m(0, 1) * m(3, 2)
                      - m(1,
                          0) * m(0, 2) * m(3, 1) - m(3, 0) * m(0, 1) * m(1, 2) + m(3, 0) * m(0, 2) * m(1, 1) ) * invdet;
      invmat(3, 3) = ( m(0, 0) * m(1, 1) * m(2, 2) - m(0, 0) * m(1, 2) * m(2, 1) - m(1, 0) * m(0, 1) * m(2, 2)
                       + m(1,
                           0)
                       * m(0, 2) * m(2, 1) + m(2, 0) * m(0, 1) * m(1, 2) - m(2, 0) * m(0, 2) * m(1, 1) ) * invdet;
      return invmat;
      }
    default:
      {
      // Fall-back to SVD-based inversion
      return vnl_matrix_inverse<T>(m);
      }
    }
}

template <class T>
vnl_matrix<T>
GetSquareRoot(const vnl_matrix<T> & m,
              const T precision,
              vnl_matrix<T> & resultM)
{
  // Declarations
  vnl_matrix<T>      Mk1, Yk1, invMk;
  unsigned int       niter = 1;
  const unsigned int niterMax = 100;

  // Initializations
  vnl_matrix<T> Mk( m );
  vnl_matrix<T> Yk( m );

  const vnl_matrix<T> Id(m.rows(), m.columns(), vnl_matrix_identity);

  T energy = (Yk * Yk - m).frobenius_norm();

  const double n = m.rows();

  // loop
  while( (niter <= niterMax) && (energy > precision) )
    {
    // std::cout << "niter=" << niter << ", energy=" << energy << "." << std::endl;

    const T gamma = vcl_pow(vcl_abs(vnl_determinant(Mk) ), -1.0 / (2.0 * n) );
    const T gamma2 = gamma * gamma;
    invMk = GetInverse(Mk);

    Mk1 = ( Id + (Mk * gamma2 + invMk / gamma2) * 0.5 ) * 0.5;
    Yk1 = Yk * (Id + invMk / gamma2) * (0.5 * gamma);

    Yk = Yk1;
    Mk = Mk1;

    energy = (Yk * Yk - m).frobenius_norm();

    ++niter;
    }

  if( niter > niterMax )
    {
    std::cout << std::endl
              << "Warning, max number of iteration reached in sqrt computation. Final energy is: "
              << energy << std::endl;
    }

  // std::cout << "niter=" << niter << std::endl;

  resultM = Mk;
  return Yk;
}

template <class T>
vnl_matrix<T>
GetPadeLogarithm(const vnl_matrix<T> & m,
                 const int numApprox)
{
  const vnl_matrix<T> Id(m.rows(), m.columns(), vnl_matrix_identity);
  vnl_matrix<T>       interm2, interm3;

  const vnl_matrix<T> diff = Id - m;
  const T             energy = diff.frobenius_norm();

  if( energy > 0.5 )
    {
    std::cout << "Warning, matrix is not close enough to Id to call Pade approximation. Frobenius Distance = "
              << energy << ". Returning original matrix." << std::endl;
    return m;
    }

  switch( numApprox )
    {
    case 1:
      {
      interm2 = -diff;
      interm3 = Id - diff * 0.5;
      break;
      }
    case 2:
      {
      const vnl_matrix<T> sqr = diff * diff;

      interm2 = sqr * 0.5 - diff;
      interm3 = Id - diff + sqr;
      break;
      }
    case 3:
      {
      const vnl_matrix<T> sqr  = diff * diff;
      const vnl_matrix<T> cube = sqr * diff;

      const double tmpcst = 11.0 / 60.0;

      interm2 = sqr + cube * tmpcst - diff;
      interm3 = Id - diff * 1.5 + sqr * 0.6 - cube * 0.05;
      break;
      }
    default:
      {
      std::cerr << "Unsupported numApprox" << std::endl;
      throw 0;
      }
    }

  return interm2 * GetInverse(interm3);
}

template <class T>
vnl_matrix<T>
GetLogarithm(const vnl_matrix<T> & m,
             const T square_root_precision,
             const int numApprox)
{
  // /\todo Use Schur factorization prior to scaling and squaring
  // Note that this would imply using complex matrices

  T factor = 1.0;

  const vnl_matrix<T> Id(m.rows(), m.columns(), vnl_matrix_identity);
  vnl_matrix<T>       resultM;

  const unsigned int niterMax = 100;
  unsigned int       niter = 1;

  vnl_matrix<T> Yi( m );
  T             energy = (Yi - Id).frobenius_norm();
  vnl_matrix<T> matrix_sum(m.rows(), m.columns(), 0.0);

  // /\todo initial version used 0.5 as threshold on the energy
  // / 0.005 seems better from unit tests -> check theory
  while( (energy > 0.005) && (niter <= niterMax) )
    {
    // std::cout << "niter=" << niter << ", energy=" << energy << "." << std::endl;

    Yi = GetSquareRoot(Yi, square_root_precision, resultM);

    matrix_sum += (Id - resultM) * factor;

    energy = (Yi - Id).frobenius_norm();

    factor *= 2.0;
    ++niter;
    }

  if( niter > niterMax )
    {
    std::cout << std::endl
              << "Warning, max number of iteration reached in logarithm computation. Final energy is: "
              << energy << std::endl;
    }

  return GetPadeLogarithm(Yi, numApprox) * factor + matrix_sum;
}

template <class T>
vnl_matrix<T>
GetExponential(const vnl_matrix<T> & m,
               const int numApprox)
{
  const vnl_matrix<T> Id(m.rows(), m.columns(), vnl_matrix_identity);
  vnl_matrix<T>       interm2, interm3;

  const T norm = m.frobenius_norm();
  int     k;

  if( norm > 1 )
    {
    k = 1 + static_cast<int>( vcl_ceil( vcl_log(norm) / vnl_math::ln2 ) );
    }
  else if( norm > 0.5 )
    {
    k = 1;
    }
  else
    {
    k = 0;
    }

  // std::cout << "The famous k=" << k << ". Norm=" << norm << std::endl;

  // Set factor to 2^k
  const T       factor(1 << k);
  vnl_matrix<T> interm = m / factor;

  switch( numApprox )
    {
    case 1:
      {
      interm2 = Id + interm * 0.5;
      interm3 = Id - interm * 0.5;
      break;
      }
    case 2:
      {
      const vnl_matrix<T> sqr = interm * interm;

      const double tmpcst = 1.0 / 12.0;

      interm2 = Id + interm * 0.5 + sqr * tmpcst;
      interm3 = Id - interm * 0.5 + sqr * tmpcst;
      break;
      }
    case 3:
      {
      const vnl_matrix<T> sqr  = interm * interm;
      const vnl_matrix<T> cube = sqr * interm;

      const double tmpcst = 1.0 / 120.0;

      interm2 = Id + interm * 0.5 + sqr * 0.1 + cube * tmpcst;
      interm3 = Id - interm * 0.5 + sqr * 0.1 - cube * tmpcst;
      break;
      }
    default:
      {
      std::cerr << "Unsupported numApprox" << std::endl;
      throw 0;
      }
    }

  interm = interm2 * GetInverse(interm3);
  for( int i = 1; i <= k; ++i )
    {
    interm *= interm;
    }

  return interm;
}

template <class T>
vnl_matrix<T>
GetLogEuclideanBarycenter(const std::vector<vnl_matrix<T> > & matrices,
                          const std::vector<T> & weights)
{
  if( matrices.empty() || (matrices.size() != weights.size() ) )
    {
    std::cerr << std::endl << "Error: number of transfos =" << matrices.size()
              << " and is different from the number of weights ="
              <<  weights.size() << "." << std::endl;

    throw 0;
    }

  T sum = weights[0];
  for( int i = 1; i < matrices.size(); ++i )
    {
    sum += weights[i];
    }

  if( vcl_abs(sum - 1.0) > 0.000000000001 )
    {
    std::cerr << std::endl << "Error: sum of weights is not equal to 1 but to "
              << sum << std::endl;

    throw 0;
    }

  vnl_matrix<T> bar = GetLogarithm(matrices[0]) * weights[0];
  for( int i = 1; i < matrices.size(); ++i )
    {
    bar += GetLogarithm(matrices[i]) * weights[i];
    }

  bar = GetExponential(bar);

  return bar;
}

template <class T>
vnl_matrix<T>
GetGroupBarycenter(const std::vector<vnl_matrix<T> > & matrices,
                   const std::vector<T> & weights,
                   const T precision)
{
  const unsigned int niterMax = 60;

  vnl_matrix<T> interm, invBar;

  // Initialization
  vnl_matrix<T> bar = GetLogEuclideanBarycenter(matrices, weights);

  // Iterative procedure
  T            energy = 1000000000; // anything higher than precision will do
  unsigned int niter = 1;

  while( (energy > precision) && (niter <= niterMax) )
    {
    invBar = GetInverse(bar);

    interm = GetLogarithm(invBar * matrices[0]) * weights[0];
    for( int i = 1; i < matrices.size(); ++i )
      {
      interm += GetLogarithm(invBar * matrices[i]) * weights[i];
      }

    energy = interm.frobenius_norm();
    bar *= GetExponential(interm);

    ++niter;
    }

  if( niter > niterMax )
    {
    std::cout << std::endl
              << "Warning, max number of iteration reached in group baryscenter computation. Final energy is: "
              << energy << std::endl;
    }

  return bar;
}

template <class T>
vnl_matrix<T>
GetArithmeticBarycenter(const std::vector<vnl_matrix<T> > & matrices,
                        const std::vector<T> & weights)
{
  if( matrices.empty() || (matrices.size() != weights.size() ) )
    {
    std::cerr << std::endl << "Error: number of transfos = " << matrices.size()
              << " and is different from the number of weights = "
              <<  weights.size() << "." << std::endl;

    throw 0;
    }

  T sum = weights[0];
  for( int i = 1; i < matrices.size(); ++i )
    {
    sum += weights[i];
    }

  if( vcl_abs(sum - 1.0) > 0.000000000001 )
    {
    std::cerr << std::endl << "Error: sum of weights is not equal to 1 but to "
              << sum << std::endl;

    throw 0;
    }

  vnl_matrix<T> bar = matrices[0] * weights[0];
  for( int i = 1; i < matrices.size(); ++i )
    {
    bar += matrices[i] * weights[i];
    }

  return bar;
}

} // end namespace

#endif
