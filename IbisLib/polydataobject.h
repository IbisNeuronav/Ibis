/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef __PolyDataObject_h_
#define __PolyDataObject_h_

#include "sceneobject.h"
#include "serializer.h"
#include <map>
#include <QVector>
#include <QObject>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>

#include "abstractpolydataobject.h"

class vtkPolyData;
class vtkTransform;
class vtkActor;
class vtkDataSetAlgorithm;
class vtkImageData;
class vtkProbeFilter;
class vtkScalarsToColors;
class ImageObject;
class vtkClipPolyData;
class vtkPassThrough;
class vtkCutter;
class vtkPlane;
class vtkPlanes;
class AbstractPolyDataObject;


class PolyDataObject : public AbstractPolyDataObject
{
    
Q_OBJECT

public:
        
    static PolyDataObject * New() { return new PolyDataObject; }
    vtkTypeMacro(PolyDataObject,AbstractPolyDataObject);
    
    PolyDataObject();
    virtual ~PolyDataObject();
    virtual void Serialize( Serializer * ser ) override;
    
    virtual void UpdatePipeline() override;
    virtual void CreateSettingsWidgets( QWidget * parent, QVector <QWidget*> *widgets) override;


    vtkImageData* Texture;
    vtkSmartPointer<vtkDataSetAlgorithm> TextureMap;
    bool      showTexture;
    QString   textureFileName;

    void SetTexture( vtkImageData * texImage );
    bool GetShowTexture() { return showTexture; }
    void SetShowTexture( bool show );
    void SetTextureFileName( QString );
    QString GetTextureFileName() { return textureFileName; }

    static vtkSmartPointer<vtkImageData> checkerBoardTexture;
};

ObjectSerializationHeaderMacro( PolyDataObject );

#endif
