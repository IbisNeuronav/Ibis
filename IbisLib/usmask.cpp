/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "usmask.h"
#include <vtkImageData.h>

ObjectSerializationMacro( USMask );

USMask::USMask()
{
    m_mask = vtkImageData::New();
    m_defaultMaskSize[0] = MASK_WIDTH;
    m_defaultMaskSize[1] = MASK_HEIGHT;
    m_defaultMaskOrigin[0] = MASK_ORIGIN_X;
    m_defaultMaskOrigin[1] = MASK_ORIGIN_Y;
    m_defaultMaskCrop[0] = MASK_CROP_LEFT;
    m_defaultMaskCrop[1] = MASK_CROP_RIGHT;
    m_defaultMaskAngles[0] = MASK_ANGLE_LEFT;
    m_defaultMaskAngles[1] = MASK_ANGLE_RIGHT;
    m_defaultMaskDepthTop = MASK_DEPTH_TOP;
    m_defaultMaskDepthBottom = MASK_DEPTH_BOTTOM;

    m_maskSize[0] = m_defaultMaskSize[0];
    m_maskSize[1] = m_defaultMaskSize[1];
    m_maskOrigin[0] = m_defaultMaskOrigin[0];
    m_maskOrigin[1] = m_defaultMaskOrigin[1];
    m_maskCrop[0] = m_defaultMaskCrop[0];
    m_maskCrop[1] = m_defaultMaskCrop[1];
    m_maskAngles[0] = m_defaultMaskAngles[0];
    m_maskAngles[1] = m_defaultMaskAngles[1];
    m_maskDepthTop = m_defaultMaskDepthTop;
    m_maskDepthBottom = m_defaultMaskDepthBottom;

    this->BuildMask();
}

USMask::~USMask()
{
    m_mask->Delete();
}

USMask::USMask( const USMask& usmask )
{
    m_maskDepthTop = usmask.m_maskDepthTop;
    m_maskDepthBottom = usmask.m_maskDepthBottom;
    m_maskSize[0] = usmask.m_maskSize[0];
    m_maskSize[1] = usmask.m_maskSize[1];
    m_maskOrigin[0] = usmask.m_maskOrigin[0];
    m_maskOrigin[1] = usmask.m_maskOrigin[1];
    m_maskCrop[0] = usmask.m_maskCrop[0];
    m_maskCrop[1] = usmask.m_maskCrop[1];
    m_maskAngles[0] = usmask.m_maskAngles[0];
    m_maskAngles[1] = usmask.m_maskAngles[1];
    this->BuildMask();
}

USMask& USMask::operator= ( const USMask &usmask )
{
    m_maskDepthTop = usmask.m_maskDepthTop;
    m_maskDepthBottom = usmask.m_maskDepthBottom;
    m_maskSize[0] = usmask.m_maskSize[0];
    m_maskSize[1] = usmask.m_maskSize[1];
    m_maskOrigin[0] = usmask.m_maskOrigin[0];
    m_maskOrigin[1] = usmask.m_maskOrigin[1];
    m_maskCrop[0] = usmask.m_maskCrop[0];
    m_maskCrop[1] = usmask.m_maskCrop[1];
    m_maskAngles[0] = usmask.m_maskAngles[0];
    m_maskAngles[1] = usmask.m_maskAngles[1];
    this->BuildMask();
    return *this;
}

void USMask::Serialize( Serializer * ser )
{
    ::Serialize( ser, "MaskSize", m_maskSize, 2 );
    ::Serialize( ser, "MaskOrigin", m_maskOrigin, 2 );
    ::Serialize( ser, "MaskCrop", m_maskCrop, 2 );
    ::Serialize( ser, "MaskAngles", m_maskAngles, 2 );
    ::Serialize( ser, "MaskDepthTop", m_maskDepthTop );
    ::Serialize( ser, "MaskDepthBottom", m_maskDepthBottom );
    if( ser->IsReader() )
        this->BuildMask();
}

