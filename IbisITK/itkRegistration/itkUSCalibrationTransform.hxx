/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

/* Nov 20, 2014, by Y. Xiao, changes were made to add z-scale for reducing shears */


#ifndef __itkUSCalibrationTransform_hxx
#define __itkUSCalibrationTransform_hxx

#include "itkUSCalibrationTransform.h"

namespace itk
{
// Constructor with default arguments
template <class TScalarType>
USCalibrationTransform<TScalarType>
::USCalibrationTransform() :
  Superclass(ParametersDimension)
{
  m_ComputeZYX = false;
  m_AngleX = m_AngleY = m_AngleZ = NumericTraits<ScalarType>::Zero;
  // m_Scale = 1.0;

 // Nov-20-2014, Y.Xiao added m_ScaleZ
  //m_ScaleX = m_ScaleY =1.0;
    m_ScaleX = m_ScaleY = m_ScaleZ =1.0;
}

// Constructor with default arguments
template <class TScalarType>
USCalibrationTransform<TScalarType>
::USCalibrationTransform(const MatrixType & matrix, const OffsetType & offset)
{
  m_ComputeZYX = false;
  this->SetMatrix(matrix);

  OffsetType off;
  off[0] = offset[0];
  off[1] = offset[1];
  off[2] = offset[2];
  this->SetOffset(off);
}

// Constructor with arguments
template <class TScalarType>
USCalibrationTransform<TScalarType>
::USCalibrationTransform(unsigned int parametersDimension) :
  Superclass(parametersDimension)
{
  m_ComputeZYX = false;
  m_AngleX = m_AngleY = m_AngleZ = NumericTraits<ScalarType>::Zero;
}

// Set Angles
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::SetVarRotation(ScalarType angleX, ScalarType angleY, ScalarType angleZ)
{
  this->m_AngleX = angleX;
  this->m_AngleY = angleY;
  this->m_AngleZ = angleZ;
}

// Set Parameters
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::SetParameters(const ParametersType & parameters)
{
  itkDebugMacro(<< "Setting parameters " << parameters);

  // Save parameters. Needed for proper operation of TransformUpdateParameters.
  if( &parameters != &(this->m_Parameters) )
    {
    this->m_Parameters = parameters;
    }

  // Set angles and scales with parameters
  m_AngleX = parameters[0];
  m_AngleY = parameters[1];
  m_AngleZ = parameters[2];

  m_ScaleX = parameters[6];
  m_ScaleY = parameters[7];
  
// Nov-20-2014, Y.Xiao added z-scale
  m_ScaleZ = parameters[6];

  // m_Scale = parameters[6];
  // m_ScaleX = m_Scale; m_ScaleY = m_Scale;
  this->ComputeMatrix();

  // Transfer the translation part
  OutputVectorType newTranslation;
  newTranslation[0] = parameters[3];
  newTranslation[1] = parameters[4];
  newTranslation[2] = parameters[5];
  this->SetVarTranslation(newTranslation);
  this->ComputeOffset();

  // Modified is always called since we just have a pointer to the
  // parameters and cannot know if the parameters have changed.
  this->Modified();

  itkDebugMacro(<< "After setting parameters ");
}

// Get Parameters
template <class TScalarType>
const typename USCalibrationTransform<TScalarType>::ParametersType
& USCalibrationTransform<TScalarType>
::GetParameters(void) const
  {
  this->m_Parameters[0] = m_AngleX;
  this->m_Parameters[1] = m_AngleY;
  this->m_Parameters[2] = m_AngleZ;
  this->m_Parameters[3] = this->GetTranslation()[0];
  this->m_Parameters[4] = this->GetTranslation()[1];
  this->m_Parameters[5] = this->GetTranslation()[2];
  // this->m_Parameters[6] = m_Scale;
  this->m_Parameters[6] = m_ScaleX;
  this->m_Parameters[7] = m_ScaleY;
  

  return this->m_Parameters;
  }

// Set Rotational Part
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::SetRotation(ScalarType angleX, ScalarType angleY, ScalarType angleZ)
{
  m_AngleX = angleX;
  m_AngleY = angleY;
  m_AngleZ = angleZ;
  this->ComputeMatrix();
  this->ComputeOffset();
}

// Set Scaling
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
// Nov-20-2014,Y.Xiao added z-scale
::SetScales(ScalarType scaleX, ScalarType scaleY)
{
  m_ScaleX = scaleX;
  m_ScaleY = scaleY;
  //m_ScaleZ = scaleZ;
  m_ScaleZ = scaleX; 

  this->ComputeMatrix();
  this->ComputeOffset();
}


// Compose
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::SetIdentity(void)
{
  Superclass::SetIdentity();
  m_AngleX = 0;
  m_AngleY = 0;
  m_AngleZ = 0;

  // m_Scale = 1.0;
  m_ScaleX = 1.0;
  m_ScaleY = 1.0;
// Nov-20-2014,Y.Xiao added z-scale
  m_ScaleZ = 1.0;

}

// Compute angles from the rotation matrix
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::ComputeMatrixParameters(void)
{


  m_ScaleX = sqrt( pow(this->GetMatrix()[0][0], 2.0) +
                   pow(this->GetMatrix()[1][0], 2.0) +
                   pow(this->GetMatrix()[2][0], 2.0) );

  m_ScaleY = sqrt( pow(this->GetMatrix()[0][1], 2.0) +
                   pow(this->GetMatrix()[1][1], 2.0) +
                   pow(this->GetMatrix()[2][1], 2.0) ); 

 // Nov-20-2014,Y.Xiao added z-scale
  m_ScaleZ = sqrt( pow(this->GetMatrix()[0][2], 2.0) +
                   pow(this->GetMatrix()[1][2], 2.0) +
                   pow(this->GetMatrix()[2][2], 2.0) ); 

  const ScalarType one = NumericTraits<ScalarType>::One;
  const ScalarType zero = NumericTraits<ScalarType>::Zero;

  Matrix<TScalarType, 3, 3> Scaling;
  Scaling[0][0] = m_ScaleX;   Scaling[0][1] = zero;       Scaling[0][2] = zero;
  Scaling[1][0] = zero;       Scaling[1][1] = m_ScaleY;   Scaling[1][2] = zero;
  Scaling[2][0] = zero;       Scaling[2][1] = zero;       Scaling[2][2] = m_ScaleZ;//Scaling[2][2] = one;
  

  Matrix<TScalarType, 3, 3> Rotation = this->GetMatrix() * Scaling.GetInverse();
  
 // Nov-20-2014, Y. Xiao, assign the scale of z back to the same as x
  m_ScaleZ = m_ScaleX;

  // std::cerr << "Scaling:" << std::endl;
  // std::cerr << Scaling << std::endl;

  // std::cerr << "Rotation:" << std::endl;
  // std::cerr << Rotation << std::endl;

  if( m_ComputeZYX )
    {
    m_AngleY = -vcl_asin(Rotation[2][0]);
    double C = vcl_cos(m_AngleY);
    if( vcl_fabs(C) > 0.00001 ) //used to be 0.00005
      {
      double x = Rotation[2][2]; // / C;
      double y = Rotation[2][1]; // / C;
      m_AngleX = vcl_atan2(y, x);
      x = Rotation[0][0]; // / C;
      y = Rotation[1][0]; // / C;
      m_AngleZ = vcl_atan2(y, x);
      }
    else
      {
      m_AngleX = NumericTraits<ScalarType>::Zero;
      double x = Rotation[1][1];
      double y = -Rotation[0][1];
      m_AngleZ = vcl_atan2(y, x);
      }
    }
  else
    {
    m_AngleX = vcl_asin(Rotation[2][1]);
    double A = vcl_cos(m_AngleX);
    //if( vcl_fabs(A) > 0.00005 )
     if( vcl_fabs(A) > 0.00001 ) 
     {
      double x = Rotation[2][2];// / A;
      double y = -Rotation[2][0];// / A;
      m_AngleY = vcl_atan2(y, x);

      x = Rotation[1][1];// / A;
      y = -Rotation[0][1];// / A;
      m_AngleZ = vcl_atan2(y, x);
      }
    else
      {
      m_AngleZ = NumericTraits<ScalarType>::Zero;
      double x = Rotation[0][0];
      double y = Rotation[1][0];
      m_AngleY = vcl_atan2(y, x);
      }
    }
  this->ComputeMatrix();

  // std::cerr << "Recomputed Matrix:" << std::endl;
  // std::cerr << this->GetMatrix() << std::endl;
}

// Compute the matrix
template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::ComputeMatrix(void)
{
  // need to check if angles are in the right order
  const ScalarType cx = vcl_cos(m_AngleX);
  const ScalarType sx = vcl_sin(m_AngleX);
  const ScalarType cy = vcl_cos(m_AngleY);
  const ScalarType sy = vcl_sin(m_AngleY);
  const ScalarType cz = vcl_cos(m_AngleZ);
  const ScalarType sz = vcl_sin(m_AngleZ);
  const ScalarType one = NumericTraits<ScalarType>::One;
  const ScalarType zero = NumericTraits<ScalarType>::Zero;

  Matrix<TScalarType, 3, 3> RotationX;
  RotationX[0][0] = one;  RotationX[0][1] = zero; RotationX[0][2] = zero;
  RotationX[1][0] = zero; RotationX[1][1] = cx;   RotationX[1][2] = -sx;
  RotationX[2][0] = zero; RotationX[2][1] = sx;   RotationX[2][2] = cx;

  Matrix<TScalarType, 3, 3> RotationY;
  RotationY[0][0] = cy;   RotationY[0][1] = zero; RotationY[0][2] = sy;
  RotationY[1][0] = zero; RotationY[1][1] = one;  RotationY[1][2] = zero;
  RotationY[2][0] = -sy;  RotationY[2][1] = zero; RotationY[2][2] = cy;

  Matrix<TScalarType, 3, 3> RotationZ;
  RotationZ[0][0] = cz;   RotationZ[0][1] = -sz;  RotationZ[0][2] = zero;
  RotationZ[1][0] = sz;   RotationZ[1][1] = cz;   RotationZ[1][2] = zero;
  RotationZ[2][0] = zero; RotationZ[2][1] = zero; RotationZ[2][2] = one;

  Matrix<TScalarType, 3, 3> Scaling;
  Scaling[0][0] = m_ScaleX;   Scaling[0][1] = zero;       Scaling[0][2] = zero;
  Scaling[1][0] = zero;       Scaling[1][1] = m_ScaleY;   Scaling[1][2] = zero;
  Scaling[2][0] = zero;       Scaling[2][1] = zero;       Scaling[2][2] = m_ScaleX;//Scaling[2][2] = one;




  /** Apply the rotation first around Y then X then Z */
  if( m_ComputeZYX )
    {
    this->SetVarMatrix(RotationZ * RotationY * RotationX * Scaling);

    // std::cerr << "Scaling:" << std::endl;
    // std::cerr << Scaling << std::endl;

    // std::cerr << "Rotation:" << std::endl;
    // std::cerr << RotationZ * RotationY * RotationX << std::endl;    
    }
  else
    {
    // Like VTK transformation order
    this->SetVarMatrix(RotationZ * RotationX * RotationY * Scaling);
    // std::cerr << "Scaling:" << std::endl;
    // std::cerr << Scaling << std::endl;

    // std::cerr << "Rotation:" << std::endl;
    // std::cerr << RotationZ * RotationY * RotationX << std::endl;      
    }
}

template <class TScalarType>
void
USCalibrationTransform<TScalarType>
::ComputeJacobianWithRespectToParameters(const InputPointType & p, JacobianType & jacobian) const
{

  itkExceptionMacro(<<"ComputeJacobianWithRespectToParameters not yet implemented for this class.");
}

// Print self
template <class TScalarType>
void
USCalibrationTransform<TScalarType>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Euler's angles: AngleX=" << m_AngleX
     << " AngleY=" << m_AngleY
     << " AngleZ=" << m_AngleZ
     << std::endl;
  os << indent << "m_ComputeZYX = " << m_ComputeZYX << std::endl;

  os << indent << "Scaling Parameters: ScaleX=" << m_ScaleX
     << " ScaleY=" << m_ScaleY
// Nov-20-2014, Y.Xiao added z-scale
     << " ScaleZ=" << m_ScaleZ
     << std::endl;  
  
}

} // namespace

#endif
