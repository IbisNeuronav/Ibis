/*=========================================================================


=========================================================================*/
#ifndef __itkOrientationMatchingBetaImageFunction_txx
#define __itkOrientationMatchingBetaImageFunction_txx

// First, make sure that we include the configuration file.
// This line may be removed once the ThreadSafeTransform gets
// integrated into ITK.
#include "itkConfigure.h"

#include "itkOrientationMatchingBetaImageFunction.h"

#include "vnl/vnl_math.h"

namespace itk
{

/**
 * Define the number of neighbors
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
const unsigned long
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::m_Neighbors = 1 << TInputImage::ImageDimension;


/**
 * Constructor
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::OrientationMatchingBetaImageFunction()
{
  m_LocalSimilarityMeasureParameter = 2;  
}


/**
 * Initialize
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::Initialize(void) throw ( ExceptionObject )
{
  //BSplineTransformPointer localBSplineTransform = 0;
  //bool transformIsBSpline = this->CheckForBSplineTransform( localBSplineTransform );
  //if ( transformIsBSpline ) this->SetBSplineTransform( localBSplineTransform );
}

/**
 * PrintSelf
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::PrintSelf(std::ostream& os, Indent indent) const
{
  this->Superclass::PrintSelf(os,indent);

  std::cout<< "\tInput Image:\t" << this->GetInputImage() << std::endl;
  std::cout<< "\tTransform:\t" << this->m_Transform << std::endl;
}


template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::GetRotationMatrix(const PointType & fixedPoint, SpatialJacobianType & rotation) const
 {
  this->m_Transform->GetSpatialJacobian(fixedPoint, rotation);
  rotation = rotation.GetTranspose();
 }

/*
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
bool
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::CheckForBSplineTransform( BSplineTransformPointer & bspline )
{
  BSplineTransformType * testPtr1
    = dynamic_cast<BSplineTransformType *>( this->m_Transform.GetPointer() );
   CombinationTransformType * testPtr2a
    = dynamic_cast<CombinationTransformType *>( this->m_Transform.GetPointer() );
  bool transformIsBSpline = false;
  if ( testPtr1 )
  {
    transformIsBSpline = true;
    bspline = testPtr1;
  }
  else if ( testPtr2a )
  {
    BSplineTransformType * testPtr2b = dynamic_cast<BSplineTransformType *>(
      (testPtr2a->GetCurrentTransform()) );
    if ( testPtr2b )
    {
      transformIsBSpline = true;
      bspline = testPtr2b;
    }
  }

  return transformIsBSpline;

} // end CheckForBSplineTransform()
*/

template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
double
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType >
::EvaluateLocalSimilarityMeasure(
    const InputPixelType fixedGradient,
    const InputPixelType movingGradient ) const
{
    //std::cout<< "EvaluateLocalSimilarityMeasure.." << std::endl;
    double value = 0;
    if(!fixedGradient.GetNorm() || !movingGradient.GetNorm())
    {
      return value;
    }

    InputPixelType vF = fixedGradient/fixedGradient.GetNorm();
    InputPixelType vM = movingGradient/movingGradient.GetNorm();

    /*
    double innerAngle = angle(fixedGradient.GetVnlVector(), movingGradient.GetVnlVector());

    if(innerAngle != innerAngle) //Check for NAN
    {
      std::cout << "fixedGradient\t" << fixedGradient << std::endl;
      std::cout << "movingGradient\t" << movingGradient << std::endl;
      itkExceptionMacro(<<"InnerAngle is NAN!");
    }
    */
    if(ImageDimension==2)
    {
      value = pow((vF[0]*vM[0] + vF[1]*vM[1]),m_LocalSimilarityMeasureParameter);
    }
    else if(ImageDimension==3)
    {
      value = pow((vF[0]*vM[0] + vF[1]*vM[1] + vF[2]*vM[2]),m_LocalSimilarityMeasureParameter);
    }
    //std::cout << "Value:\t" << value << std::endl;
    return value;
}

template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType >
::EvaluateLocalSimilarityDerivative(
    const InputPixelType fixedGradient,
    const InputPixelType movingGradient,
    double & derivativeOrientation1,
    double & derivativeOrientation2 ) const
{
  derivativeOrientation1 = 0; derivativeOrientation2 = 0;

  if(!fixedGradient.GetNorm() || !movingGradient.GetNorm())
  {
    return;
  }

  //double innerAngle = angle(fixedGradient.GetVnlVector(), movingGradient.GetVnlVector());
  InputPixelType vF = fixedGradient/fixedGradient.GetNorm();
  InputPixelType vM = movingGradient/movingGradient.GetNorm();

  if(ImageDimension==2)
  {
    double temp = (vF[0]*vM[0] + vF[1]*vM[1]);
    //value = pow(temp, m_LocalSimilarityMeasureParameter);
    derivativeOrientation1 = m_LocalSimilarityMeasureParameter*pow(temp, m_LocalSimilarityMeasureParameter-1)*(-vF[0]*vM[1] + vF[1]*vM[0]);
    std::cout << "derivativeOrientation1:\t" << derivativeOrientation1 << std::endl;
    std::cout << "temp:\t" << temp << std::endl;
    std::cout << "-vF[0]*vM[1] + vF[1]*vM[0]) = " << (-vF[0]*vM[1] + vF[1]*vM[0]) << std::endl;
    std::cout << "vF:\t" << vF << std::endl;
    std::cout << "vM:\t" << vM << std::endl;
  }
  else if(ImageDimension==3)
  {
    double temp = (vF[0]*vM[0] + vF[1]*vM[1] + vF[2]*vM[2]);
    //value = pow(temp, m_LocalSimilarityMeasureParameter);
    derivativeOrientation1 = m_LocalSimilarityMeasureParameter*pow(temp, m_LocalSimilarityMeasureParameter-1);
    derivativeOrientation2 = derivativeOrientation1;
    
    double temp2 =  sqrt(1-pow(vM[2],2));
    derivativeOrientation1 *= ( vF[0]*vM[2]*(vM[0]/temp2) + vF[1]*vM[2]*(vM[1]/temp2) - vF[2]*temp2 );

    derivativeOrientation2 *= (-vF[0]*vM[1] + vF[1]*vM[0]);

  }

}

/**
 * Evaluate at image index position
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
double
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType >
::EvaluateValueAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient ) const
{
    double value = 0;

    if(!this->m_Transform)
    {
      itkExceptionMacro(<<"Transform has not been set!");
    }

    const PointType transformedPoint = this->m_Transform->TransformPoint(point);

    if(!this->IsInsideBuffer(transformedPoint))
    {
      return value;
    }

    SpatialJacobianType rotation;
    this->GetRotationMatrix(point, rotation);
    InputPixelType preMovingGradient = this->Evaluate(transformedPoint);
    const InputPixelType movingGradient = rotation*preMovingGradient;

    if( movingGradient[0] != movingGradient[0] ) // isNaN
    {
       std::cout << "rotation" << rotation << std::endl;
       std::cout << "preMovingGradient" << preMovingGradient << std::endl;    
       std::cout << "movingGradient" << movingGradient << std::endl;
       itkExceptionMacro(<<"movingGradient is a NAN")
    }


    value = EvaluateLocalSimilarityMeasure(fixedGradient, movingGradient);

    if(value != value) // isNan?
    {
      itkExceptionMacro(<<"EvaluateLocalSimilarityMeasureValue returns a NAN")
    }
    return value;
}

/**
 * Evaluate at image index position
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType >
::EvaluateLocalValueAndDerivativeAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient,
    double & value,
    double & derivativeOrientation1,
    double & derivativeOrientation2 ) const
{

    if(!this->m_Transform)
    {
      itkExceptionMacro(<<"Transform has not been set!");
    }

    value = 0;
    derivativeOrientation1 = 0; 
    derivativeOrientation2 = 0;
    const PointType transformedPoint = this->m_Transform->TransformPoint(point);

    if(!this->IsInsideBuffer(transformedPoint))
    {
      return;
    }

    SpatialJacobianType rotation;
    this->GetRotationMatrix(point, rotation);
    InputPixelType preMovingGradient = this->Evaluate(transformedPoint);
    const InputPixelType movingGradient = rotation*preMovingGradient;

    if( movingGradient[0] != movingGradient[0] ) // isNaN
    {
       std::cout << "rotation" << rotation << std::endl;
       std::cout << "preMovingGradient" << preMovingGradient << std::endl;    
       std::cout << "movingGradient" << movingGradient << std::endl;
       itkExceptionMacro(<<"movingGradient is a NAN")
    }


    value = EvaluateLocalSimilarityMeasure(fixedGradient, movingGradient);

    if(value != value) // isNan?
    {
      itkExceptionMacro(<<"EvaluateLocalSimilarityMeasureValue returns a NAN")
    }

    this->EvaluateLocalSimilarityDerivative(fixedGradient, movingGradient, derivativeOrientation1, derivativeOrientation2);
}


template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
double
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType >
::EvaluateValueAtTransformedPoint(
    const PointType & transformedPoint,
    const InputPixelType fixedGradient,
    const SpatialJacobianType rotation) const
{
    double value = 0;

    if(!this->IsInsideBuffer(transformedPoint))
    {
      return value;
    }

    InputPixelType preMovingGradient = this->Evaluate(transformedPoint);
    const InputPixelType movingGradient = rotation*preMovingGradient;

    value = EvaluateLocalSimilarityMeasure(fixedGradient, movingGradient);

    if(value != value) // isNAN?
    {
      itkExceptionMacro(<<"EvaluateLocalSimilarityMeasureValue returns a NAN")
    }
    return value;
}


template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
typename OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::InputPixelType
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::EvaluateGradientAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient) const
{

  PointType transformedPoint = this->m_Transform->TransformPoint(point);

  PointType neighPoint = transformedPoint;
  InputPixelType tempDerivative;
  tempDerivative.Fill(0.0);

  const InputImageType * inputImage = this->GetInputImage();

  const typename InputImageType::SpacingType spacing =
    inputImage->GetSpacing();

  const typename InputImageType::RegionType region =
    inputImage->GetBufferedRegion();

  const typename InputImageType::SizeType& size   = region.GetSize();
  const typename InputImageType::IndexType& start = region.GetIndex();

  IndexType neighIndex;

  inputImage->TransformPhysicalPointToIndex(transformedPoint, neighIndex);

  bool computeGradient = true;

  if(!fixedGradient.GetNorm())
  {
    computeGradient = false;
  }

  for ( unsigned int dim = 0; dim < TInputImage::ImageDimension; dim++ )
  {
    if(neighIndex[dim]<(static_cast<long>(start[dim]) + 3) || neighIndex[dim]>(start[dim] + static_cast<long>(size[dim]) - 4 ))
    {
      computeGradient = false;
    }
  }

  if(!computeGradient)
  {
    return tempDerivative;
  }
  else
  {
    SpatialJacobianType rotation;
    this->GetRotationMatrix(point, rotation);
    tempDerivative.Fill( 0.0 );
    for ( unsigned int dim = 0; dim < TInputImage::ImageDimension; dim++ )
      {
      // bounds checking
      if( neighIndex[dim] < static_cast<long>(start[dim]) + 1 ||
          neighIndex[dim] > (start[dim] + static_cast<long>(size[dim]) - 2 ) )
        {
        continue;
        }

      transformedPoint[dim] += spacing[dim];
      tempDerivative[dim] = this->EvaluateValueAtTransformedPoint( transformedPoint, fixedGradient, rotation );

      transformedPoint[dim] -= 2*spacing[dim];
      tempDerivative[dim] -= this->EvaluateValueAtTransformedPoint( transformedPoint, fixedGradient, rotation );

      tempDerivative[dim] *= 0.5 / spacing[dim];

      transformedPoint[dim] += spacing[dim];
      }
#ifdef ITK_USE_ORIENTED_IMAGE_DIRECTION
  if( this->m_UseImageDirection )
    {
    InputPixelType orientedDerivative;
    inputImage->TransformLocalVectorToPhysicalVector( tempDerivative, orientedDerivative );
    tempDerivative =   orientedDerivative;
    }
#endif
    return tempDerivative;
  }


}


/**
 * Evaluate at image index position
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::EvaluateValueAndGradientAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient,
    double & value,
    InputPixelType & measureGradient ) const
{
    value = EvaluateValueAtPoint(point, fixedGradient);
    measureGradient = EvaluateGradientAtPoint(point, fixedGradient);

}

/**
 * Evaluate at image index position
 */
template<class TInputImage, class TCoordRep, class TInterpolatorPrecisionType>
void
OrientationMatchingBetaImageFunction< TInputImage, TCoordRep, TInterpolatorPrecisionType  >
::EvaluateValueAndDerivativeAtPoint(
    const PointType & point,
    const InputPixelType fixedGradient,
    const InputPixelType orientation1Gradient,
    const InputPixelType orientation2Gradient,
    double & value,
    DerivativeType & measureGradient ) const
{
    measureGradient = DerivativeType( this->m_Transform->GetNumberOfParameters() );
    measureGradient.Fill(0.0);

    //value = EvaluateValueAtPoint(point, fixedGradient);
    
    double derivativeOrientation1, derivativeOrientation2;
  
    this->EvaluateLocalValueAndDerivativeAtPoint(point, fixedGradient, value, derivativeOrientation1, derivativeOrientation2);

    /*  (dM/DAngle1*dAngle1/dTx + dM/dAngle2*dAngle2/dTx)*dTx/dp  */
    JacobianType jacobian;
    NonZeroJacobianIndicesType nzji;
    this->m_Transform->GetJacobian(point, jacobian, nzji);
   
    vnl_vector<double> tempDerivative; 
    if(ImageDimension==2)
    {
      tempDerivative = derivativeOrientation1*vnl_vector<double>(orientation1Gradient.GetVnlVector()*jacobian);
    }
    else if(ImageDimension==3)
    {
      tempDerivative = (derivativeOrientation1*orientation1Gradient.GetVnlVector() + derivativeOrientation2*orientation2Gradient.GetVnlVector())*jacobian;
    }

    //std::cout << "orientation1Gradient:\t" << orientation1Gradient << std::endl;
    //std::cout << "tempDerivative:\t" << tempDerivative << std::endl;

    if ( nzji.size() == this->m_Transform->GetNumberOfParameters() )
    {
      measureGradient = tempDerivative;
    }
    else
    {
      for ( unsigned int i = 0; i < tempDerivative.size(); ++i )
      {
        const unsigned int index = nzji[ i ];
        measureGradient[ index ] = tempDerivative[i];
      }
    }

    //std::cout << "measureGradient:\t" << measureGradient << std::endl;

    NonZeroJacobianIndicesType nzji2;
    JacobianOfSpatialJacobianType jacobianOfSpatialJacobian;
    this->m_Transform->GetJacobianOfSpatialJacobian(point, jacobianOfSpatialJacobian, nzji2);

    SpatialJacobianType rotation;
    this->GetRotationMatrix(point, rotation);

    std::cout << "rotation:\t" << rotation << std::endl;

    const PointType transformedPoint = this->m_Transform->TransformPoint(point);

    if(!this->IsInsideBuffer(transformedPoint))
    {
      return;
    }

    InputPixelType preMovingGradient = this->Evaluate(transformedPoint);
    const InputPixelType movingGradient = rotation*preMovingGradient;

    SpatialJacobianType rotationDerivative;

    if( (fixedGradient.GetNorm()) && (preMovingGradient.GetNorm()) && (movingGradient.GetNorm()) && derivativeOrientation1 && 0)
    {
      InputPixelType u = preMovingGradient;
      InputPixelType w = movingGradient;

      InputPixelType uN = preMovingGradient/preMovingGradient.GetNorm();
      InputPixelType wN = movingGradient/movingGradient.GetNorm();

      if(ImageDimension==2)
      {
        
        vnl_matrix<double> jacobianDerivative(2,2);
        
        jacobianDerivative[0][0] = -w[1]*u[0]; jacobianDerivative[0][1] = -w[1]*u[1];
        jacobianDerivative[1][0] = w[0]*u[0]; jacobianDerivative[1][1] = w[0]*u[1];
        jacobianDerivative *= (1/w.GetSquaredNorm());

        double r = (pow(rotation[0][0],2) + pow(rotation[1][0],2))*pow(uN[0],2);
        r += 2*(rotation[0][0]*rotation[0][1] + rotation[1][0]*rotation[1][1])*uN[0]*uN[1];
        r += (pow(rotation[0][1],2) + pow(rotation[1][1],2))*pow(uN[1],2);
        double r32 = pow(r,3/2);
        double rSqR = sqrt(r);
        
        if(r==0)
        {
          jacobianDerivative.fill(0.0);
        }
        else if(abs(wN[0])>abs(wN[1]))
        {

          double tempA = (rotation[1][0]*uN[0] + rotation[1][1]*uN[1]);
          jacobianDerivative[0][0] = -(1/r32)*tempA*(rotation[0][0]*pow(uN[0],2) + rotation[0][1]*uN[0]*uN[1]); 
          jacobianDerivative[0][1] = -(1/r32)*tempA*(rotation[0][1]*pow(uN[1],2) + rotation[0][0]*uN[0]*uN[1]);
          jacobianDerivative[1][0] = (uN[0]/rSqR) - (1/r32)*tempA*(rotation[1][0]*pow(uN[0],2) + rotation[1][1]*uN[0]*uN[1]);
          jacobianDerivative[1][1] = (uN[1]/rSqR) - (1/r32)*tempA*(rotation[1][1]*pow(uN[1],2) + rotation[1][0]*uN[0]*uN[1]);
          jacobianDerivative *= (1/wN[0]);
        }
        else
        {
          double tempA = (rotation[0][0]*uN[0] + rotation[0][1]*uN[1]);
          jacobianDerivative[0][0] = (uN[0]/rSqR) - (1/r32)*tempA*(rotation[0][0]*pow(uN[0],2) + rotation[0][1]*uN[0]*uN[1]); 
          jacobianDerivative[0][1] = (uN[1]/rSqR) - (1/r32)*tempA*(rotation[0][1]*pow(uN[1],2) + rotation[0][0]*uN[0]*uN[1]);
          jacobianDerivative[1][0] = - (1/r32)*tempA*(rotation[1][0]*pow(uN[0],2) + rotation[1][1]*uN[0]*uN[1]);
          jacobianDerivative[1][1] = - (1/r32)*tempA*(rotation[1][1]*pow(uN[1],2) + rotation[1][0]*uN[0]*uN[1]);
          jacobianDerivative *= -(1/wN[1]);
        }
        

        /*
        for ( unsigned int i = 0; i < nzji2.size(); ++i )
        {
          const unsigned int index = nzji2[ i ];
          //std::cout << "nzji2 Index:\t" << index << std::endl;
          //std::cout << "jacobianOfSpatialJacobian[i]" << jacobianOfSpatialJacobian[i] << std::endl;
          if( vnl_determinant(jacobianOfSpatialJacobian[i].GetVnlMatrix()) > 0 )
          {  
            measureGradient[ index ] -= derivativeOrientation1;
            std::cout << "nzji2 Index:\t" << index << "\t Rotation!" << std::endl;

          }
        }
        */
        //std::cout << "measureGradient:\t" << measureGradient << std::endl;

        //std::cout << "jacobianDerivative:\t" << jacobianDerivative << std::endl;
        
        for ( unsigned int i = 0; i < nzji2.size(); ++i )
        {
          const unsigned int index = nzji2[ i ];
          rotationDerivative = (jacobianOfSpatialJacobian[i]).GetTranspose();  
          //std::cout << "rotationDerivative:\t" << rotationDerivative << std::endl;
          for(unsigned int r=0; r<ImageDimension; r++)
          {
            for(unsigned int c=0; c<ImageDimension; c++)
            {            
              double temp = derivativeOrientation1*jacobianDerivative[r][c]*rotationDerivative[r][c];
              if(temp != temp)
              {
                std::cout << "jacobianDerivative[r][c]:\t" << jacobianDerivative[r][c] << std::endl;
                std::cout << "rotationDerivative[r][c]:\t" << rotationDerivative[r][c] << std::endl;
                std::cout << "derivativeOrientatoin1:\t" << derivativeOrientation1 << std::endl;
                itkExceptionMacro(<<" jacobianDerivative*rotationDerivative is NAN ");
              }
              measureGradient[ index ]  += derivativeOrientation1*jacobianDerivative[r][c]*rotationDerivative[r][c];            
            }
          }
        }
        

      }
      else if(ImageDimension==3)
      {
        double WxySq = pow(w[0],2) + pow(w[1],2);
        double WSq = w.GetSquaredNorm();
        vnl_matrix<double> jacobianDerivative1(3,3);
        jacobianDerivative1[0][0] = -w[1]*u[0]; jacobianDerivative1[0][1] = -w[1]*u[1]; jacobianDerivative1[0][2] = -w[1]*u[2] ;
        jacobianDerivative1[1][0] = w[0]*u[0]; jacobianDerivative1[1][1] = w[0]*u[1]; jacobianDerivative1[1][2] = w[0]*u[2];
        jacobianDerivative1[2][0] = 0; jacobianDerivative1[2][1] = 0; jacobianDerivative1[2][2] = 0;
        jacobianDerivative1 *= (1/WxySq);

        double WxySqR = sqrt(WxySq);
        double WzNorm = w[2]/(WSq*WxySqR);
        double WzSq = pow(w[2],2);

        vnl_matrix<double> jacobianDerivative2(3,3);
        jacobianDerivative2[0][0] = WzNorm*w[0]*u[0]; jacobianDerivative2[0][1] = WzNorm*w[0]*u[1]; jacobianDerivative2[0][2] = WzNorm*w[0]*u[2];
        jacobianDerivative2[1][0] = WzNorm*w[1]*u[0]; jacobianDerivative2[1][1] = WzNorm*w[1]*u[1]; jacobianDerivative2[1][2] = WzNorm*w[1]*u[2];
        jacobianDerivative2[2][0] = -WxySq*u[0]/WzSq; jacobianDerivative2[2][1] = -WxySq*u[1]/WzSq; jacobianDerivative2[2][2] = -WxySq*u[2]/WzSq;


        for ( unsigned int i = 0; i < nzji2.size(); ++i )
        {
          const unsigned int index = nzji[ i ];
          rotationDerivative = (jacobianOfSpatialJacobian[i]).GetTranspose();  

          for(unsigned int r=0; r<ImageDimension; r++)
          {
            for(unsigned int c=0; c<ImageDimension; c++)
            {            
              measureGradient[ index ]  += jacobianDerivative1[r][c]*rotationDerivative[r][c] + jacobianDerivative2[r][c]*rotationDerivative[r][c];            
            }
          }
        }

      }
     
    } 

}

} // end namespace itk

#endif

