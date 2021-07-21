/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __TransferFunctionKey_h_
#define __TransferFunctionKey_h_

#include "Animation.h"
#include "serializer.h"
#include <vtkSmartPointer.h>

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class TransferFunctionKey
{

public:

    TransferFunctionKey();
    TransferFunctionKey( const TransferFunctionKey & );
    ~TransferFunctionKey();

    void operator=(const TransferFunctionKey&);

    void Serialize( Serializer * ser );

    void SetFunctions( vtkColorTransferFunction * color, vtkPiecewiseFunction * opacity );
    vtkColorTransferFunction * GetColorFunc();
    vtkPiecewiseFunction * GetOpacityFunc();
    void Interpolate( TransferFunctionKey & keyA, TransferFunctionKey & keyB, double ratio );

    int frame;

protected:

    vtkSmartPointer<vtkColorTransferFunction> m_colorFunc;
    vtkSmartPointer<vtkPiecewiseFunction> m_opacityFunc;

};

ObjectSerializationHeaderMacro( TransferFunctionKey );

class TFAnimation : public Animation<TransferFunctionKey>
{

public:

    virtual ~TFAnimation() {}
    void Serialize( Serializer * ser );
};

ObjectSerializationHeaderMacro( TFAnimation );

#endif
