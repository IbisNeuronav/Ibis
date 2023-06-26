/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef USMASK_H
#define USMASK_H

#include <vtkObject.h>
#include <vtkSmartPointer.h>

#include <QObject>

#include "ibismath.h"
#include "serializer.h"

class vtkImageData;

class USMask : public QObject, public vtkObject
{
    Q_OBJECT

public:
    static USMask * New() { return new USMask; }

    vtkTypeMacro( USMask, vtkObject );

    USMask();
    virtual ~USMask();
    USMask( const USMask & usmask );
    USMask & operator=( const USMask & usmask );

    virtual void Serialize( Serializer * ser );

    vtkImageData * GetMask();

    void ResetToDefault();
    void SetAsDefault();

    void SetMaskSize( int size[2] );
    void SetMaskOrigin( double orig[2] );
    void SetMaskCrop( double crop[2] );
    void SetMaskAngles( double angles[2] );
    void SetMaskDepthTop( double depthTop );
    void SetMaskDepthBottom( double depthBottom );
    void SetMaskSize( int width, int height )
    {
        m_defaultMaskSize[0] = width;
        m_defaultMaskSize[1] = height;
    }

    int * GetMaskSize() { return &m_maskSize[0]; }
    double * GetMaskCrop() { return &m_maskCrop[0]; }
    double * GetMaskOrigin() { return &m_maskOrigin[0]; }
    double GetMaskBottom() { return m_maskDepthBottom; }
    double GetMaskTop() { return m_maskDepthTop; }
    double * GetMaskAngles() { return &m_maskAngles[0]; }

    // define default
#define MASK_WIDTH 640
#define MASK_HEIGHT 480
#define MASK_ORIGIN_X 318.0
#define MASK_ORIGIN_Y 448.0
#define MASK_CROP_LEFT 50.0
#define MASK_CROP_RIGHT 540.0
#define MASK_ANGLE_LEFT ( -IbisMath::pi4 )
#define MASK_ANGLE_RIGHT ( -IbisMath::pi4 )
#define MASK_DEPTH_TOP 24.0  // as In Xiao's code ?
#define MASK_DEPTH_BOTTOM 400.0

signals:

    void MaskChanged();

protected:
    int m_maskSize[2];  // width, height
    double m_maskCrop[2];
    double m_maskOrigin[2];
    double m_maskDepthTop;
    double m_maskDepthBottom;
    double m_maskAngles[2];  // radians

    int m_defaultMaskSize[2];
    double m_defaultMaskCrop[2];
    double m_defaultMaskOrigin[2];
    double m_defaultMaskDepthTop;
    double m_defaultMaskDepthBottom;
    double m_defaultMaskAngles[2];

    vtkSmartPointer<vtkImageData> m_mask;

    void BuildMask();
};

ObjectSerializationHeaderMacro( USMask );

#endif  // USMASK_H
