/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Dante De Nigris for writing this class

#ifndef __itkGradientOrientationDifferenceFilter_txx
#define __itkGradientOrientationDifferenceFilter_txx
 
#include "itkGradientOrientationDifferenceFilter.h"
 
#include "itkObjectFactory.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "vnl/vnl_math.h" 

namespace itk
{
 
template< class TImage>
GradientOrientationDifferenceFilter<TImage>::GradientOrientationDifferenceFilter()
{
  this->SetNumberOfRequiredOutputs(2);
  this->SetNumberOfRequiredInputs(1);
 
  this->SetNthOutput( 0, this->MakeOutput(0) );
  this->SetNthOutput( 1, this->MakeOutput(1) );
}
 
template< class TImage>
void GradientOrientationDifferenceFilter<TImage>::GenerateData()
{
 

  if( ImageDimension != 3 )
    {
    itkExceptionMacro(<<"No support for image dimension different than 3");
    }

  typename TImage::ConstPointer input = this->GetInput();

  // Setup output 1
  typename TImage::Pointer output1 = this->GetOutput1();
  output1->SetRegions(this->GetInput()->GetLargestPossibleRegion());
  output1->SetOrigin(this->GetInput()->GetOrigin());
  output1->SetDirection(this->GetInput()->GetDirection());
  output1->SetSpacing(this->GetInput()->GetSpacing());  
  output1->Allocate();  
  PixelType orientationDerivativeNull;
  orientationDerivativeNull.Fill(0); 
  output1->FillBuffer( orientationDerivativeNull ); 

  // Setup output 2
  typename TImage::Pointer output2 = this->GetOutput2();
  output2->SetRegions(this->GetInput()->GetLargestPossibleRegion());
  output2->SetOrigin(this->GetInput()->GetOrigin());
  output2->SetDirection(this->GetInput()->GetDirection());
  output2->SetSpacing(this->GetInput()->GetSpacing());  
  output2->Allocate();    
  output2->FillBuffer( orientationDerivativeNull );
 
  //Set up iterators
  itk::ImageRegionConstIteratorWithIndex<TImage> inputIterator(input, input->GetLargestPossibleRegion());
  inputIterator.GoToBegin();

  itk::ImageRegionIterator<TImage> outputIterator1(output1, output1->GetLargestPossibleRegion());
  outputIterator1.GoToBegin();
 
  itk::ImageRegionIterator<TImage> outputIterator2(output2, output2->GetLargestPossibleRegion());
  outputIterator2.GoToBegin();
 
  while(!inputIterator.IsAtEnd() && !outputIterator1.IsAtEnd() && !outputIterator2.IsAtEnd())
    {
    //Magic happens here!!!
    IndexType imageIndex  = inputIterator.GetIndex();
    PixelType orientationDerivative1, orientationDerivative2;
    orientationDerivative1.Fill(0);
    orientationDerivative2.Fill(0);

    bool isValid = true;
    //Compute central difference 
    for( unsigned int d = 0; d < ImageDimension; ++d )
      {
      IndexType indexP = imageIndex;
      indexP[d] += 1;   

      IndexType indexN = imageIndex;
      indexN[d] -= 1;         

      if( !input->GetLargestPossibleRegion().IsInside(indexP) || !input->GetLargestPossibleRegion().IsInside(indexP) )
        {
        isValid = false;
        break;  
        }

      PixelType vectorP = input->GetPixel( indexP );
      PixelType vectorN = input->GetPixel( indexN );

      // std::cout << "vectorP " << std::endl << vectorP << std::endl;
      // std::cout << "vectorN " << std::endl << vectorN << std::endl;  

      if( !(vectorP.GetNorm()>0) || !(vectorN.GetNorm()>0) )
        {
        isValid = false;
        break;  
        }
      vectorP.Normalize();
      vectorN.Normalize();


      double thetaP = vcl_acos( vectorP[2] );
      double phiP = vcl_atan2( vectorP[1], vectorP[0] );

      double thetaN = vcl_acos( vectorN[2] );
      double phiN = vcl_atan2( vectorN[1], vectorN[0] );

      double thetaDiff = vcl_atan2( vcl_sin(thetaP-thetaN), vcl_cos(thetaP-thetaN) ) / 2.0;
      double phiDiff = vcl_atan2( vcl_sin(phiP-phiN), vcl_cos(phiP-phiN) ) / 2.0;  

      // std::cout << "thetaDiff " << std::endl << thetaDiff << std::endl;
      // std::cout << "phiDiff " << std::endl << phiDiff << std::endl;  

      orientationDerivative1[d] = thetaDiff;
      orientationDerivative2[d] = phiDiff;  

      }

    if( isValid )
      {
      // std::cout << "orientationDerivative1 " << std::endl << orientationDerivative1 << std::endl;
      // std::cout << "orientationDerivative2 " << std::endl << orientationDerivative2 << std::endl;  
      outputIterator1.Set( orientationDerivative1 );
      outputIterator2.Set( orientationDerivative2 );
      }

    ++inputIterator;
    ++outputIterator1;
    ++outputIterator2;
    }
 
}
 
template< class TImage>
DataObject::Pointer GradientOrientationDifferenceFilter<TImage>::MakeOutput(unsigned int idx)
{
  DataObject::Pointer output;
 
  switch ( idx )
    {
    case 0:
      output = ( TImage::New() ).GetPointer();
      break;
    case 1:
      output = ( TImage::New() ).GetPointer();
      break;
    default:
      std::cerr << "No output " << idx << std::endl;
      output = NULL;
      break;
    }
  return output.GetPointer();
}
 
template< class TImage>
TImage* GradientOrientationDifferenceFilter<TImage>::GetOutput1()
{
  return dynamic_cast< TImage * >(
           this->ProcessObject::GetOutput(0) );
}
 
template< class TImage>
TImage* GradientOrientationDifferenceFilter<TImage>::GetOutput2()
{
  return dynamic_cast< TImage * >(
           this->ProcessObject::GetOutput(1) );
}
 
}// end namespace
 
 
#endif