void USMask::ResetToDefault()
{
    m_maskSize[0] = m_defaultMaskSize[0];
    m_maskSize[1] = m_defaultMaskSize[1];
    m_maskOrigin[0] = m_defaultMaskOrigin[0];
    m_maskOrigin[1] = m_defaultMaskOrigin[1];
    m_maskCrop[0] = m_defaultMaskCrop[0];
    m_maskCrop[1] = m_defaultMaskCrop[1];
    m_maskAngles[0] = m_defaultMaskAngles[0];
    m_maskAngles[1] = m_defaultMaskAngles[1];
    m_maskDepthTop = m_defaultMaskDepthTop;
    m_maskDepthBottom = m_defaultMaskDepthBottom;
    this->BuildMask();
}

void USMask::SetAsDefault()
{
    m_defaultMaskSize[0] = m_maskSize[0];
    m_defaultMaskSize[1] = m_maskSize[1];
    m_defaultMaskOrigin[0] = m_maskOrigin[0];
    m_defaultMaskOrigin[1] = m_maskOrigin[1];
    m_defaultMaskCrop[0] = m_maskCrop[0];
    m_defaultMaskCrop[1] = m_maskCrop[1];
    m_defaultMaskAngles[0] = m_maskAngles[0];
    m_defaultMaskAngles[1] = m_maskAngles[1];
    m_defaultMaskDepthTop = m_maskDepthTop;
    m_defaultMaskDepthBottom = m_maskDepthBottom;
}

void USMask::SetMaskSize( int size[2] )
{
    m_maskSize[0] = size[0];
    m_maskSize[1] = size[1];
    this->BuildMask();
}

void USMask::SetMaskOrigin( int orig[2] )
{
    m_maskOrigin[0] = orig[0];
    m_maskOrigin[1] = orig[1];
    this->BuildMask();
}

void USMask::SetMaskCrop( int crop[2] )
{
    m_maskCrop[0] = crop[0];
    m_maskCrop[1] = crop[1];
    this->BuildMask();
}

void USMask::SetMaskAngles( double angles[2] )
{
    m_maskAngles[0] = angles[0];
    m_maskAngles[1] = angles[1];
    this->BuildMask();
}

void USMask::SetMaskDepthTop( double depthTop )
{
    m_maskDepthTop = depthTop;
    this->BuildMask();
}

void USMask::SetMaskDepthBottom( double depthBottom )
{
    m_maskDepthBottom = depthBottom;
    this->BuildMask();
}

void USMask::BuildMask()
{
    m_mask->SetDimensions( m_maskSize[0], m_maskSize[1], 1 );
    m_mask->SetExtent(0, 639, 0, 479, 0, 0);
    m_mask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

    unsigned char * pix = (unsigned char *)m_mask->GetScalarPointer();

    for( int iy = 0; iy < m_maskSize[1]; ++iy )
    {
        for( int ix = 0; ix < m_maskSize[0]; ++ix )
        {
            // Test 1 : x, y bounds
            if( ix < m_maskCrop[0] || ix > m_maskCrop[1] || iy >= m_maskOrigin[1] )
            {
                *pix = 0;
            }
            else
            {
                // Test 2 : Distance from origin
                double diff[2];
                diff[0] = (double)ix - m_maskOrigin[0];
                diff[1] = (double)iy - m_maskOrigin[1];
                double dist = sqrt( diff[0] * diff[0] + diff[1] * diff[1] );
                if( dist > m_maskDepthBottom || dist < m_maskDepthTop )
                    *pix = 0;
                else
                {
                    // Test 3 : between angles
                    double angle =  atan( abs( diff[0] ) / diff[1] );
                    if( ( diff[0] < 0.0 && angle < m_maskAngles[0] ) || diff[0] > 0.0 && angle < m_maskAngles[1] )
                        *pix = 0;
                    else
                        *pix = 255;
                }
            }
            ++pix;
        }
    }
    emit MaskChanged();
}
