/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __TractogramObject_h_
#define __TractogramObject_h_

#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>

#include <QVector>

#include "polydataobject.h"

class vtkPassThrough;
class vtkTubeFilter;
class PolyDataObject;

class TractogramObject : public PolyDataObject
{
    Q_OBJECT

public:
    static TractogramObject * New() { return new TractogramObject; }
    vtkTypeMacro( TractogramObject, PolyDataObject );

    TractogramObject();
    virtual ~TractogramObject();

    virtual void Setup( View * view ) override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector<QWidget *> * widgets ) override;
    void SetVertexColorMode( int mode );
    void SetRenderingMode( int mode );

protected:
    vtkSmartPointer<vtkTubeFilter> tubeFilter;
    vtkSmartPointer<vtkPassThrough> m_tubeSwitch;
    bool tube_enabled;

    void UpdatePipeline();

private:
    void GenerateLocalColoring();
    void GenerateEndPtsColoring();
    void AddLocalColor( vtkSmartPointer<vtkUnsignedCharArray> colors, unsigned long vts1_id, unsigned long vts2_id,
                        unsigned long current_id );
};

ObjectSerializationHeaderMacro( PolyDataObject );

#endif
